set (This parselogs)

find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH FALSE)
pkg_check_modules (GLIB2 glib-2.0>2.6)
pkg_check_modules (GIO2 gio-2.0)
message(STATUS "glib2 version: ${GLIB2_VERSION}")
message(STATUS "glib2 include dirs: ${GLIB2_INCLUDE_DIRS}")
message(STATUS "glib2 libraries:  ${GLIB2_LIBRARIES}")
message(STATUS "gio2 libraries:  ${GIO2_LIBRARIES}")


add_executable(${This} main.c)

target_compile_options(${This} PRIVATE -Wall -Wextra -Wpedantic -std=gnu17 )

target_include_directories(${This} PRIVATE ${GLIB2_INCLUDE_DIRS} ${GIO2_INCLUDE_DIRS} )
target_link_libraries(${This} PUBLIC  ${GIO2_LIBRARIES} )

