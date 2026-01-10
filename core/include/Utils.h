#ifndef INCLUDE_INCLUDE_UTILS_H_
#define INCLUDE_INCLUDE_UTILS_H_

#include <Eigen/Dense>

namespace Noddy {
namespace Utils {
using Eigen::ArrayXi;

inline const Eigen::IOFormat cleanFmt(Eigen::StreamPrecision, 0, " ", "", "",
                                      "", "[", "]");

ArrayXi arange(const int start, int stop, const int step);
} // namespace Utils
} // namespace Noddy

#endif // INCLUDE_INCLUDE_UTILS_H_
