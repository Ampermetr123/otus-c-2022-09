#ifndef __PARSER_H__
#define __PARSER_H__

#include <stddef.h>
#include <stdbool.h>
#define ERR_LEN 256

bool parse_wrrt(const char *json_str, char *out, size_t out_sz, char errmsg[ERR_LEN]);

#endif // __PARSER_H__