#ifndef INCLUDE_CORE_FILTER_H_
#define INCLUDE_CORE_FILTER_H_

#include <complex>
#include <cstddef>
#include <numbers>
#include <vector>

namespace Noddy {
namespace Filter {
using namespace std::complex_literals;

using std::numbers::pi;

using Complex = std::complex<double>;
using Signal  = std::vector<double>;

struct Coeffs {
  std::vector<double> b{};
  std::vector<double> a{};
};

bool          operator==(const Coeffs& first, const Coeffs& second);
std::ostream& operator<<(std::ostream& os, const Coeffs& coeffs);

struct ZPK {
  std::vector<Complex> z{};
  std::vector<Complex> p{};
  double               k{};
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

// Filtering functions (IIR compatible)
Signal linearFilter(const Coeffs& filter, const Signal& x, Signal& si);
Signal linearFilter(const Coeffs& filter, const Signal& x);

// Caclulate the approximate impulse response of a filter. Useful to convert an
// IIR filter to a FIR filter
Signal findEffectiveIR(const Coeffs& filter, const double epsilon = 1e-12,
                       const std::size_t maxLength = 10000);
// fftFilter() uses findEffectiveIR() to transform the IIR filter to a FIR
// filter and gets the filtered signal by convolution (fast convolution by
// multiplication in the frequency domain, thus "fft" in the name)
Signal fftFilter(const Coeffs& filter, const Signal& x,
                 const double      epsilon   = 1e-12,
                 const std::size_t maxLength = 10000);
} // namespace Filter
} // namespace Noddy

#endif // INCLUDE_CORE_FILTER_H_
