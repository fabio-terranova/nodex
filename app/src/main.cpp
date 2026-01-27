#include "Core.h"
#include "GuiNodes.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <Eigen/Dense>
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <iostream>

using Nodex::Filter::Signal;

void framebuffer_size_callback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);

// settings
constexpr int kWinWidth{1600};
constexpr int kWinHeight{1200};

using namespace Nodex::App;

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

  // initialize graph
  Graph graph;

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Node editor
    graphWindow(graph);

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
