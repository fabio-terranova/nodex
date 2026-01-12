#ifndef INCLUDE_CORE_FILTER_H_
#define INCLUDE_CORE_FILTER_H_

#include <Eigen/Dense>
#include <cassert>
#include <numbers>

namespace Noddy {
namespace Filter {
using namespace std::complex_literals;

using Eigen::Array2d;
using Eigen::ArrayX;
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::ArrayXi;
using Eigen::VectorXcd;
using Eigen::VectorXd;
using Eigen::VectorXi;

using std::numbers::pi;

using Complex = std::complex<double>;

struct Coeffs {
  VectorXd b{};
  VectorXd a{};
};

bool          operator==(const Coeffs& first, const Coeffs& second);
std::ostream& operator<<(std::ostream& os, const Coeffs& coeffs);

struct ZPK {
  VectorXcd z{};
  VectorXcd p{};
  double    k{};
};

std::ostream& operator<<(std::ostream& os, const ZPK& zpk);

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

ZPK analog2digital(ZPK analog, double fc, double fs, Type type);

ZPK bilinearTransform(const ZPK& analog, const double fs);

ZPK lp2lp(const ZPK& input, const double wc);
ZPK lp2hp(const ZPK& input, const double wc);

template <ZPK (*F)(const int), Type type>
ZPK iirFilter(const int n, double fc, double fs) {
  return analog2digital(F(n), fc, fs, type);
}

template <ZPK (*F)(const int, const double), Type type>
ZPK iirFilter(const int n, double fc, double fs, const double param) {
  return analog2digital(F(n, param), fc, fs, type);
}

Coeffs zpk2tf(const ZPK& zpk);

ArrayXd linearFilter(const Coeffs& filter, const VectorXd& x, VectorXd& si);
ArrayXd linearFilter(const Coeffs& filter, const VectorXd& x);
ArrayXd findEffectiveIR(const Coeffs& filter, const double epsilon = 1e-12,
                        const int maxLength = 10000);
ArrayXd fftFilter(const Coeffs& filter, const VectorXd& x,
                  const double epsilon = 1e-12, const int maxLength = 10000);
} // namespace Filter
} // namespace Noddy

#endif // INCLUDE_CORE_FILTER_H_
