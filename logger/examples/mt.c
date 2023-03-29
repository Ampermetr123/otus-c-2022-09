#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "troll.h"
#define LOG_LEVEL_FATAL 1
#define TROLL_LEVEL_MASK 1
#define TROLL_LEVEL_NAME fatal
#define TROLL_LEVEL_FORMAT // todo
#include "troll_reg_level.h"

#define LOG_LEVEL_NOTE 2
#define TROLL_LEVEL_MASK 2
#define TROLL_LEVEL_NAME note
#define TROLL_LEVEL_FORMAT // todo
#include "troll_reg_level.h"

#define LOG_LEVEL_WARN 3
#define TROLL_LEVEL_MASK 3
#define TROLL_LEVEL_NAME warn
#define TROLL_LEVEL_FORMAT // todo
#include "troll_reg_level.h"

#define LOG_LEVEL_INFO 4
#define TROLL_LEVEL_MASK 4
#define TROLL_LEVEL_NAME info
#define TROLL_LEVEL_FORMAT // todo
#include "troll_reg_level.h"

void release(__attribute__((unused)) int status, __attribute__((unused)) void *arg) { troll_release(); }

void cleanup(__attribute__((unused)) int signo) { exit(0); }

pthread_barrier_t bar;


int say_name(void *name) {
  pthread_barrier_wait(&bar); // Отвечать будем хором!
  troll_info("%s", (const char *)name);
  return 0;
}

void test_fatal(unsigned i) {
  if (i) {
    test_fatal(i - 1);
    return;
  }
  troll_fatal("%s", "Это учбеная тревога!");
}


void rollcall(const char *guys[], int sz) {
  // sfl_set_verbosility(SFL_ALL & ~SFL_DEBUG);
  // sfl_debug("Если видите это сообщение, то что-то пошло не так");
  thrd_t *threads = (thrd_t *)calloc(sz, sizeof(thrd_t));
  if (!threads) {
    troll_fatal("Ошибка выделения памяти");
    exit(1);
  }

  if (pthread_barrier_init(&bar, NULL, sz) != 0) {
    troll_fatal("%s", strerror(errno));
    free(threads);
    exit(1);
  }

  troll_note("Внимание, парни, на счет три перекличка! 1, 2, 3 !");
  for (int i = 0; i < sz; i++) {
    thrd_create(&threads[i], say_name, (void *)guys[i]);
  }
  for (int i = 0; i < sz; i++) {
    thrd_join(threads[i], NULL);
  }
  free(threads);
  pthread_barrier_destroy(&bar);
  test_fatal(5);
  troll_warn("Мы еще вернемся! Пока!");
}

int main() {


  on_exit(release, NULL);
  signal(SIGINT, cleanup);

  int ret = troll_init("127.0.0.1:19999", 2048);
  if (ret != ts_ok){
    printf("troll init error %d", ret);
  }
    

  const char *guys[] = {"Толик!",  "Серега!", "Семен!",   "Славик!",
                        "Мишаня!", "Вовчик!", "Андрюха!", "Витя!"};
  int sz = sizeof(guys) / sizeof(guys[0]);

  on_exit(release, NULL);

  while (1) {
    rollcall(guys, sz);
    sleep(5);
  }

  return 0;
}