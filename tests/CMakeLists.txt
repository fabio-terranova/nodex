# Executable
add_executable(noddy_test
    ./src/test_filter.cpp
)

target_link_libraries(noddy_test PRIVATE
    eigen
    noddy_core
)
