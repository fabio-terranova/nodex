#include "Filter.h"
#include "FilterEigen.h"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

namespace py = pybind11;

py::array_t<double>
lfilter_multi(py::array_t<double, py::array::c_style | py::array::forcecast> b,
              py::array_t<double, py::array::c_style | py::array::forcecast> a,
              py::array_t<double, py::array::c_style | py::array::forcecast> x);

PYBIND11_MODULE(noddy_py, m, py::mod_gil_not_used()) {
  using Noddy::Filter::Signal;

  m.doc() = "Python bindings for Noddy filter operations.";

  m.def(
      "fft_filter",
      [](const std::vector<double>& b, const std::vector<double>& a,
         const Signal& x, const double epsilon, const std::size_t max_length) {
        Signal output{Noddy::Filter::fftFilter({b, a}, x, epsilon, max_length)};

        return output;
      },
      py::arg("b"), py::arg("a"), py::arg("x"), py::arg("epsilon") = 1e-12,
      py::arg("max_length") = 10000);

  m.def(
      "lfilter",
      [](const std::vector<double>& b, const std::vector<double>& a,
         const Signal& x) { return Noddy::Filter::linearFilter({b, a}, x); },
      py::arg("b"), py::arg("a"), py::arg("x"));

  m.def("lfilter_multi", &lfilter_multi, py::arg("b"), py::arg("a"),
        py::arg("x"));
}

py::array_t<double> lfilter_multi(
    py::array_t<double, py::array::c_style | py::array::forcecast> b,
    py::array_t<double, py::array::c_style | py::array::forcecast> a,
    py::array_t<double, py::array::c_style | py::array::forcecast> x) {
  using namespace Noddy::Filter;

  const auto n_channels{static_cast<Index>(x.shape(0))};
  const auto n_samples{static_cast<Index>(x.shape(1))};

  Eigen::Map<const VectorXd>         b_map(b.data(), b.size());
  Eigen::Map<const VectorXd>         a_map(a.data(), a.size());
  Eigen::Map<const RowMajorMatrixXd> x_map(x.data(), n_channels, n_samples);

  py::array_t<double> y_out({static_cast<py::ssize_t>(n_channels),
                             static_cast<py::ssize_t>(n_samples)});

  Eigen::Map<RowMajorMatrixXd> y_map(y_out.mutable_data(), n_channels,
                                     n_samples);

  RowMajorMatrixXd state{RowMajorMatrixXd::Zero(
      n_channels, std::max(b_map.size(), a_map.size()) - 1)};

  y_map = linearFilter({b_map, a_map}, x_map, state);

  return y_out;
}
