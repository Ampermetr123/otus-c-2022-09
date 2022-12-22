#pragma once

/**
 * @brief Проверка единственносси экземпляра процесса
 * @param lockfile путь к файлу блокировку
 * @param [out] error значние errno, в случае ошибки
 * @return  0 - единственный процесс
 *          >0 - существует другой процесс и его pid
 *          -1 -ошибка выполнения
 */
long single_process(const char *lockfile, int* error);