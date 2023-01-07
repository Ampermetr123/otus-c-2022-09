#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  void *data;
  size_t data_size;
  size_t buffer_size;
} Buffer;

bool buffer_append(Buffer *pb, void *data, size_t len);
bool buffer_resize(Buffer *pb, size_t req_size);
void buffer_free(Buffer *pb);