cmake_minimum_required(VERSION 3.12)

project("sflog" 
  VERSION 0.0.1 
  DESCRIPTION "Simple log-to-file logger with backtrace" 
  LANGUAGES C 
)

add_compile_options(-std=gnu17 -ggdb -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration)
add_link_options(-rdynamic)

add_subdirectory(sflog)

add_subdirectory(test)