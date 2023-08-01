# lib
add_library(sqlite)

target_sources(sqlite
  PRIVATE src/sqlite3.c
)

target_compile_features(sqlite
  PRIVATE c_std_99
)

target_compile_definitions(sqlite
  PUBLIC SQLITE_OMIT_LOAD_EXTENSION
)

target_compile_options(sqlite
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

find_library(math
  NAMES m
  REQUIRED
)

find_library(pthreads
  NAMES pthread
  REQUIRED
)

target_link_libraries(sqlite
  PRIVATE
  ${math}
  ${pthreads}
)

target_include_directories(sqlite BEFORE
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# shell
add_executable(sqlite_shell)

target_sources(sqlite_shell
  PRIVATE src/shell.c
)

target_compile_features(sqlite_shell
  PRIVATE c_std_99
)

target_compile_options(sqlite_shell
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

target_link_libraries(sqlite_shell
  PRIVATE sqlite
)

add_custom_command(TARGET sqlite_shell
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sqlite_shell> ${CMAKE_SOURCE_DIR}/bin/shell
)
