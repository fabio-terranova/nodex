#ifndef INCLUDE_INCLUDE_GUINODES_H_
#define INCLUDE_INCLUDE_GUINODES_H_

#include "Filter.h"
#include "Node.h"
#include "imgui.h"
#include <Eigen/Dense>
#include <string_view>

namespace Nodex::App {
using namespace Nodex::Filter;
using namespace Nodex::Core;

struct DragDropState {
  Port*  draggedPort{nullptr};
  ImVec2 dragStartPos{
      ImVec2{0, 0}
  };
  bool isDragging{false};
};

void graphWindow(Graph& graph);

class MixerNode : public Node {
public:
  MixerNode(std::string_view name);

  Eigen::ArrayXd getData(double k1, double k2);
  void           render() override;

private:
  double m_gain1{1.0};
  double m_gain2{1.0};
};

class ViewerNode : public Node {
public:
  ViewerNode(std::string_view name);

  void render() override;
};

class RandomDataNode : public Node {
public:
  RandomDataNode(std::string_view name, int size);

  void render() override;

private:
  int            m_samples{1000};
  Eigen::ArrayXd m_data{};
};

class SineNode : public Node {
public:
  SineNode(std::string_view name);

  void render() override;

private:
  Eigen::ArrayXd generateWave();

  int    m_samples{1000};
  double m_frequency{1.0};
  double m_amplitude{1.0};
  double m_phase{0.0};
  double m_samplingFreq{100.0};
  double m_bias{0.0};
};

class DataNode : public Node {
public:
  DataNode(std::string_view name, Eigen::ArrayXd& data);

  void render() override;

private:
  Eigen::ArrayXd m_data{};
};

class FilterNode : public Node {
public:
  FilterNode(std::string_view name);

  void render() override;

private:
  Nodex::Filter::Mode m_filterMode{};
  Nodex::Filter::Type m_filterType{};
  int                 m_filterOrder{2};
  double              m_cutoffFreq{100.0};
  double              m_samplingFreq{1000.0};
};
} // namespace Nodex::App

#endif // INCLUDE_INCLUDE_GUINODES_H_
