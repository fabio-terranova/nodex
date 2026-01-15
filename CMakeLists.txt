cmake_minimum_required(VERSION 3.16...3.28)

project(noddy LANGUAGES CXX C)

include(CTest)

find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Dependencies
add_subdirectory(external/glfw)
add_subdirectory(external/eigen)

# Modules
add_subdirectory(core)
add_subdirectory(app)
add_subdirectory(bindings)
add_subdirectory(tests)
