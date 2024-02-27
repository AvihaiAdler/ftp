add_library(tasks)

target_sources(tasks
  PRIVATE
  src/cwd
)

target_compile_features(tasks
  PRIVATE c_std_11
)

target_compile_definitions(tasks
  PRIVATE _XOPEN_SOURCE=700
)

target_compile_options(tasks
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
)

target_link_libraries(tasks
  PRIVATE
  dbm
  logger
  parser
  requests
  thread_pool
  util
)