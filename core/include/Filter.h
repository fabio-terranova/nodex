#ifndef INCLUDE_CORE_FILTER_H_
#define INCLUDE_CORE_FILTER_H_

#include <Eigen/Dense>
#include <numbers>

namespace Noddy {
namespace Filter {
using namespace std::complex_literals;

using Eigen::Array2d;
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::ArrayXi;
using Eigen::VectorXcd;
using Eigen::VectorXd;
using Eigen::VectorXi;

using std::numbers::pi;

using Complex = std::complex<double>;

struct Coeffs {
  VectorXcd b{};
  VectorXcd a{};
};

struct ZPK {
  VectorXcd z{};
  VectorXcd p{};
  double    k{};
};

enum Type {
  lowpass,
  highpass,
  bandpass,
  bandstop,
  maxFilters,
};

ZPK buttap(const int n);
ZPK cheb1ap(const int n, const double rp);
ZPK cheb2ap(const int n, const double rs);

ZPK bilinearTransform(const ZPK& zpkA, const double fs);

ZPK lp2lp(const ZPK& input, const double wc);
ZPK lp2hp(const ZPK& input, const double wc);

inline ZPK analog2digital(ZPK analog, double fc, double fs,
                          Type type = Type::lowpass) {
  assert(type == Type::lowpass or
         type == Type::highpass && "iirFilter(): only lowPass and highPass are "
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

template <ZPK (*F)(const int), Type type>
ZPK iirFilter(const int n, double fc, double fs) {
  ZPK analog{F(n)};

  return analog2digital(analog, fc, fs, type);
}

template <ZPK (*F)(const int, const double), Type type>
ZPK iirFilter(const int n, double fc, double fs, const double param) {
  ZPK analog{F(n, param)};

  return analog2digital(analog, fc, fs, type);
}

Coeffs zpk2tf(const ZPK& zpk);
} // namespace Filter
} // namespace Noddy

#endif // INCLUDE_CORE_FILTER_H_
