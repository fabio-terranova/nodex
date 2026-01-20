#include "Filter.h"
#include "FilterEigen.h"
#include <complex>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

namespace py = pybind11;

py::array_t<double>
lfilter_multi(py::array_t<double, py::array::c_style | py::array::forcecast> b,
              py::array_t<double, py::array::c_style | py::array::forcecast> a,
              py::array_t<double, py::array::c_style | py::array::forcecast> x);

PYBIND11_MODULE(pynodex, m, py::mod_gil_not_used()) {
  using Nodex::Filter::Signal;

  m.doc() = "Python bindings for Nodex operations.";

  m.def(
      "fft_filter",
      [](const std::vector<double>& b, const std::vector<double>& a,
         const Signal& x, const double epsilon, const std::size_t max_length) {
        return Nodex::Filter::fftFilter({b, a}, x, epsilon, max_length);
      },
      py::arg("b"), py::arg("a"), py::arg("x"), py::arg("epsilon") = 1e-12,
      py::arg("max_length") = 10000);

  m.def(
      "lfilter",
      [](const std::vector<double>& b, const std::vector<double>& a,
         const Signal& x) { return Nodex::Filter::linearFilter({b, a}, x); },
      py::arg("b"), py::arg("a"), py::arg("x"));

  m.def("lfilter_multi", &lfilter_multi, py::arg("b"), py::arg("a"),
        py::arg("x"));

  m.def(
      "freqz",
      [](const std::vector<std::complex<double>>& z,
         const std::vector<std::complex<double>>& p, const double k,
         const std::vector<double>& w) {
        Nodex::Filter::ZPK digitalFilter{z, p, k};

        return Nodex::Filter::freqz(digitalFilter, w);
      },
      py::arg("z"), py::arg("p"), py::arg("k"), py::arg("w"));
}

py::array_t<double> lfilter_multi(
    py::array_t<double, py::array::c_style | py::array::forcecast> b,
    py::array_t<double, py::array::c_style | py::array::forcecast> a,
    py::array_t<double, py::array::c_style | py::array::forcecast> x) {
  using namespace Nodex::Filter;

  const auto n_channels{static_cast<Index>(x.shape(0))};
  const auto n_samples{static_cast<Index>(x.shape(1))};

  Eigen::Map<const ArrayXd>          b_map(b.data(), b.size());
  Eigen::Map<const ArrayXd>          a_map(a.data(), a.size());
  Eigen::Map<const RowMajorMatrixXd> x_map(x.data(), n_channels, n_samples);

  py::array_t<double>          y_out({static_cast<py::ssize_t>(n_channels),
                                      static_cast<py::ssize_t>(n_samples)});
  Eigen::Map<RowMajorMatrixXd> y_map(y_out.mutable_data(), n_channels,
                                     n_samples);

  RowMajorMatrixXd state{RowMajorMatrixXd::Zero(
      n_channels, std::max(b_map.size(), a_map.size()) - 1)};

  y_map = linearFilter({b_map, a_map}, x_map, state);

  return y_out;
}
