find_package(OpenGL REQUIRED)

add_executable(noddy_gui
    ./src/main.cpp
    ${CMAKE_SOURCE_DIR}/external/glad/src/glad.c
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/external/implot/implot.cpp
    ${CMAKE_SOURCE_DIR}/external/implot/implot_items.cpp
)

target_include_directories(noddy_gui
   PRIVATE ${CMAKE_SOURCE_DIR}/external/glad/include
   PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui
   PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui/backends
   PRIVATE ${CMAKE_SOURCE_DIR}/external/implot
)

target_link_libraries(noddy_gui PRIVATE
    noddy_core
    glfw
    OpenGL::GL
)
