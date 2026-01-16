#include "Filter.h"
#include "Utils.h"
#include <Eigen/Dense>
#include <cassert>
#include <cstddef>
#include <ostream>
#include <ranges>
#include <unsupported/Eigen/FFT>
#include <vector>

namespace Noddy {
namespace Filter {

using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::ArrayXi;
using Eigen::Index;
using Eigen::VectorXcd;
using Eigen::VectorXd;
using Eigen::VectorXi;
using Utils::arange;

template <typename T>
using EigenMap = Eigen::Map<T>;

bool operator==(const Coeffs& first, const Coeffs& second) {
  if (first.a != second.a)
    return false;
  if (first.b != second.b)
    return false;

  return true;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
  os << "[";
  if (!v.empty()) {
    os << v[0];
    for (const auto& vi : v | std::views::drop(1)) {
      os << ", " << vi;
    }
  }
  os << "]";

  return os;
}

std::ostream& operator<<(std::ostream& os, const Coeffs& coeffs) {
  os << "b: " << coeffs.b << '\n';
  os << "a: " << coeffs.a << '\n';

  return os;
}

std::ostream& operator<<(std::ostream& os, const ZPK& zpk) {
  os << "k: " << zpk.k << "\n";
  os << "z: " << zpk.z << "\n";
  os << "p: " << zpk.p << "\n";
  return os;
}

ZPK analog2digital(ZPK analog, double fc, double fs,
                   Type type = Type::lowpass) {
  assert(type == Type::lowpass or
         type == Type::highpass and
             "iirFilter(): only lowPass and highPass are "
             "implemented for single cutoff frequency");

  fc /= (fs / 2);
  fs = 2.0;
  const double warped{2.0 * fs * std::tan(std::numbers::pi * fc / fs)};

  if (type == Type::lowpass)
    analog = lp2lp(analog, warped);
  else if (type == Type::highpass) {
    analog = lp2hp(analog, warped);
  }

  return bilinearTransform(analog, fs);
}

ZPK cheb1ap(const int n, const double rp) {
  if (n == 0)
    return {{}, {}, std::pow(10, -rp / 20.0)};

  // Map the p vector memory
  std::vector<Complex> pVec(static_cast<std::size_t>(n));
  EigenMap<ArrayXcd>   p(pVec.data(), n);

  const double eps{std::sqrt(std::pow(10, 0.1 * rp) - 1)};
  const double mu{1.0 / n * std::asinh(1 / eps)};

  const ArrayXd m{arange(-n + 1, n, 2).cast<double>()};
  const ArrayXd theta{pi * m / (2 * n)};
  p = -(mu + 1i * theta.cast<Complex>()).sinh();

  double k{std::real((-p).prod())};
  if (n % 2 == 0)
    k /= std::sqrt(1 + eps * eps);

  return {{}, pVec, k};
}

ZPK cheb2ap(const int n, const double rs) {
  if (n == 0)
    return {{}, {}, 1};

  const double de{1.0 / std::sqrt(std::pow(10, 0.1 * rs) - 1)};
  const double mu{std::asinh(1.0 / de) / n};

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

  const std::vector<Complex> zVec(z.data(), z.data() + z.size());
  const std::vector<Complex> pVec(p.data(), p.data() + p.size());

  return ZPK{zVec, pVec, k};
}

ZPK buttap(const int n) {
  // no zeros
  std::vector<Complex> zVec(0);
  std::vector<Complex> pVec(static_cast<std::size_t>(n));
  EigenMap<ArrayXcd>   z(zVec.data(), n);
  EigenMap<ArrayXcd>   p(pVec.data(), n);

  // p_k = wc * exp(j * (2k + n - 1) * pi / 2n)
  // m = 2k + n - 1
  // theta = pi * m / (2n)
  // p_k = -exp(j * theta)
  const ArrayXd m{arange(-n + 1, n, 2).cast<double>()};
  const ArrayXd theta{pi * m / (2 * n)};
  p = -(1i * theta).exp();

  return ZPK{zVec, pVec, 1.0};
}

double warpFreq(const double fc, const double fs) {
  return std::tan(pi * fc / fs);
}

ZPK bilinearTransform(const ZPK& analog, const double fs) {
  ZPK    digital{};
  double fs2{2.0 * fs};

  digital.z.resize(analog.z.size());
  digital.p.resize(analog.p.size());

  EigenMap<ArrayXcd> dz(digital.z.data(), static_cast<Index>(digital.z.size()));
  EigenMap<ArrayXcd> dp(digital.p.data(), static_cast<Index>(digital.p.size()));

  EigenMap<const ArrayXcd> az(analog.z.data(),
                              static_cast<Index>(analog.z.size()));
  EigenMap<const ArrayXcd> ap(analog.p.data(),
                              static_cast<Index>(analog.p.size()));

  // z = (2fs + s) / (2fs - s)
  dz = (fs2 + az) / (fs2 - az);
  dp = (fs2 + ap) / (fs2 - ap);

  // if (numZeros > 0)
  //   digital.z = (fs2 + analog.z) / (fs2 - analog.z);
  // else
  //   digital.z = VectorXcd{0};
  //
  // if (numPoles > 0)
  //   digital.p = (fs2 + analog.p.array()) / (fs2 - analog.p.array());
  // else
  //   digital.p = VectorXcd{0};

  const auto degree{analog.p.size() - analog.z.size()};
  for (std::size_t i{0}; i < degree; ++i) {
    digital.z.push_back(-1.0);
  }

  // Recalculate gain
  // k' = k * prod(2fs - z)/prod(2fs - p)
  Complex num{(fs2 - az).prod()};
  Complex den{(fs2 - ap).prod()};
  digital.k = analog.k * std::real(num / den);

  return digital;
}

ZPK lp2lp(const ZPK& input, const double wc) {
  ZPK output{};
  output.z.resize(input.z.size());
  output.p.resize(input.p.size());

  EigenMap<VectorXcd> oz(output.z.data(), static_cast<Index>(output.z.size()));
  EigenMap<VectorXcd> op(output.p.data(), static_cast<Index>(output.p.size()));

  EigenMap<const VectorXcd> iz(input.z.data(),
                               static_cast<Index>(input.z.size()));
  EigenMap<const VectorXcd> ip(input.p.data(),
                               static_cast<Index>(input.p.size()));

  // Transformation
  oz = iz * wc;
  op = ip * wc;

  // Update gain
  const auto degree{input.p.size() - input.z.size()};
  output.k = input.k * std::pow(wc, degree);

  return output;
}

ZPK lp2hp(const ZPK& input, const double wc) {
  const auto degree{input.p.size() - input.z.size()};

  ZPK output{};
  output.z.resize(input.z.size() + degree);
  output.p.resize(input.p.size());

  EigenMap<ArrayXcd> oz(output.z.data(), static_cast<Index>(output.z.size()));
  EigenMap<ArrayXcd> op(output.p.data(), static_cast<Index>(output.p.size()));

  EigenMap<const ArrayXcd> iz(input.z.data(),
                              static_cast<Index>(input.z.size()));
  EigenMap<const ArrayXcd> ip(input.p.data(),
                              static_cast<Index>(input.p.size()));

  oz = wc * iz.cwiseInverse();
  op = wc * ip.cwiseInverse();

  if (degree > 0) {
    EigenMap<VectorXcd>(output.z.data() + input.z.size(),
                        static_cast<Index>(degree))
        .setZero();
  }

  output.k = input.k * std::real((-iz).prod() / (-ip).prod());

  return output;
}

std::vector<double> roots2poly(const std::vector<Complex>& roots) {
  VectorXcd coeffs{1};
  coeffs(0) = 1.0;

  for (std::size_t i{0}; i < roots.size(); ++i) {
    const Complex& r{roots[i]};

    VectorXcd temp{VectorXcd::Zero(coeffs.size() + 1)};
    temp.head(coeffs.size()) += coeffs;
    temp.tail(coeffs.size()) -= r * coeffs;

    coeffs = std::move(temp);
  }

  std::vector<double> result(static_cast<std::size_t>(coeffs.size()));
  for (std::size_t i{0}; i < result.size(); ++i) {
    result[i] = coeffs(static_cast<Index>(i)).real();
  }

  return result;
}

Coeffs zpk2tf(const ZPK& zpk) {
  Coeffs tf{roots2poly(zpk.z), roots2poly(zpk.p)};

  for (auto& c : tf.b) {
    c *= zpk.k;
  }

  return tf;
}

Signal linearFilter(const Coeffs& filter, const Signal& x, Signal& si) {
  const auto nB{filter.b.size()};
  const auto nA{filter.a.size()};
  const auto nX{x.size()};
  const auto nS{std::max(nB, nA) - 1};

  // normalize filter coeffs
  const double a0{filter.a[0]};

  const auto bMap{
      EigenMap<const VectorXd>(filter.b.data(), static_cast<Index>(nB))};
  const auto aMap{
      EigenMap<const VectorXd>(filter.a.data(), static_cast<Index>(nA))};
  VectorXd b{bMap / a0};
  VectorXd a{aMap / a0};

  if (si.size() < nS)
    si.resize(nS, 0.0);
  EigenMap<VectorXd> state(si.data(), static_cast<Index>(nS));

  Signal            yVec(nX);
  EigenMap<ArrayXd> y(yVec.data(), static_cast<Index>(nX));

  for (std::size_t k{0}; k < nX; ++k) {
    const double xk = x[k];

    y(static_cast<Index>(k)) = state(0) + b(0) * xk;

    if (nS > 1) {
      state.head(nS - 1) = state.tail(nS - 1) + b.segment(1, nS - 1) * xk -
                           a.segment(1, nS - 1) * y(static_cast<Index>(k));
    }

    state(nS - 1) = b(nB - 1) * xk - a(nA - 1) * y(k);
  }

  return yVec;
}

Signal linearFilter(const Coeffs& filter, const Signal& x) {
  const auto nS{std::max(filter.b.size(), filter.a.size()) - 1};
  Signal     si(nS, 0.0);

  return linearFilter(filter, x, si);
}

Signal findEffectiveIR(const Coeffs& filter, const double epsilon,
                       const std::size_t maxLength) {
  const std::size_t   nS{std::max(filter.b.size(), filter.a.size()) - 1};
  std::vector<double> si(nS, 0.0);

  Signal impulse(maxLength, 0.0);
  impulse[0] = 1.0;
  Signal ir{linearFilter(filter, impulse, si)};

  // find effective length
  std::size_t irLength{ir.size()};
  for (std::size_t i{ir.size() - 1}; i > 0; --i) {
    if (std::abs(ir[i]) >= epsilon) {
      irLength = i + 1;
      break;
    }
  }

  ir.resize(irLength);

  return ir;
}

Signal fastConvolve(const Signal& f, const Signal& g) {
  Eigen::FFT<double> fft;

  const std::size_t L{f.size()};
  const std::size_t M{g.size()};
  const auto        N = L + M - 1;

  // Find the next power of 2 for FFT efficiency
  int N_fft = 1;
  while (static_cast<std::size_t>(N_fft) < N)
    N_fft <<= 1;

  EigenMap<const ArrayXd> fMap(f.data(), static_cast<Index>(f.size()));
  EigenMap<const ArrayXd> gMap(g.data(), static_cast<Index>(g.size()));

  // Zero-pad both signals to N_fft
  VectorXd fPadded{VectorXd::Zero(N_fft)};
  VectorXd gPadded{VectorXd::Zero(N_fft)};
  fPadded.head(L) = fMap;
  gPadded.head(M) = gMap;

  VectorXcd F, G;
  fft.fwd(F, fPadded);
  fft.fwd(G, gPadded);

  VectorXcd FG{F.array() * G.array()};

  VectorXd yPad;
  fft.inv(yPad, FG);

  Signal y(N);
  EigenMap<VectorXd>(y.data(), static_cast<Index>(N)) = yPad.head(N);

  return y;
}

Signal fftFilter(const Coeffs& filter, const Signal& x, const double epsilon,
                 const std::size_t maxLength) {
  const auto filterIR{findEffectiveIR(filter, epsilon, maxLength)};

  auto y{fastConvolve(filterIR, x)};

  if (y.size() > x.size())
    y.resize(x.size());

  return y;
}
} // namespace Filter
} // namespace Noddy
