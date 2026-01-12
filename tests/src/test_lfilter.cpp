#include "Filter.h"
#include "Utils.h"
#include <iostream>

bool test_lfilter() {
  using namespace Noddy::Filter;
  using Noddy::Utils::cleanFmt;

  const int    order{2};
  const double fc{100.0};
  const double fs{10000.0};
  const auto   ftype{lowpass};

  // TODO: provide file
  ArrayXd data{Noddy::Utils::readVectorFromFile("data.txt")};

  Noddy::Utils::Timer timer;

  // ZPK digitalFilter{iirFilter<cheb1ap, ftype>(order, fc, fs, 5.0)};
  // ZPK digitalFilter{iirFilter<cheb2ap, ftype>(order, fc, fs, 5.0)};
  ZPK    digitalFilter{iirFilter<buttap, ftype>(order, fc, fs)};
  Coeffs coeffs{zpk2tf(digitalFilter)};

  timer.reset();
  // auto output{linearFilter(coeffs, data)};
  auto output{fftFilter(coeffs, data, 1e-12)};
  auto elapsed{timer.elapsed()};

  std::cout << "Elapsed time: " << elapsed << '\n';
  std::cout << output.format(cleanFmt) << '\n';

  return true;
}

int main() {
  test_lfilter();

  return 0;
}
