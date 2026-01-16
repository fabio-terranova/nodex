#include "Filter.h"
#include <cstddef>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(noddy_py, m, py::mod_gil_not_used()) {
  using namespace Noddy::Filter;

  m.doc() = "Python bindings for Noddy filter operations.";

  m.def(
      "fft_filter",
      [](const Signal& b, const Signal& a, const Signal& x,
         const double epsilon, const std::size_t max_length) {
        Signal output{Noddy::Filter::fftFilter({b, a}, x, epsilon, max_length)};

        return output;
      },
      py::arg("b"), py::arg("a"), py::arg("x"), py::arg("epsilon") = 1e-12,
      py::arg("max_length") = 10000);

  m.def(
      "lfilter",
      [](const Signal& b, const Signal& a, const Signal& x) {
        return Noddy::Filter::linearFilter({b, a}, x);
      },
      py::arg("b"), py::arg("a"), py::arg("x"));
}
