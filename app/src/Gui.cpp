#include "Gui.h"
#include "Core.h"
#include "FilterEigen.h"
#include "Serializer.h"
#include "Utils.h"
#include "imgui.h"
#include "implot.h"
#include "nfd.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <string>

namespace ImGui {

// Helper for double sliders since ImGui doesn't have one by default
inline bool SliderDouble(const char* label, double* v, double v_min,
                         double v_max, const char* format = nullptr,
                         ImGuiSliderFlags flags = 0) {
  return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format,
                      flags);
}
} // namespace ImGui

namespace Nodex::Gui {
using namespace Filter;
using namespace Core;

static int         s_mixerInputs          = 2;
static bool        s_openMixerModal       = false;
static std::string s_pendingMixerNodeName = {};

static int         s_multiViewerInputs          = 2;
static bool        s_openMultiViewerModal       = false;
static std::string s_pendingMultiViewerNodeName = {};

// MixerNode
MixerNode::MixerNode(const std::string_view name, const std::size_t inputs,
                     const std::vector<double>& gains)
    : Node{name, "Mixer"}, m_inputs{inputs}, m_gains{gains} {
  if (m_gains.empty()) {
    m_gains = std::vector<double>(m_inputs, Constants::kDefaultGain);
  }

  for (std::size_t i{0}; i < m_inputs; ++i) {
    std::string portName = "In " + std::to_string(i + 1);
    addInput<Eigen::ArrayXd>(portName, Eigen::ArrayXd{});
  }

  addOutput<Eigen::ArrayXd>("Out", [this]() { return getData(); });
}

Eigen::ArrayXd MixerNode::getData() {
  Eigen::ArrayXd result;

  Eigen::Index maxSize = 0;
  for (std::size_t i{0}; i < m_inputs; ++i) {
    auto data = inputValue<Eigen::ArrayXd>("In " + std::to_string(i + 1));
    if (data.size() > maxSize) {
      maxSize = data.size();
    }
  }

  result = Eigen::ArrayXd::Zero(maxSize);
  for (std::size_t i{0}; i < m_inputs; ++i) {
    auto data = inputValue<Eigen::ArrayXd>("In " + std::to_string(i + 1));
    if (data.size() > 0) {
      // match size by padding with zeros if needed
      Eigen::ArrayXd dataResized    = Eigen::ArrayXd::Zero(maxSize);
      dataResized.head(data.size()) = data;
      result += m_gains[i] * dataResized;
    }
  }

  return result;
}

void MixerNode::render() {
  for (std::size_t i{0}; i < m_inputs; ++i) {
    ImGui::InputDouble(("Gain " + std::to_string(i + 1)).c_str(), &m_gains[i],
                       0.1, 1.0, "%.2f");
  }
}

nlohmann::json MixerNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "MixerNode";
  j["parameters"]  = {
      {"inputs", m_inputs},
      { "gains",  m_gains}
  };

  return j;
}

// ViewerNode
ViewerNode::ViewerNode(const std::string_view name) : Node{name, "Viewer"} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
}

void ViewerNode::render() {
  using namespace Constants;

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

nlohmann::json ViewerNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "ViewerNode";

  return j;
}

// MultiViewerNode
MultiViewerNode::MultiViewerNode(const std::string_view name,
                                 const std::size_t      inputs)
    : Node{name, "Multi-Viewer"}, m_inputs{inputs} {
  for (std::size_t i{0}; i < m_inputs; ++i) {
    std::string portName = "In " + std::to_string(i + 1);
    addInput<Eigen::ArrayXd>(portName, Eigen::ArrayXd{});
  }
}

void MultiViewerNode::render() {
  using namespace Constants;

  if (ImPlot::BeginPlot("Time plot", ImVec2{kPlotWidth, kPlotHeight})) {
    for (std::size_t i{0}; i < m_inputs; ++i) {
      auto data = inputValue<Eigen::ArrayXd>("In " + std::to_string(i + 1));
      if (data.size() > 0) {
        ImPlot::PlotLine(("Input " + std::to_string(i + 1)).c_str(),
                         data.data(), static_cast<int>(data.size()));
      }
    }
    ImPlot::EndPlot();
  }
}

nlohmann::json MultiViewerNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "MultiViewerNode";
  j["parameters"]  = {
      {"inputs", m_inputs}
  };

  return j;
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

nlohmann::json RandomDataNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "RandomDataNode";
  j["parameters"]  = {
      {"samples", m_samples}
  };

  return j;
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

Eigen::ArrayXd SineNode::generateWave() const {
  Eigen::ArrayXd sineWave(m_samples);

  const double freqPhaseScale{Constants::kTwoPi * m_frequency / m_samplingFreq};

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
  ImGui::SliderDouble("Phase (rad)", &m_phase, 0.0, Constants::kTwoPi, "%.2f");
  ImGui::InputDouble("fs (Hz)", &m_samplingFreq, 10.0, 100.0, "%.2f");
  ImGui::InputDouble("Offset", &m_offset, 0.1, 1.0, "%.2f");
}

nlohmann::json SineNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "SineNode";
  j["parameters"]  = {
      {  "samples",      m_samples},
      {"frequency",    m_frequency},
      {"amplitude",    m_amplitude},
      {    "phase",        m_phase},
      {       "fs", m_samplingFreq},
      {   "offset",       m_offset}
  };

  return j;
}

// FilterNode
FilterNode::FilterNode(const std::string_view    name,
                       const Nodex::Filter::Mode mode,
                       const Nodex::Filter::Type type, const int order,
                       const double cutoffFreq, const double samplingFreq,
                       const double cutoffFreq2)
    : Node{name, "Filter"}, m_filterMode{mode}, m_filterType{type},
      m_filterOrder{order}, m_cutoffFreq{cutoffFreq},
      m_samplingFreq{samplingFreq}, m_cutoffFreq2{cutoffFreq2} {
  addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
  addOutput<Eigen::ArrayXd>("Out", [this]() {
    auto inputData{inputValue<Eigen::ArrayXd>("In")};
    ZPK  filterCoeffs{};

    if (m_filterMode == Mode::bandpass || m_filterMode == Mode::bandstop) {
      filterCoeffs = iirFilter(m_filterOrder, m_cutoffFreq, m_cutoffFreq2,
                               m_samplingFreq, m_filterType, m_filterMode);
    } else {
      filterCoeffs = iirFilter(m_filterOrder, m_cutoffFreq, m_samplingFreq,
                               m_filterType, m_filterMode);
    }

    EigenCoeffs filter{};
    filter = zpk2tf(EigenZPK(filterCoeffs));

    return linearFilter(filter, inputData);
  });
}

void FilterNode::render() {
  ImGui::Text("Parameters:");
  static constexpr const char* filterTypes[] = {"Butterworth", "Chebyshev I",
                                                "Chebyshev II"};
  static constexpr const char* filterModes[] = {"Lowpass", "Highpass",
                                                "Bandpass", "Bandstop"};

  int filterTypeIdx = static_cast<int>(m_filterType);
  if (ImGui::Combo("Type", &filterTypeIdx, filterTypes, 3)) {
    m_filterType = static_cast<Type>(filterTypeIdx);
  }

  int filterModeIdx = static_cast<int>(m_filterMode);
  if (ImGui::Combo("Mode", &filterModeIdx, filterModes, 4)) {
    m_filterMode = static_cast<Mode>(filterModeIdx);
  }

  ImGui::SliderInt("Order", &m_filterOrder, 1, 10);

  if (m_filterMode == Mode::bandpass || m_filterMode == Mode::bandstop) {
    ImGui::SliderDouble("f low (Hz)", &m_cutoffFreq, 1.0, m_samplingFreq / 2,
                        "%.1f");
    ImGui::SliderDouble("f high (Hz)", &m_cutoffFreq2, m_cutoffFreq,
                        m_samplingFreq / 2, "%.1f");
  } else {
    ImGui::SliderDouble("fc (Hz)", &m_cutoffFreq, 1.0, m_samplingFreq / 2,
                        "%.1f");
  }

  ImGui::SliderDouble("fs (Hz)", &m_samplingFreq, 10.0, 10000.0, "%.1f");
}

nlohmann::json FilterNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "FilterNode";
  j["parameters"]  = {
      { "mode", static_cast<int>(m_filterMode)},
      { "type", static_cast<int>(m_filterType)},
      {"order",                  m_filterOrder},
      {   "fc",                   m_cutoffFreq},
      {  "fc2",                  m_cutoffFreq2},
      {   "fs",                 m_samplingFreq}
  };

  return j;
}

// CSVNode
CSVNode::CSVNode(const std::string_view name, const std::string& filePath)
    : Node{name, "CSV Import"}, m_filePath{filePath} {
  if (!m_filePath.empty()) {
    loadCsvFile(m_filePath);
  }
}

void CSVNode::loadCsvFile(const std::string& filePath) {
  try {
    m_csvData  = Nodex::Utils::loadCsvData(filePath);
    m_filePath = filePath;

    // Remove old outputs
    m_outputs.clear();

    // Create output port for each column
    for (const auto& colName : m_csvData.columnNames) {
      // Capture column data in lambda
      addOutput<Eigen::ArrayXd>(colName, [this, colName]() {
        auto it = m_csvData.columns.find(colName);
        return it != m_csvData.columns.end() ? it->second : Eigen::ArrayXd{};
      });
    }
  } catch (const std::exception& e) {
    std::cerr << "Error loading CSV: " << e.what() << "\n";
  }
}

void CSVNode::render() {
  ImGui::Text("File: %s", m_filePath.empty() ? "(none)" : m_filePath.c_str());
  ImGui::Text("Columns: %zu, Rows: %ld", m_csvData.columnNames.size(),
              m_csvData.columnNames.empty()
                  ? 0
                  : m_csvData.columns.begin()->second.size());

  if (ImGui::Button("Load CSV...")) {
    NFD::UniquePath outPath;
    nfdfilteritem_t filterItem[1] = {
        {"CSV Files", "csv"}
    };
    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

    if (result == NFD_OKAY) {
      loadCsvFile(outPath.get());
    }
  }
}

nlohmann::json CSVNode::serialize() const {
  nlohmann::json j = Node::serialize();
  j["type"]        = "CSVNode";
  j["parameters"]  = {
      {"filePath", m_filePath}
  };

  return j;
}

void drawConnections(Graph&                                   graph,
                     std::unordered_map<const Port*, ImVec2>& portPositions,
                     DragDropState&                           dragDropState) {
  using namespace Constants;

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

  if (ImGui::MenuItem("CSV Import"))
    graph.createNode<CSVNode>(nodeName);

  if (ImGui::MenuItem("Mixer")) {
    s_pendingMixerNodeName = nodeName;
    s_openMixerModal       = true;
  }

  if (ImGui::MenuItem("Filter"))
    graph.createNode<FilterNode>(nodeName);

  if (ImGui::MenuItem("Viewer"))
    graph.createNode<ViewerNode>(nodeName);

  if (ImGui::MenuItem("Multi-Viewer")) {
    s_pendingMultiViewerNodeName = nodeName;
    s_openMultiViewerModal       = true;
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
      if (ImGui::MenuItem("Save")) {
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
      }
      if (ImGui::MenuItem("Load")) {
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
            graph = Serializer::loadFromJson(jsonString);
          } catch (const std::exception& e) {
            std::cerr << "Error loading JSON: " << e.what() << "\n";
          }
        }
      }
      if (ImGui::BeginMenu("Export")) {
        // Export submenu for each output node
        bool anyOutputNodes = false;
        for (auto& node : graph.getNodes()) {
          auto csvNode = dynamic_cast<CSVNode*>(node.get());

          // Special handling for CSVNode - export all columns
          if (csvNode) {
            const auto& csvData = csvNode->getData();
            if (!csvData.columnNames.empty()) {
              if (ImGui::MenuItem(
                      (std::string(node->name()) + " (all columns)").c_str())) {
                try {
                  NFD::UniquePath savePath;
                  nfdfilteritem_t filterItem[1] = {
                      {"CSV Files", "csv"}
                  };
                  nfdresult_t result =
                      NFD::SaveDialog(savePath, filterItem, 1, nullptr);

                  if (result == NFD_OKAY) {
                    Nodex::Utils::saveCsvData(savePath.get(), csvData);
                    std::cout << "Exported to: " << savePath.get() << "\n";
                  }
                } catch (const std::exception& e) {
                  std::cerr << "Error exporting CSV: " << e.what() << "\n";
                }
              }
              anyOutputNodes = true;
            }
            continue;
          }

          if (node->outputNames().empty()) {
            continue;
          }

          // Create menu item for each node with outputs
          for (const auto& outputName : node->outputNames()) {
            std::string menuLabel =
                std::string(node->name()) + " - " + std::string(outputName);
            if (ImGui::MenuItem(menuLabel.c_str())) {
              try {
                auto viewerNode = dynamic_cast<ViewerNode*>(node.get());

                // Try to extract data based on node type
                Eigen::ArrayXd data;
                if (viewerNode) {
                  data = viewerNode->inputValue<Eigen::ArrayXd>("In");
                } else {
                  // For other nodes, try to get value from output port
                  try {
                    auto outPortPtr = node->output<Eigen::ArrayXd>(outputName);
                    data            = outPortPtr->value();
                  } catch (...) {
                    std::cerr << "Cannot export from this output type\n";
                    continue;
                  }
                }

                NFD::UniquePath savePath;
                nfdfilteritem_t filterItem[1] = {
                    {"CSV Files", "csv"}
                };
                nfdresult_t result =
                    NFD::SaveDialog(savePath, filterItem, 1, nullptr);

                if (result == NFD_OKAY) {
                  Nodex::Utils::saveCsvData(savePath.get(), data);
                  std::cout << "Exported to: " << savePath.get() << "\n";
                }
              } catch (const std::exception& e) {
                std::cerr << "Error exporting CSV: " << e.what() << "\n";
              }
            }
            anyOutputNodes = true;
          }
        }

        if (!anyOutputNodes) {
          ImGui::MenuItem("(No outputs available)", nullptr, false, false);
        }

        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::BeginMenu("Add")) {
        renderNodeMenu(graph);
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("Clear all")) {
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
    if (ImGui::Button("Ok")) {
      graph.createNode<MixerNode>(s_pendingMixerNodeName, s_mixerInputs);
      s_pendingMixerNodeName.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (s_openMultiViewerModal) {
    ImGui::OpenPopup("Multi-Viewer Inputs");
    s_openMultiViewerModal = false;
  }
  if (ImGui::BeginPopupModal("Multi-Viewer Inputs", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputInt("Number of inputs", &s_multiViewerInputs);
    if (ImGui::Button("Ok")) {
      graph.createNode<MultiViewerNode>(s_pendingMultiViewerNodeName,
                                        s_multiViewerInputs);
      s_pendingMultiViewerNodeName.clear();
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
} // namespace Nodex::Gui
