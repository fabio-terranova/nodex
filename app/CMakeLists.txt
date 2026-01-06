find_package(OpenGL REQUIRED)

add_executable(noddy_gui
    src/main.cpp
    ${CMAKE_SOURCE_DIR}/external/glad/src/glad.c
)

target_include_directories(noddy_gui
   PRIVATE ${CMAKE_SOURCE_DIR}/external/glad/include
)

target_link_libraries(noddy_gui PRIVATE
    noddy_core
    glfw
    OpenGL::GL
)
