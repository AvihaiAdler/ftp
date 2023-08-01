cmake_minimum_required(VERSION 3.20)
project(ftp VERSION 0.0.1 LANGUAGES C)

include(CTest)

add_subdirectory(lib/ds)
add_subdirectory(lib/logger)
add_subdirectory(lib/thread_pool)
add_subdirectory(lib/requests)
add_subdirectory(lib/util)
