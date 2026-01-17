set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(nodex_py ./python/src/module.cpp)
target_link_libraries(nodex_py PRIVATE nodex_core)