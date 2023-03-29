cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

add_executable(test_cq test_cq.cpp)



target_link_libraries(
   test_cq
   gtest_main
   gmock
   troll
   pthread
)



include(GoogleTest)
gtest_discover_tests(test_cq)

