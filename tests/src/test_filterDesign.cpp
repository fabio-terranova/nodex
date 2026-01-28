#include "Filter.h"
#include <cassert>
#include <iostream>

using namespace Nodex::Filter;

bool isAlmostEqual(const Complex& a, const Complex& b,
                   const double tol = 1e-6) {
  return std::abs(a - b) < tol;
}

bool operator==(const ZPK& first, const ZPK& second) {
  if (first.z.size() != second.z.size())
    return false;
  if (first.p.size() != second.p.size())
    return false;

  if (!isAlmostEqual(first.k, second.k))
    return false;

  for (std::size_t i{0}; i < first.z.size(); ++i) {
    if (!isAlmostEqual(first.z[i], second.z[i]))
      return false;
  }

  for (std::size_t i{0}; i < first.p.size(); ++i) {
    if (!isAlmostEqual(first.p[i], second.p[i]))
      return false;
  }

  return true;
}

template <Nodex::Filter::Mode fmode>
bool testButterworth(const int order, const double fc, const double fs,
                     const ZPK& expected) {
  std::cout << "--- Testing Butterworth filter design ---\n";

  ZPK digitalFilter{iirFilter(order, fc, fs, butter, fmode)};

  std::cout << digitalFilter << '\n';

  return digitalFilter == expected;
}

template <Nodex::Filter::Mode fmode>
bool testChebyshevI(const int order, const double fc, const double fs,
                    const double rp, const ZPK& expected) {
  std::cout << "--- Testing Chebyshev I filter design ---\n";

  ZPK digitalFilter{iirFilter(order, fc, fs, cheb1, fmode, rp)};

  std::cout << digitalFilter << '\n';

  return digitalFilter == expected;
}

template <Nodex::Filter::Mode fmode>
bool testChebyshevII(const int order, const double fc, const double fs,
                     const double rs, const ZPK& expected) {
  std::cout << "--- Testing Chebyshev II filter design ---\n";

  ZPK digitalFilter{iirFilter(order, fc, fs, cheb2, fmode, rs)};
  std::cout << digitalFilter << '\n';

  return digitalFilter == expected;
}

int main() {
  using namespace Nodex::Filter;
  using namespace std::complex_literals;

  const auto order{2};
  const auto fnode{lowpass};
  const auto fc{100.0};
  const auto fs{1000.0};
  const auto rp{5.0};
  const auto rs{5.0};

  auto expectedButterworth{
      ZPK{{-1.0 + 0.0i, -1.0 + 0.0i},
          {0.57149025 + 0.2935992i, 0.57149025 - 0.2935992i},
          0.0674552738890719}
  };

  auto expectedChebyshevI{
      ZPK{{-1.0 + 0.0i, -1.0 + 0.0i},
          {0.77209728 + 0.39831468i, 0.77209728 - 0.39831468i},
          0.02960646023643654}
  };

  auto expectedChebyshevII{
      ZPK{{0.6513291 - 0.75879537i, 0.6513291 + 0.75879537i},
          {0.61151327 - 0.42266258i, 0.61151327 + 0.42266258i},
          0.47260267144001655}
  };

  if (!testButterworth<fnode>(order, fc, fs, expectedButterworth)) {
    std::cerr << "Butterworth filter test failed.\n";
    return 1;
  }

  if (!testChebyshevI<fnode>(order, fc, fs, rp, expectedChebyshevI)) {
    std::cerr << "Chebyshev I filter test failed.\n";
    return 1;
  }

  if (!testChebyshevII<fnode>(order, fc, fs, rs, expectedChebyshevII)) {
    std::cerr << "Chebyshev II filter test failed.\n";
    return 1;
  }

  return 0;
}
