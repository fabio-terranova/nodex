#include "Filter.h"
#include "Utils.h"
#include <Eigen/Dense>

namespace Noddy {
namespace Filter {
using Utils::arange;

ZPK cheb1ap(const int n, const double rp) {
  if (n == 0)
    return {{}, {}, std::pow(10, -rp / 20.0)};

  Eigen::ArrayXcd z{0};

  const auto eps{std::sqrt(std::pow(10, 0.1 * rp) - 1)};
  const auto mu{1.0 / n * std::asinh(1 / eps)};

  const ArrayXd m{arange(-n + 1, n, 2).cast<double>()};
  const auto    theta{pi * m / (2 * n)};
  const auto    p{-(mu + 1i * theta).sinh()};

  double k{std::real((-p).prod())};
  if (n % 2 == 0)
    k /= std::sqrt(1 + eps * eps);

  return {z, p, k};
}

ZPK cheb2ap(const int n, const double rs) {
  if (n == 0)
    return {{}, {}, 1};

  const auto de{1.0 / std::sqrt(std::pow(10, 0.1 * rs) - 1)};
  const auto mu{std::asinh(1.0 / de) / n};

  VectorXi m{};
  if (n % 2) {
    ArrayXi m1{arange(-n + 1, 0, 2)};
    ArrayXi m2{arange(2, n, 2)};
    m.conservativeResize(m1.size() + m2.size());
    m << m1, m2;
  } else {
    m = arange(-n + 1, n, 2);
  }

  const VectorXcd z{
      -(1i / (pi * m.cast<double>() / (2.0 * n)).array().sin()).conjugate()};

  VectorXcd p{
      -(1i * pi * arange(-n + 1, n, 2).cast<Complex>() / (2 * n)).exp(),
  };
  p = std::sinh(mu) * p.real() + 1i * std::cosh(mu) * p.imag();
  p = p.cwiseInverse();

  const double k{((-p).prod() / (-z).prod()).real()};

  return ZPK{z, p, k};
}

ZPK buttap(const int n) {
  // no zeros
  ArrayXcd z{0};

  // p_k = wc * exp(j * (2k + n - 1) * pi / 2n)
  // m = 2k + n - 1
  // theta = pi * m / (2n)
  // p_k = -exp(j * theta)
  const ArrayXd  m{arange(-n + 1, n, 2).cast<double>()};
  const ArrayXd  theta{pi * m / (2 * n)};
  const ArrayXcd p{-(1i * theta).exp()};

  return ZPK{z, p, 1.0};
}

constexpr double warpFreq(const double fc, const double fs) {
  return std::tan(pi * fc / fs);
}

ZPK bilinearTransform(const ZPK& analog, const double fs) {
  ZPK    digital{};
  double fs2{2.0 * fs};

  const long numZeros{analog.z.size()};
  const long numPoles{analog.p.size()};
  const long degree{numPoles - numZeros};

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
    digital.z.tail(degree).setConstant(-1.0);
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

  output.k = input.k * std::real((-input.z).prod() / (-input.p).prod());

  return output;
}

VectorXcd roots2poly(const VectorXcd& roots) {
  VectorXcd coeffs{1};
  coeffs[0] = 1.0;

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
