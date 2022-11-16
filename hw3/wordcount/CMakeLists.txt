set (This wordcount)

add_executable(${This} main.c hash.c hash.h)
target_link_libraries(${This} PRIVATE maplib)
target_include_directories(${This} PRIVATE ${CMAKE_SOURCE_DIR}/maplib)
target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17 )

