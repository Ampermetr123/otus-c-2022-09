#include "demonize.h"
#include "single_process.h"

#include <ciniparser.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

#ifndef PROG_NAME
#define PROG_NAME "mydaemon"
#endif
#define LOCKFILE "./" PROG_NAME ".pid"
#define DEFAULT_CONFIG_FILE PROG_NAME ".ini"
#define DEFAULT_SOCK_NAME "/tmp/" PROG_NAME

// команда запроса размера файла
#define CMD_GET_FSIZE "size"
#define CMD_GET_FSIZE_LEN 5

// размер буфер для приема команд и передачи данных
#define BUF_SIZE 32

// глобальная константа определяет режим демона
bool gb_daemon = false;

#define print_error(STR, ...)                                                                      \
  if (gb_daemon)                                                                                   \
    syslog(LOG_ERR, STR, __VA_ARGS__);                                                             \
  else                                                                                             \
    fprintf(stderr, STR "\n", __VA_ARGS__)


void print_usage(const char *prog_name) {
  fprintf(stdout,
          "Usage: %s [-d] [-c <config_file>]\n"
          " -d - run as a daemon\n -c <config_file> path to config file, default mydeaemon.cfg",
          prog_name);
}

long file_size(const char *file) {
  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    print_error("ошибка доступа к файлу (open) %s: %s", file, strerror(errno));
    return -1;
  }
  struct stat st;
  if (fstat(fd, &st) == -1) {
    print_error("ошибка получения свойств файла (fstat) %s: %s", file, strerror(errno));
    close(fd);
    return -1;
  }
  close(fd);
  return st.st_size;
}

bool is_command_get_file_size(char *buf, int sz) {
  return (sz == CMD_GET_FSIZE_LEN && strncmp(CMD_GET_FSIZE, buf, CMD_GET_FSIZE_LEN) == 0);
}

void on_command_get_file_size(int ssock, const char *target) {
  char buf[BUF_SIZE];
  long sz = file_size(target);
  if (sz > 0) {
    sprintf(buf, "%ld", sz);
    size_t buf_size = strlen(buf) + 1;
    size_t bytes_sent = 0;
    while (bytes_sent < buf_size) {
      int ret = send(ssock, buf + bytes_sent, buf_size, bytes_sent);
      if (ret < 0 && errno != EINTR) {
        print_error("Ошибка передачи данных в сокет: %s", strerror(errno));
      }
      bytes_sent += ret;
    }
  }
}

volatile bool g_run_flag = true;

void on_terminate(int signo) {
  (void)(signo); // supress warning unused
  g_run_flag = false;
  //psignal(signo,"Received:");
}


int main(int argc, char *argv[]) {

  // контроль единственности
  int err = 0;
  long ret = single_process(LOCKFILE, &err);
  if (ret == -1) {
    fprintf(stderr, "Ошибка контроля уникльности процесса по lock-файлу %s: %s\n", LOCKFILE,
            strerror(err));
  } else if (ret > 0) {
    fprintf(stderr, "Ошибка: экземпляр программы PID=%ld уже запущен\n", ret);
    return EXIT_FAILURE;
  }

  // разбор коммандной строки
  const char *cfg_path = DEFAULT_CONFIG_FILE;
  int opt;
  while ((opt = getopt(argc, argv, "dc:")) != -1) {
    switch (opt) {
    case 'd':
      gb_daemon = true;
      break;
    case 'c':
      cfg_path = optarg;
      break;
    default: /* '?' */
      print_usage(argv[0]);
      exit(-1);
    }
  }

  // демонизация
  if (gb_daemon) {
    if (daemonize(PROG_NAME) == false) {
      print_error("%s", "Ошбика запуска программы в качестве демона");
    }
  }

  // забрать параметры из конфигурационного файла
  char *target = NULL;
  char *sock_name = DEFAULT_SOCK_NAME;
  dictionary *dict;
  if ((dict = ciniparser_load(cfg_path)) == NULL) {
    print_error("Ошибка чтения файла конфигурации %s", cfg_path);
    ciniparser_freedict(dict);
    return EXIT_FAILURE;
  }
  if ((target = ciniparser_getstring(dict, "common:target", NULL)) == NULL) {
    print_error("Не найден параметр target в секции common в файле %s", cfg_path);
    ciniparser_freedict(dict);
    return EXIT_FAILURE;
  }
  char *sn;
  if ((sn = ciniparser_getstring(dict, "common:socket_name", NULL)) != NULL) {
    sock_name = sn;
  }

  // создаем сокет сервера
  int lsock;
  if ((lsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    print_error("Ошибка создания сокета %s: %s", sock_name, strerror(errno));
    ciniparser_freedict(dict);
    return EXIT_FAILURE;
  }
  struct sockaddr_un un;
  bzero(&un, sizeof(un));
  un.sun_family = AF_UNIX;
  strncpy(un.sun_path, sock_name, strnlen(sock_name, 104));
  size_t un_size = SUN_LEN(&un);
  unlink(sock_name);
  if (bind(lsock, (struct sockaddr *)&un, un_size) < 0) {
    print_error("Ошибка привязки (bind) сокета %s: %s", sock_name, strerror(errno));
    ciniparser_freedict(dict);
    return EXIT_FAILURE;
  }
  const int query_size = 5;
  if (listen(lsock, query_size) < 0) {
    print_error("Ошибка прослушки (listen) сокета %s: %s", sock_name, strerror(errno));
    ciniparser_freedict(dict);
    return EXIT_FAILURE;
  };

  // установка сигналов для вежливого завершения работы
  g_run_flag = true;
  struct sigaction act = { 0 };
  act.sa_handler = on_terminate;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (SIGINT, &act, NULL);
  sigaction (SIGTERM, &act, NULL);


  // цикл обработки сервера
  while (g_run_flag) {
    int ssock;
    do {
      ssock=accept(lsock, 0, 0);
    } while (ssock<0 && errno==EINTR && g_run_flag == true);
    
    if (ssock == -1 && errno!=EINTR) {
      print_error("Ошибка установки соединения (accept) сокета %s: %s", sock_name, strerror(errno));
      break;
    }
    if (g_run_flag == false){
      break;
    }

    char buf[BUF_SIZE];
    int rval;
    do {
      rval = read(ssock, buf, BUF_SIZE - 1);
    } while (rval < 0 && errno == EINTR && g_run_flag == true);

    if (rval < 0) {
      print_error("Ошибка чтения данных из сокета %s: %s", sock_name, strerror(errno));
    } else if (rval > 0) {
      // удаляем перевод строки в конце, если имеется (для терминальной сессии netcat)
      if (buf[rval - 1] == '\n') {
        buf[rval - 1] = '\0';
      }
      if (is_command_get_file_size(buf, rval)) {
        on_command_get_file_size(ssock, target);
      } else {
        buf[rval] = '\0';
        print_error("Получена неизвестная команда: [%s]", buf);
      }
    }
    close(ssock);
  }

  close(lsock);
  ciniparser_freedict(dict);
  return EXIT_SUCCESS;
}