#ifndef INCLUDE_APPLICATION_H_
#define INCLUDE_APPLICATION_H_

#include "Node.h"

// Forward declarations
struct GLFWwindow;

namespace Nodex::App {

/**
 * Main application class that manages the GUI, rendering, and event loop.
 * Encapsulates all GLFW, OpenGL, and ImGui initialization and cleanup.
 */
class Application {
public:
  Application();
  ~Application();

  /**
   * Initializes the application (GLFW, OpenGL, ImGui).
   * @return true if initialization was successful
   */
  bool initialize();

  void run();
  bool isRunning() const;
  void close();

private:
  GLFWwindow* m_window{nullptr};
  Core::Graph m_graph{};
  bool        m_isRunning{false};

  // Initialization helpers
  bool initializeGlfw();
  bool initializeOpenGL();
  bool initializeImGui();
  void initializeNodeGraph();

  // Cleanup helpers
  void shutdownImGui();
  void shutdownGlfw();

  // Main loop helpers
  void processInput();
  void updateFrame();
  void render();
};

} // namespace Nodex::App

#endif // INCLUDE_APPLICATION_H_
