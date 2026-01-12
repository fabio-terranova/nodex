#include "Filter.h"
#include <iostream>

using Noddy::Filter::Coeffs;
using Noddy::Filter::Complex;
using Noddy::Filter::ZPK;

bool test_zpk2tf() {
  using namespace Noddy::Filter;

  // Test zeros-poles-gain to polynomial coefficents
  ZPK filter{VectorXcd{2}, VectorXcd{2}, 5.0};
  filter.z << 2, 6;
  filter.p << 1, 8;

  Coeffs expected{VectorXd{3}, VectorXd{3}};
  expected.b << 5.0, -40.0, 60.0;
  expected.a << 1.0, -9.0, 8.0;

  Coeffs result{zpk2tf(filter)};

  std::cout << "--- ZPK to transfer function coefficients ---" << '\n';
  std::cout << "Expected coeffs:\n" << expected << '\n';
  std::cout << "Result coeffs:\n" << result << '\n';

  return (expected != result);
}

int main() { return test_zpk2tf(); }
