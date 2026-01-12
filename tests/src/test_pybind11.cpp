#include "Filter.h"
#include "Utils.h"
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(noddy_bind, m, py::mod_gil_not_used()) {
  using namespace Noddy::Filter;

  m.doc() = "pybind11";

  m.def(
      "fft_filter",
      [](const VectorXd& b, const VectorXd& a, const VectorXd& x,
         double epsilon, int max_length) {
        Coeffs              filter(b, a);
        Noddy::Utils::Timer timer{};

        timer.reset();
        VectorXd output{
            Noddy::Filter::fftFilter(filter, x, epsilon, max_length)};
        auto elapsed{timer.elapsed()};

        py::print("Time taken (normal):\t", elapsed, "s");

        return output;
      },
      py::arg("b"), py::arg("a"), py::arg("x"), py::arg("epsilon") = 1e-10,
      py::arg("max_length") = 10000);

  m.def(
      "lfilter",
      [](const VectorXd& b, const VectorXd& a, const VectorXd& x) {
        Coeffs              filter(b, a);
        Noddy::Utils::Timer timer{};

        timer.reset();
        VectorXd output{Noddy::Filter::linearFilter(filter, x)};
        auto     elapsed{timer.elapsed()};

        py::print("Time taken (fft):\t", elapsed, "s");

        return output;
      },
      py::arg("b"), py::arg("a"), py::arg("x"));
}
