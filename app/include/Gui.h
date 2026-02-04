#ifndef INCLUDE_INCLUDE_GUI_H_
#define INCLUDE_INCLUDE_GUI_H_

#include "Constants.h"
#include "Filter.h"
#include "Node.h"
#include "Utils.h"
#include "imgui.h"
#include "nlohmann/json_fwd.hpp"
#include <Eigen/Dense>
#include <cstddef>
#include <string_view>
#include <vector>

// Forward declare Serializer namespace
namespace Nodex::Gui::Serializer {
Core::Graph loadFromJson(const std::string&);
}

/**
 * GUI node implementations.
 */
namespace Nodex::Gui {
/**
 * State for drag-and-drop connection creation.
 */
struct DragDropState {
  Core::Port* draggedPort{nullptr};
  ImVec2      dragStartPos{0, 0};
  bool        isDragging{false};
};

/**
 * Begins the main graph window for the given graph.
 *
 * @param graph The graph to display.
 */
void graphWindow(Core::Graph& graph);

class ViewerNode : public Core::Node {
public:
  ViewerNode(const std::string_view name,
             const double samplingFreq = Constants::kDefaultSamplingFreq);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  double m_samplingFreq{};
};

class MultiViewerNode : public Core::Node {
public:
  MultiViewerNode(const std::string_view name, const std::size_t inputs = 2,
                  const double samplingFreq = Constants::kDefaultSamplingFreq);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  std::size_t m_inputs{};
  double      m_samplingFreq{};
};

class MixerNode : public Core::Node {
public:
  MixerNode(const std::string_view name, const std::size_t inputs = 2,
            const std::vector<double>& gains = std::vector<double>{});

  Eigen::ArrayXd getData();
  void           render() override;
  nlohmann::json serialize() const override;

private:
  std::size_t         m_inputs{};
  std::vector<double> m_gains{};
};

class RandomDataNode : public Core::Node {
public:
  RandomDataNode(const std::string_view name,
                 const int              size = Constants::kDefaultSamples);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  int            m_samples{};
  Eigen::ArrayXd m_data{};
};

class SineNode : public Core::Node {
public:
  SineNode(const std::string_view name, int size = Constants::kDefaultSamples,
           const double frequency = Constants::kDefaultFrequency,
           const double amplitude = Constants::kDefaultAmplitude,
           const double phase     = Constants::kDefaultPhase,
           const double fs        = Constants::kDefaultSamplingFreq,
           const double offset    = Constants::kDefaultOffset);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  Eigen::ArrayXd generateWave() const;

  int    m_samples{};
  double m_frequency{};
  double m_amplitude{};
  double m_phase{};
  double m_samplingFreq{};
  double m_offset{};
};

class FilterNode : public Core::Node {
public:
  FilterNode(const std::string_view name,
             const Filter::Mode     mode       = Constants::kDefaultFilterMode,
             const Filter::Type     type       = Constants::kDefaultFilterType,
             const int              order      = Constants::kDefaultFilterOrder,
             const double           cutoffFreq = Constants::kDefaultCutoffFreq,
             const double samplingFreq = Constants::kDefaultSamplingFreq,
             const double cutoffFreq2  = Constants::kDefaultCutoffFreq2);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  Nodex::Filter::Mode m_filterMode{};
  Nodex::Filter::Type m_filterType{};
  int                 m_filterOrder{};
  double              m_cutoffFreq{};
  double              m_samplingFreq{};
  double              m_cutoffFreq2{};
};

class CSVNode : public Core::Node {
public:
  CSVNode(const std::string_view name, const std::string& filePath = "");

  void           render() override;
  nlohmann::json serialize() const override;

  const Utils::CsvData& getData() const { return m_csvData; }

private:
  std::string    m_filePath{};
  Utils::CsvData m_csvData{};

  void loadCsvFile(const std::string& filePath);
};

} // namespace Nodex::Gui

#endif // INCLUDE_INCLUDE_GUI_H_
