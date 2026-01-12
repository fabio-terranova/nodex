# Library target
add_library(noddy_core SHARED
  ./src/Utils.cpp
  ./src/Filter.cpp
  ./src/Core.cpp
)

target_include_directories(noddy_core PUBLIC include)

target_link_libraries(noddy_core PUBLIC
    eigen
)
