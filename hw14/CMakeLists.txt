cmake_minimum_required(VERSION 3.12)
project(cube LANGUAGES C)

find_package(PkgConfig REQUIRED) 
pkg_check_modules(PQ REQUIRED libpq)
set(This getstat)

add_executable(${This} main.c)
target_include_directories(${This} PUBLIC ${PQ_INCLUDE_DIRS} )
target_link_libraries(${This} PUBLIC ${PQ_LIBRARIES} )
target_compile_options(${This} PUBLIC -Wall -Wextra -Wpedantic -std=gnu11)