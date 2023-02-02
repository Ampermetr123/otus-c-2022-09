set(This sflog)

add_library(${This} sflog.c)
target_include_directories(${This} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})