/**
 * @file buffer.h
 * @author Sergey Simonov
 * @brief Расширяемый буфер
 */

#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  char *data;
  unsigned long data_size;   /// размер полезных данных
  unsigned long buffer_size; /// размер выделенной памяти
} Buffer;

Buffer buffer_make();

void buffer_init(Buffer *pb);

bool buffer_extend(Buffer *pb, size_t req_size);

bool buffer_reserve(Buffer *pb, size_t req_size);

bool buffer_append(Buffer *pb, const char *data, size_t len);

bool buffer_append_string(Buffer *pb, const char *str);

char* buffer_end(Buffer *pb);

bool buffer_push_back(Buffer *pb, char ch);

void buffer_pop_back(Buffer *pb);

void buffer_clear(Buffer *pb);

void buffer_free(Buffer *pb);