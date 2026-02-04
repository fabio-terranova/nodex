#ifndef INCLUDE_INCLUDE_FILTEREIGEN_H_
#define INCLUDE_INCLUDE_FILTEREIGEN_H_

#include "Filter.h"
#include <Eigen/Dense>

/**
 * @file FilterEigen.h
 * Eigen-based digital filter design and application functions.
 */
namespace Nodex::Filter {
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::Index;

using RowMajorMatrixXd =
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

/**
 * Eigen-based filter coefficients representation.
 */
struct EigenCoeffs {
  ArrayXd b{};
  ArrayXd a{};

  EigenCoeffs() = default;

  EigenCoeffs(const Eigen::Ref<const ArrayXd>& bCoeffs,
              const Eigen::Ref<const ArrayXd>& aCoeffs)
      : b{bCoeffs}, a{aCoeffs} {};

  EigenCoeffs(Coeffs& coeffs)
      : b{Eigen::Map<ArrayXd>{coeffs.b.data(),
                               static_cast<Index>(coeffs.b.size())}},
        a{Eigen::Map<ArrayXd>{coeffs.a.data(),
                               static_cast<Index>(coeffs.a.size())}} {}
};

/**
 * Eigen-based zero-pole-gain representation.
 */
struct EigenZPK {
  ArrayXcd z{};
  ArrayXcd p{};
  double   k{};

  EigenZPK() = default;

  EigenZPK(const Eigen::Ref<const ArrayXcd>& zeros,
           const Eigen::Ref<const ArrayXcd>& poles, const double gain)
      : z{zeros}, p{poles}, k{gain} {};

  EigenZPK(const ZPK& zpk)
      : z{Eigen::Map<const ArrayXcd>{zpk.z.data(),
                                static_cast<Index>(zpk.z.size())}},
        p{Eigen::Map<const ArrayXcd>{zpk.p.data(),
                                static_cast<Index>(zpk.p.size())}},
        k{zpk.k} {}
};

/**
 * Applies a linear filter to the input signal x using the given filter
 * coefficients and state. Matrix version.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @param state The filter state (should be maintained between calls)
 * @return The filtered output signal
 */
RowMajorMatrixXd linearFilter(const EigenCoeffs&&                       filter,
                              const Eigen::Ref<const RowMajorMatrixXd>& x,
                              Eigen::Ref<RowMajorMatrixXd>              state);

/**
 * Applies a linear filter to the input signal x using the given filter
 * coefficients and state.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @param state The filter state (should be maintained between calls)
 * @return The filtered output signal
 */
ArrayXd linearFilter(const EigenCoeffs&               filter,
                     const Eigen::Ref<const ArrayXd>& x,
                     Eigen::Ref<ArrayXd>              state);

/**
 * Applies a linear filter to the input signal x using the given filter
 * coefficients. No state version.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @return The filtered output signal
 */
ArrayXd linearFilter(const EigenCoeffs&               filter,
                     const Eigen::Ref<const ArrayXd>& x);

/**
 * Applies an FFT-based filter to the input signal x using the given filter
 * coefficients.
 * @param filter The filter coefficients (b and a)
 * @param x The input signal
 * @param epsilon Small constant to account for approximation error
 * @param maxLength Maximum length for FFT (zero-padded if necessary)
 * @return The filtered output signal
 */
ArrayXd fftFilter(const EigenCoeffs& filter, const Eigen::Ref<const ArrayXd>& x,
                  const double epsilon, const std::size_t maxLength);

/**
 * Converts zero-pole-gain representation to transfer function coefficients.
 * @param zpk The zero-pole-gain representation
 * @return The transfer function coefficients (b and a)
 */
EigenCoeffs zpk2tf(const EigenZPK& zpk);

/**
 * Converts transfer function coefficients to zero-pole-gain representation.
 * @param tf The transfer function coefficients (b and a)
 * @return The zero-pole-gain representation
 */
ArrayXd roots2poly(const Eigen::Ref<const ArrayXd>& roots);
} // namespace Nodex::Filter

#endif // INCLUDE_INCLUDE_FILTEREIGEN_H_
