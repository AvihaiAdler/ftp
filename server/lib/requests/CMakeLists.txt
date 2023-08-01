add_library(requests)

target_sources(requests
  PRIVATE src/requests.c
)

target_compile_features(requests
  PRIVATE c_std_99
)

target_compile_definitions(requests
  PRIVATE -D_XOPEN_SOURCE=700
)

target_compile_options(requests
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

target_include_directories(requests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(requests
  PRIVATE ds
  PRIVATE util
)