#include "Filter.h"
#include <Eigen/Dense>
#include <cassert>
#include <cmath>
#include <complex>
#include <functional>
#include <numbers>

namespace Noddy {
namespace Filter {
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::ArrayXi;
using Eigen::VectorXcd;

ArrayXi arange(const int start, int stop, const int step) {
  if (step == 0)
    return ArrayXi{};

  int num{(stop - start + (step > 0 ? step - 1 : step + 1)) / step};
  if (num <= 0)
    return ArrayXi{};

  int actualStop{start + (num - 1) * step};

  return ArrayXi::LinSpaced(num, start, actualStop);
}

ZPK buttap(const int n) {
  // no zeros
  ArrayXcd z{0};

  // p_k = wc * exp(j * (2k + n - 1) * pi / 2n)
  // m = 2k + n - 1
  // theta = pi * m / (2n)
  // p_k = -exp(j * theta)
  ArrayXd  m{arange(-n + 1, n, 2).cast<double>()};
  ArrayXd  theta{std::numbers::pi * m / (2 * n)};
  ArrayXcd p{-(Complex(0.0, 1.0) * theta).exp()};

  return ZPK{z, p, 1.0};
}

constexpr double warpFreq(const double fc, const double fs) {
  return std::tan(std::numbers::pi * fc / fs);
}

ZPK bilinearTransform(const ZPK& analog, const double fs) {
  ZPK    digital{};
  double fs2{2.0 * fs};

  long numZeros{analog.z.size()};
  long numPoles{analog.p.size()};
  long degree{numPoles - numZeros};

  // z = (2fs + s) / (2fs - s)
  if (numZeros > 0)
    digital.z = (fs2 + analog.z.array()) / (fs2 - analog.z.array());
  else
    digital.z = VectorXcd{0};

  if (numPoles > 0)
    digital.p = (fs2 + analog.p.array()) / (fs2 - analog.p.array());
  else
    digital.p = VectorXcd{0};

  if (degree > 0) {
    digital.z.conservativeResize(numPoles);
    digital.z.tail(degree).setConstant(Complex{-1.0, 0.0});
  }

  // Recalculate gain
  // k' = k * prod(2fs - z)/prod(2fs - p)
  auto num{(fs2 - analog.z.array()).prod()};
  auto den{(fs2 - analog.p.array()).prod()};

  digital.k = analog.k * std::real(num / den);

  return digital;
}

ZPK lp2lp(const ZPK& input, const double wc) {
  ZPK  output{};
  auto degree{input.p.size() - input.z.size()};

  output.z = input.z * wc;
  output.p = input.p * wc;
  output.k = input.k * std::pow(wc, degree);

  return output;
}

ZPK lp2hp(const ZPK& input, const double wc) {
  ZPK  output{};
  auto degree{input.p.size() - input.z.size()};

  output.z = wc * input.z.cwiseInverse();
  output.p = wc * input.p.cwiseInverse();

  output.z.conservativeResize(output.z.size() + degree);
  output.z.tail(degree).setConstant(0);

  output.k = input.k * std::real(input.z.prod() / input.p.prod());

  return output;
}

ZPK iirFilter(const int n, double fc, double fs,
              std::function<ZPK(const int)> filter, Type type = Type::lowPass) {
  assert(type == Type::lowPass or
         type == Type::highPass &&
             "iirFilter(): Single frequency value provided but FilterType != "
             "lowPass or highPass");

  ZPK analog{filter(n)};

  fc /= (fs / 2);

  fs = 2.0;
  const double warped{2.0 * fs * std::tan(std::numbers::pi * fc / fs)};

  std::function<ZPK(const ZPK&, const double)> convFunc;
  switch (type) {
  case Type::lowPass:
    convFunc = lp2lp;
    break;
  case Type::highPass:
    convFunc = lp2hp;
    break;
  default:
    break;
  }
  analog = convFunc(analog, warped);

  return bilinearTransform(analog, fs);
}

// TODO integrate bandpass and bandstop
ZPK iirFilter(const int n, Array2d fc, double fs,
              std::function<ZPK(const int)> filter, Type type) {
  ZPK analog{filter(n)};

  fc /= (fs / 2);

  fs = 2.0;
  const Array2d warped{2.0 * fs * tan(std::numbers::pi * fc / fs)};

  std::function<ZPK(const ZPK&, const double)> convFunc;
  switch (type) {
  case Type::bandPass:
    convFunc = lp2lp;
    break;
  case Type::bandStop:
  default:
    break;
  }

  // analog = convFunc(analog, wc)

  return bilinearTransform(analog, fs);
}

VectorXcd roots2poly(const VectorXcd& roots) {
  VectorXcd coeffs{1};
  coeffs[0] = Complex{1.0, 0.0};

  for (int i{0}; i < roots.size(); ++i) {
    const Complex& r{roots[i]};

    VectorXcd temp{VectorXcd::Zero(coeffs.size() + 1)};
    temp.head(coeffs.size()) += coeffs;
    temp.tail(coeffs.size()) -= r * coeffs;

    coeffs = std::move(temp);
  }

  return coeffs;
}

Coeffs zpk2tf(const ZPK& zpk) {
  return {zpk.k * roots2poly(zpk.z), roots2poly(zpk.p)};
}
} // namespace Filter
} // namespace Noddy
