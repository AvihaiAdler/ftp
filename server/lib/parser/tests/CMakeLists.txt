set(PARSER_SANITY_TESTS
  lexer_sanity
  parser_sanity
)

foreach(test ${PARSER_SANITY_TESTS})
  add_executable(${test})
  target_sources(${test}
    PRIVATE ${test}.c
  )

  add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)

  target_compile_features(${test}
    PRIVATE c_std_99
  )

  target_compile_options(${test}
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Og
    -g
    -fsanitize=address,undefined
  )

  target_link_options(${test}
    PRIVATE
    -fsanitize=address,undefined
  )

  target_link_libraries(${test}
    PRIVATE
    parser
  )
endforeach()

add_custom_command(TARGET parser_sanity
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} ARGS -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/commands
  COMMAND ${CMAKE_COMMAND} ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/commands/valid_commands
  ${CMAKE_CURRENT_SOURCE_DIR}/commands/invalid_commands
  ${CMAKE_CURRENT_SOURCE_DIR}/commands/unsupported_commands
  ${CMAKE_CURRENT_BINARY_DIR}/commands
)