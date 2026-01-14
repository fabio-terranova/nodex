#define GLFW_INCLUDE_NONE

#include "Core.h"
#include "Filter.h"
#include "ImNodeFlow.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <string>

class DataNode : public ImFlow::BaseNode {
public:
  DataNode() {
    setTitle("Data");
    setStyle(ImFlow::NodeStyle::green());
    ImFlow::BaseNode::addIN<Eigen::VectorXd>(
        "in", Eigen::VectorXd{}, ImFlow::ConnectionFilter::SameType());
    ImFlow::BaseNode::addOUT<Eigen::VectorXd>("out", nullptr)
        ->behaviour([this]() { return data_; });
  }

  void draw() override {
    ImGui::SetNextItemWidth(100.f);
    if (ImPlot::BeginPlot("", ImVec2(250, 150))) {
      ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
                        ImPlotAxisFlags_NoDecorations);
      if (isSource_)
        ImPlot::PlotLine("", data_.data(), static_cast<int>(data_.size()));
      else
        ImPlot::PlotLine(
            "", getInVal<Eigen::VectorXd>("in").data(),
            static_cast<int>(getInVal<Eigen::VectorXd>("in").size()));

      ImPlot::EndPlot();
    }
  }

  void setData(const Eigen::VectorXd& data) {
    isSource_ = true;
    data_     = data;
  };

private:
  bool            isSource_{false};
  Eigen::VectorXd data_{};
};

class ButterNode : public ImFlow::BaseNode {
public:
  ButterNode() {
    setTitle("Lowpass");
    setStyle(ImFlow::NodeStyle::brown());
    ImFlow::BaseNode::addIN<Eigen::VectorXd>(
        "in", Eigen::VectorXd{}, ImFlow::ConnectionFilter::SameType());
    ImFlow::BaseNode::addOUT<Eigen::VectorXd>("out", nullptr)
        ->behaviour([this]() {
          int    filterOrder{4};
          double fs{1000.0};
          double fc{25.0};

          auto filter{Noddy::Filter::zpk2tf(
              Noddy::Filter::iirFilter<Noddy::Filter::buttap,
                                       Noddy::Filter::lowpass>(filterOrder, fc,
                                                               fs))};
          return Noddy::Filter::linearFilter(filter,
                                             getInVal<Eigen::VectorXd>("in"));
        });
  }

  void draw() override { ImGui::SetNextItemWidth(100.f); }

private:
};

/* Node editor that sets up the grid to place nodes */
struct NodeEditor : ImFlow::BaseNode {
  ImFlow::ImNodeFlow mINF;

  NodeEditor(float d, std::size_t r) : BaseNode() {
    mINF.setSize({d, d});
    if (r > 0) {
      auto n1 = mINF.addNode<DataNode>({50, 50});
      auto nf = mINF.addNode<ButterNode>({550, 100});
      auto n2 = mINF.addNode<DataNode>({750, 50});

      n1.get()->setData(Eigen::VectorXd::Random(1000));

      n1->outPin("out")->createLink(nf->inPin("in"));
      nf->outPin("out")->createLink(n2->inPin("in"));
    }
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
  std::cout << "Noddy::Core v" << Noddy::Core::version() << "\n";

  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  float mainScale =
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  GLFWwindow* window{
      glfwCreateWindow(kWinWidth, kWinHeight, "Noddy GUI", nullptr, nullptr)};
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
  style.ScaleAllSizes(mainScale);
  style.FontScaleDpi     = mainScale;
  style.AntiAliasedLines = true;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // Sample data
  Eigen::VectorXd y{Eigen::VectorXd::Random(1000)};
  auto            y_     = y.data();
  int             y_size = y.size();

  int    filterOrder{4};
  double fs{1000.0};
  double fc{100.0};

  std::vector<Eigen::VectorXd> fData{};
  for (int i{0}; i < 4; ++i) {
    Noddy::Filter::ZPK digitalFilter{
        Noddy::Filter::iirFilter<Noddy::Filter::buttap, Noddy::Filter::lowpass>(
            filterOrder, fc - 25 * i, fs)};
    fData.push_back(
        Noddy::Filter::linearFilter(Noddy::Filter::zpk2tf(digitalFilter), y));
  }
  //
  // Create a node editor with width and height
  NodeEditor* nodeEditor     = new (NodeEditor)(1400, 500);
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
    {
      ImGui::Begin("Simple window");
      ImGui::Text("FPS: %0.3f", ImGui::GetIO().Framerate);

      if (ImPlot::BeginPlot("")) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
                          ImPlotAxisFlags_NoDecorations |
                              ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Raw", y_, y_size);

        for (int i{}; i < 4; ++i) {
          std::string fString{"Filtered (fc = " + std::to_string(100 - 25 * i) +
                              " Hz)"};
          ImPlot::PlotLine(fString.c_str(), fData[i].data(), fData[i].size());
        }
        ImPlot::EndPlot();
      }

      ImGui::Begin("Node Editor", nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
      nodeEditor->set_size(nodeEditorSize);
      nodeEditor->draw();
      ImGui::End();

      // ImGui::ShowStackToolWindow();

      ImGui::End();
    }

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}
