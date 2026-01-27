#include "GuiNodes.h"
#include "FilterEigen.h"
#include "implot.h"
#include <numbers>

namespace ImGui {
inline bool SliderDouble(const char* label, double* v, double v_min,
                         double v_max, const char* format = NULL,
                         ImGuiSliderFlags flags = 0) {
  return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format,
                      flags);
}
} // namespace ImGui

namespace Nodex::App {
// MixerNode
MixerNode::MixerNode(std::string_view name) : Node{name, "Mixer"} {
  addInput<Eigen::ArrayXd>("In 1", Eigen::ArrayXd{});
  addInput<Eigen::ArrayXd>("In 2", Eigen::ArrayXd{});
  addOutput<Eigen::ArrayXd>("Out",
                            [this]() { return getData(m_gain1, m_gain2); });
}

Eigen::ArrayXd MixerNode::getData(double k1, double k2) {
  auto in1{inputValue<Eigen::ArrayXd>("In 1")};
  auto in2{inputValue<Eigen::ArrayXd>("In 2")};

  if (in1.size() < in2.size()) {
    in1.conservativeResize(in2.size());
    in1.tail(in2.size() - in1.size()).setZero();
  } else if (in2.size() < in1.size()) {
    in2.conservativeResize(in1.size());
    in2.tail(in1.size() - in2.size()).setZero();
  }

  return k1 * in1 + k2 * in2;
}

void MixerNode::render() {
  ImGui::InputDouble("Gain 1", &m_gain1, 0.1, 1.0, "%.2f");
  ImGui::InputDouble("Gain 2", &m_gain2, 0.1, 1.0, "%.2f");
}

// ViewerNode
ViewerNode::ViewerNode(std::string_view name) : Node{name, "Viewer"} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
}

void ViewerNode::render() {
  auto data{inputValue<Eigen::ArrayXd>("In")};
  if (data.size() > 0) {
    if (ImPlot::BeginPlot("Time plot", ImVec2{200.0f, 150.0f})) {
      ImPlot::PlotLine("", data.data(), static_cast<int>(data.size()));
      ImPlot::EndPlot();
    }
  } else {
    ImGui::Text("No data connected.");
  }
}

// RandomDataNode
RandomDataNode::RandomDataNode(std::string_view name, int size)
    : Node{name, "Random data"}, m_samples{size},
      m_data{Eigen::ArrayXd::Random(size)} {
  addOutput<Eigen::ArrayXd>("Out", [this]() { return m_data; });
}

void RandomDataNode::render() {
  if (ImGui::InputInt("Number of samples", &m_samples))
    m_data = Eigen::ArrayXd::Random(m_samples);
}

// SineNode
SineNode::SineNode(std::string_view name) : Node{name, "Sine wave"} {
  addOutput<Eigen::ArrayXd>("Out", [this]() { return generateWave(); });
}

Eigen::ArrayXd SineNode::generateWave() {
  Eigen::ArrayXd sineWave(m_samples);
  for (int i = 0; i < m_samples; ++i) {
    double t = i / m_samplingFreq;
    sineWave[i] =
        m_amplitude *
            std::sin(2 * std::numbers::pi * m_frequency * t + m_phase) +
        m_bias;
  }
  return sineWave;
}

void SineNode::render() {
  ImGui::Text("Parameters:");
  ImGui::InputInt("Number of samples", &m_samples);
  ImGui::SliderDouble("Frequency (Hz)", &m_frequency, 0.1, m_samplingFreq / 2,
                      "%.2f");
  ImGui::InputDouble("Amplitude", &m_amplitude, 0.1, 1.0, "%.2f");
  ImGui::SliderDouble("Phase (rad)", &m_phase, 0.0, 2 * std::numbers::pi,
                      "%.2f");
  ImGui::InputDouble("Sampling Frequency (Hz)", &m_samplingFreq, 10.0, 10000.0,
                     "%.2f");
  ImGui::InputDouble("Bias", &m_bias, 0.1, 1.0, "%.2f");
}

// DataNode
DataNode::DataNode(std::string_view name, Eigen::ArrayXd& data)
    : Node{name, "Data"}, m_data{data} {
  addOutput<Eigen::ArrayXd>("Out", [this]() { return m_data; });
}

void DataNode::render() {
  ImGui::Text("Number of samples: %d", static_cast<int>(m_data.size()));
}

// FilterNode
FilterNode::FilterNode(std::string_view name) : Node{name, "Filter"} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
  addOutput<Eigen::ArrayXd>("Out", [this]() {
    auto inputData{inputValue<Eigen::ArrayXd>("In")};
    auto filterCoeffs{Nodex::Filter::iirFilter(m_filterOrder, m_cutoffFreq,
                                               m_samplingFreq, m_filterType,
                                               m_filterMode)};
    Nodex::Filter::EigenCoeffs filter{};
    filter = Nodex::Filter::zpk2tf(Nodex::Filter::EigenZPK(filterCoeffs));

    return Nodex::Filter::linearFilter(filter, inputData);
  });
}

void FilterNode::render() {
  ImGui::Text("Parameters:");
  static const char* filterTypes[] = {"Butterworth", "Chebyshev I",
                                      "Chebyshev II"};
  int                filterTypeIdx = static_cast<int>(m_filterType);
  if (ImGui::Combo("Type", &filterTypeIdx, filterTypes, 3)) {
    m_filterType = static_cast<Nodex::Filter::Type>(filterTypeIdx);
  }

  static const char* filterModes[] = {"Lowpass", "Highpass"};
  int                filterModeIdx = static_cast<int>(m_filterMode);
  if (ImGui::Combo("Mode", &filterModeIdx, filterModes, 2)) {
    m_filterMode = static_cast<Nodex::Filter::Mode>(filterModeIdx);
  }

  ImGui::SliderInt("Order", &m_filterOrder, 1, 10);
  ImGui::SliderDouble("fc (Hz)", &m_cutoffFreq, 1.0, m_samplingFreq / 2,
                      "%.1f");
  ImGui::SliderDouble("fs (Hz)", &m_samplingFreq, 10.0, 10000.0, "%.1f");
}

void drawConnections(Graph&                                   graph,
                     std::unordered_map<const Port*, ImVec2>& portPositions,
                     DragDropState&                           dragDropState) {
  ImDrawList* drawList      = ImGui::GetForegroundDrawList();
  const ImU32 linkColor     = IM_COL32(255, 100, 100, 255); // flashy red
  const ImU32 dragLineColor = IM_COL32(100, 200, 255, 200); // blue for drag
  const float linkThickness = 2.0f;

  // Draw existing connections
  for (auto& node : graph.getNodes()) {
    for (auto& inputName : node->inputNames()) {
      const Port* inPort    = node->inputPort(inputName);
      const Port* connected = inPort->connected();
      if (!connected)
        continue;

      auto itA = portPositions.find(inPort);
      auto itB = portPositions.find(connected);
      if (itA != portPositions.end() && itB != portPositions.end()) {
        drawList->AddBezierCubic(itB->second, itB->second + ImVec2{50.0f, 0.0f},
                                 itA->second - ImVec2{50.0f, 0.0f}, itA->second,
                                 linkColor, linkThickness);
      }
    }
  }

  // Draw drag preview line if dragging
  if (dragDropState.isDragging && dragDropState.draggedPort) {
    ImVec2 currentMousePos = ImGui::GetMousePos();

    // Find the dragged port position
    auto itDragged = portPositions.find(dragDropState.draggedPort);
    if (itDragged != portPositions.end()) {
      ImVec2 startPos = itDragged->second;
      drawList->AddBezierCubic(startPos, startPos + ImVec2{50.0f, 0.0f},
                               currentMousePos - ImVec2{50.0f, 0.0f},
                               currentMousePos, dragLineColor, linkThickness);
    }
  }
}

void graphWindow(Graph& graph) {
  auto style = ImGui::GetStyle();

  std::unordered_map<const Port*, ImVec2> portPositions;
  std::unordered_map<const Port*, ImVec2> portSizes; // Store button sizes
  static DragDropState                    dragDropState;

  ImGui::SetNextWindowPos(ImVec2{0.0f, 0.0f});
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGuiWindowFlags mainWindowFlags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_MenuBar;
  ImGui::Begin("Node Graph", nullptr, mainWindowFlags);
  {
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        // TODO: Handle exit action
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Add Node")) {
      std::string nodeName{"Node " + std::to_string(graph.numberOfNodes())};
      if (ImGui::MenuItem("Random data"))
        graph.createNode<RandomDataNode>(nodeName, 1000);

      if (ImGui::MenuItem("Sine wave"))
        graph.createNode<SineNode>(nodeName);

      if (ImGui::MenuItem("Mixer"))
        graph.createNode<MixerNode>(nodeName);

      if (ImGui::MenuItem("Filter"))
        graph.createNode<FilterNode>(nodeName);

      if (ImGui::MenuItem("Viewer"))
        graph.createNode<ViewerNode>(nodeName);

      ImGui::EndMenu();
    }

    ImGui::Text("Nodes: %zu", graph.numberOfNodes());

    ImGui::EndMenuBar();
  }
  ImGui::End();

  // Track which port was hovered during this frame
  Port* hoveredPort = nullptr;

  // For each node in the graph
  for (auto& node : graph.getNodes()) {
    // create a simple window for each node
    // the identifier/label is node label plus its unique ID to avoid
    // conflicts
    std::string winIdentifier{node->label()};
    winIdentifier += "##";
    winIdentifier += std::to_string(node->id());
    // Closable window
    bool isOpen = true;
    ImGui::Begin(winIdentifier.c_str(), &isOpen,
                 ImGuiWindowFlags_AlwaysAutoResize);

    // Inputs in left column, outputs in right column
    ImGui::Columns(2, nullptr, false);
    for (const auto& inputName : node->inputNames()) {
      ImGui::Button(inputName.data());
      auto inPort = node->inputPort(inputName);

      // Get button position and size
      auto pos = ImGui::GetItemRectMin() +
                 ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
      auto size             = ImGui::GetItemRectSize();
      portPositions[inPort] = pos;
      portSizes[inPort]     = size;

      // Handle drag-drop for input ports
      if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          if (!dragDropState.isDragging) {
            dragDropState.isDragging   = true;
            dragDropState.draggedPort  = inPort;
            dragDropState.dragStartPos = pos;
          }
        }
      }

      // During dragging, check manual hit detection
      if (dragDropState.isDragging && dragDropState.draggedPort != inPort) {
        ImVec2 itemMin  = ImGui::GetItemRectMin();
        ImVec2 itemMax  = itemMin + size;
        ImVec2 mousePos = ImGui::GetMousePos();

        if (mousePos.x >= itemMin.x && mousePos.x <= itemMax.x &&
            mousePos.y >= itemMin.y && mousePos.y <= itemMax.y) {
          hoveredPort = inPort;
        }
      }
    }

    ImGui::NextColumn();
    for (const auto& outputName : node->outputNames()) {
      float width = ImGui::CalcTextSize(outputName.data()).x +
                    style.FramePadding.x * 2 + style.ItemSpacing.x;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                           (ImGui::GetColumnWidth() - width));
      ImGui::Button(outputName.data());
      auto outPort = node->outputPort(outputName);

      auto pos = ImGui::GetItemRectMin() + ImGui::GetItemRectSize() -
                 ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
      auto size              = ImGui::GetItemRectSize();
      portPositions[outPort] = pos;
      portSizes[outPort]     = size;

      // Handle drag-drop for output ports
      if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          if (!dragDropState.isDragging) {
            dragDropState.isDragging   = true;
            dragDropState.draggedPort  = outPort;
            dragDropState.dragStartPos = pos;
          }
        }
      }

      // During dragging, check manual hit detection
      if (dragDropState.isDragging && dragDropState.draggedPort != outPort) {
        ImVec2 itemMin  = ImGui::GetItemRectMin();
        ImVec2 itemMax  = itemMin + size;
        ImVec2 mousePos = ImGui::GetMousePos();

        if (mousePos.x >= itemMin.x && mousePos.x <= itemMax.x &&
            mousePos.y >= itemMin.y && mousePos.y <= itemMax.y) {
          hoveredPort = outPort;
        }
      }
    }
    ImGui::Columns(1);
    ImGui::Separator();

    // Render node-specific UI
    node->render();

    ImGui::End();

    if (!isOpen) {
      graph.removeNode(node->name());
    }
  }

  // Handle connection on mouse release
  if (dragDropState.isDragging &&
      ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (hoveredPort && hoveredPort != dragDropState.draggedPort) {
      // if already connected, disconnect
      if (hoveredPort->connected(dragDropState.draggedPort)) {
        hoveredPort->disconnect(dragDropState.draggedPort);
      } else {
        dragDropState.draggedPort->connect(hoveredPort);
      }
    }
    dragDropState.isDragging  = false;
    dragDropState.draggedPort = nullptr;
  }

  drawConnections(graph, portPositions, dragDropState);

  graph.update();
}
} // namespace Nodex::App
