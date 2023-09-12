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
add_subdirectory(lib/parser)

add_executable(ftpd)

target_sources(ftpd
  PRIVATE src/ftpd.c
)

target_compile_features(ftpd
  PRIVATE c_std_11
)

target_compile_definitions(ftpd
  PRIVATE -D_XOPEN_SOURCE=700
)

target_compile_options(ftpd
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(ftpd
    PRIVATE
    -g
    -fsanitize=address,undefined
  )
  target_link_options(ftpd
    PRIVATE
    -fsanitize=address,undefined
  )
endif()

target_link_libraries(ftpd
  PRIVATE
  dbm
  ds
  logger
  parser
  requests
  thread_pool
  util
)
