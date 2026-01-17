#ifndef INCLUDE_INCLUDE_FILTEREIGEN_H_
#define INCLUDE_INCLUDE_FILTEREIGEN_H_

#include "Filter.h"
#include <Eigen/Dense>

namespace Nodex {
namespace Filter {
using Eigen::Index;
using Eigen::VectorXcd;
using Eigen::VectorXd;

using RowMajorMatrixXd =
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

struct EigenCoeffs {
  VectorXd b{};
  VectorXd a{};

  EigenCoeffs() = default;

  EigenCoeffs(const Eigen::Ref<const VectorXd>& bCoeffs,
              const Eigen::Ref<const VectorXd>& aCoeffs)
      : b{bCoeffs}, a{aCoeffs} {};

  EigenCoeffs(Coeffs& coeffs)
      : b{Eigen::Map<VectorXd>{coeffs.b.data(),
                               static_cast<Index>(coeffs.b.size())}},
        a{Eigen::Map<VectorXd>{coeffs.a.data(),
                               static_cast<Index>(coeffs.a.size())}} {}
};

struct EigenZPK {
  VectorXcd z{};
  VectorXcd p{};
  double    k{};

  EigenZPK() = default;

  EigenZPK(const Eigen::Ref<const VectorXcd>& zeros,
           const Eigen::Ref<const VectorXcd>& poles, const double gain)
      : z{zeros}, p{poles}, k{gain} {};

  EigenZPK(ZPK& zpk)
      : z{Eigen::Map<VectorXcd>{zpk.z.data(),
                                static_cast<Index>(zpk.z.size())}},
        p{Eigen::Map<VectorXcd>{zpk.p.data(),
                                static_cast<Index>(zpk.p.size())}},
        k{zpk.k} {}
};

RowMajorMatrixXd linearFilter(const EigenCoeffs&&                       filter,
                              const Eigen::Ref<const RowMajorMatrixXd>& x,
                              Eigen::Ref<RowMajorMatrixXd>              state);
VectorXd         linearFilter(const EigenCoeffs&                filter,
                              const Eigen::Ref<const VectorXd>& x,
                              Eigen::Ref<VectorXd>              state);
VectorXd         linearFilter(const EigenCoeffs&                filter,
                              const Eigen::Ref<const VectorXd>& x);

VectorXd fftFilter(const EigenCoeffs&                filter,
                   const Eigen::Ref<const VectorXd>& x, const double epsilon,
                   const std::size_t maxLength);

EigenCoeffs zpk2tf(const EigenZPK& zpk);
VectorXd    roots2poly(const Eigen::Ref<const VectorXcd>& roots);
} // namespace Filter
} // namespace Nodex

#endif // INCLUDE_INCLUDE_FILTEREIGEN_H_
