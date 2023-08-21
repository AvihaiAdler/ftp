add_library(parser)

target_sources(parser
  PRIVATE
  src/lexer.c
  src/parser.c
)

target_compile_features(parser
  PRIVATE c_std_99
)

target_compile_options(parser
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

target_include_directories(parser
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(parser
  PUBLIC ds
)

add_subdirectory(tests)
