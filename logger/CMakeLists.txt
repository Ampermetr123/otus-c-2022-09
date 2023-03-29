cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

project(Troll 
  VERSION 0.0.1 
  DESCRIPTION "Troll - logger" 
  LANGUAGES C CXX
)

add_subdirectory(troll)
add_subdirectory(tests)
add_subdirectory(examples)