#include <stdlib.h>
#include <string.h>

#include "buffer.h"

/*******************************************************************************
 *                               BUFFER STORAGE                                *
 *******************************************************************************/
static const size_t gc_initial_buffer_size = 1024;
static const size_t gc_max_buffer_size = 5UL * 1024 * 1024;
static const float gc_buffer_resize_factor = 1.5;



/** Расширяет память в Buffer под требуемый размер данных req_size */
bool buffer_resize(Buffer *pb, size_t req_size) {
  if (!pb)
    return false;
  if (pb->buffer_size > req_size) {
    return true;
  }

  // Определяем новый размер памяти
  size_t sz = (pb->data == NULL || pb->buffer_size == 0) ? gc_initial_buffer_size : pb->buffer_size;
  while (sz < req_size && sz < gc_max_buffer_size) {
    sz = sz * gc_buffer_resize_factor;
  }
  if (sz > gc_max_buffer_size) {
    return false;
  }

  // Первичное выделение памяти
  if (pb->data == NULL) {
    pb->data = malloc(sz);
    if (pb->data == NULL) {
      pb->buffer_size = 0;
      return false;
    }
    pb->buffer_size = sz;
    return true;
  }

  // расширение c запасом
  void *ptr = realloc(pb->data, sz);
  if (ptr != NULL) {
    pb->buffer_size = sz;
    pb->data = ptr;
    return true;
  }

  // расширение без запаса
  ptr = realloc(pb->data, req_size);
  if (ptr == NULL) {
    return false;
  }
  pb->buffer_size = req_size;
  pb->data = ptr;
  return true;
}

bool buffer_append(Buffer *pb, void *data, size_t size){
  if (!pb)
    return false;
  if (pb->data == NULL || pb->buffer_size - pb->data_size < size){
    if (!buffer_resize(pb, pb->data_size + size)) return false;
  }
  memcpy(pb->data+pb->data_size, data, size);
}

void buffer_free(Buffer *pb){
  if (pb) {
    free(pb->data);
    pb->buffer_size=0;
    pb->data_size=0;
  }
}