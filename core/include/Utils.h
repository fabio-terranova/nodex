#ifndef INCLUDE_INCLUDE_UTILS_H_
#define INCLUDE_INCLUDE_UTILS_H_

#include <Eigen/Dense>

const static Eigen::IOFormat g_cleanFmt(Eigen::StreamPrecision, 0, " ", "", "",
                                        "", "[", "]");
namespace Noddy {
namespace Utils {
using Eigen::ArrayXi;

ArrayXi arange(const int start, int stop, const int step);
} // namespace Utils
} // namespace Noddy

#endif // INCLUDE_INCLUDE_UTILS_H_
