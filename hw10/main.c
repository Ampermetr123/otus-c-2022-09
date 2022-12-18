#include "crc32.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// если определено, то обработка по страницам 
#define PAGE_SIZE 0x200000l

// если определено, то выводить процент выполнения в stdout 
// после обработки каждой страницы
#define PRINT_PROC

static inline void exit_with_error(char *msg, int fd_to_close) {
  perror(msg);
  if (fd_to_close >= 0) {
    close(fd_to_close);
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  // Проверка аргументов
  if (argc < 2) {
    printf("Calculating CRC32 on huge file using mmap.\nUsage: %s <path_to_file>\n\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Открываем файл, определяем размер
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    exit_with_error("ошибка доступа к файлу (open)", -1);
  struct stat st;
  if (fstat(fd, &st) == -1) {
    exit_with_error("ошибка получения свойств файла (fstat)", fd);
  }
  if ((st.st_mode & S_IFMT) != S_IFREG) {
     fprintf(stderr, "ошибка - указан не регулярный файл\n");
    close(fd);
    return EXIT_FAILURE;
  }   
  off_t size = st.st_size;

  // Отображаем  
  uint8_t *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    exit_with_error("ошибка отображения файла в память (mmap)", fd);
  }
  close(fd);
  // Бытрее ~ 3%
  if (madvise(addr, size, MADV_SEQUENTIAL) == -1){
    exit_with_error("ошибка отображения файла в память (mapadvise)", fd);
  };

  // Расчет CRC по страницам
  #ifdef PAGE_SIZE
  size_t pg_size = PAGE_SIZE;
  #else
  size_t pg_size = size
  #endif
  
  uint32_t crc;
  size_t pages = size / pg_size;
  size_t tail_size = size % pg_size;
  size_t offset = 0;
  crc32_start(&crc);
  while (offset < pages * pg_size) {
    crc32_proc(addr + offset, pg_size, &crc);
    if (munmap(addr + offset, pg_size) == -1) {
      exit_with_error("ошибка освобождения памяти (munmap)", -1);
    }
    offset += pg_size;
#ifdef PRINT_PROC
    fprintf(stdout, "\r%ld%%", offset *100 / size );
    fflush(stdout);
#endif
  }
  crc32_proc(addr + offset, tail_size, &crc);
  if (munmap(addr + offset, tail_size) == -1) {
    exit_with_error("ошибка освобождения памяти (munmap)", -1);
  }
  crc32_end(&crc);
  
  // Выводим результат
  fprintf(stdout, "\r%u\n", crc);
  return 0;
}
