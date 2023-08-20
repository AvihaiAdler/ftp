add_library(util)

target_sources(util
  PRIVATE
  src/algorithm.c
  src/session.c
)

target_compile_features(util
  PRIVATE c_std_99
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

target_link_libraries(util
  PRIVATE ds
)