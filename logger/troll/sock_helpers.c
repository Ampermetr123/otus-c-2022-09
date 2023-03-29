#include "sock_helpers.h"
#include "dprint.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * @brief Конвертация строки общего вида в сетевые адрес и порт
 *
 * @param [in] str  строка вида x.x.x.x:yyyy  или x.x.x.x
 * @param [out] addr сетевой адрес
 * @param [out] port порт
 * @param [out] straddr , если не NULL, то содержит строкове представление сетевого адреса (вызывающая
 * сторона отвечает за очистку памяти под строку)
 * @return true , если парсинг успешный
 * @return false, если ошибка.
 * @note функция сообщает об ошибках через логгер.
 */
bool parse_ipv4_endpoint(const char *str, uint32_t *addr, uint16_t *port, char **straddr) {
  char *str_copy = strdup(str);
  char *token = strtok(str_copy, ":");
  if (token != NULL) { // no delim - just address
    if (inet_pton(AF_INET, token, (void *)(addr)) != 1) {
      dprint("error read IPv4 address from string '%s'", str_copy);
      free(str_copy);
      return false;
    }
    if (straddr) {
      *straddr = strdup(token);
    }
  }

  token = strtok(NULL, ":");
  if (token != NULL) {
    char *end;
    long long_port = strtol(token, &end, 0);
    if (end == token || long_port > UINT16_MAX || long_port < 0) {
      dprint("error read port from string '%s'", str);
      free(str_copy);
      return false;
    }
    *port = htons((uint16_t)long_port);
  } else {
    *port = 0; // any port
  }
  free(str_copy);
  return true;
}


/**
 * @brief Создает и 'привязанный' сокет
 * @param bind_address строка вида x.x.x.x:yyyy или x.x.x.x или NULL
 * @return true, on success
 * @note check errno for error details
 */
int make_binded_socket(const char *bind_address) {
  uint32_t addr = htonl(INADDR_ANY);
  uint16_t port = 0;

  if (bind_address) {
    char *straddr;
    if (parse_ipv4_endpoint(bind_address, &addr, &port, &straddr) == false) {
      return -1;
    }
    dprint("binding to address: %s", straddr);
    dprint("binding to port: %d", ntohs(port));
    free(straddr);
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    dprint("failed to create socket: %s", strerror(errno));
    return sock;
  }

  struct sockaddr_in servaddr = {0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = addr;
  servaddr.sin_port = port;

  if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    dprint("failed to bind socket: %s", strerror(errno));
    close(sock);
    return -1;
  }
  return sock;
}


bool make_nonblock(int fd) {
  int opts = fcntl(fd, F_GETFL);
  if (opts < 0) {
    dprint("fcntl(F_GETFL) %s", strerror(errno));
    return false;
  }
  opts = (opts | O_NONBLOCK);
  if (fcntl(fd, F_SETFL, opts) < 0) {
    dprint("fcntl(F_SETFL) %s", strerror(errno));
    return false;
  }
  return true;    
}