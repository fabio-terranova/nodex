#ifndef INCLUDE_INCLUDE_GUINODES_H_
#define INCLUDE_INCLUDE_GUINODES_H_

#include "Constants.h"
#include "Filter.h"
#include "Node.h"
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
  ViewerNode(const std::string_view name);

  void           render() override;
  nlohmann::json serialize() const override;
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
  RandomDataNode(const std::string_view name, const int size = kDefaultSamples);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  int            m_samples{};
  Eigen::ArrayXd m_data{};
};

class SineNode : public Core::Node {
public:
  SineNode(const std::string_view name, int size = kDefaultSamples,
           const double frequency = kDefaultFrequency,
           const double amplitude = kDefaultAmplitude,
           const double phase     = kDefaultPhase,
           const double fs        = kDefaultSamplingFreq,
           const double offset    = kDefaultOffset);

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
             const Filter::Mode     mode         = kDefaultFilterMode,
             const Filter::Type     type         = kDefaultFilterType,
             const int              order        = kDefaultFilterOrder,
             const double           cutoffFreq   = kDefaultCutoffFreq,
             const double           samplingFreq = kDefaultSamplingFreq);

  void           render() override;
  nlohmann::json serialize() const override;

private:
  Nodex::Filter::Mode m_filterMode{kDefaultFilterMode};
  Nodex::Filter::Type m_filterType{kDefaultFilterType};
  int                 m_filterOrder{kDefaultFilterOrder};
  double              m_cutoffFreq{kDefaultCutoffFreq};
  double              m_samplingFreq{kDefaultSamplingFreq};
};
} // namespace Nodex::Gui

#endif // INCLUDE_INCLUDE_GUINODES_H_
