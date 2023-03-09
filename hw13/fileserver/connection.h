#pragma once

typedef struct Connection Connection;

typedef enum  {
  state_wait_request,
  state_sending_answer,
  state_sending_file,
  state_end
} ConnState;

Connection* fs_new_conn(int conn_fd, const char* basedir);

int fs_get_conn_fd(Connection* conn);

ConnState fs_get_conn_state(Connection* conn);

void fs_on_ready_read(Connection* conn);

void fs_on_ready_write(Connection* conn);

void fs_close_conn(Connection* conn);

void fs_free_conn(Connection* conn);


