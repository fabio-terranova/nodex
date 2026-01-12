#define GLFW_INCLUDE_NONE

#include "Core.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);

// settings
constexpr int kWinWidth{800};
constexpr int kWinHeight{600};

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

  GLFWwindow* window{
      glfwCreateWindow(kWinWidth, kWinHeight, "Noddy", nullptr, nullptr)};
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";

    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw the object
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // swap buffers and poll IO events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

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
