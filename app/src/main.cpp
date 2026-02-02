#include "Application.h"
#include <iostream>

int main() {
  try {
    Nodex::App::Application app;
    app.run();

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Application error: " << e.what() << "\n";

    return 1;
  }
}
