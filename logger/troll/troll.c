#include "troll.h"
#include "connection.h"
#include "cqueue.h"
#include "dprint.h"
#include "logmsg.h"
#include "sock_helpers.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>


#define MINIMUM_MEM_SIZE 1024
#define PIPE_QUEUE_SIZE 0x2000





static CQueue *msg_storage = NULL;
static CQueue *pipe_cq = NULL;
int pipe_event_fd = -1;
pthread_mutex_t pipe_mtx;
int epoll_fd = -1;
pthread_t thread_fd = -1;
static volatile bool run_flag = false;
static volatile bool intitilized = false;
static int listenfd = -1;

static void *troll_thread(void *ptr);

troll_status troll_init(const char *addr, unsigned mem_size) {
  dprint("troll_init()");
  if (intitilized)
    return ts_already_initialised;

  if (mem_size < MINIMUM_MEM_SIZE)
    return ts_invalid_argument;

  init_levels();
  
  if (pthread_mutex_init(&pipe_mtx, NULL) != 0) {
    dprint("error on mutex init: %s", strerror(errno));
    return ts_error;
  };

  intitilized = true;

  listenfd = make_bound_socket(addr);
  if (listenfd < 0) {
    dprint("error bind socket: %s", strerror(errno));
    troll_release();
    return ts_address_error;
  }

  if (!make_nonblock(listenfd)) {
    dprint("error make socket nonblock: %s", strerror(errno));
    troll_release();
    return ts_error;
  }

  int flag = 0; // =EFD_SEMAPHORE  - to process packet by packet
  if ((pipe_event_fd = eventfd(0, flag)) == -1) {
    dprint("error init evenetfd: %s", strerror(errno));
    troll_release();
    return ts_error;
  };

  if (!make_nonblock(pipe_event_fd)) {
    dprint("error make eventfd nonblock: %s", strerror(errno));
    troll_release();
    return ts_error;
  }

  if ((epoll_fd = epoll_create1(0)) == -1) {
    dprint("error create epoll: %s", strerror(errno));
    troll_release();
    return ts_error;
  }

  struct epoll_event listen_ev = {.events = EPOLLIN | EPOLLET, .data.fd = listenfd};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &listen_ev) < 0) {
    dprint("error create epoll: %s", strerror(errno));
    troll_release();
    return ts_error;
  }

  struct epoll_event pipe_event_ev = {.events = EPOLLIN, .data.fd = pipe_event_fd};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_event_fd, &pipe_event_ev) < 0) {
    dprint("error epol_ctl: %s", strerror(errno));
    troll_release();
    return ts_error;
  }

  if ((msg_storage = cq_new(mem_size, true)) == NULL) {
    dprint("memory error on create message storage");
    troll_release();
    return ts_memory_error;
  }

  if ((pipe_cq = cq_new(PIPE_QUEUE_SIZE, false)) == NULL) {
    dprint("memory error on create pipe queue");
    troll_release();
    return ts_memory_error;
  }

  if (listen(listenfd, MAX_CONNECTIONS) < 0) {
    dprint("error on listen(): %s", strerror(errno));
    return EXIT_FAILURE;
  }                 

  run_flag = true;
  if (pthread_create(&thread_fd, NULL, troll_thread, NULL) != 0) {
    dprint("error on creating troll thread: %s", strerror(errno));
    run_flag = false;
    troll_release();
    return ts_error;
  }

  return ts_ok;
}


void troll_release() {
  if (!intitilized) {
    return;
  }
  if (run_flag == true) {
    // Запущен клиентским потоком, после успешной инициализации
    dprint("troll_release() user call");
    run_flag = false;
    if (eventfd_write(pipe_event_fd, 1) != 0) {
      dprint("error on eventfd_write: %s", strerror(errno));
    };
    pthread_join(thread_fd, NULL);
  } else {
    // Запущен при не-усппешной инициализации
    // или при завершении работы основного потока troll_thread
    dprint("troll_release() internal call");

    pthread_mutex_destroy(&pipe_mtx);

    if (pipe_event_fd > 0) {
      close(pipe_event_fd);
      pipe_event_fd = -1;
    }

    if (epoll_fd > 0) {
      close(epoll_fd);
      epoll_fd = -1;
    }

    free(msg_storage);
    msg_storage = NULL;
    free(pipe_cq);
    pipe_cq = NULL;

    if (listenfd > 0) {
      close(listenfd);
      listenfd = -1;
    }

    intitilized = false;
  }
}


void troll_print(int level, const char *level_name, const char *level_format, const char *format,
                 va_list *va_list_ptr) {
  if (!intitilized) {
    return;
  }
  
  assert(va_list_ptr);
  if (level >= TROLL_MAX_LEVELS)
    return;
 
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  
  pthread_mutex_lock(&pipe_mtx);
  LevelInfo* li = level_info(level);
  if (li == NULL){
    li = register_level(level, level_name, level_format);
    if (!li){
      dprint("error regiser level %d", level);
      pthread_mutex_unlock(&pipe_mtx);
      return;
    }
  }
  
  size_t max_body_len = li->max_body_len;
  size_t body_len = 0;
  va_list vlist;
  va_copy(vlist, *va_list_ptr);


  // Резервируем буфер  


  int push_ret = cq_push(pipe_cq, NULL, sizeof(LogMsg) + max_body_len);
  if (push_ret == cq_res_lost_tail) {
    // Асинхронный поток не вычитывает очередь? 
    dprint("Pipe to async logger overflowed");
    eventfd_write(pipe_event_fd, 0);
    pthread_mutex_unlock(&pipe_mtx);
    return;
  }
  if (push_ret != cq_res_ok) {
    dprint("Pipe to async logger overflowed with error %d", push_ret);
    pthread_mutex_unlock(&pipe_mtx);
    return;
  }
 
  bool inserted = false;

  while (!inserted) {
    char *buf;
    if (cq_back(pipe_cq, &buf, NULL) != cq_res_ok) {
       dprint("troll internal error on FILE %s line %d", __FILE__, __LINE__);
       pthread_mutex_unlock(&pipe_mtx);
      return;
    };
    LogMsg *pmsg = (LogMsg *)buf;
    pmsg->level = level;
    pmsg->sp = ts;
    body_len = vsnprintf(pmsg->body, max_body_len, format, vlist);
    if (body_len > max_body_len) {
      max_body_len = body_len * 4 / 3;
      li->max_body_len = max_body_len;
      if (cq_replace_back(pipe_cq, NULL, max_body_len) == cq_res_error) {
        dprint("troll internal error on FILE %s line %d", __FILE__, __LINE__);
        pthread_mutex_unlock(&pipe_mtx);
        return;
      };
      va_end(vlist);
      va_copy(vlist, *va_list_ptr);
      
    } else {
      inserted = true;
      cq_back_shrink(pipe_cq, sizeof(LogMsg)+body_len);
    }
    //dprint("pushed to pipe %s", pmsg->body);
  }
  pthread_mutex_unlock(&pipe_mtx);
  va_end(vlist);
  eventfd_write(pipe_event_fd, 1);
  // dprint("push 1 to pipe-queue");
}


static void *troll_thread( __attribute__((unused)) void *ptr) {
  dprint("troll_thread strarted");
  int connections_count = 0;
  static const int max_epoll_events = 16;
  struct epoll_event events[max_epoll_events];

  while (run_flag) {
    int events_count = epoll_wait(epoll_fd, events, max_epoll_events, -1);
    if (events_count == -1) {
      if (errno != EINTR) {
        run_flag = false;
      }
      continue;
    }

    for (int i = 0; i < events_count; i++) {
      //dprint("epoll event %d", events[i].data.u32);
      if (events[i].data.fd == listenfd) {
        connections_count = troll_accept_connections(listenfd, epoll_fd, msg_storage);
        dprint("accept => connections_count=%d", connections_count);
      } else if (events[i].data.fd == pipe_event_fd) {
        if (!run_flag) {
          continue;
        }
        // Новые данные от клиентского потока
        eventfd_t len;
        eventfd_read(pipe_event_fd, &len);
        //dprint("pipe_event_fd , len=%d, pipe_queue_len=%d", (int)(len), cq_length(pipe_cq));

        if (troll_any_conn_send_storage()) {
          dprint("suspend reading from pipe");
          continue; // Откладываем обработку очереди
        }
        cq_iterator_t it=cq_begin(pipe_cq);
        int count = len;
        // Копируем в strorage и отправляем
        while (it != NULL && count > 0) {
          cq_push_from_iter(msg_storage, pipe_cq, it);
          const char *pmsg;
          unsigned msg_size;
          int ret = cq_get(pipe_cq, it, &pmsg, &msg_size);
          // dprint("got from pipe %s", ((LogMsg *)pmsg)->body);
          if (ret == cq_res_ok) {
            troll_on_new_messge((LogMsg *)(pmsg), msg_size);
          } else {
            dprint("Error on get data from pipe-queue: %d", ret);
          }
          count--;
          it = cq_next(pipe_cq, it); 
        }
   

        count = len;
        pthread_mutex_lock(&pipe_mtx);
        // Очищаем скопированные данные с мьютексом
        while ((it = cq_begin(pipe_cq)) != NULL && count > 0) {
          count--;
          cq_pop(pipe_cq);
        }
        pthread_mutex_unlock(&pipe_mtx);
        
        // dprint("%d records added", len);
        // check_storage(msg_storage);

      } else {
        Connection *pconn = (Connection *)events[i].data.ptr;
        if (events[i].events & EPOLLIN)
          troll_on_ready_read(pconn);
        if (events[i].events & EPOLLOUT)
          troll_on_ready_write(pconn);
        if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
          connections_count = troll_terminate_connection(&events[i], epoll_fd);
        }
        if (!run_flag || troll_get_conn_state(pconn) == state_end) {
          connections_count = troll_terminate_connection(&events[i], epoll_fd);
        }
      }
    }

  } // while

  troll_terminate_all_connections(epoll_fd);
  dprint("troll_thread finished");
  troll_release();
  return NULL;
}


/** В будущем мы будем сохранять в памяти не готовую к записи строку, а некоторую структуру*/
// struct CachedMsg {
//   // int size time
//   // level
//   // seialized list of {next, fmt_size, arg_size, arg, fmt};
// };

// static int store_msg(char* dest, int max_size, const char* fmt, ... );
// static int restore_msg(char *dets, struct CachedMsg* src);
