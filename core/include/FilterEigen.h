#ifndef INCLUDE_INCLUDE_FILTEREIGEN_H_
#define INCLUDE_INCLUDE_FILTEREIGEN_H_

#include "Filter.h"
#include <Eigen/Dense>

namespace Nodex {
namespace Filter {
using Eigen::ArrayXcd;
using Eigen::ArrayXd;
using Eigen::Index;

using RowMajorMatrixXd =
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

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

RowMajorMatrixXd linearFilter(const EigenCoeffs&&                       filter,
                              const Eigen::Ref<const RowMajorMatrixXd>& x,
                              Eigen::Ref<RowMajorMatrixXd>              state);
ArrayXd          linearFilter(const EigenCoeffs&               filter,
                              const Eigen::Ref<const ArrayXd>& x,
                              Eigen::Ref<ArrayXd>              state);
ArrayXd          linearFilter(const EigenCoeffs&               filter,
                              const Eigen::Ref<const ArrayXd>& x);

ArrayXd fftFilter(const EigenCoeffs& filter, const Eigen::Ref<const ArrayXd>& x,
                  const double epsilon, const std::size_t maxLength);

EigenCoeffs zpk2tf(const EigenZPK& zpk);
ArrayXd     roots2poly(const Eigen::Ref<const ArrayXd>& roots);
} // namespace Filter
} // namespace Nodex

#endif // INCLUDE_INCLUDE_FILTEREIGEN_H_
