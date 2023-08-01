cmake_minimum_required(VERSION 3.20)
project(ds VERSION 0.0.1)

add_library(ds)

target_sources(ds 
  PRIVATE src/vec.c
  PRIVATE src/list.c
  PRIVATE src/hash_table.c
  PRIVATE src/bst.c
  PRIVATE src/ascii_str.c
)

target_include_directories(ds 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ds 
  PRIVATE c_std_99
)

set(compile_flags "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(compile_flags -Wall -Wextra -pedantic -O3)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(compile_flags -W4)
endif()

target_compile_options(ds PRIVATE ${compile_flags})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  include(CTest)
  add_subdirectory(tests/vector)
  add_subdirectory(tests/linked_list)
  add_subdirectory(tests/hash_table)
  add_subdirectory(tests/bst)
  add_subdirectory(tests/ascii_str)
endif()
