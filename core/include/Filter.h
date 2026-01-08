#ifndef INCLUDE_CORE_FILTER_H_
#define INCLUDE_CORE_FILTER_H_

#include "Eigen/Core"
#include <complex>
#include <functional>

namespace Noddy {
namespace Filter {
using Eigen::Array2d;
using Eigen::VectorXcd;
using Complex = std::complex<double>;

struct Coeffs {
  VectorXcd b{};
  VectorXcd a{};
};

struct ZPK {
  VectorXcd z{};
  VectorXcd p{};
  double    k{1.0};
};

enum Type {
  lowPass,
  highPass,
  bandPass,
  bandStop,
  maxFilters,
};

ZPK buttap(const int n);

ZPK bilinearTransform(const ZPK& zpkA, const double fs);

ZPK iirFilter(const int n, double fc, double fs,
              std::function<ZPK(const int)> filter, Type type);
ZPK iirFilter(const int n, Array2d fc, double fs,
              std::function<ZPK(const int)> filter, Type type);

Coeffs zpk2tf(const ZPK& zpk);
} // namespace Filter
} // namespace Noddy

#endif // INCLUDE_CORE_FILTER_H_
