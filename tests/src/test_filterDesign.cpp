#include "Filter.h"
#include "Utils.h"
#include <chrono>
#include <iostream>

using Noddy::Filter::Coeffs;
using Noddy::Filter::Complex;
using Noddy::Filter::ZPK;

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

std::ostream& operator<<(std::ostream& os, const ZPK& zpk) {
  os << "k: " << zpk.k << "\n";
  os << "z: " << zpk.z.format(g_cleanFmt) << "\n";
  os << "p: " << zpk.p.format(g_cleanFmt) << "\n";
  return os;
}

bool test_filterDesign() {
  using namespace Noddy::Filter;

  const auto order{2};
  const auto fc{100.0};
  const auto fs{1000.0};
  const auto ftype{lowpass};

  { // Butterworth
    ZPK digitalFilter{iirFilter<buttap, ftype>(order, fc, fs)};

    std::cout << "--- Design Butterworth filter ---" << '\n';
    std::cout << digitalFilter << '\n';
  }

  { // Chebyshev I
    const auto rp{5.0};

    ZPK digitalFilter{iirFilter<cheb1ap, ftype>(order, fc, fs, rp)};

    std::cout << "--- Design Chebyshev I filter ---" << '\n';
    std::cout << digitalFilter << '\n';
  }

  { // Chebyshev II
    const auto rs{4.0};

    ZPK digitalFilter{iirFilter<cheb2ap, ftype>(order, fc, fs, rs)};

    std::cout << "--- Design Chebyshev II filter ---" << '\n';
    std::cout << digitalFilter << '\n';
  }

  return true;
}

int main() {
  test_filterDesign();

  return 0;
}
