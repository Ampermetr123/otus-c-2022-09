cmake_minimum_required(VERSION 3.13.0)
project("fileserver" 
    VERSION 0.0.1
    DESCRIPTION "hw13 otus-c http file server"
    LANGUAGES C )


add_compile_options(-std=gnu17 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration)  
add_link_options(-rdynamic) 
add_subdirectory(sflog)
add_subdirectory(fileserver)
