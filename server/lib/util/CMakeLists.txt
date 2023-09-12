add_library(util)

target_sources(util
  PRIVATE
  src/algorithm.c
  src/session.c
  src/task_args.c
  src/util.c
)

target_compile_features(util
  PRIVATE c_std_11
)

target_compile_definitions(util
  PRIVATE -D_XOPEN_SOURCE=700
)

target_compile_options(util
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -g
  -O3
)

target_include_directories(util
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_library(threads
  NAMES pthread
  REQUIRED
)

target_link_libraries(util
  PRIVATE ds
  PRIVATE logger
  PRIVATE dbm
  PRIVATE ${threads}
  PRIVATE parser
)