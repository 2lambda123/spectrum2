add_subdirectory(libtransport)

if(ENABLE_TESTS)

  add_custom_target(test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
  )

  add_custom_target(extended_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/start.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
  )

endif()

