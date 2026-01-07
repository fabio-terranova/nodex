#include "core/NoddyCore.h"
#include <Eigen/Dense>
#include <iostream>

namespace Noddy {
std::string getCoreVersion() { return "Noddy Core v0.0.1"; }

void test() {
  Eigen::MatrixXd m(2, 2);
  m(0, 0) = 3;
  m(1, 0) = 2.5;
  m(0, 1) = -1;
  m(1, 1) = m(1, 0) + m(0, 1);

  std::cout << m << '\n';
}
} // namespace Noddy
