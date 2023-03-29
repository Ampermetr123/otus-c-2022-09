cmake_minimum_required(VERSION 3.12)

set(This troll)

add_compile_options(-std=gnu17 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration)
##add_link_options(-rdynamic)



add_library(${This} 
troll.c 
cqueue.c
sock_helpers.c
connection.c
buffer.c
logmsg.c
)
target_include_directories(${This} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})