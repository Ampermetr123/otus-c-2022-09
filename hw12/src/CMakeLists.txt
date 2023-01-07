set (This telehack)

add_executable(${This} main.c buffer.c)
target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17 )


