# Executable
add_executable(noddy_test
    ./src/test_core.cpp
)

target_link_libraries(noddy_test PRIVATE
    eigen
    noddy_core
)
