#include "Core.h"
#include "Filter.h"
#include "FilterEigen.h"
#include "NodeGraphWindow.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <Eigen/Dense>
#include <cstddef>
#include <vector>
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <iostream>

using Nodex::Filter::Signal;

namespace ImGui {
bool SliderDouble(const char* label, double* v, double v_min, double v_max,
                  const char* format = NULL, ImGuiSliderFlags flags = 0) {
  return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format,
                      flags);
}
} // namespace ImGui

void framebuffer_size_callback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);

// settings
constexpr int kWinWidth{1600};
constexpr int kWinHeight{1200};

const char* vertexShaderSource{R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)"};

const char* fragmentShaderSource{R"(
#version 330 core
out vec4 FragColor;
void main() {
  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)"};

class AdderNode : public Node {
public:
  AdderNode(std::string_view name) : Node{name, "Adder"} {
    addInput<Eigen::ArrayXd>("In 1", Eigen::ArrayXd{});
    addInput<Eigen::ArrayXd>("In 2", Eigen::ArrayXd{});
    addOutput<Eigen::ArrayXd>("Out", [this]() {
      return inputValue<Eigen::ArrayXd>("In 1") +
             inputValue<Eigen::ArrayXd>("In 2");
    });
  }

  void render() override { ImGui::Text("Adder node"); }

private:
};

class DataNode : public Node {
public:
  DataNode(std::string_view name, const Eigen::ArrayXd& data)
      : Node{name, "Data"}, m_data{data} {
    addOutput<Eigen::ArrayXd>("Out", [this]() { return m_data; });
  }

  void render() override {
    ImGui::Text("%d samples", static_cast<int>(m_data.size()));
  }

private:
  Eigen::ArrayXd m_data;
};

class DataViewerNode : public Node {
public:
  DataViewerNode(std::string_view name) : Node{name, "Data Viewer"} {
    addInput<Eigen::ArrayXd>("In", Eigen::ArrayXd{});
  }

  void render() override {
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

private:
};

class FilterNode : public Node {
public:
  FilterNode(std::string_view name) : Node{name, "Filter"} {
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

  void render() override {
    ImGui::Text("Filter Parameters:");
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

private:
  Nodex::Filter::Mode m_filterMode{};
  Nodex::Filter::Type m_filterType{};
  int                 m_filterOrder{2};
  double              m_cutoffFreq{100.0};
  double              m_samplingFreq{1000.0};
};

Eigen::ArrayXd randomData{Eigen::ArrayXd::Random(1000)};

int main(void) {
  std::cout << "Nodex::Core v" << Nodex::Core::version() << "\n";

  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  float mainScale{
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor())};
  GLFWwindow* window{
      glfwCreateWindow(kWinWidth, kWinHeight, "Nodex GUI", nullptr, nullptr)};
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";

    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize glad\n";

    return -1;
  }

  glViewport(0, 0, 800, 600);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();

  ImGuiIO& io{ImGui::GetIO()};
  (void)io;
  ImGui::StyleColorsDark();

  // Setup scaling
  ImGuiStyle& style{ImGui::GetStyle()};
  // style.ScaleAllSizes(mainScale);
  // style.FontScaleDpi     = mainScale;
  style.AntiAliasedLines = true;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // Make sinusoidal data frequency of 5 Hz
  // Amplitude of 1.0, sampled at 1000 Hz for 1 second
  Eigen::ArrayXd sinData = Eigen::sin(Eigen::ArrayXd::LinSpaced(
      1000, 0.0, 2.0 * std::numbers::pi * 5.0 * (999.0 / 1000.0)));

  NodeGraph graph;
  auto      dataNode{graph.createNode<DataNode>("Data 1", randomData)};
  auto      filterNode{graph.createNode<FilterNode>("Filter 1")};
  auto      dataNode2{graph.createNode<DataNode>("Data 2", sinData)};
  auto      dataViewerNode{graph.createNode<DataViewerNode>("View 1")};
  auto      dataViewerNode2{graph.createNode<DataViewerNode>("View 2")};
  auto      dataViewerNode3{graph.createNode<DataViewerNode>("View 3")};
  auto      adderNode{graph.createNode<AdderNode>("Adder")};

  filterNode->input<Eigen::ArrayXd>("In")->connect(
      dataNode->output<Eigen::ArrayXd>("Out"));
  dataViewerNode->input<Eigen::ArrayXd>("In")->connect(
      filterNode->output<Eigen::ArrayXd>("Out"));
  adderNode->input<Eigen::ArrayXd>("In 1")->connect(
      filterNode->output<Eigen::ArrayXd>("Out"));
  adderNode->input<Eigen::ArrayXd>("In 2")->connect(
      dataNode2->output<Eigen::ArrayXd>("Out"));
  dataViewerNode2->input<Eigen::ArrayXd>("In")->connect(
      adderNode->output<Eigen::ArrayXd>("Out"));
  dataViewerNode3->input<Eigen::ArrayXd>("In")->connect(
      dataNode2->output<Eigen::ArrayXd>("Out"));

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Node editor
    NodeGraphWindow(graph);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap buffers and poll IO events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    std::cout << "Closing window\n";
    glfwSetWindowShouldClose(window, true);
  }
}

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window, int width,
                               int height) {
  glViewport(0, 0, width, height);
}
