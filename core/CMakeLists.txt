# Library target
add_library(noddy_core STATIC
  src/NoddyCore.cpp
)

target_include_directories(noddy_core PUBLIC include)
