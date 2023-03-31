#pragma once

#include <stdbool.h>
#include <stdint.h>


bool parse_ipv4_endpoint(const char *str, uint32_t *addr, uint16_t *port, char **straddr);

int make_bound_socket(const char* bind_address);

bool make_nonblock(int sock);
