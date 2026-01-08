#include "Eigen/Core"
#include "Filter.h"
#include <Eigen/Dense>
#include <cassert>
#include <iostream>

using Eigen::VectorXcd;
using Noddy::Filter::Coeffs;
using Noddy::Filter::Complex;
using Noddy::Filter::ZPK;

Eigen::IOFormat g_cleanFmt(Eigen::StreamPrecision, 0, " ", "", "", "", "[",
                           "]");

bool operator==(const Coeffs& first, const Coeffs& second) {
  if (first.a != second.a)
    return false;
  if (first.b != second.b)
    return false;

  return true;
}

bool test_zpk2tf() {
  // Test zeros-poles-gain to polynomial coefficents
  ZPK filter{VectorXcd{2}, VectorXcd{2}, 5.0};
  filter.z << Complex{2, 0}, Complex{6, 0};
  filter.p << Complex{1, 0}, Complex{8, 0};

  Coeffs expected{VectorXcd{3}, VectorXcd{3}};
  expected.b << Complex{5.0, 0.0}, Complex{-40.0, 0.0}, Complex{60.0, 0.0};
  expected.a << Complex{1.0, 0.0}, Complex{-9.0, 0.0}, Complex{8.0, 0.0};

  Coeffs result{Noddy::Filter::zpk2tf(filter)};

  return (expected == result);
}

bool test_bilinearTransform() {
  double fs{100.0};
  ZPK    analogFilter{VectorXcd{0}, VectorXcd{3}, 1.0};
  analogFilter.p << Complex(-1.0, 2.0), Complex(-1.0, -1.0), Complex(-1.0, 3.0);

  ZPK digitalFilter{Noddy::Filter::bilinearTransform(analogFilter, fs)};

  std::cout << "--- Bilinear transform result ---" << '\n';
  std::cout << "Gain (k): " << digitalFilter.k << '\n';
  std::cout << "Poles (p):\n" << digitalFilter.p.format(g_cleanFmt) << '\n';
  std::cout << "Zeros (z):\n" << digitalFilter.z.format(g_cleanFmt) << '\n';

  return true;
}

bool test_iirFilter() {
  const auto order{2};
  const auto fc{10.0};
  const auto fs{100.0};
  const auto fType{Noddy::Filter::highPass};

  ZPK digitalFilter{
      Noddy::Filter::iirFilter(order, fc, fs, Noddy::Filter::buttap, fType),
  };

  std::cout << "--- Design Butterworth filter ---" << '\n';
  std::cout << "Gain: " << digitalFilter.k << '\n';
  std::cout << "Zeros (" << digitalFilter.z.size()
            << "): " << digitalFilter.z.format(g_cleanFmt) << '\n';
  std::cout << "Poles (" << digitalFilter.p.size()
            << "): " << digitalFilter.p.format(g_cleanFmt) << '\n';

  return true;
}

int main() {
  // assert(test_zpk2tf());
  // assert(test_bilinearTransform());
  assert(test_iirFilter());

  return 0;
}
