#ifndef __TROLL_H__
#define __TROLL_H__

#define TROLL_MAX_LEVELS 32

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

typedef enum {
  ts_ok,
  ts_invalid_argument,
  ts_config_error,
  ts_address_error,
  ts_memory_error,
  ts_already_initialised,
  ts_error,
  ts_not_initialised
} troll_status;

/**
 * @brief Инициализация с указанием имени лог-файла
 * @return false, если ошибка открытия файла на запись
 * addr:  x.x.x.x:yyyy   (ipv4address:port)
 *          path to file*size_limit
 * */
troll_status troll_init(const char *addr, unsigned mem_size);

/** Очистка ресурсов. */
void troll_release();

/**
 * @brief Задает фильтр обработки по типу сообщения
 * @note По умолчанию выводятся все сообщения */
void troll_verbose(int level_flags);

/**
 * @brief Вывод сообщения в лог-файл (служебная функция)
 * @param level уровень/тип сообщения
 * @param format printf format string
 * @param ... args
 */
void troll_print(int level, const char *level_name, const char *level_format, const char *format,
                 va_list* vlist);


#ifdef __cplusplus
} // extern "C"
#endif


#endif // __TROLL_H__