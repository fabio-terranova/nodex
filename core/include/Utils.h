#ifndef INCLUDE_INCLUDE_UTILS_H_
#define INCLUDE_INCLUDE_UTILS_H_

#include "Eigen/Core"
#include <Eigen/Dense>
#include <chrono>
#include <complex>
#include <map>
#include <string>
#include <vector>

namespace Nodex::Utils {
// Aliases for commonly used Eigen types
using Eigen::ArrayXi;

// Aliases for commonly used standard types
using Complex = std::complex<double>;
using Signal  = std::vector<double>;

// Define a clean format for Eigen output
inline const Eigen::IOFormat cleanFmt(Eigen::StreamPrecision, 0, " ", ",", "",
                                      "", "[", "]");

// Simple timer class for performance measurement
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

/**
 * Structure representing loaded CSV data with column information.
 */
struct CsvData {
  std::vector<std::string>              columnNames; // Column names/headers
  std::map<std::string, Eigen::ArrayXd> columns;     // Column data by name
};

ArrayXi arange(const int start, int stop, const int step);

/**
 * Loads signal data from a CSV file with multiple columns.
 * Automatically detects and parses columns.
 * First row is treated as header if it contains non-numeric values.
 *
 * @param filePath Path to the CSV file
 * @return CsvData structure containing column names and data
 * @throws std::runtime_error if file cannot be read or contains invalid data
 */
CsvData loadCsvData(const std::string& filePath);

/**
 * Saves signal data to a CSV file.
 * Writes one value per line.
 *
 * @param filePath Path to the CSV file to write
 * @param data Signal data to save
 * @param precision Number of decimal places for numeric precision
 * @throws std::runtime_error if file cannot be written
 */
void saveCsvData(const std::string& filePath, const Eigen::ArrayXd& data,
                 int precision = 6);

/**
 * Saves multiple columns to a CSV file.
 *
 * @param filePath Path to the CSV file to write
 * @param data CsvData structure with columns to save
 * @param precision Number of decimal places for numeric precision
 * @throws std::runtime_error if file cannot be written
 */
void saveCsvData(const std::string& filePath, const CsvData& data,
                 int precision = 6);

/**
 * Computes the Fast Fourier Transform (FFT) of a real-valued signal.
 * @param signal Input real-valued signal
 * @return Complex-valued FFT of the input signal
 */
Eigen::ArrayXd computeFFT(const Eigen::Ref<const Eigen::VectorXd>& signal);

/**
 * Generates a time vector given the length and sampling frequency.
 * @param length Length of the signal in samples
 * @param fs Sampling frequency in Hz
 * @return Time vector as an Eigen array
 */
Eigen::ArrayXd generateTimeVector(const Eigen::Index length, const double fs);

/**
 * Generates a frequency vector given the length and sampling frequency.
 * @param length Length of the signal in samples
 * @param fs Sampling frequency in Hz
 * @return Frequency vector as an Eigen array
 */
Eigen::ArrayXd generateFrequencyVector(const Eigen::Index length,
                                       const double       fs);
} // namespace Nodex::Utils

#endif // INCLUDE_INCLUDE_UTILS_H_
