#include "Application.h"
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

namespace Nodex::App {

using namespace Nodex::Constants;

namespace {
// Static callback for GLFW
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
  glViewport(0, 0, width, height);
}
} // anonymous namespace

Application::Application() = default;
Application::~Application() { shutdownImGui(); }

bool Application::initialize() {
  if (!initializeGlfw()) {
    return false;
  }

  if (!initializeOpenGL()) {
    shutdownGlfw();
    return false;
  }

  if (!initializeImGui()) {
    shutdownGlfw();
    return false;
  }

  initializeNodeGraph();
  m_isRunning = true;

  return true;
}

bool Application::initializeGlfw() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return false;
  }

  m_window = glfwCreateWindow(Constants::kWinWidth, Constants::kWinHeight,
                              "Nodex GUI", nullptr, nullptr);

  if (!m_window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(m_window);
  glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
  glfwSwapInterval(1);

  return true;
}

bool Application::initializeOpenGL() {
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize glad\n";
    return false;
  }

  glViewport(0, 0, Constants::kWinWidth, Constants::kWinHeight);
  return true;
}

bool Application::initializeImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();

  ImGuiIO& io{ImGui::GetIO()};
  (void)io;
  ImGui::StyleColorsDark();

  ImGuiStyle& style{ImGui::GetStyle()};
  style.AntiAliasedLines = true;

  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init(Constants::kGlslVersion);

  return true;
}

void Application::initializeNodeGraph() {
  // Graph is initialized by default constructor
}

void Application::shutdownImGui() {
  if (m_window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
  }
}

void Application::shutdownGlfw() {
  if (m_window) {
    glfwTerminate();
    m_window = nullptr;
  }
}

void Application::run() {
  NFD::Guard nfdGuard;

  if (!initialize()) {
    std::cerr << "Failed to initialize application\n";
    return;
  }

  // Main loop
  while (!glfwWindowShouldClose(m_window) && m_isRunning) {
    processInput();
    updateFrame();
    render();

    glfwSwapBuffers(m_window);
    glfwPollEvents();
  }

  shutdownGlfw();
}

void Application::processInput() {
  if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) {
    std::cout << "Closing window\n";
    close();
  }
}

void Application::updateFrame() {
  using namespace Nodex::Constants;

  glClearColor(kClearColor[0], kClearColor[1], kClearColor[2], kClearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void Application::render() {
  // Render node editor
  Nodex::Gui::graphWindow(m_graph);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Application::isRunning() const { return m_isRunning; }

void Application::close() { glfwSetWindowShouldClose(m_window, true); }

} // namespace Nodex::App
