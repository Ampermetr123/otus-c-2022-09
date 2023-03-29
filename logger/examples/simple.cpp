#include <stdlib.h>   
#include <unistd.h>
#include <signal.h>

#include "stdio.h"
#include "troll.h"

#define LOG_LEVEL_FATAL 1
#define TROLL_LEVEL_MASK LOG_LEVEL_FATAL
#define TROLL_LEVEL_NAME fatal
#define TROLL_LEVEL_FORMAT 
#include "troll_reg_level.h"

#define LOG_LEVEL_INFO 2
#define TROLL_LEVEL_MASK LOG_LEVEL_INFO
#define TROLL_LEVEL_NAME info
#define TROLL_LEVEL_FORMAT
#include "troll_reg_level.h"


void release(__attribute__((unused)) int status, __attribute__((unused)) void *arg) {
   troll_release(); 
} 

void cleanup(int signo){
  puts("SIGTERM");
  exit(0);
}

int main(){
  int ret = troll_init("127.0.0.1:19999", 2048);
  if (ret!=ts_ok){
    printf("init error: %d\n",ret);
    return 1;
  }
  on_exit(release, NULL);     
  signal(SIGINT, cleanup);   

  int i = 0;
  while (i>=0) {
    i+=1;
    troll_info("Hello world %d!", i);
    sleep(1);
  }
  return 0;
}