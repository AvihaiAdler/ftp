set(DBM_TESTS dbm_sanity)

foreach(test ${DBM_TESTS})
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
    PRIVATE dbm
  )

endforeach()
