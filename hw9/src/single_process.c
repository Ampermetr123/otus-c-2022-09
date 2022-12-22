#include "single_process.h"

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


long single_process(const char *lockfile, int* error) {
  int fd = open(lockfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    if (error) *error=errno;
    return -1;
  }

  struct flock fl;
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0; // 0 = блокировка всего файла
  int lkres = fcntl(fd, F_SETLK, &fl);
  
  if (lkres < 0){ 
    if (errno == EACCES || errno == EAGAIN) {
      close(fd);
      // Имеется блокировка, возвращает pid из файла
      FILE* f= fopen(lockfile, "r");
      if (f==NULL){
        if (error) *error=errno;
        return -1;
      }
      long pid=-1;
      fscanf(f,"%ld", &pid);
      fclose(f);
      return pid;
    } else {
      if (error) *error = errno; 
      close(fd);
      return -1;
    }
  }
  ftruncate(fd, 0);
  char buf[32];
  snprintf(buf,32, "%ld", (long)getpid());
  write(fd, buf, strlen(buf)+1);
  return 0;
}