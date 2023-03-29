#pragma once

#include <time.h>
#include <stdbool.h>

typedef struct {
  struct timespec sp;
  short level;
  char body[];
} LogMsg;

typedef struct {
  const char *format;
  const char *name;
  bool active;
  size_t max_body_len;
} LevelInfo;

void init_levels();

LevelInfo* register_level(int level, const char* level_name, const char* level_format);

LevelInfo* level_info(int level);

int make_prefix(int level, struct timespec sp, char *buf, size_t buf_sz);