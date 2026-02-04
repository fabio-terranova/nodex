#ifndef INCLUDE_CORE_FILTER_H_
#define INCLUDE_CORE_FILTER_H_

#include "Utils.h"
#include <cstddef>
#include <vector>

/**
 * @file Filter.h
 * @brief Digital filter design and application functions.
 */
namespace Nodex::Filter {
using Nodex::Utils::Complex;
using Nodex::Utils::Signal;

// Filter transfer function coefficients representation
struct Coeffs {
  std::vector<double> b{};
  std::vector<double> a{};
};

bool          operator==(const Coeffs& first, const Coeffs& second);
std::ostream& operator<<(std::ostream& os, const Coeffs& coeffs);

// Filter zeros-poles-gain representation
struct ZPK {
  std::vector<Complex> z{};
  std::vector<Complex> p{};
  double               k{};
};

/**
 * Outputs the zeros-poles-gain representation to a stream.
 * @param os The output stream
 * @param zpk The zeros-poles-gain representation
 * @return The output stream
 */
std::ostream& operator<<(std::ostream& os, const ZPK& zpk);

/**
 * Applies a linear filter to the input signal x using the given filter
 * coefficients and state.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @param state The filter state (should be maintained between calls)
 * @return The filtered output signal
 */
Signal linearFilter(const Coeffs& filter, const Signal& x, Signal& si);

/**
 * Applies a linear filter to the input signal x using the given filter
 * coefficients. No state version.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @return The filtered output signal
 */
Signal linearFilter(const Coeffs& filter, const Signal& x);

/**
 * Computes the effective impulse response of a filter given its coefficients.
 * @param filter The filter coefficients
 * @param epsilon The tolerance for the effective impulse response calculation
 * @param maxLength The maximum length of the effective impulse response
 * @return The effective impulse response as a signal
 */
Signal findEffectiveIR(const Coeffs& filter, const double epsilon = 1e-12,
                       const std::size_t maxLength = 10000);

/**
 * Applies a filter to a signal using FFT-based convolution.
 * @param filter The filter coefficients
 * @param x The input signal
 * @param epsilon The tolerance for the effective impulse response calculation
 * @param maxLength The maximum length of the effective impulse response
 * @return The filtered signal
 */
Signal fftFilter(const Coeffs& filter, const Signal& x,
                 const double      epsilon   = 1e-12,
                 const std::size_t maxLength = 10000);

// Zeros-poles-gain to transfer function coefficients conversion
Coeffs zpk2tf(const ZPK& zpk);

// Standard filter modes
enum Mode {
  lowpass,
  highpass,
  bandpass, // TODO: implement
  bandstop, // TODO: implement
  maxMode,
};

// Standard filter types
enum Type {
  butter,
  cheb1,
  cheb2,
  maxType,
};

/**
 * Transforms an analogue filter to a digital filter using the given mode and
 * bilinear transform.
 * @param analog The analogue filter in zero-pole-gain representation
 * @param fc The cutoff frequency
 * @param fs The sampling frequency
 * @param mode The filter mode (lowpass or highpass)
 * @return The digital filter in zero-pole-gain representation
 */
ZPK analog2digital(ZPK analog, double fc, double fs, Mode mode);

/**
 * Transforms an analogue filter to a digital bandpass or bandstop filter using
 * the bilinear transform.
 * @param analog The analogue filter in zero-pole-gain representation
 * @param fLow The lower cutoff frequency
 * @param fHigh The upper cutoff frequency
 * @param fs The sampling frequency
 * @param mode The filter mode (bandpass or bandstop)
 * @return The digital filter in zero-pole-gain representation
 */
ZPK analog2digital(ZPK analog, double fLow, double fHigh, double fs,
                   Mode mode = bandpass);

/**
 * Applies the bilinear transform to an analogue filter.
 * @param analog The analogue filter in zero-pole-gain representation
 * @param fs The sampling frequency
 * @return The digital filter in zero-pole-gain representation
 */
ZPK bilinearTransform(const ZPK& analog, const double fs);

/**
 * Transforms a lowpass filter to a lowpass filter with different cutoff
 * frequency.
 * @param input The input lowpass filter in zero-pole-gain representation
 * @param wc The new cutoff frequency
 * @return The transformed lowpass filter in zero-pole-gain representation
 */
ZPK lp2lp(const ZPK& input, const double wc);

/**
 * Transforms a lowpass filter to a highpass filter.
 * @param input The input lowpass filter in zero-pole-gain representation
 * @param wc The cutoff frequency
 * @return The transformed highpass filter in zero-pole-gain representation
 */
ZPK lp2hp(const ZPK& input, const double wc);

/**
 * Transforms a lowpass filter to a bandpass filter.
 * @param input The input lowpass filter in zero-pole-gain representation
 * @param wc The central frequency
 * @param bw The bandwidth
 * @return The transformed bandpass filter in zero-pole-gain representation
 */
ZPK lp2bp(const ZPK& input, const double wc, const double bw);

/**
 * Transforms a lowpass filter to a bandstop filter.
 * @param input The input lowpass filter in zero-pole-gain representation
 * @param wc The central frequency
 * @param bw The bandwidth
 * @return The transformed bandstop filter in zero-pole-gain representation
 */
ZPK lp2bs(const ZPK& input, const double wc, const double bw);

/**
 * Computes the frequency response of a digital filter given in zero-pole-gain
 * form.
 *
 * @param digitalFilter The digital filter in zero-pole-gain representation
 * @param w The frequencies at which to compute the response
 * @return The frequency response as a vector of complex numbers
 */
std::vector<Complex> freqz(const ZPK&                 digitalFilter,
                           const std::vector<double>& w);

/**
 * Designs an IIR filter using the given analogue prototype function and
 * transforms it to a digital filter using the bilinear transform.
 * @tparam F The analogue prototype function
 * @tparam mode The filter mode (lowpass, highpass, etc.)
 * @param n The filter order
 * @param fc The cutoff frequency
 * @param fs The sampling frequency
 * @return The designed digital filter in zero-pole-gain representation
 */
template <ZPK (*F)(const int), Mode mode>
ZPK iirFilter(const int n, double fc, double fs) {
  return analog2digital(F(n), fc, fs, mode);
}

/**
 * Designs an IIR filter using the given analogue prototype function and
 * transforms it to a digital filter using the bilinear transform.
 * @tparam F The analogue prototype function
 * @tparam mode The filter mode (lowpass, highpass, etc.)
 * @param n The filter order
 * @param fc The cutoff frequency
 * @param fs The sampling frequency
 * @param param Additional parameter for the prototype function (e.g., ripple)
 * @return The designed digital filter in zero-pole-gain representation
 */
template <ZPK (*F)(const int, const double), Mode mode>
ZPK iirFilter(const int n, double fc, double fs, const double param) {
  return analog2digital(F(n, param), fc, fs, mode);
}

/**
 * Designs an IIR filter using the given type and mode.
 * @param n The filter order
 * @param fc The cutoff frequency
 * @param fs The sampling frequency
 * @param type The filter type (butterworth, chebyshev, etc.)
 * @param mode The filter mode (lowpass, highpass, etc.)
 * @param param Additional parameter for the prototype function (e.g., ripple)
 * @return The designed digital filter in zero-pole-gain representation
 */
ZPK iirFilter(const int n, double fc, double fs, const Type type = butter,
              const Mode mode = lowpass, const double param = 5.0);

/**
 * Designs a bandpass IIR filter using the given type.
 * @param n The filter order
 * @param fLow The lower cutoff frequency
 * @param fHigh The upper cutoff frequency
 * @param fs The sampling frequency
 * @param type The filter type (butterworth, chebyshev, etc.)
 * @param param Additional parameter for the prototype function (e.g., ripple)
 * @return The designed digital filter in zero-pole-gain representation
 */
ZPK iirFilter(const int n, double fLow, double fHigh, double fs,
              const Type type = butter, const Mode mode = bandpass,
              const double param = 5.0);

/**
 * Analogue Butterworth lowpass filter prototype.
 * @param n Filter order
 * @return The filter in zero-pole-gain representation
 */
ZPK buttap(const int n);

/**
 * Analogue Chebyshev Type I lowpass filter prototype.
 * @param n Filter order
 * @param rp Passband ripple in dB
 * @return The filter in zero-pole-gain representation
 */
ZPK cheb1ap(const int n, const double rp);

/**
 * Analogue Chebyshev Type II lowpass filter prototype.
 * @param n Filter order
 * @param rs Stopband ripple in dB
 * @return The filter in zero-pole-gain representation
 */
ZPK cheb2ap(const int n, const double rs);

} // namespace Nodex::Filter

#endif // INCLUDE_CORE_FILTER_H_
