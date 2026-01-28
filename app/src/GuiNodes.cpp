#include "GuiNodes.h"
#include "Core.h"
#include "FilterEigen.h"
#include "imgui.h"
#include "implot.h"
#include "nfd.h"
#include "nfd.hpp"
#include "nlohmann/json_fwd.hpp"
#include <fstream>
#include <iostream>
#include <numbers>
#include <string>

namespace ImGui {
inline bool SliderDouble(const char* label, double* v, double v_min,
                         double v_max, const char* format = nullptr,
                         ImGuiSliderFlags flags = 0) {
  return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format,
                      flags);
}
} // namespace ImGui

namespace Nodex::App {
using namespace Nodex::Filter;

constexpr ImU32 kLinkColor     = IM_COL32(255, 100, 100, 255); // Flashy red
constexpr ImU32 kDragLineColor = IM_COL32(100, 200, 255, 200); // Blue drag line
constexpr float kLinkThickness = 2.0f;
constexpr float kBezierOffset  = 50.0f; // Bezier curve control point offset
constexpr double kTwoPi        = 2.0 * std::numbers::pi;
constexpr float  kPlotWidth    = 200.0f;
constexpr float  kPlotHeight   = 150.0f;

static int         s_mixerInputs          = 2;
static bool        s_openMixerModal       = false;
static std::string s_pendingMixerNodeName = {};

Graph loadJson(const std::string& jsonString) {
  Graph          graph;
  nlohmann::json j = nlohmann::json::parse(jsonString);
  std::cout << "Parsed JSON:\n" << j.dump(4) << "\n";

  for (const auto& nodeJson : j["nodes"]) {
    std::string nodeType = nodeJson["type"];
    std::string nodeName = nodeJson["name"];

    if (nodeType == "RandomDataNode") {
      int samples = nodeJson["parameters"]["samples"];
      graph.createNode<RandomDataNode>(nodeName, samples);
    } else if (nodeType == "SineNode") {
      int    samples   = nodeJson["parameters"]["samples"];
      double frequency = nodeJson["parameters"]["frequency"];
      double amplitude = nodeJson["parameters"]["amplitude"];
      double phase     = nodeJson["parameters"]["phase"];
      double fs        = nodeJson["parameters"]["fs"];
      double offset    = nodeJson["parameters"]["offset"];
      graph.createNode<SineNode>(nodeName, samples, frequency, amplitude, phase,
                                 fs, offset);
    } else if (nodeType == "MixerNode") {
      std::size_t         inputs = nodeJson["parameters"]["inputs"];
      std::vector<double> gains =
          nodeJson["parameters"]["gains"].get<std::vector<double>>();
      graph.createNode<MixerNode>(nodeName, inputs, gains);
    } else if (nodeType == "FilterNode") {
      auto mode =
          static_cast<Filter::Mode>(nodeJson["parameters"]["filter_mode"]);
      auto type =
          static_cast<Filter::Type>(nodeJson["parameters"]["filter_type"]);
      int    order        = nodeJson["parameters"]["filter_order"];
      double cutoffFreq   = nodeJson["parameters"]["cutoff_freq"];
      double samplingFreq = nodeJson["parameters"]["sampling_freq"];
      graph.createNode<FilterNode>(nodeName, mode, type, order, cutoffFreq,
                                   samplingFreq);
    } else if (nodeType == "ViewerNode") {
      graph.createNode<ViewerNode>(nodeName);
    }
  }

  return graph;
}

// MixerNode

// ViewerNode
ViewerNode::ViewerNode(const std::string_view name) : Node{name, "Viewer"} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
}

void ViewerNode::render() {
  auto data{inputValue<Eigen::ArrayXd>("In")};
  if (data.size() > 0) {
    if (ImPlot::BeginPlot("Time plot", ImVec2{kPlotWidth, kPlotHeight})) {
      ImPlot::PlotLine("", data.data(), static_cast<int>(data.size()));
      ImPlot::EndPlot();
    }
  } else {
    ImGui::Text("No data connected.");
  }
}

// RandomDataNode
RandomDataNode::RandomDataNode(const std::string_view name, const int size)
    : Node{name, "Random data"}, m_samples{size},
      m_data{Eigen::ArrayXd::Random(size)} {
  addOutput<Eigen::ArrayXd>("Out", [this]() { return m_data; });
}

void RandomDataNode::render() {
  if (ImGui::InputInt("Number of samples", &m_samples))
    m_data = Eigen::ArrayXd::Random(m_samples);
}

// SineNode
SineNode::SineNode(const std::string_view name, const int size,
                   const double frequency, const double amplitude,
                   const double phase, const double fs, const double offset)
    : Node{name, "Sine wave"}, m_samples{size}, m_frequency{frequency},
      m_amplitude{amplitude}, m_phase{phase}, m_samplingFreq{fs},
      m_offset{offset} {
  addOutput<Eigen::ArrayXd>("Out", [this]() { return generateWave(); });
}

Eigen::ArrayXd SineNode::generateWave() {
  Eigen::ArrayXd sineWave(m_samples);
  const double   freqPhaseScale = kTwoPi * m_frequency / m_samplingFreq;
  for (int i = 0; i < m_samples; ++i) {
    sineWave[i] =
        m_amplitude * std::sin(freqPhaseScale * i + m_phase) + m_offset;
  }
  return sineWave;
}

void SineNode::render() {
  ImGui::Text("Parameters:");
  ImGui::InputInt("Number of samples", &m_samples);
  ImGui::SliderDouble("f (Hz)", &m_frequency, 0.1, m_samplingFreq / 2, "%.2f");
  ImGui::InputDouble("Amplitude", &m_amplitude, 0.1, 1.0, "%.2f");
  ImGui::SliderDouble("Phase (rad)", &m_phase, 0.0, kTwoPi, "%.2f");
  ImGui::InputDouble("fs (Hz)", &m_samplingFreq, 10.0, 100.0, "%.2f");
  ImGui::InputDouble("Offset", &m_offset, 0.1, 1.0, "%.2f");
}

// FilterNode
FilterNode::FilterNode(const std::string_view    name,
                       const Nodex::Filter::Mode mode,
                       const Nodex::Filter::Type type, const int order,
                       const double cutoffFreq, const double samplingFreq)
    : Node{name, "Filter"}, m_filterMode{mode}, m_filterType{type},
      m_filterOrder{order}, m_cutoffFreq{cutoffFreq},
      m_samplingFreq{samplingFreq} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
  addOutput<Eigen::ArrayXd>("Out", [this]() {
    auto inputData{inputValue<Eigen::ArrayXd>("In")};
    auto filterCoeffs{iirFilter(m_filterOrder, m_cutoffFreq, m_samplingFreq,
                                m_filterType, m_filterMode)};
    EigenCoeffs filter{};
    filter = zpk2tf(EigenZPK(filterCoeffs));

    return linearFilter(filter, inputData);
  });
}

void FilterNode::render() {
  ImGui::Text("Parameters:");
  static constexpr const char* filterTypes[] = {"Butterworth", "Chebyshev I",
                                                "Chebyshev II"};
  static constexpr const char* filterModes[] = {"Lowpass", "Highpass"};

  int filterTypeIdx = static_cast<int>(m_filterType);
  if (ImGui::Combo("Type", &filterTypeIdx, filterTypes, 3)) {
    m_filterType = static_cast<Type>(filterTypeIdx);
  }

  int filterModeIdx = static_cast<int>(m_filterMode);
  if (ImGui::Combo("Mode", &filterModeIdx, filterModes, 2)) {
    m_filterMode = static_cast<Mode>(filterModeIdx);
  }

  ImGui::SliderInt("Order", &m_filterOrder, 1, 10);
  ImGui::SliderDouble("fc (Hz)", &m_cutoffFreq, 1.0, m_samplingFreq / 2,
                      "%.1f");
  ImGui::SliderDouble("fs (Hz)", &m_samplingFreq, 10.0, 10000.0, "%.1f");
}

void drawConnections(Graph&                                   graph,
                     std::unordered_map<const Port*, ImVec2>& portPositions,
                     DragDropState&                           dragDropState) {
  ImDrawList* drawList = ImGui::GetForegroundDrawList();

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
        drawList->AddBezierCubic(itB->second,
                                 itB->second + ImVec2{kBezierOffset, 0.0f},
                                 itA->second - ImVec2{kBezierOffset, 0.0f},
                                 itA->second, kLinkColor, kLinkThickness);
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
      drawList->AddBezierCubic(startPos, startPos + ImVec2{kBezierOffset, 0.0f},
                               currentMousePos - ImVec2{kBezierOffset, 0.0f},
                               currentMousePos, kDragLineColor, kLinkThickness);
    }
  }
}

bool isMouseOverItem() {
  ImVec2 mousePos{ImGui::GetMousePos()};
  ImVec2 itemMin{ImGui::GetItemRectMin()};
  ImVec2 itemMax{ImGui::GetItemRectMax()};

  return (mousePos.x >= itemMin.x && mousePos.x <= itemMax.x &&
          mousePos.y >= itemMin.y && mousePos.y <= itemMax.y);
}

std::string getNodeWindowId(const SharedPtr<Node>& node) {
  // Cache to avoid repeated string allocations in hot path
  static thread_local std::string buffer;
  buffer.clear();
  buffer.append(node->label());
  buffer.append("##");
  buffer.append(std::to_string(node->id()));
  return buffer;
}

void renderNodeMenu(Graph& graph) {
  static thread_local std::string nodeName;
  nodeName = "Node " + std::to_string(graph.numberOfNodes());

  if (ImGui::MenuItem("Random data"))
    graph.createNode<RandomDataNode>(nodeName);

  if (ImGui::MenuItem("Sine wave"))
    graph.createNode<SineNode>(nodeName);

  if (ImGui::MenuItem("Mixer")) {
    s_pendingMixerNodeName = nodeName;
    s_openMixerModal       = true;
  }

  if (ImGui::MenuItem("Filter"))
    graph.createNode<FilterNode>(nodeName);

  if (ImGui::MenuItem("Viewer"))
    graph.createNode<ViewerNode>(nodeName);
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
      if (ImGui::MenuItem("Save")) {
        // TODO: implement save
        nlohmann::json j = graph.serialize();

        NFD::UniquePath outPath;

        nfdfilteritem_t filterItem[1] = {
            {"JSON Files", "json"}
        };
        nfdresult_t result = NFD::SaveDialog(outPath, filterItem, 1, nullptr);
        if (result == NFD_OKAY) {
          try {
            std::ofstream file{outPath.get()};
            file << j.dump();
          } catch (const std::exception& e) {
            std::cerr << "Error saving JSON: " << e.what() << "\n";
          }
        }
      } else if (ImGui::MenuItem("Load")) {
        // TODO: implement load
        // Open file dialog to select JSON file
        NFD::UniquePath outPath;

        nfdfilteritem_t filterItem[1] = {
            {"JSON Files", "json"}
        };
        nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY) {
          try {
            std::ifstream file{outPath.get()};
            std::string   jsonString{std::istreambuf_iterator<char>(file),
                                   std::istreambuf_iterator<char>()};
            graph = loadJson(jsonString);
          } catch (const std::exception& e) {
            std::cerr << "Error loading JSON: " << e.what() << "\n";
          }
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::BeginMenu("Add")) {
        renderNodeMenu(graph);
        ImGui::EndMenu();
      } else if (ImGui::MenuItem("Clear all")) {
        graph.clear();
      }
      ImGui::EndMenu();
    }

    ImGui::Text("Nodes: %zu", graph.numberOfNodes());

    ImGui::EndMenuBar();
  }

  if (ImGui::BeginPopupContextWindow("NodeGraphContextMenu")) {
    renderNodeMenu(graph);
    ImGui::EndPopup();
  }

  if (s_openMixerModal) {
    ImGui::OpenPopup("Mixer Inputs");
    s_openMixerModal = false;
  }
  if (ImGui::BeginPopupModal("Mixer Inputs", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputInt("Number of inputs", &s_mixerInputs);
    if (ImGui::Button("OK")) {
      graph.createNode<MixerNode>(s_pendingMixerNodeName, s_mixerInputs);
      s_pendingMixerNodeName.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::End();

  // Track which port was hovered during this frame
  Port* hoveredPort = nullptr;

  // For each node in the graph
  for (auto& node : graph.getNodes()) {
    // Closable window
    bool isOpen = true;
    ImGui::Begin(getNodeWindowId(node).c_str(), &isOpen,
                 ImGuiWindowFlags_AlwaysAutoResize);

    // Inputs in left column, outputs in right column
    ImGui::Columns(2, nullptr, false);

    // Helper lambda to handle port UI and drag-drop
    auto handlePortButton = [&](const std::string& portName, Port* port,
                                bool isOutput) {
      if (isOutput) {
        float width = ImGui::CalcTextSize(portName.data()).x +
                      style.FramePadding.x * 2 + style.ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             (ImGui::GetColumnWidth() - width));
      }

      ImGui::Button(portName.data());
      ImVec2 itemMin = ImGui::GetItemRectMin();
      ImVec2 size    = ImGui::GetItemRectSize();
      // For output ports, use right edge; for input ports, use left edge
      ImVec2 pos =
          isOutput
              ? itemMin + size - ImVec2{0.0f, ImGui::GetTextLineHeight() / 2}
              : itemMin + ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};

      portPositions[port] = pos;
      portSizes[port]     = size;

      // Handle drag-drop
      if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!dragDropState.isDragging) {
          dragDropState.isDragging   = true;
          dragDropState.draggedPort  = port;
          dragDropState.dragStartPos = pos;
        }
      }

      // Check manual hit detection during dragging
      if (dragDropState.isDragging && dragDropState.draggedPort != port &&
          isMouseOverItem()) {
        hoveredPort = port;
      }
    };

    for (const auto& inputName : node->inputNames()) {
      handlePortButton(std::string(inputName), node->inputPort(inputName),
                       false);
    }

    ImGui::NextColumn();
    for (const auto& outputName : node->outputNames()) {
      handlePortButton(std::string(outputName), node->outputPort(outputName),
                       true);
    }
    ImGui::Columns(1);
    ImGui::Separator();

    node->render();

    ImGui::End();

    if (!isOpen) {
      graph.removeNode(node->name());
    }
  }

  // Connect or disconnect ports on mouse release
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
