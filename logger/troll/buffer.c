#include "buffer.h"
#include <string.h>

static const size_t gc_initial_buffer_size = 1024;
static const size_t gc_max_buffer_size = 5UL * 1024 * 1024;
static const float gc_buffer_resize_factor = 1.5;


/**
 * @brief Инициализация полей буфера для дальнейшей работы
 */
void buffer_init(Buffer *pb) {
  if (pb) {
    pb->buffer_size = 0;
    pb->data_size = 0;
    pb->data = NULL;
  }
}

Buffer buffer_make(){
  Buffer buf;
  buffer_init(&buf);
  return buf;
}

/**
 * @brief Увеличивает размер памяти буфера. При расширении данные сохраняются.
 * @return false - ошибка памяти; размер буфера и данные остаются, как до вызова функции.
 * @return true, нет ошибок.
 */
bool buffer_extend(Buffer *pb, size_t req_size) {
  if (!pb)
    return false;
  if (pb->buffer_size > req_size) {
    return true;
  }

  // определяем новый размер памяти
  size_t sz = (pb->data == NULL || pb->buffer_size == 0) ? gc_initial_buffer_size : pb->buffer_size;
  while (sz < req_size && sz < gc_max_buffer_size) {
    sz = sz * gc_buffer_resize_factor;
  }
  if (sz > gc_max_buffer_size) {
    return false;
  }

  // первичное выделение памяти
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

/**
 * @brief Расширяет размер буфера, если свободный размер меньше req_size 
 */
bool buffer_reserve(Buffer *pb, size_t req_size){
  return buffer_extend(pb, pb->data_size + req_size);
}


/**
 * @brief возвращает указатель на свобободный участок памяти
 * @return char* 
 */
char* buffer_end(Buffer *pb){
  if (!pb || pb->data_size == pb->buffer_size) return NULL;
  return pb->data + pb->data_size;
}

/**
 * @brief Дополняет буфер данными из памяти *data указанной длинны
 * Если текущего размер буфера не достаточно, то он будет расширен.
 * @return true - ok, false - memory error
 */
bool buffer_append(Buffer *pb, const char *data, size_t size) {
  if (!pb)
    return false;
  if (pb->data == NULL || pb->buffer_size - pb->data_size < size) {
    if (!buffer_extend(pb, pb->data_size + size))
      return false;
  }
  memcpy((char *)(pb->data) + pb->data_size, data, size);
  pb->data_size = pb->data_size + size;
  return true;
}


/** Add data from NULL-terminated string - NULL charcter not stored!*/
bool buffer_append_string(Buffer *pb, const char *str) { return buffer_append(pb, str, strlen(str)); }


bool buffer_push_back(Buffer *pb, char ch) { return buffer_append(pb, &ch, 1); }


void buffer_pop_back(Buffer *pb) {
  if (pb && pb->data_size > 0)
    pb->buffer_size--;
}


void buffer_clear(Buffer *pb) {
  if (pb)
    pb->data_size = 0;
}


void buffer_free(Buffer *pb) {
  if (pb) {
    free(pb->data);
    pb->buffer_size = 0;
    pb->data_size = 0;
  }
}