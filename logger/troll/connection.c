#include "connection.h"
#include "buffer.h"
#include "dprint.h"
#include "html.h"
#include "sock_helpers.h"

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
#include <unistd.h>

struct Connection {
  ConnState state;
  int conn_fd;
  Buffer request;
  Buffer answer;
  size_t answer_bytes_sent;
  CQueue *storage;
  cq_iterator_t cq_iter;
  ConnState next_state;
};

static bool start_send_answer(Connection *conn, ConnState state_on_finih);
static void send_storage(Connection *conn);
static void on_http_request(Connection *conn);
static void send_error(Connection *conn, const char *err_answer);


static const char *msg200 = "HTTP/1.1 200 ОК";
// static const char *msg403 = "HTTP/1.1 403 Forbidden\r\nConnection: close";
// static const char *msg404 = "HTTP/1.1 404 Not Found\r\nConnection: close";
static const char *msg405 = "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
// static const char *msg414 = "HTTP/1.1 414 URI Too Long\r\nConnection: close";
static const char *msg500 = "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n";
static const char *msg505 = "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n";
static const char *http_request_end = "\r\n\r\n";
static const int http_request_end_len = 4;


static Connection *connections[MAX_CONNECTIONS] = {NULL};
static int conn_count = 0;

void troll_close_conn(Connection *conn);
void troll_free_conn(Connection *conn);

int get_conn_index(Connection *conn) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i] == conn) {
      return i;
    }
  }
  return -1;
}

void check_storage(CQueue *cq) {
  int storage_len = cq_length(cq);
  dprint("Checking iteration on storage with length %d", storage_len);
  int i = 0;
  unsigned int sz;
  cq_iterator_t it = cq_begin(cq);
  while (it != NULL) {
    int ret = cq_get(cq, it, NULL, &sz);
    if (ret == cq_res_bad_iterator) {
      dprint("iterator error");
      return;
    }
    dprint("record %d of size %d", i, sz);
    i++;
    if (i > storage_len + 2) {
      dprint("storage corrupted!");
      break;
    }
    it = cq_next(cq, it);
  };
}
// bool troll_any_conn_of_state(ConnState state) {
//   for (int i = 0; i < MAX_CONNECTIONS; i++) {
//     if (connections[i] && connections[i]->state == state) {
//       return true;
//     }
//   }
//   return false;
// }

bool troll_any_conn_send_storage() {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i] &&
        (connections[i]->state == state_send_storage || connections[i]->next_state == state_send_storage)) {
      return true;
    }
  }
  return false;
}

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


/**
 * @brief Проверяет протокол и метод
 * @param httphead - NULL-терминатеd HTTP запрос
 * @return NULL, при успехе или указатель на строку с http ответом об ошибке
 */

// static const char *parse_http_head(char *request) {
//   char *method, *target, *httpver;
//   if (!split_http_head(request, &method, &target, &httpver)) {
//     return msg500;
//   }
//   if (strcmp(httpver, "HTTP/1.1") != 0) {
//     return msg505;
//   }
//   if (strcmp(method, "GET") != 0) {
//     return msg405;
//   }
//   return NULL;
// }


/*******************************************************************************
 *                            CONNECTION PROCESSING                            *
 *******************************************************************************/


Connection *troll_new_conn(int conn_fd, CQueue *storage) {
  Connection *conn = calloc(1, sizeof(Connection));
  if (conn == NULL)
    return NULL;
  conn->state = state_wait_request;
  conn->conn_fd = conn_fd;
  buffer_init(&conn->answer);
  buffer_init(&conn->request);
  conn->answer_bytes_sent = 0;
  conn->storage = storage;
  conn->cq_iter = NULL;
  conn->next_state = state_end;
  return conn;
}

ConnState troll_get_conn_state(Connection *conn) {
  assert(conn);
  return conn ? conn->state : state_end;
}


// int fs_get_conn_fd(Connection *conn) {
//   return conn ? conn->conn_fd : -1;
// }


int troll_accept_connections(int listen_fd, int epoll_fd, CQueue *storage) {
  //dprint("in accept_connecion()");
  int conn_fd = 0;
  int new_conn_count = 0;
  while ((conn_fd = accept(listen_fd, NULL, NULL)) > 0) {
    if (make_nonblock(conn_fd) == false) {
      shutdown(conn_fd, SHUT_RDWR);
      close(conn_fd);
      continue;
    }
    int i = get_conn_index(NULL);
    new_conn_count += 1;
    Connection *conn;
    if (i < 0 || (conn = troll_new_conn(conn_fd, storage)) == NULL) {
      shutdown(conn_fd, SHUT_RDWR);
      close(conn_fd);
      continue;
    }
    connections[i] = conn;
    conn_count += 1;

    struct epoll_event conn_ev;
    conn_ev.data.ptr = (void *)conn;
    conn_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &conn_ev) < 0) {
      dprint("error on add listen socket event to epoll: %s", strerror(errno));
      troll_terminate_connection(&conn_ev, epoll_fd);
    }
  }
  if (conn_count == 0 && conn_count != MAX_CONNECTIONS) {
    dprint("accept error: %s", strerror(errno));
  }

  return conn_count;
}

void append_message(Connection *conn, LogMsg *msg, unsigned len) {
  if (!buffer_reserve(&conn->answer, len + 64)) {
    dprint("memory allocation error on  %s:%d", __FILE__, __LINE__);
    return;
  };
  if (buffer_append_string(&conn->answer, "data: ") ){
    int prefix_len = make_prefix(msg->level, msg->sp, buffer_end(&conn->answer), len + 64);
    conn->answer.data_size += prefix_len;
    if (buffer_append_string(&conn->answer, msg->body) && 
        buffer_append_string(&conn->answer, "\n\n")) {
      return;
    }
  }
  dprint("memory allocation error on  %s:%d", __FILE__, __LINE__);
  buffer_clear(&conn->answer);
}

int troll_terminate_connection(struct epoll_event *ev, int epoll_fd) {
  Connection *conn = ev->data.ptr;
  int i = get_conn_index(conn);
  if (conn && i >= 0) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->conn_fd, ev);
    connections[i] = NULL;
    troll_close_conn(conn);
    troll_free_conn(conn);
    conn_count -= 1;
  }
  return conn_count;
}

void troll_terminate_all_connections(int epoll_fd) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    Connection *conn = connections[i];
    if (conn) {
      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->conn_fd, NULL);
      troll_close_conn(conn);
      troll_free_conn(conn);
      connections[i] = NULL;
      conn_count -= 1;
    }
  }
}

// static void send_file(Connection *conn) {
//   assert(conn);
//   assert(conn->state == state_sending_file);
//   if (conn->state != state_sending_file)
//     return;
//   off_t sz_to_send = conn->file_size - conn->file_bytes_sent;
//   ssize_t sz_sent = 0;
//   if (sz_to_send > 0) {
//     sz_sent = sendfile(conn->conn_fd, conn->file_fd, NULL, sz_to_send);
//     if (sz_sent < 0 && errno != EAGAIN) {
//       sfl_error("sendfile() error: %s", strerror(errno));
//     } else {
//       conn->file_bytes_sent += sz_sent;
//       sfl_debug("file sent %ld/%ld", conn->file_bytes_sent, conn->file_size);
//     }
//   }
//   if (conn->file_bytes_sent >= conn->file_size) {
//     conn->state = state_end;
//   }
// }

static void start_send_storage(Connection *conn) {
  buffer_clear(&conn->answer);
  bool buffer_ret = buffer_append_string(&conn->answer, msg200) &&
                    buffer_append_string(&conn->answer, "\r\nCache-Control: no-store") &&
                    buffer_append_string(&conn->answer, "\r\nContent-Type: text/event-stream") &&
                    buffer_append_string(&conn->answer, http_request_end);
  if (buffer_ret == false) {
    send_error(conn, msg500);
    return;
  }
  // dprint("in send_storate()");
  // check_storage(conn->storage);
  conn->cq_iter = cq_begin(conn->storage);
  if (conn->cq_iter == NULL) {
    conn->state = state_send_new_msgs;
    return;
  }
  conn->state = state_send_storage;
  start_send_answer(conn, state_send_storage);
  send_storage(conn);
}


static void send_storage(Connection *conn) {
  if (conn->state != state_send_storage) {
    dprint("state machine error on send_storage, when state is %d", conn->state);
  };

  while (conn->cq_iter != NULL) {
    char *logmsg;
    unsigned logmsg_len;
    if (cq_get(conn->storage, conn->cq_iter, &logmsg, &logmsg_len) != cq_res_ok) {
      dprint("sotrage corupted!");
      conn->state = state_end;
      return;
    };
    conn->cq_iter = cq_next(conn->storage, conn->cq_iter);
    append_message(conn, (LogMsg *)logmsg, logmsg_len);
    bool sent = start_send_answer(conn, conn->cq_iter == NULL ? state_send_new_msgs : state_send_storage);
    if (!sent){
      break; // send_storage() will be called later
    }
  }
}

/// return true, if all data sent without EAGAIN
/// return false, if some data needs will be sent later
static bool send_answer(Connection *conn) {
  if (!conn || conn->state != state_sending_answer) {
    dprint("state machine error on send_answer");
    return true;
  }
  int sz_to_send = (int)(conn->answer.data_size) - conn->answer_bytes_sent;
  if (sz_to_send > 0) {
    int ret = send(conn->conn_fd, conn->answer.data + conn->answer_bytes_sent, sz_to_send, 0);
    if (ret < 0 && errno != EAGAIN) {
      dprint("on send(): %s", strerror(errno));
      conn->state = conn->next_state;
      conn->next_state = state_end;
      buffer_clear(&conn->answer);
      return true;
    } else {
      conn->answer_bytes_sent += ret;
      dprint("sent %d bytes", ret);
    }
  }
  if (conn->answer_bytes_sent >= conn->answer.data_size) {
    buffer_clear(&conn->answer);
    conn->state = conn->next_state;
    conn->next_state = state_end;
    //dprint("switch to state %d", conn->state);
    return true;
  }
  return false;
}

// return true, if all data sent without EAGAIN, stat is state_send_answer
// return false, if some data needs will be sent later, state is state_on_finih
static bool start_send_answer(Connection *conn, ConnState state_on_finih) {
  if (conn->state != state_sending_answer) {
    conn->state = state_sending_answer;
    conn->answer_bytes_sent = 0;
  }
  conn->next_state = state_on_finih;
  return send_answer(conn);
}


void troll_on_ready_write(Connection *conn) {
  assert(conn);
  if (conn == NULL)
    return;
  //dprint("troll_on_ready_write() with state %d", conn->state);
  switch (conn->state) {
  case state_sending_answer:
    send_answer(conn);
    break;
  case state_send_storage:
    send_storage(conn);
    break;
  case state_send_new_msgs:
  case state_wait_request:
  case state_end:
    break;
  default:
    break;
  };
}


void troll_on_ready_read(Connection *conn) {
  assert(conn);
  if (conn == NULL)
    return;
  if (conn->state != state_wait_request)
    return;
  char buf[128];
  int ret = 0;
  while ((ret = recv(conn->conn_fd, buf, sizeof(buf) / sizeof(buf[0]) - 1, 0)) > 0) {
    if (buffer_append(&conn->request, buf, ret) == false) {
      conn->state = state_end;
      return;
    };
  }
  if (catch_http_request(&conn->request)) {
    on_http_request(conn);
  }
}

static void send_error(Connection *conn, const char *err_answer) {
  if (!err_answer)
    return;
  buffer_clear(&conn->answer);
  if (!buffer_append_string(&conn->answer, err_answer)) {
    dprint("memory allocation error on  %s:%d", __FILE__, __LINE__);
    return;
  }
  start_send_answer(conn, state_end);
}

static void method_get_handler(Connection *conn, const char *target) {
  if (strcmp(target, "/events") == 0) {
    if (conn->state != state_send_new_msgs && conn->next_state != state_send_new_msgs) {
      // Выгурзука events
      start_send_storage(conn);
    }
  } else {
    // Выгрузка основной страницы
    char lenstr[64];
    snprintf(lenstr, 64, "\r\nContent-Length: %ld", strlen(html_index));
    bool buffer_ret = buffer_append_string(&conn->answer, msg200) &&
                      buffer_append_string(&conn->answer, "\r\nContent-Type: text/html") &&
                      buffer_append_string(&conn->answer, lenstr) &&
                      buffer_append_string(&conn->answer, http_request_end) &&
                      buffer_append_string(&conn->answer, html_index);
    if (!buffer_ret) {
      dprint("memory allocation error on  %s:%d", __FILE__, __LINE__);
      send_error(conn, msg500);
    }
    start_send_answer(conn, state_wait_request);
  }
}

static void method_post_handler(Connection *conn) { dprint("TODO: processing POST method"); }


static void on_http_request(Connection *conn) {
  // dprint("HTTP:request:\n %s", conn->request.data);
  char *method, *target, *httpver;
  if (!split_http_head(conn->request.data, &method, &target, &httpver)) {
    send_error(conn, msg500);
    buffer_clear(&conn->request);
    return;
  }
  if (strcmp(httpver, "HTTP/1.1") != 0) {
    send_error(conn, msg505);
    buffer_clear(&conn->request);
    return;
  }
  dprint("HTTP request: method %s, target %s", method, target);
  if (strcmp(method, "GET") == 0) {
    method_get_handler(conn, target);
  } else if (strcmp(method, "POST") != 0) {
    method_post_handler(conn);
  } else {
    send_error(conn, msg405);
  }
  buffer_clear(&conn->request);
}


void troll_on_new_messge(LogMsg *msg, unsigned len) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i] && 
     (connections[i]->state == state_send_new_msgs ||
     connections[i]->next_state == state_send_new_msgs))
    {
      append_message(connections[i], msg, len);
      start_send_answer(connections[i], state_send_new_msgs);
    }
  }
}


void troll_close_conn(Connection *conn) {
  if (conn) {
    shutdown(conn->conn_fd, SHUT_RDWR);
    close(conn->conn_fd);
    dprint("connection %d closed", conn->conn_fd);
  }
}


void troll_free_conn(Connection *conn) {
  if (conn) {
    buffer_free(&conn->request);
    buffer_free(&conn->answer);
    free(conn);
  }
}