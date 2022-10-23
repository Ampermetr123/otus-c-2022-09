cmake_minimum_required(VERSION 3.0.0)

set(This cp2utf8)
project(${This} VERSION 0.0.1
    DESCRIPTION "Converter from code-paged text to utf8; otus-c-programmer homework 2 by S. Simonov"
    LANGUAGES C)

add_executable(${This} main.c)
target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17)

# Auto generating charset.h 
add_custom_command(
    OUTPUT charset.h
    COMMAND python3 conv.py ${CMAKE_SOURCE_DIR}/charset ${CMAKE_SOURCE_DIR}/charset.h
    DEPENDS conv.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(gen_charset_h
    DEPENDS charset.h
)
add_dependencies(${This} gen_charset_h)


if(UNIX)
    enable_testing()

    set(TestDir ${CMAKE_SOURCE_DIR}/test)

    add_test(NAME cp1251_test COMMAND sh -c
        "${CMAKE_BINARY_DIR}/${This} ${TestDir}/cp1251.txt cp1251 out.txt && diff ${TestDir}/utf8.txt out.txt")

    add_test(NAME iso8859_test COMMAND sh -c
        "${CMAKE_BINARY_DIR}/${This} ${TestDir}/iso-8859-5.txt iso-8859-5 out.txt && diff ${TestDir}/utf8.txt out.txt")

    add_test(NAME koi8_test COMMAND sh -c
        "${CMAKE_BINARY_DIR}/${This} ${TestDir}/koi8.txt koi8-r out.txt && diff ${TestDir}/utf8.txt out.txt")

    add_test(NAME wrong_cp_test COMMAND sh -c
        "${CMAKE_BINARY_DIR}/${This} ${TestDir}/koi8.txt cp1251 out.txt && diff ${TestDir}/utf8.txt out.txt")
    set_tests_properties(wrong_cp_test PROPERTIES WILL_FAIL true)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
