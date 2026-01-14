# Noddy

<img width="1934" height="482" alt="Noddy Application Screenshot" src="https://github.com/user-attachments/assets/7c42ef72-ff72-4803-84f8-805c2b6efcbf" />

## Building

This project uses CMake. To build:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Dependencies

All dependencies are included as submodules in the `external/` directory:

- **ImGui** - Immediate mode GUI library
- **ImPlot** - Plotting extension for ImGui
- **GLFW** - Window and input handling
- **Eigen** - Linear algebra library
- **glad** - OpenGL loader

## Project Structure

```
├── app/          # Main application code
├── core/         # Core library
├── external/     # Third-party dependencies
└── tests/        # Test files
```

## Usage

Run the application from the build directory:

```bash
./bin/noddy_gui
```

## License

See [LICENSE](LICENSE) for details.
