# Library target
add_library(nodex_core SHARED
  ./src/Core.cpp
  ./src/Utils.cpp
  ./src/Filter.cpp
)

target_include_directories(nodex_core PUBLIC include)

target_link_libraries(nodex_core PUBLIC
    eigen
    OpenMP::OpenMP_CXX
)

# Enable import library generation on Windows
set_target_properties(nodex_core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
