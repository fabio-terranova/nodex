# GLFW
add_subdirectory(glfw)

# GLAD
add_library(glad ./glad/src/glad.c)
target_include_directories(glad PUBLIC ./glad/include/)
