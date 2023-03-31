#pragma once

#include "cqueue.h"
#include "logmsg.h"
#include <sys/epoll.h>

#define MAX_CONNECTIONS 5

typedef struct Connection Connection;

typedef enum  {
  state_wait_request,
  state_sending_answer,
  state_send_storage,
  state_send_new_msgs,
  state_end
} ConnState;


int troll_accept_connections(int listen_fd, int epoll_fd, CQueue* storage);

int troll_terminate_connection(struct epoll_event *ev, int epoll_fd);

ConnState troll_get_conn_state(Connection* conn);

void troll_on_ready_read(Connection* conn);

void troll_on_ready_write(Connection* conn);

bool troll_any_conn_send_storage();

void troll_on_new_messge(LogMsg* msg, unsigned len);

void troll_terminate_all_connections(int epoll_fd);

void check_storage(CQueue* cq);