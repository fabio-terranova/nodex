set(TEST_NAMES
    test_zpk2tf
    test_filterDesign
    test_lfilter
)

# Executables
foreach(test_name ${TEST_NAMES})
    add_executable(${test_name} ./src/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE noddy_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(noddy_bind ./src/test_pybind11.cpp)
target_link_libraries(noddy_bind PRIVATE noddy_core)
