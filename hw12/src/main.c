#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "buffer.h"

#define SERVER_URL "telehack.com"
#define SERVER_SERVICE "telnet"

// Параметры командной строки (значения по умолчанию)
const char *text = "C PO*DECTBOM!";
const char *font = "";

void print_usage(char *prog_name) {
  fprintf(stdout,
          "ASCII art text generator \n"
          "Usage: %s [-f <font>] [-t <text>]\n"
          "-f <font> - optional font name \n"
          "-t <text> - text to convert, default %s\n",
          prog_name, text);
}

int main(int argc, char *argv[]) {
  // разбор коммандной строки
  int opt;
  while ((opt = getopt(argc, argv, "f:t:h")) != -1) {
    switch (opt) {
    case 'f':
      font = optarg;
      break;
    case 't':
      text = optarg;
      break;
    case 'h':
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    default: /* '?' */
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  };

  struct addrinfo hints, *addr_info;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG;


  int res = getaddrinfo(SERVER_URL, SERVER_SERVICE, &hints, &addr_info);
  if (res != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
    exit(EXIT_FAILURE);
  }

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }

  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) != 0) {
    perror("setsockopt:");
    close(sock);
  }

  if (connect(sock, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
    perror("on connect: ");
    close(sock);
    freeaddrinfo(addr_info);
    return EXIT_FAILURE;
  }

  // Чтение до приглашения
  {                               
    uint8_t buf[2] = {'\n', '.'}; // строка приглашение
    const int buf_sz = sizeof(buf) / sizeof(char);
    int i = 0;
    uint8_t ch;
    uint tryes = 3;
    int count = 0;
    while (tryes > 0) {
      int r = recv(sock, &ch, 1, 0);
      count += r;
      if (r == 1) {
        i = (buf[i] == ch) ? i + 1 : 0;
        if (i == buf_sz)
          break;
      } else {
        tryes--;
      }
    }
    if (tryes == 0) {
      fprintf(stderr, "отстутствует приглашение от сервера\n");
      goto onerror;
    }
  }

  // Запрос
  const char *cmd = "figlet";
  int req_len = strlen(cmd) + 2 + strlen(font) + 1 + strlen(text) + 2 + 1;
  char *req = (char *)malloc(req_len);
  if (!req) {
    fprintf(stderr, "Ошибка памяти");
    goto onerror;
  }
  if (strlen(font))
    req_len = snprintf(req, req_len, "%s /%s %s\r\n", cmd, font, text);
  else
    req_len = snprintf(req, req_len, "%s %s\r\n", cmd, text);
  int sent = 0;
  do {
    sent = send(sock, req + sent, req_len - sent, 0);
    if (sent < 0 && errno != EINTR) {
      perror("send");
      break;
    }
  } while (sent < req_len - sent);
  free(req);
  if (sent < req_len) {
    goto onerror;
  }

  // Ответ
  Buffer answer = {NULL, 0, 0};
  uint8_t buf[1024];
  int ret;
  int received = 0;
  do {
    ret = recv(sock, buf, 1024, 0);
    if (ret > 0) {
      if (received > req_len) {
        if (!buffer_append(&answer, buf, ret)) {
          fprintf(stderr, "Memory error");
          goto onerror;
        };
      } else if (received + ret > req_len) {
        if (!buffer_append(&answer, buf + req_len - received, ret - req_len + received)) {
          fprintf(stderr, "Memory error");
          goto onerror;
        }
      }
      received += ret;
    }
  } while (ret >= 0 || (ret < 0 && errno == EINTR));

  if (answer.data_size > 1) {
    ((char *)(answer.data))[answer.data_size - 1] = '\0'; // заменяем точку концом строки
    puts(answer.data);
  } else {
    fprintf(stderr, "no answer from " SERVER_URL "\n");
    goto onerror;
  }
  buffer_free(&answer);
  shutdown(sock, SHUT_RDWR);
  close(sock);
  freeaddrinfo(addr_info);
  return EXIT_SUCCESS;

onerror:
  buffer_free(&answer);
  shutdown(sock, SHUT_RDWR);
  close(sock);
  freeaddrinfo(addr_info);
  return EXIT_FAILURE;
}
