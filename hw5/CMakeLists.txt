cmake_minimum_required(VERSION 3.12)
project(GstPlugin LANGUAGES C)

find_package(PkgConfig REQUIRED) 
pkg_check_modules (GST gstreamer-base-1.0)

add_library(myelement SHARED gstmyelement.c)
target_include_directories("myelement" PUBLIC ${GST_INCLUDE_DIRS})
target_link_libraries("myelement" PUBLIC ${GST_LIBRARIES})
target_compile_options(myelement PUBLIC -Wall -Wextra -Wpedantic -std=c11)