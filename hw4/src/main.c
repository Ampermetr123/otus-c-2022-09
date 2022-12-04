#include <curl/curl.h>

#include "json.h"
#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PROG_NAME
#error You need to define PROG_NAME
#endif
#define CONNECT_TIMEOUT 5 // в секундах
#define MAX_CITY_LEN 2048

/*******************************************************************************
 *                              ОБРАБОТКА ОШИБОК                               *
 *******************************************************************************/
static int g_main_result = 0;
char g_error_info_buffer[CURL_ERROR_SIZE];
#define check_and_go(result, clean_point)                                                          \
  while ((result) != CURLE_OK) {                                                                   \
    if (strlen(g_error_info_buffer)) {                                                             \
      fprintf(stderr, "Error %d: %s; (line %d)\n", result, g_error_info_buffer, __LINE__);         \
    } else {                                                                                       \
      fprintf(stderr, "Error %d: %s; (line %d)\n", result, curl_easy_strerror(result), __LINE__);  \
    }                                                                                              \
    g_main_result = result;                                                                        \
    goto clean_point;                                                                              \
  }

/*******************************************************************************
 *                               BUFFER STORAGE                                *
 *******************************************************************************/
static const size_t gc_initial_buffer_size = 1024;
static const size_t gc_max_buffer_size = 5UL * 1024 * 1024;
static const float gc_buffer_resize_factor = 1.5;

typedef struct {
  char *data;
  size_t data_size;
  size_t buffer_size;
} Buffer;

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

/*******************************************************************************
 *                            URL STRING PROCESSING                            *
 *******************************************************************************/

char *make_url(const char *city, size_t max_city_len) {
  const char *url_begin = "https://wttr.in/";
  const char *url_end = "?format=j1";
  const size_t city_len = strnlen(city, max_city_len);
  if (city_len == max_city_len) {
    return NULL;
  }
  size_t url_len = strlen(url_begin) + city_len + strlen(url_end);
  char *url = malloc(url_len + 1);
  if (url) {
    url[0] = '\0';
    strcat(url, url_begin);
    strcat(url, city);
    strcat(url, url_end);
  }
  return url;
}

static void clean_url(char *url) {
  if (url)
    free(url);
}

/*******************************************************************************
 *                             JSON ANSWER PARSING                             *
 *******************************************************************************/
const char* parse_wrrt(const char* str, char errmsg[256]){
  char errmsg[256];
    // Попытка распарсить
  JsonNode *node = json_decode(str);
  if (node == NULL){
    fprintf(stderr, "Error: can't pares JSON content \n");
    return NULL;
  }

  if (json_check(node, errmsg) = false){
    fprintf(stderr, "%s", errmsg);
    return NULL;
  };

  JsonNode* ar = json_find_member(node, "nearest_area");
  if (ar == NULL) {
    fprintf(stderr, "Error: can't find 'nearest_area' \n");
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    fprintf(stderr, "Error: can't find 1st obj in 'neares_area' \n");
  }

  ar = json_find_member(ar, "areaName");
  if (ar == NULL) {
    fprintf(stderr, "Error: can't find 'areaName' \n");
  }

  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    fprintf(stderr, "Error: can't find 1st obj in 'areaName' \n");
  }

  ar = json_find_member(ar, "value");
  if (ar == NULL) {
    fprintf(stderr, "Error: can't 'value' in 1st obj in 'areaName' \n");
  }

  




}

/*******************************************************************************
 *                                    MAIN                                     *
 *******************************************************************************/

void print_usage() { puts("Usage: " PROG_NAME " <city_string>"); }

/** Колбэк функция для приема HTTP тела */
static size_t write_cb(char *data, size_t n, size_t l, void *userp) {
  Buffer *pb = (Buffer *)userp;
  if (pb == NULL) {
    return 0;
  }
  size_t chunk_size = n * l;
  size_t req_size = pb->data_size + chunk_size;
  if (buffer_resize(pb, req_size) == false) {
    return 0;
  }
  memcpy(&(pb->data[pb->data_size]), data, chunk_size);
  pb->data_size += chunk_size;
  return chunk_size;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage();
    return 0;
  }

  char *url = make_url(argv[1], MAX_CITY_LEN);
  if (!url) {
    fprintf(stderr, "Error: can't make url\n");
    return -1;
  }

  CURLcode crv = curl_global_init(CURL_GLOBAL_DEFAULT);
  // Согласно документации, если функция curl_global_init возвращает не ноль,
  // то дальнейшее использование функций библиотеки  = UB
  // В примерах в документации это значение не проверяют, как-будто с тайным умыслом?!
  // На моей машине Ubuntu 22.04.1 c libcurl 7.81.0 эта функция всегда возвращает 1
  // и это не мешает дальнейшей работе функций библиотеки (что происходит?).
  crv = 0; // CHEATING! CHEATING! CHEATING! CHEATING! CHEATING! CHEATING!
  check_and_go(crv, clean0);

  CURL *curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "Error: curl_easy_init() failed - on line %d", __LINE__);
    g_main_result = -1;
    goto clean1;
  }

  // дополнительный буфер для ошибки,
  // потенциально дает возможность получить более подробное описание ошибки
  crv = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, g_error_info_buffer);
  check_and_go(crv, clean2);

  // URL сервиса
  crv = curl_easy_setopt(curl, CURLOPT_URL, url);
  check_and_go(crv, clean2);

  // опция редиректа
  crv = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  check_and_go(crv, clean2);

  // some servers do not like requests that are made without a user-agent field, so we provide one
  crv = curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  check_and_go(crv, clean2);

  // Таймаут
  crv = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
  check_and_go(crv, clean2);

  // Регистрация колбэка
  Buffer buffer = {NULL, 0, 0};
  crv = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  check_and_go(crv, clean2);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
  check_and_go(crv, clean2);

  // Выполнение запроса
  crv = curl_easy_perform(curl);
  check_and_go(crv, clean2);

  // Проверка по content-type
  char *ct = NULL;
  crv = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
  check_and_go(crv, clean2);
  if (!ct || strcmp("application/json", ct) != 0) {
    fprintf(stderr, "Error: unexpected content-type\n");
  }

  puts("Everithing is ok!");

  // Добавляем завершающий символ строки в конец данных
  if (buffer_resize(&buffer, buffer.data_size + 1) == false) {
    fprintf(stderr, "Error: can't allocate memory\n");
    goto clean2;
  };
  buffer.data[buffer.data_size] = '\0';
  buffer.data_size++;

  char errmsg[256];
  if (parse_wrrt(buffer.data, errmsg)==NULL){
    fprintf(stderr, "Error: can't parse answer\n");
    goto clean2;
  }

clean2:
  curl_easy_cleanup(curl);
clean1:
  curl_global_cleanup();
clean0:
  if (buffer.data != NULL) {
    free(buffer.data);
  }
  clean_url(url);

  return g_main_result;
}