#include "Filter.h"
#include "Utils.h"
#include <iostream>

using Noddy::Filter::Complex;
using Noddy::Filter::ZPK;
using Noddy::Utils::Timer;

bool test_filterDesign() {
  using namespace Noddy::Filter;

  const auto order{7};
  const auto fc{100.0};
  const auto fs{1000.0};
  const auto ftype{lowpass};

  Timer timer{};
  { // Butterworth
    timer.reset();
    ZPK  digitalFilter{iirFilter<buttap, ftype>(order, fc, fs)};
    auto elapsed{timer.elapsed()};

    std::cout << "--- Design Butterworth filter ---" << '\n';
    std::cout << "Time taken: " << elapsed << '\n';
    std::cout << digitalFilter << '\n';
  }

  { // Chebyshev I
    const auto rp{5.0};

    timer.reset();
    ZPK  digitalFilter{iirFilter<cheb1ap, ftype>(order, fc, fs, rp)};
    auto elapsed{timer.elapsed()};

    std::cout << "--- Design Chebyshev I filter ---" << '\n';
    std::cout << "Time taken: " << elapsed << '\n';
    std::cout << digitalFilter << '\n';
  }

  { // Chebyshev II
    const auto rs{4.0};

    timer.reset();
    ZPK  digitalFilter{iirFilter<cheb2ap, ftype>(order, fc, fs, rs)};
    auto elapsed{timer.elapsed()};

    std::cout << "--- Design Chebyshev II filter ---" << '\n';
    std::cout << "Time taken: " << elapsed << '\n';
    std::cout << digitalFilter << '\n';
  }

  return true;
}

int main() {
  test_filterDesign();

  return 0;
}
