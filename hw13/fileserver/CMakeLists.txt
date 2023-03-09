cmake_minimum_required(VERSION 3.12)

set(This fileserver)
add_executable(${This} 
  main.c 
  connection.c
  sock_helpers.c
  signal_helpers.c
  buffer.c
)
target_link_libraries(${This} sflog)
