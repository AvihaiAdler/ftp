add_library(db_manager)

target_sources(db_manager
  PRIVATE src/db_manager.c
)

target_compile_features(db_manager
  PRIVATE c_std_99
)

target_compile_options(db_manager
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -g
)

target_link_libraries(db_manager
  PRIVATE sqlite
)

target_include_directories(db_manager
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)