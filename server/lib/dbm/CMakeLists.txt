add_library(dbm)

target_sources(dbm
  PRIVATE src/db_manager.c
)

target_compile_features(dbm
  PRIVATE c_std_99
)

target_compile_options(dbm
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

target_link_libraries(dbm
  PUBLIC sqlite
  PUBLIC ds
)

target_include_directories(dbm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(tests)