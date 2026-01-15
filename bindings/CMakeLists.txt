set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(noddy_py ./python/src/module.cpp)
target_link_libraries(noddy_py PRIVATE noddy_core)
