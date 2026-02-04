#include "Filter.h"
#include "FilterEigen.h"
#include "Utils.h"
#include <Eigen/Dense>
#include <cassert>
#include <cmath>
#include <numbers>
#include <ostream>
#include <ranges>
#include <unsupported/Eigen/FFT>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace Nodex {
namespace Filter {
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::ArrayXi;
using Eigen::Index;
using Eigen::VectorXcd;
using Eigen::VectorXd;
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

ZPK analog2digital(ZPK analog, double fc, double fs, Mode mode = lowpass) {
  fc /= (fs / 2);
  fs = 2.0;
  const double warped{2.0 * fs * std::tan(std::numbers::pi * fc / fs)};

  if (mode == Mode::lowpass)
    analog = lp2lp(analog, warped);
  else if (mode == Mode::highpass) {
    analog = lp2hp(analog, warped);
  }

  return bilinearTransform(analog, fs);
}

ZPK analog2digital(ZPK analog, double fl, double fh, double fs, Mode mode) {
  double fc{std::sqrt(fl * fh)};
  double bw{fh - fl};

  fc /= (fs / 2);
  bw /= (fs / 2);
  fs = 2.0;
  const double fcWarped{2.0 * fs * std::tan(std::numbers::pi * fc / fs)};
  const double bwWarped{2.0 * fs * std::tan(std::numbers::pi * bw / fs)};

  if (mode == Mode::bandpass) {
    analog = lp2bp(analog, fcWarped, bwWarped);
  } else if (mode == Mode::bandstop) {
    analog = lp2bs(analog, fcWarped, bwWarped);
  }

  return bilinearTransform(analog, fs);
}

ZPK cheb1ap(const int n, const double rp) {
  using namespace std::complex_literals;

  if (n == 0)
    return {{}, {}, std::pow(10, -rp / 20.0)};

  // Map the p vector memory
  std::vector<Complex> pVec(static_cast<std::size_t>(n));
  EigenMap<ArrayXcd>   p(pVec.data(), n);

  const double eps{std::sqrt(std::pow(10, 0.1 * rp) - 1)};
  const double mu{1.0 / n * std::asinh(1 / eps)};

  const ArrayXd m{arange(-n + 1, n, 2).cast<double>()};
  const ArrayXd theta{std::numbers::pi * m / (2 * n)};
  p = -(mu + 1i * theta.cast<Complex>()).sinh();

  double k{std::real((-p).prod())};
  if (n % 2 == 0)
    k /= std::sqrt(1 + eps * eps);

  return {{}, pVec, k};
}

ZPK cheb2ap(const int n, const double rs) {
  using namespace std::complex_literals;
  if (n == 0)
    return {{}, {}, 1};

  const double de{1.0 / std::sqrt(std::pow(10, 0.1 * rs) - 1)};
  const double mu{std::asinh(1.0 / de) / n};

  ArrayXi m{};
  if (n % 2) {
    ArrayXi m1{arange(-n + 1, 0, 2)};
    ArrayXi m2{arange(2, n, 2)};
    m.conservativeResize(m1.size() + m2.size());
    m << m1, m2;
  } else {
    m = arange(-n + 1, n, 2);
  }

  const ArrayXcd z{
      -(1i / (std::numbers::pi * m.cast<double>() / (2.0 * n)).sin())
           .conjugate()};

  ArrayXcd p{
      -(1i * std::numbers::pi * arange(-n + 1, n, 2).cast<Complex>() / (2 * n))
           .exp(),
  };
  p = std::sinh(mu) * p.real() + 1i * std::cosh(mu) * p.imag();
  p = p.cwiseInverse();

  const double k{((-p).prod() / (-z).prod()).real()};

  const std::vector<Complex> zVec(z.data(), z.data() + z.size());
  const std::vector<Complex> pVec(p.data(), p.data() + p.size());

  return ZPK{zVec, pVec, k};
}

ZPK buttap(const int n) {
  using namespace std::complex_literals;

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
  const ArrayXd theta{std::numbers::pi * m / (2 * n)};
  p = -(1i * theta).exp();

  return ZPK{zVec, pVec, 1.0};
}

ZPK iirFilter(const int n, double fc, double fs, const Type type,
              const Mode mode, const double param) {
  ZPK analogFilter{};
  switch (type) {
  case butter:
    analogFilter = buttap(n);
    break;
  case cheb1:
    analogFilter = cheb1ap(n, param);
    break;
  case cheb2:
    analogFilter = cheb2ap(n, param);
    break;
  default:
    break;
  }

  return analog2digital(analogFilter, fc, fs, mode);
}

ZPK iirFilter(const int n, double fLow, double fHigh, double fs,
              const Type type, const Mode mode, const double param) {
  ZPK analogFilter{};
  switch (type) {
  case butter:
    analogFilter = buttap(n);
    break;
  case cheb1:
    analogFilter = cheb1ap(n, param);
    break;
  case cheb2:
    analogFilter = cheb2ap(n, param);
    break;
  default:
    break;
  }

  return analog2digital(analogFilter, fLow, fHigh, fs, mode);
}

double warpFreq(const double fc, const double fs) {
  return std::tan(std::numbers::pi * fc / fs);
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

ArrayXcd freqz(const EigenZPK&                  digitalFilter,
               const Eigen::Ref<const ArrayXd>& w) {
  using namespace std::complex_literals;

  ArrayXcd h{w.size()};
  ArrayXcd zm1{(w * 1i).exp()};

  for (auto i{0}; i < w.size(); i++) {
    h(i) = digitalFilter.k * (zm1(i) - digitalFilter.z).prod() /
           (zm1(i) - digitalFilter.p).prod();
  }

  return h;
}

std::vector<Complex> freqz(const ZPK&                 digitalFilter,
                           const std::vector<double>& w) {
  std::vector<Complex> h;
  h.resize(w.size());
  EigenMap<ArrayXcd> hMap{h.data(), static_cast<Index>(h.size())};

  EigenMap<const ArrayXd>  wMap{w.data(), static_cast<Index>(w.size())};
  EigenMap<const ArrayXcd> zMap{digitalFilter.z.data(),
                                static_cast<Index>(digitalFilter.z.size())};
  EigenMap<const ArrayXcd> pMap{digitalFilter.p.data(),
                                static_cast<Index>(digitalFilter.p.size())};

  hMap = freqz(EigenZPK(digitalFilter), wMap);

  return h;
}

ZPK lp2lp(const ZPK& input, const double wc) {
  ZPK output{};
  output.z.resize(input.z.size());
  output.p.resize(input.p.size());

  EigenMap<ArrayXcd> oz(output.z.data(), static_cast<Index>(output.z.size()));
  EigenMap<ArrayXcd> op(output.p.data(), static_cast<Index>(output.p.size()));

  EigenMap<const ArrayXcd> iz(input.z.data(),
                              static_cast<Index>(input.z.size()));
  EigenMap<const ArrayXcd> ip(input.p.data(),
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

  if (iz.size() > 0)
    oz.head(iz.size()) = wc * iz.cwiseInverse();
  if (ip.size() > 0)
    op = wc * ip.cwiseInverse();

  if (degree > 0) {
    EigenMap<ArrayXcd>(output.z.data() + input.z.size(),
                       static_cast<Index>(degree))
        .setZero();
  }

  output.k = input.k * std::real((-iz).prod() / (-ip).prod());

  return output;
}

ZPK lp2bp(const ZPK& input, const double wc, const double bw) {
  const auto degree = input.p.size() - input.z.size();

  ZPK output{};
  output.z.resize(2 * input.z.size() + degree);
  output.p.resize(2 * input.p.size());

  EigenMap<ArrayXcd> oz(output.z.data(), static_cast<Index>(output.z.size()));
  EigenMap<ArrayXcd> op(output.p.data(), static_cast<Index>(output.p.size()));

  EigenMap<const ArrayXcd> iz(input.z.data(),
                              static_cast<Index>(input.z.size()));
  EigenMap<const ArrayXcd> ip(input.p.data(),
                              static_cast<Index>(input.p.size()));

  if (iz.size() > 0) {
    ArrayXcd z_lp{iz * (bw / 2.0)};
    ArrayXcd term{(z_lp.square() - (wc * wc)).sqrt()};
    oz.head(iz.size())               = z_lp + term;
    oz.segment(iz.size(), iz.size()) = z_lp - term;
  }

  if (degree > 0) {
    oz.tail(degree).setZero();
  }

  if (ip.size() > 0) {
    ArrayXcd p_lp{ip * (bw / 2.0)};
    ArrayXcd term{(p_lp.square() - (wc * wc)).sqrt()};
    op.head(ip.size()) = p_lp + term;
    op.tail(ip.size()) = p_lp - term;
  }

  output.k = input.k * std::pow(bw, degree);

  return output;
}

ZPK lp2bs(const ZPK& input, const double wc, const double bw) {
  const auto degree{input.p.size() - input.z.size()};

  ZPK output{};
  output.z.resize(2 * input.z.size() + 2 * degree);
  output.p.resize(2 * input.p.size());

  EigenMap<ArrayXcd> oz(output.z.data(), static_cast<Index>(output.z.size()));
  EigenMap<ArrayXcd> op(output.p.data(), static_cast<Index>(output.p.size()));

  EigenMap<const ArrayXcd> iz(input.z.data(),
                              static_cast<Index>(input.z.size()));
  EigenMap<const ArrayXcd> ip(input.p.data(),
                              static_cast<Index>(input.p.size()));

  if (iz.size() > 0) {
    ArrayXcd z_hp{(bw / 2.0) * iz.cwiseInverse()};
    ArrayXcd term{(z_hp.square() - (wc * wc)).sqrt()};
    oz.head(iz.size())               = z_hp + term;
    oz.segment(iz.size(), iz.size()) = z_hp - term;
  }

  if (degree > 0) {
    oz.segment(2 * static_cast<Index>(input.z.size()), degree)
        .setConstant(std::complex<double>(0, wc));
    oz.tail(degree).setConstant(std::complex<double>(0, -wc));
  }

  if (ip.size() > 0) {
    ArrayXcd p_hp      = (bw / 2.0) * ip.cwiseInverse();
    ArrayXcd term      = (p_hp.square() - (wc * wc)).sqrt();
    op.head(ip.size()) = p_hp + term;
    op.tail(ip.size()) = p_hp - term;
  }

  output.k = input.k * std::real((-iz).prod() / (-ip).prod());

  return output;
}

ArrayXd roots2poly(const Eigen::Ref<const ArrayXcd>& roots) {
  ArrayXcd coeffs{1};
  coeffs(0) = 1.0;

  for (Index i{0}; i < roots.size(); ++i) {
    const Complex& r{roots[i]};

    ArrayXcd temp{ArrayXcd::Zero(coeffs.size() + 1)};
    temp.head(coeffs.size()) += coeffs;
    temp.tail(coeffs.size()) -= r * coeffs;

    coeffs = std::move(temp);
  }

  return coeffs.real();
}

std::vector<double> roots2poly(const std::vector<Complex>& roots) {
  EigenMap<const ArrayXcd> rMap(roots.data(), static_cast<Index>(roots.size()));
  ArrayXd                  coeffs{roots2poly(rMap)};
  return std::vector<double>(coeffs.data(), coeffs.data() + coeffs.size());
}

EigenCoeffs zpk2tf(const EigenZPK& zpk) {
  EigenCoeffs tf{roots2poly(zpk.z), roots2poly(zpk.p)};

  tf.b *= zpk.k;

  return tf;
}

Coeffs zpk2tf(const ZPK& zpk) {
  Coeffs tf{roots2poly(zpk.z), roots2poly(zpk.p)};

  for (auto& c : tf.b) {
    c *= zpk.k;
  }

  return tf;
}

RowMajorMatrixXd linearFilter(const EigenCoeffs&&                       filter,
                              const Eigen::Ref<const RowMajorMatrixXd>& x,
                              Eigen::Ref<RowMajorMatrixXd>              state) {
  const Index nRows{static_cast<Index>(x.rows())};

  RowMajorMatrixXd y(nRows, x.cols());
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (Index r = 0; r < nRows; ++r) {
    y.row(r) = linearFilter(filter, Eigen::Ref<const ArrayXd>{x.row(r)},
                            Eigen::Ref<ArrayXd>{state.row(r)});
  }

  return y;
}

ArrayXd linearFilter(const EigenCoeffs&               filter,
                     const Eigen::Ref<const ArrayXd>& x,
                     Eigen::Ref<ArrayXd>              state) {
  const Index nB{filter.b.size()};
  const Index nA{filter.a.size()};
  const Index nX{x.size()};
  const Index nS{std::max(nB, nA) - 1};

  if (state.size() < nS) {
    ArrayXd newState = ArrayXd::Zero(nS);
    if (state.size() > 0) {
      newState.head(state.size()) = state;
    }
    state = newState;
  }
  ArrayXd y(nX);

  for (Index k{0}; k < nX; ++k) {
    const double xk = x(k);

    y(k) = state(0) + filter.b(0) * xk;

    if (nS > 1) {
      state.head(nS - 1) = state.tail(nS - 1) +
                           filter.b.segment(1, nS - 1) * xk -
                           filter.a.segment(1, nS - 1) * y(k);
    }

    state(nS - 1) = filter.b(nB - 1) * xk - filter.a(nA - 1) * y(k);
  }

  return y;
}

ArrayXd linearFilter(const EigenCoeffs&               filter,
                     const Eigen::Ref<const ArrayXd>& x) {
  const auto nS{std::max(filter.b.size(), filter.a.size()) - 1};
  ArrayXd    state{ArrayXd::Zero(nS)};

  return linearFilter(filter, x, state);
}

Signal linearFilter(const Coeffs& filter, const Signal& x, Signal& si) {
  const EigenMap<const ArrayXd> xMap(x.data(), static_cast<Index>(x.size()));
  EigenMap<ArrayXd>             siMap(si.data(), static_cast<Index>(si.size()));

  EigenCoeffs eigenFilter{
      EigenMap<const ArrayXd>(filter.b.data(),
                              static_cast<Index>(filter.b.size())),
      EigenMap<const ArrayXd>(filter.a.data(),
                              static_cast<Index>(filter.a.size()))};

  Eigen::Ref<const ArrayXd> xVec{xMap};
  Eigen::Ref<ArrayXd>       siVec{siMap};
  const ArrayXd             yMap{linearFilter(eigenFilter, xVec, siVec)};
  Signal                    y(static_cast<std::size_t>(yMap.size()));
  EigenMap<ArrayXd>(y.data(), static_cast<Index>(y.size())) = yMap;

  return y;
}

Signal linearFilter(const Coeffs& filter, const Signal& x) {
  const auto nS{std::max(filter.b.size(), filter.a.size()) - 1};
  Signal     si(nS, 0.0);

  return linearFilter(filter, x, si);
}

ArrayXd findEffectiveIR(const EigenCoeffs& filter, const double epsilon,
                        const Index maxLength) {
  const Index nS{std::max(filter.b.size(), filter.a.size()) - 1};
  ArrayXd     si{ArrayXd::Zero(nS)};

  ArrayXd impulse{ArrayXd::Zero(maxLength)};
  impulse(0) = 1.0;
  Eigen::Ref<const ArrayXd> impulseRef{impulse};
  Eigen::Ref<ArrayXd>       siRef{si};
  ArrayXd                   ir{linearFilter(filter, impulseRef, siRef)};

  // find effective length
  Index irLength{ir.size()};
  for (auto i{ir.size() - 1}; i > 0; --i) {
    if (std::abs(ir(i)) >= epsilon) {
      irLength = i + 1;
      break;
    }
  }

  return ir.head(irLength);
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

ArrayXd fastConvolve(const Eigen::Ref<const ArrayXd>& f,
                     const Eigen::Ref<const ArrayXd>& g) {
  Eigen::FFT<double> fft;

  const std::size_t L{static_cast<std::size_t>(f.size())};
  const std::size_t M{static_cast<std::size_t>(g.size())};
  const auto        N = L + M - 1;

  // Find the next power of 2 for FFT efficiency
  int N_fft = 1;
  while (static_cast<std::size_t>(N_fft) < N)
    N_fft <<= 1;

  // Zero-pad both signals to N_fft
  VectorXd fPadded{VectorXd::Zero(N_fft)};
  VectorXd gPadded{VectorXd::Zero(N_fft)};
  fPadded.head(static_cast<Index>(L)) = f;
  gPadded.head(static_cast<Index>(M)) = g;

  VectorXcd F, G;
  fft.fwd(F, fPadded);
  fft.fwd(G, gPadded);

  VectorXcd FG{F.array() * G.array()};

  VectorXd yPad;
  fft.inv(yPad, FG);

  return yPad.head(static_cast<Index>(N));
}

ArrayXd fastConvolve(const Signal& f, const Eigen::Ref<const ArrayXd>& g) {
  const EigenMap<const ArrayXd> fMap(f.data(), static_cast<Index>(f.size()));

  return fastConvolve(fMap, g);
}

Signal fastConvolve(const Signal& f, const Signal& g) {
  const EigenMap<const ArrayXd> fMap(f.data(), static_cast<Index>(f.size()));
  const EigenMap<const ArrayXd> gMap(g.data(), static_cast<Index>(g.size()));

  const ArrayXd yMap{fastConvolve(fMap, gMap)};

  Signal y(static_cast<std::size_t>(yMap.size()));
  EigenMap<ArrayXd>(y.data(), static_cast<Index>(y.size())) = yMap;

  return y;
}

ArrayXd fftFilter(const EigenCoeffs& filter, const Eigen::Ref<const ArrayXd>& x,
                  const double epsilon, const Index maxLength) {
  const ArrayXd filterIR{findEffectiveIR(filter, epsilon, maxLength)};

  return fastConvolve(filterIR, x).head(x.size());
}

Signal fftFilter(const Coeffs& filter, const Signal& x, const double epsilon,
                 const std::size_t maxLength) {
  const Signal filterIR{findEffectiveIR(filter, epsilon, maxLength)};

  auto y{fastConvolve(filterIR, x)};

  if (y.size() > x.size())
    y.resize(x.size());

  return y;
}
} // namespace Filter
} // namespace Nodex
