#include "Utils.h"
#include <fstream>

namespace Noddy {
namespace Utils {
ArrayXi arange(const int start, int stop, const int step) {
  if (step == 0)
    return ArrayXi{};

  const int num{(stop - start + (step > 0 ? step - 1 : step + 1)) / step};
  if (num <= 0)
    return ArrayXi{};

  const int actualStop{start + (num - 1) * step};

  return ArrayXi::LinSpaced(num, start, actualStop);
}

Eigen::VectorXd readVectorFromFile(const std::string& filename) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  std::vector<double> values{};
  double              x{};
  while (infile >> x) {
    values.push_back(x);
  }

  Eigen::VectorXd vec(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    vec(static_cast<Eigen::Index>(i)) = values[i];
  }

  return vec;
}
} // namespace Utils
} // namespace Noddy
