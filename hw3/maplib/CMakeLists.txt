set (This maplib)
add_library(${This} impl/maplib.c maplib.h)
target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17 )
target_compile_definitions(${This} PUBLIC MAP_TYPE=STR_TO_INTMAX)

