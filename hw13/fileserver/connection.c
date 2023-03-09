#include "connection.h"
#include "buffer.h"
#include "sflog.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>

static const char *msg200 = "HTTP/1.1 200 ОК\r\nConnection: close";
static const char *msg403 = "HTTP/1.1 403 Forbidden\r\nConnection: close";
static const char *msg404 = "HTTP/1.1 404 Not Found\r\nConnection: close";
static const char *msg405 = "HTTP/1.1 405 Method Not Allowed\r\nConnection: close";
static const char *msg500 = "HTTP/1.1 500 Internal Server Error\r\nConnection: close";
static const char *msg505 = "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close";
static const char *http_request_end = "\r\n\r\n";
static const int http_request_end_len = 4;


/*******************************************************************************
 *                                   HELPERS                                   *
 *******************************************************************************/
/**
 * @brief Анализ buf == http запрос ?
 * @return 1 - да
 *         0 - нет
 *        -1 - memory error */
static int catch_http_request(Buffer *buf) {
  assert(buf);
  if (!buffer_push_back(buf, '\0'))
    return -1;
  char *ptr;
  if ((ptr = strstr(buf->data, http_request_end)) != NULL) {
    *(ptr + http_request_end_len) = '\0';
    return 1;
  }
  buffer_pop_back(buf);
  return 0;
}

/**
 * @brief Выделяет из http запроса метод, цель и версию
 * @note request is changed during parsing!
 * @return true, найдены и установлены поля
 * @return false - ошибка
 */
static bool split_http_head(char *request, char **method, char **target, char **httpver) {
  char **outvals[] = {method, target, httpver};
  char *str = request;
  int i = 0;
  char *sep;
  while (i < 3 && (sep = strpbrk(str, " \r\n")) != NULL) {
    *sep = '\0';
    *(outvals[i]) = str;
    i++;
    str = sep + 1;
  };
  return i == 3;
}

typedef enum {
  open_res_ok,
  open_res_eacess, // нет прав доступа
  open_res_not_regular,
  open_res_not_found,
  open_res_other_error
} open_res;
/**
 * @brief Открывает регулрный файл с проверкой, возваращает дескриптор и размер
 * @return open_res
 */
static open_res open_regular_file(const char *path, int *fd, off_t *file_size) {
  struct stat st;
  int ret = stat(path, &st);
  if (ret < 0) {
    if (errno == EACCES)
      return open_res_eacess;
    if (errno == ENOENT)
      return open_res_not_found;
    return open_res_other_error;
  }
  if (!S_ISREG(st.st_mode)) {
    return open_res_not_regular;
  }

  int loc_fd = open(path, O_RDONLY);
  if (loc_fd < 0) {
    sfl_debug("Unexpected error on open file '%s': %s", path, strerror(errno));
    return open_res_other_error;
  }
  *fd = loc_fd;
  *file_size = st.st_size;
  return open_res_ok;
}


/**
 * @brief Возаращает дескриптор запрашиваемого в HTTP запросе файла
 * @param httphead - NULL-терминатеd HTTP запрос
 * @param basedir - корневой каталог для поиска файла.
 * @param [out] fd - при успехе записыается дескриптор файла
 * @param [out] file_size - при успехе записывается размер файла
 * @return NULL, при успехе или указатель на строку с http ответом об ошибке
 */
const char *parse_http_head(char *request, const char *basedir, int *fd, off_t *file_size) {
  char *method, *target, *httpver;
  if (!split_http_head(request, &method, &target, &httpver)) {
    return msg500;
  }
  sfl_debug("http method %s", method);
  sfl_debug("http target %s", target);
  sfl_debug("http version %s", httpver);

  if (strcmp(httpver, "HTTP/1.1") != 0) {
    return msg505;
  }
  if (strcmp(method, "GET") != 0) {
    return msg405;
  }

  Buffer path = buffer_make();
  if (!buffer_append_string(&path, basedir) || !buffer_append_string(&path, target) ||
      !buffer_push_back(&path, '\0')) {
    sfl_fatal("memory allocation error on  %s:%d", __FILE__, __LINE__);
    return msg500;
  }
  sfl_debug("path to target file: %s", path.data);
  int open_res = open_regular_file(path.data, fd, file_size);
  buffer_free(&path);

  switch (open_res) {
  case open_res_ok:
    break;
  case open_res_eacess:
  case open_res_not_regular:
    return msg403;
  case open_res_not_found:
    return msg404;
  default:
    return msg500;
  };
  return NULL;
}

/*******************************************************************************
 *                            CONNECTION PROCESSING                            *
 *******************************************************************************/

struct Connection {
  ConnState state;
  int conn_fd;
  Buffer request;
  Buffer answer;
  size_t answer_bytes_sent;
  const char *basedir;
  int file_fd;
  off_t file_size;
  off_t file_bytes_sent;
};

Connection *fs_new_conn(int conn_fd, const char *basedir) {
  Connection *conn = calloc(1, sizeof(Connection));
  if (conn == NULL) {
    sfl_fatal("memory allocation error on  %s:%d", __FILE__, __LINE__);
  } else {
    conn->state = state_wait_request;
    conn->conn_fd = conn_fd;
    buffer_init(&conn->answer);
    buffer_init(&conn->request);
    conn->answer_bytes_sent = 0;
    conn->basedir = basedir;
    conn->file_fd = -1;
    conn->file_size = 0;
    conn->file_bytes_sent = 0;
  }
  return conn;
}

ConnState fs_get_conn_state(Connection *conn) {
  assert(conn);
  return conn ? conn->state : state_end;
}


int fs_get_conn_fd(Connection *conn) { 
  return conn ? conn->conn_fd : -1; 
}


static void send_file(Connection *conn) {
  assert(conn);
  assert(conn->state == state_sending_file);
  if (conn->state != state_sending_file)
    return;
  off_t sz_to_send = conn->file_size - conn->file_bytes_sent;
  ssize_t sz_sent = 0;
  if (sz_to_send > 0) {
    sz_sent = sendfile(conn->conn_fd, conn->file_fd, NULL, sz_to_send);
    if (sz_sent < 0 && errno != EAGAIN) {
      sfl_error("sendfile() error: %s", strerror(errno));
    } else {
      conn->file_bytes_sent += sz_sent;
      sfl_debug("file sent %ld/%ld", conn->file_bytes_sent, conn->file_size);
    }
  }
  if (conn->file_bytes_sent >= conn->file_size) {
    conn->state = state_end;
  }
}

static void start_send_file(Connection *conn) {
  if (!conn)
    return;
  if (conn->file_fd > 0) {
    sfl_debug("fs_start_sending_file()");
    int tcp_cork = 1;
    if (setsockopt(conn->conn_fd, IPPROTO_TCP, TCP_CORK, &tcp_cork, sizeof(tcp_cork)) < 0) {
      sfl_warn("unable to set TCP_CORK option: %s", strerror(errno));
    };
    conn->state = state_sending_file;
    send_file(conn);
  } else {
    conn->state = state_end;
    sfl_debug("fs_start_sending_file() - no file to send");
  }
}


static void send_answer(Connection *conn) {
  assert(conn);
  assert(conn->state == state_sending_answer);
  if (conn && conn->state == state_sending_answer) {
    int sz_to_send = (int)(conn->answer.data_size) - conn->answer_bytes_sent;
    if (sz_to_send > 0) {
      int ret = send(conn->conn_fd, conn->answer.data + conn->answer_bytes_sent, sz_to_send, 0);
      if (ret < 0 && errno != EAGAIN) {
        sfl_error("on send(): %s", strerror(errno));
      } else {
        conn->answer_bytes_sent += ret;
      }
    }
    if (conn->answer_bytes_sent >= conn->answer.data_size) {
      start_send_file(conn);
    }
  }
}


static void start_send_answer(Connection *conn, const char *msg) {
  assert(conn);
  if (!conn || !msg)
    return;
  sfl_debug("fs_start_sending_answer() with msg %s", msg);
  buffer_clear(&conn->answer);
  if (!buffer_append_string(&conn->answer, msg) ||
      !buffer_append_string(&conn->answer, http_request_end)) {
    sfl_fatal("memory allocation error on  %s:%d", __FILE__, __LINE__);
    conn->state = state_end;
    return;
  }
  conn->state = state_sending_answer;
  conn->answer_bytes_sent = 0;
  send_answer(conn);
}


void fs_on_ready_write(Connection *conn) {
  assert(conn);
  if (conn == NULL)
    return;
  sfl_debug("fs_on_ready_write() with state %d", conn->state);
  switch (conn->state) {
  case state_sending_answer:
    send_answer(conn);
    break;
  case state_sending_file:
    send_file(conn);
    break;
  default:
    break;
  };
}


void fs_on_ready_read(Connection *conn) {
  assert(conn);
  if (conn == NULL)
    return;
  sfl_debug("fs_on_ready_read() with state %d", conn->state);
  if (conn->state != state_wait_request) 
    return;
  
  char buf[128];
  int ret = 0;
  while ((ret = recv(conn->conn_fd, buf, sizeof(buf) / sizeof(buf[0]) - 1, 0)) > 0) {
    if (buffer_append(&conn->request, buf, ret) == false) {
      sfl_fatal("memory allocation error on  %s:%d", __FILE__, __LINE__);
      conn->state = state_end;
      return;
    };
  }

  if (!catch_http_request(&conn->request)) {
    return;
  }

  sfl_debug("recieved http request: %s", conn->request.data);

  const char *err_answer =
      parse_http_head(conn->request.data, conn->basedir, &conn->file_fd, &conn->file_size);
  
  if (err_answer) {
    start_send_answer(conn, err_answer);
    return;
  }

  char ok_answer[256];
  snprintf(ok_answer, 256, "%s\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld", msg200,
            conn->file_size);
  start_send_answer(conn, ok_answer);
}


void fs_close_conn(Connection *conn) {
  assert(conn);
  if (conn == NULL)
    return;
  sfl_debug("fs_close_conn");
  shutdown(conn->conn_fd, SHUT_RDWR);
  close(conn->conn_fd);
  if (conn->file_fd > 0) {
    close(conn->file_fd);
    conn->file_fd = -1;
  }
}


void fs_free_conn(Connection *conn) {
  if (conn) {
    buffer_free(&conn->request);
    buffer_free(&conn->answer);
    free(conn);
  }
}