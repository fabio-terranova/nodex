#include "Constants.h"
#include "Core.h"
#include "Gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "nfd.hpp"
#include <Eigen/Dense>
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <iostream>

using Nodex::Filter::Signal;
using namespace Nodex::Gui;

// function declarations
void framebuffer_size_callback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);
bool initGlfw();

int main(void) {
  std::cout << "Nodex::Core v" << Nodex::Core::version() << "\n";

  // init NFD
  NFD::Guard nfdGuard;

  if (!initGlfw()) {
    return -1;
  }

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

  glViewport(0, 0, kWinWidth, kWinHeight);

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
  ImGui_ImplOpenGL3_Init(kGlslVersion);

  // initialize graph
  Nodex::Core::Graph graph;

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(kClearColor[0], kClearColor[1], kClearColor[2],
                 kClearColor[3]);
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

bool initGlfw() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return false;
  }

  return true;
}
