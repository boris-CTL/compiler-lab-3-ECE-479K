# Find all test files in the test directories
file(GLOB REGRESSION_CLS CONFIGURE_DEPENDS regression/*.cl)
file(GLOB FAIL_CLS CONFIGURE_DEPENDS fail/*.cl)
file(GLOB CHECK_CLS CONFIGURE_DEPENDS check/*.cl)
file(GLOB CHECK_REFOUTS CONFIGURE_DEPENDS check/*.refout)

# Copy the tests to the build folder so we can use them
file(COPY ${REGRESSION_CLS} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${FAIL_CLS} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CHECK_CLS} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CHECK_REFOUTS} DESTINATION ${CMAKE_BINARY_DIR})

foreach (test ${REGRESSION_CLS})
    run_test(FILE ${test})
endforeach()

foreach (test ${FAIL_CLS})
    run_test(FILE ${test} FAIL True)
endforeach()

foreach (test ${CHECK_CLS})
    run_test(FILE ${test} REFOUT True)
endforeach ()