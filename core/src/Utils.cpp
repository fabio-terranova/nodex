#include "Utils.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Nodex {
namespace Utils {

// Helper function to trim whitespace from string
static std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, last - first + 1);
}

// Helper function to check if string is numeric
static bool isNumeric(const std::string& str) {
  if (str.empty())
    return false;
  std::stringstream ss(str);
  double            val;
  ss >> std::noskipws >> val;
  return ss.eof() && !ss.fail();
}

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

CsvData loadCsvData(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open CSV file: " + filePath);
  }

  CsvData                          result;
  std::vector<std::vector<double>> allRows;
  std::vector<std::string>         headers;
  bool                             headerProcessed = false;
  int                              lineNum         = 0;

  std::string line;
  while (std::getline(file, line)) {
    lineNum++;
    // Skip empty lines and lines starting with '#' (comments)
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Parse CSV line
    std::vector<std::string> tokens;
    std::stringstream        ss(line);
    std::string              token;

    while (std::getline(ss, token, ',')) {
      token = trim(token);
      if (!token.empty()) {
        tokens.push_back(token);
      }
    }

    if (tokens.empty())
      continue;

    // First data row - check if it's a header
    if (!headerProcessed) {
      bool isHeader = false;
      for (const auto& tok : tokens) {
        if (!isNumeric(tok)) {
          isHeader = true;
          break;
        }
      }

      if (isHeader) {
        headers         = tokens;
        headerProcessed = true;
        continue;
      } else {
        // No header, generate default names
        for (size_t i = 0; i < tokens.size(); ++i) {
          headers.push_back("Col" + std::to_string(i + 1));
        }
        headerProcessed = true;
      }
    }

    // Parse numeric values
    if (tokens.size() != headers.size()) {
      throw std::runtime_error("Inconsistent column count at line " +
                               std::to_string(lineNum) + ": expected " +
                               std::to_string(headers.size()) + ", got " +
                               std::to_string(tokens.size()));
    }

    std::vector<double> row;
    for (const auto& tok : tokens) {
      try {
        row.push_back(std::stod(tok));
      } catch (const std::exception& e) {
        throw std::runtime_error("Invalid numeric value '" + tok + "' at line " +
                                 std::to_string(lineNum) + ": " + e.what());
      }
    }
    allRows.push_back(row);
  }

  if (allRows.empty()) {
    throw std::runtime_error("CSV file contains no data rows");
  }

  // Convert row-major to column-major storage
  result.columnNames = headers;
  for (size_t colIdx = 0; colIdx < headers.size(); ++colIdx) {
    std::vector<double> columnData;
    for (const auto& row : allRows) {
      columnData.push_back(row[colIdx]);
    }
    result.columns[headers[colIdx]] = Eigen::ArrayXd::Map(
        columnData.data(), static_cast<Eigen::Index>(columnData.size()));
  }

  return result;
}

void saveCsvData(const std::string& filePath, const Eigen::ArrayXd& data,
                 int precision) {
  std::ofstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open CSV file for writing: " + filePath);
  }

  file << std::fixed << std::setprecision(precision);

  for (Eigen::Index i = 0; i < data.size(); ++i) {
    file << data(i) << "\n";
  }

  if (!file) {
    throw std::runtime_error("Error writing to CSV file: " + filePath);
  }
}

void saveCsvData(const std::string& filePath, const CsvData& data,
                 int precision) {
  std::ofstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open CSV file for writing: " + filePath);
  }

  file << std::fixed << std::setprecision(precision);

  // Write header
  for (size_t i = 0; i < data.columnNames.size(); ++i) {
    file << data.columnNames[i];
    if (i < data.columnNames.size() - 1)
      file << ",";
  }
  file << "\n";

  // Find max rows
  size_t maxRows = 0;
  for (const auto& col : data.columnNames) {
    auto it = data.columns.find(col);
    if (it != data.columns.end()) {
      maxRows = std::max(maxRows, static_cast<size_t>(it->second.size()));
    }
  }

  // Write rows
  for (size_t row = 0; row < maxRows; ++row) {
    for (size_t col = 0; col < data.columnNames.size(); ++col) {
      auto it = data.columns.find(data.columnNames[col]);
      if (it != data.columns.end() &&
          row < static_cast<size_t>(it->second.size())) {
        file << it->second(static_cast<Eigen::Index>(row));
      }
      if (col < data.columnNames.size() - 1)
        file << ",";
    }
    file << "\n";
  }

  if (!file) {
    throw std::runtime_error("Error writing to CSV file: " + filePath);
  }
}

} // namespace Utils
} // namespace Nodex
