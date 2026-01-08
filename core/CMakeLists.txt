# Library target
add_library(noddy_core STATIC
  ./src/Core.cpp
  ./src/Filter.cpp
)

target_include_directories(noddy_core PUBLIC include)

target_link_libraries(noddy_core PRIVATE
    eigen
)
