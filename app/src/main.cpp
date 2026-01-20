#include "Core.h"
#include "Filter.h"
#include "FilterEigen.h"
#include "ImNodeFlow.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <Eigen/Dense>
#include <complex>
#include <cstddef>
#include <vector>
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <iostream>
#include <string>

using Nodex::Filter::Signal;

namespace ImGui {
bool SliderDouble(const char* label, double* v, double v_min, double v_max,
                  const char* format = NULL, ImGuiSliderFlags flags = 0) {
  return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format,
                      flags);
}
} // namespace ImGui

class DataViewerNode : public ImFlow::BaseNode {
public:
  DataViewerNode() {
    setTitle("Data viewer");
    setStyle(ImFlow::NodeStyle::green());
    ImFlow::BaseNode::addIN<Signal>(">", Signal{},
                                    ImFlow::ConnectionFilter::SameType());
  }

  void draw() override {
    ImGui::SetNextItemWidth(100.f);
    if (ImPlot::BeginPlot("", ImVec2(300, 200))) {
      ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit,
                        ImPlotAxisFlags_AutoFit);
      ImPlot::PlotLine("", getInVal<Signal>(">").data(),
                       static_cast<int>(getInVal<Signal>(">").size()));

      ImPlot::EndPlot();
    }
  }

private:
};

class DataNode : public ImFlow::BaseNode {
public:
  DataNode() {
    setTitle("Data");
    setStyle(ImFlow::NodeStyle::green());
    ImFlow::BaseNode::addOUT<Signal>(">", nullptr)->behaviour([this]() {
      return data_;
    });
  }

  void draw() override {
    ImGui::Text("Samples: %d", static_cast<int>(data_.size()));
  }

  void setData(const Signal& data) { data_ = data; };

private:
  Signal data_{};
};

class FilterNode : public ImFlow::BaseNode {
public:
  FilterNode() {
    setTitle("Filter");
    setStyle(ImFlow::NodeStyle::brown());
    ImFlow::BaseNode::addIN<Signal>(">", Signal(),
                                    ImFlow::ConnectionFilter::SameType());
    ImFlow::BaseNode::addOUT<Signal>(">", nullptr)->behaviour([this]() {
      auto zpk{Nodex::Filter::iirFilter(m_order, m_fc, m_fs, m_filterType,
                                        m_filterMode, m_param)};
      auto filter{Nodex::Filter::zpk2tf(zpk)};

      return Nodex::Filter::linearFilter(filter, getInVal<Signal>(">"));
    });
  }

  void draw() override {
    float itemsWidth{150.0f};
    ImGui::SetNextItemWidth(itemsWidth);
    // Filter type dropdown
    static const char* filterTypes[] = {"Butterworth", "Cheby1", "Cheby2"};
    int                filterTypeIdx = static_cast<int>(m_filterType);
    if (ImGui::Combo("Type", &filterTypeIdx, filterTypes, 3)) {
      m_filterType = static_cast<Nodex::Filter::Type>(filterTypeIdx);
    }

    ImGui::SetNextItemWidth(itemsWidth);
    // Filter mode dropdown
    static const char* filterModes[] = {"Lowpass", "Highpass"};
    int                filterModeIdx = static_cast<int>(m_filterMode);
    if (ImGui::Combo("Mode", &filterModeIdx, filterModes, 2)) {
      m_filterMode = static_cast<Nodex::Filter::Mode>(filterModeIdx);
    }

    ImGui::SetNextItemWidth(itemsWidth);
    // Filter order slider
    ImGui::SliderInt("Order", &m_order, 1, 8);

    ImGui::SetNextItemWidth(itemsWidth);
    // Cutoff frequency slider
    ImGui::SliderDouble("fc (Hz)", &m_fc, 0.1,
                        m_fs / 2 - std::numeric_limits<double>::epsilon(),
                        "%.2f");

    ImGui::SetNextItemWidth(itemsWidth);
    // Sampling frequency slider
    ImGui::InputDouble("fs (Hz)", &m_fs, 0.1, 1e6, "%.2f",
                       ImGuiInputTextFlags_AutoSelectAll |
                           ImGuiInputTextFlags_CharsScientific);

    ImGui::SetNextItemWidth(itemsWidth);
    // Ripple controls for Chebyshev filters
    if (m_filterType == Nodex::Filter::cheb1) {
      ImGui::SliderDouble("Passband ripple (dB)", &m_param, 0.1, 12.0, "%.2f");
    } else if (m_filterType == Nodex::Filter::cheb2) {
      ImGui::SliderDouble("Stopband attenuation (dB)", &m_param, 0.1, 80.0,
                          "%.2f");
    }

    ImGui::SetNextItemWidth(itemsWidth);
    if (ImPlot::BeginPlot("Freq response", ImVec2(300, 200))) {
      Nodex::Filter::ZPK zpk{Nodex::Filter::iirFilter(
          m_order, m_fc, m_fs, m_filterType, m_filterMode, m_param)};

      std::vector<double> w{};
      // Frequency vector from 0 to fs/2, log spaced
      std::size_t nPoints{512};
      w.reserve(nPoints);
      double logMin{std::log10(1.0)};
      double logMax{std::log10(m_fs / 2.0)};
      for (std::size_t i{0}; i < nPoints; ++i) {
        double exponent{logMin + (logMax - logMin) * static_cast<double>(i) /
                                     static_cast<double>(nPoints - 1)};
        w.push_back(std::pow(10.0, exponent));
      }

      const std::vector<std::complex<double>> h{Nodex::Filter::freqz(zpk, w)};
      Eigen::Map<const Eigen::ArrayXcd>       hMap{
          h.data(), static_cast<Eigen::Index>(h.size())};

      // Magnitude of the filter response
      Eigen::ArrayXd data{(hMap.abs())};

      ImPlot::SetupAxes("Frequency (Hz)", "Magnitude", 0,
                        ImPlotAxisFlags_AutoFit);
      ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
      ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
      ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, m_fs / 2.0);
      ImPlot::SetupAxisLimits(ImAxis_Y1, 1e-6, 10.0);
      ImPlot::PlotLine("", data.data(), data.size());

      // draw cutoff frequency line
      ImPlot::DragLineX(1234, &m_fc, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

      ImPlot::EndPlot();
    }
  }

  void setFrequency(double fc) { m_fc = fc; }
  void setOrder(int order) { m_order = order; }
  void setParam(double param) { m_param = param; }
  void setType(Nodex::Filter::Type type) { m_filterType = type; }
  void setMode(Nodex::Filter::Mode mode) { m_filterMode = mode; }

private:
  double              m_fs{10000.0};
  int                 m_order{2};
  double              m_fc{100.0};
  double              m_param{5.0f}; // Used for Chebyshev filters
  Nodex::Filter::Type m_filterType{Nodex::Filter::butter};
  Nodex::Filter::Mode m_filterMode{Nodex::Filter::lowpass};
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor : ImFlow::BaseNode {
  ImFlow::ImNodeFlow mINF;

  NodeEditor() : BaseNode() {
    mINF.getGrid().config().zoom_enabled = true;

    auto dataNode{mINF.addNode<DataNode>({50, 50})};

    auto n1{mINF.addNode<DataViewerNode>({250, 150})};
    auto nf1{mINF.addNode<FilterNode>({550, 100})};
    auto nf2{mINF.addNode<FilterNode>({550, 500})};
    nf1->setFrequency(50.0f);
    nf2->setFrequency(150.0);
    nf2->setMode(Nodex::Filter::highpass);
    nf2->setType(Nodex::Filter::cheb2);

    auto n2{mINF.addNode<DataViewerNode>({950, 50})};
    auto n3{mINF.addNode<DataViewerNode>({950, 300})};

    // Sample data
    Signal y(1000);
    // Gaussian noise between -1.0 and 1.0
    std::generate(y.begin(), y.end(), []() {
      return 2.0 *
             (static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX) -
              0.5);
    });

    dataNode.get()->setData(y);
    dataNode->outPin(">")->createLink(n1->inPin(">"));
    dataNode->outPin(">")->createLink(nf1->inPin(">"));
    dataNode->outPin(">")->createLink(nf2->inPin(">"));
    nf1->outPin(">")->createLink(n2->inPin(">"));
    nf2->outPin(">")->createLink(n3->inPin(">"));
  }

  void set_size(ImVec2 d) { mINF.setSize(d); }

  void draw() override { mINF.update(); }
};

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

  float vertices[]{
      -0.5f, -0.5f, 0.0f, //
      0.5f,  -0.5f, 0.0f, //
      0.0f,  0.5f,  0.0f  //
  };

  // Vertex shader
  GLuint vertexShader{glCreateShader(GL_VERTEX_SHADER)};
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  // Log variables
  int  success{};
  char infoLog[512];

  // Check if vertex shader got compiled
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);

    std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << "\n";
  }

  // Fragment shader
  GLuint fragmentShader{glCreateShader(GL_FRAGMENT_SHADER)};
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);
  // check compilation
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << "\n";
  }

  // Shader program
  GLuint shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  // check linking
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infoLog << "\n";
  }
  // delete shader objects
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // vertex buffer object
  GLuint VBO{};
  GLuint VAO{};

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  // bind vertex array object
  glBindVertexArray(VAO);

  // copy vertices in a buffer
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // set vertex attributes pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Unbind the VAO so other VAO calls won't accidentally modify this VAO.
  glBindVertexArray(0);

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

  // Sample data
  Eigen::ArrayXd y(1000);
  // Gaussian noise between -1.0 and 1.0
  std::generate(y.data(), y.data() + y.size(), []() {
    return 2.0 *
           (static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX) -
            0.5);
  });

  int    filterOrder{2};
  double fs{1000.0};
  double fc{100.0}; // starting cutoff freq

  std::vector<Eigen::ArrayXd> yf(4);
  for (std::size_t i{0}; i < 4; ++i) {
    Nodex::Filter::ZPK digitalFilter{
        Nodex::Filter::iirFilter<Nodex::Filter::buttap, Nodex::Filter::lowpass>(
            filterOrder, fc - 25 * i, fs)};

    Nodex::Filter::EigenCoeffs filter{};
    filter = Nodex::Filter::zpk2tf(Nodex::Filter::EigenZPK(digitalFilter));

    yf[i] = linearFilter(filter, y);
  }

  // Create a node editor with width and height
  NodeEditor* nodeEditor     = new (NodeEditor)();
  const auto  nodeEditorSize = ImVec2(1400, 600);

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // draw the object
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Sample window
    const auto window_size      = io.DisplaySize - ImVec2(1, 1);
    const auto window_pos       = ImVec2(1, 1);
    const auto node_editor_size = window_size - ImVec2(16, 16);
    ImGui::SetNextWindowSize(window_size);
    ImGui::SetNextWindowPos(window_pos);
    {
      ImGui::Begin("Node Editor", nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
      nodeEditor->set_size(node_editor_size);
      nodeEditor->draw();
      ImGui::End();
    }

    {
      // ImGui::Begin("Simple window");
      // ImGui::Text("FPS: %0.3f", ImGui::GetIO().Framerate);
      //
      // if (ImPlot::BeginPlot("Data plot")) {
      //   ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
      //                     ImPlotAxisFlags_NoDecorations |
      //                         ImPlotAxisFlags_AutoFit);
      //   ImPlot::PlotLine("Raw", y.data(), static_cast<int>(y.size()));
      //
      //   for (std::size_t i{0}; i < 4; ++i) {
      //     std::string fString{"Filtered (fc = " + std::to_string(100 - 25 *
      //     i) +
      //                         " Hz)"};
      //     ImPlot::PlotLine(fString.c_str(), yf[i].data(),
      //                      static_cast<int>(yf[i].size()));
      //   }
      //   ImPlot::EndPlot();
      // }
      // ImGui::End();
    }

    // ImGui::ShowStackToolWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap buffers and poll IO events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // optional: deallocate resources once outlived their purpose.
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shaderProgram);

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
