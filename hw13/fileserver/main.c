#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"
#include "sflog.h"
#include "signal_helpers.h"
#include "sock_helpers.h"

#include <arpa/inet.h>
#include <sys/epoll.h>

#define _unused_ __attribute__((unused))

// The backlog argument defines the maximum length to which the
// queue of pending connections for sockfd may grow
#define BACKLOG 128
// Max events count that could be hadled on one epoll_wait()
#define MAX_EPOLL_EVENTS 128

static struct epoll_event events[MAX_EPOLL_EVENTS];

void print_usage(const char *progname) {
  printf("Usage:\n%s [-d <dir>] [-b <addr>] [-v]"
         "\n <dir> - direcory to share file, default cwd"
         "\n <addr> - address:port to bind server",
         progname);
}

void close_on_exit(_unused_ int status, void *args) {
  int fd = *((int *)args);
  if (fd >= 0)
    close(fd);
}

bool run_flag = false;

void on_stop_signal(int signo) {
  sfl_info("Terminating server on %s signal", strsignal(signo));
  run_flag = false;
}

void on_error_signal(int signo) {
  sfl_fatal("Catched %s signal", strsignal(signo));
  exit(1);
}


int add_connections(int listen_fd, int epoll_fd, const char *basedir) {
  int conn_fd = 0;
  int conn_number = 0;
  while ((conn_fd = accept(listen_fd, NULL, NULL)) > 0) {
    if (make_nonblock(conn_fd) == false) {
      sfl_error("error on make_nonblock(): %s", strerror(errno));
      shutdown(conn_fd, SHUT_RDWR);
      close(conn_fd);
      return false;
    }
    Connection *conn = fs_new_conn(conn_fd, basedir);
    if (conn) {
      struct epoll_event conn_ev;
      conn_ev.data.ptr = (void *)conn;
      conn_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLHUP;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &conn_ev) < 0) {
        sfl_error("error on add listen socket event to epoll: %s", strerror(errno));
        fs_close_conn(conn);
        fs_free_conn(conn);
      }
      conn_number++;
    }
  }
  return conn_number;
}


void terminate_connection(struct epoll_event *ev, int epoll_fd) {
  if (ev == NULL)
    return;
  Connection *conn = (Connection *)ev->data.ptr;
  if (conn) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fs_get_conn_fd(conn), ev);
    ev->data.ptr = NULL;
    fs_close_conn(conn);
    fs_free_conn(conn);
  }
}


int main(int argc, char *argv[]) {

  // Passing command-line options
  const char *resources_dir = getenv("PWD");
  const char *bind_address = NULL;
  int verbose = 0;
  int opt;
  while ((opt = getopt(argc, argv, "d:b:v")) != -1) {
    if (opt == 'd') {
      resources_dir = optarg;
    } else if (opt == 'b') {
      bind_address = optarg;
    } else if (opt == 'v') {
      verbose = 1;
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  // Logger setup
  sfl_init_fstream(stdout);
  on_exit(sfl_release, NULL);
  sfl_set_verbosility(verbose ? SFL_ALL : (SFL_FATAL | SFL_ERROR | SFL_INFO));
  sfl_prefix_format(false, true, true);

  // Signals setup
  set_stop_signal_handler(on_stop_signal);
  set_error_signal_handler(on_error_signal);
  signal(SIGPIPE, SIG_IGN); // lector says it can be raised

  sfl_info("start fserver");
  sfl_info("resoucres dir: %s", resources_dir);
  sfl_info("bind_address: %s", bind_address ? bind_address : "not set (any_address:any_port)");

  // Listening socket setup
  static int listenfd = -1;
  listenfd = make_binded_socket(bind_address);
  if (listenfd < 0) {
    return EXIT_FAILURE;
  }
  on_exit(close_on_exit, &listenfd);
  if (make_nonblock(listenfd) == false) {
    sfl_error("error on make_nonblock(): %s", strerror(errno));
    return EXIT_FAILURE;
  };
  if (listen(listenfd, BACKLOG) < 0) {
    sfl_error("error on listen(): %s", strerror(errno));
    return EXIT_FAILURE;
  }

  // Epoll setup
  static int epoll_fd = -1;
  if ((epoll_fd = epoll_create1(0)) == -1) {
    sfl_error("error on epoll_create(): %s", strerror(errno));
    return EXIT_FAILURE;
  }
  on_exit(close_on_exit, &epoll_fd);

  // Adding listen event (read | edge-triggered)
  struct epoll_event listen_ev = {.events = EPOLLIN | EPOLLET, .data.fd = listenfd};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &listen_ev) < 0) {
    sfl_error("error on add listen socket event to epoll: %s", strerror(errno));
    return EXIT_FAILURE;
  }

  // Runing loop
  run_flag = true;
  int connections_count = 0;
  while (run_flag || connections_count) {
    int events_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
    if (events_count == -1) {
      if (errno == EINTR) {
        continue;
      }
      sfl_error("error on epoll_wait(): %s", strerror(errno));
      return EXIT_FAILURE;
    }

    for (int i = 0; i < events_count; i++) {
      if (events[i].data.fd == listenfd) {
        int cnt = add_connections(listenfd, epoll_fd, resources_dir);
        connections_count += cnt;
      } else {
        Connection *pconn = (Connection *)events[i].data.ptr;
        if (events[i].events & EPOLLIN)
          fs_on_ready_read(pconn);
        if (events[i].events & EPOLLOUT)
          fs_on_ready_write(pconn);
        if (events[i].events & (EPOLLHUP | EPOLLET | EPOLLRDHUP)) {
          if (events[i].events & EPOLLRDHUP)
            sfl_debug("catch EPOLLRDHUP");
          if (events[i].events & EPOLLHUP)
            sfl_debug("catch EPOLHUP");
          if (events[i].events & EPOLLET)
            sfl_debug("catch EPOLLET");
          terminate_connection(&events[i], epoll_fd);
          connections_count--;
        }
        if (!run_flag || fs_get_conn_state(pconn) == state_end) {
          terminate_connection(&events[i], epoll_fd);
          connections_count--;
        }
      }
    }
  } // while(true)
  return EXIT_SUCCESS;
}