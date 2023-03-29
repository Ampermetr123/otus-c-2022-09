
add_executable(troll_mt mt.c)
target_link_libraries(troll_mt troll pthread)
#target_compile_definitions(troll_mt _POSIX_BARRIERS=1)
target_compile_options(troll_mt PRIVATE -std=gnu17 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration)


add_executable(troll_simple simple.cpp)
target_link_libraries(troll_simple troll)
