cmake_minimum_required(VERSION 3.20)
project(ftp VERSION 0.0.1 LANGUAGES C)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  include(CTest)
endif()

include(FetchContent)
FetchContent_Declare(ds
    SOURCE_DIR lib/ds
    GIT_REPOSITORY https://github.com/AvihaiAdler/common-data-structures
    GIT_TAG main
)
FetchContent_MakeAvailable(ds)

add_subdirectory(lib/logger)
add_subdirectory(lib/thread_pool)
add_subdirectory(lib/requests)
add_subdirectory(lib/util)
add_subdirectory(lib/sqlite)
add_subdirectory(lib/dbm)
