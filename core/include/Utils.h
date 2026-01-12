#ifndef INCLUDE_INCLUDE_UTILS_H_
#define INCLUDE_INCLUDE_UTILS_H_

#include <Eigen/Dense>
#include <chrono>

namespace Noddy {
namespace Utils {
using Eigen::ArrayXi;

inline const Eigen::IOFormat cleanFmt(Eigen::StreamPrecision, 0, " ", ",", "",
                                      "", "[", "]");

Eigen::VectorXd readVectorFromFile(const std::string& filename);

class Timer {
  using Second = std::chrono::duration<double, std::ratio<1>>;
  using Clock  = std::chrono::steady_clock;

public:
  void reset() { m_beg = Clock::now(); }

  double elapsed() const {
    return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
  }

private:
  std::chrono::time_point<Clock> m_beg{Clock::now()};
};

ArrayXi arange(const int start, int stop, const int step);
} // namespace Utils
} // namespace Noddy

#endif // INCLUDE_INCLUDE_UTILS_H_
