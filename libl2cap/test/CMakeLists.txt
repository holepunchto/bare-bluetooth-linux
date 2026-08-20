list(APPEND tests
  addr
  echo
  backpressure
  reentrant-close
  mtu
  events
)

foreach(test IN LISTS tests)
  add_executable(${test} ${test}.c)

  set_target_properties(
    ${test}
    PROPERTIES
    C_STANDARD 23
  )

  # Asserts are the test; strip the NDEBUG that release flags define
  target_compile_options(
    ${test}
    PRIVATE
      -UNDEBUG
  )

  target_link_libraries(
    ${test}
    PRIVATE
      l2cap_static
  )

  add_test(
    NAME ${test}
    COMMAND ${test}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  set_tests_properties(
    ${test}
    PROPERTIES
    TIMEOUT 30
  )
endforeach()
