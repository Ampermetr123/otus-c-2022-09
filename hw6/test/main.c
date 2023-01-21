#include "errno.h"
#include "pthread.h"
#include "sflog.h"
#include "stdlib.h"
#include "string.h"
#include "threads.h"

#define _unused_ __attribute__((unused))

pthread_barrier_t bar;

void release(_unused_ int status, _unused_ void *arg) { sfl_release(); }

int say_name(void *name) {
  pthread_barrier_wait(&bar); // Отвечать будем хором!
  sfl_info("%s", (const char *)name);
  return 0;
}

void test_fatal(unsigned i) {
  if (i){
    test_fatal(i - 1);
    return;
  }
  sfl_fatal("%s", "Это учбеная тревога!");
}

int main() {

  const char *guys[] = {"Толик!",  "Серега!", "Семен!",   "Славик!",
                        "Мишаня!", "Вовчик!", "Андрюха!", "Витя!"};
  int sz = sizeof(guys) / sizeof(guys[0]);
  thrd_t *threads = (thrd_t *)calloc(sz, sizeof(thrd_t));
  if (!threads) {
    sfl_fatal("Ошибка выделения памяти");
    return 1;
  }
  sfl_init_fstream(stdout);
  on_exit(release, NULL);

  if (pthread_barrier_init(&bar, NULL, sz) != 0) {
    sfl_fatal("%s", strerror(errno));
    return EXIT_FAILURE;
  }

  sfl_set_verbosility(SFL_ALL & ~SFL_DEBUG);
  sfl_debug("Если видите это сообщение, то что-то пошло не так");
  sfl_note("Внимание, парни, на счет три перекличка! 1, 2, 3 !");
  for (int i = 0; i < sz; i++) {
    thrd_create(&threads[i], say_name, (void *)guys[i]);
  }
  for (int i = 0; i < sz; i++) {
    thrd_join(threads[i], NULL);
  }
  free(threads);
  pthread_barrier_destroy(&bar);
  test_fatal(5);
  sfl_warn("Мы еще вернемся! Пока!");
  
  sfl_init_fpath("./test_log.txt", false);
  sfl_info("%s","Это запись в текстовый файл.");
  
  return 0;
}