cmake_minimum_required(VERSION 3.0.0)

set (This zjg)
project(${This} VERSION 0.0.1 
DESCRIPTION "zj = ZipJpeg; Otus-c-prog homework 1 by S.Simonov"
LANGUAGES C )

add_executable(${This} main.c)
target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17 )

if(UNIX)
    enable_testing()
    add_test(NAME zipjpeg_test COMMAND sh -c "${CMAKE_BINARY_DIR}/${This} ${CMAKE_SOURCE_DIR}/test/zipjpeg.jpg | wc -l")
    set_tests_properties(zipjpeg_test PROPERTIES PASS_REGULAR_EXPRESSION "185")

    add_test(NAME jpeg_test COMMAND sh -c "${CMAKE_BINARY_DIR}/${This} ${CMAKE_SOURCE_DIR}/test/non-zipjpeg.jpg")
    set_tests_properties(jpeg_test PROPERTIES PASS_REGULAR_EXPRESSION "is a Jpeg")

    add_test(NAME empty_file_test COMMAND sh -c "${CMAKE_BINARY_DIR}/${This} ${CMAKE_SOURCE_DIR}/test/empty")
    set_tests_properties(empty_file_test PROPERTIES PASS_REGULAR_EXPRESSION "is not a Jpeg or ZipJpeg")

    add_test(NAME text_file_test COMMAND sh -c "${CMAKE_BINARY_DIR}/${This} ${CMAKE_SOURCE_DIR}/test/text")
    set_tests_properties(text_file_test PROPERTIES PASS_REGULAR_EXPRESSION "is not a Jpeg or ZipJpeg")

    add_test(NAME nonexist_file_test COMMAND sh -c "${CMAKE_BINARY_DIR}/${This} file_not_exist")
    set_tests_properties(nonexist_file_test PROPERTIES WILL_FAIL true)

endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
