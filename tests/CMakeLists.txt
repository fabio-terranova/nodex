set(TEST_NAMES
    test_filterDesign
)

foreach(test_name ${TEST_NAMES})
    add_executable(${test_name} ./src/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE noddy_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
