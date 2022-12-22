#pragma once

#include <stdbool.h>

/**
 * @brief Демонизация текущего процесса
 * @param имя программы (для syslog) 
 * @return true, елси успех
 * @return false, если возинкла ошибка в процессе домонизации. Ошибка выводится в stderr или в syslog,
 * в зависимости от того, на каком этапе возникла ошибка
 */
bool daemonize(const char *prog_name);