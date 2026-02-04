# Nodex

A visual node-based graph editor for digital signal processing (DSP).

![Nodex GUI Preview](https://github.com/user-attachments/assets/67cc4b52-fd08-4a48-bbcb-f8c42f8633ff)

## Features

- **Visual node editor**: Intuitive drag-and-drop interface for creating signal processing graphs
- **Node management**: Create, delete, and configure signal processing nodes with context menus
- **Graph persistence**: Save and load node graphs in JSON format for reproducible workflows
- **Built-in nodes**: Common signal processing operations including:
  - IIR/FIR digital filters
  - Signal generators
  - Mixer
  - Visualization nodes
- **Python bindings**: Access to core DSP functionality via Python

## Requirements

- **CMake** 3.16 or higher
- **C++ compiler** with C++20 support (GCC, Clang, or MSVC)
- **OpenGL** 
- **OpenMP** (for parallel processing)
- **Python 3.x** (for Python bindings)

### Dependencies

The project uses the following third-party libraries (included as submodules):

- **[Eigen](https://libeigen.gitlab.io/)**: Linear algebra library
- **[ImGui](https://github.com/ocornut/imgui)**: Immediate mode GUI framework
- **[ImPlot](https://github.com/epezent/implot)**: Real-time plotting library for ImGui
- **[GLFW](https://www.glfw.org/)**: OpenGL window and input management
- **[nlohmann/json](https://github.com/nlohmann/json)**: Modern C++ JSON library
- **[pybind11](https://github.com/pybind/pybind11)**: C++/Python interoperability
- **[Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended)**: Cross-platform file dialog library

All dependencies are managed via CMake and included in the `external/` directory.

## Building

### Quick Start

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/fabio-terranova/nodex.git
cd nodex

# Create build directory and configure
mkdir -p build
cd build
cmake ..
```

### Building

To build the entire project:

```bash
cmake --build .
```

To only build the GUI application:

```bash
cmake --build . --target nodex_gui
```

To only build the Python bindings:

```bash
cmake --build . --target pynodex
```

## Project Structure

```
├── app/                  # Main GUI application
│   ├── include/          # Application headers
│   └── src/              # Application implementation
├── core/                 # Core signal processing library
│   ├── include/          # Core headers
│   └── src/              # Core implementation
├── bindings/             # Language bindings
│   └── python/           # Python bindings with pybind11
├── external/             # Third-party dependencies
├── examples/             # Example scripts and usage demos
└── tests/                # Tests
```

## Usage

### Running the GUI Application

After building, run the graphical interface:

```bash
./build/bin/nodex_gui
```

#### Basic Controls

- **Right-click** on canvas to open the context menu
- **Create nodes** by selecting from the context menu
- **Connect nodes** by dragging from an input/output port to an output/input port
- **Save/Load graphs** via the File menu

### Python Bindings

After building, the Python module is located in `build/lib/`. To use it, ensure that this directory is in your `PYTHONPATH`.

#### Example Usage

See `examples/` directory for more complete examples:
- `test_lfilter.py` - Basic IIR filtering example
- `test_lfilter_multi.py` - Multi-channel filtering example

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for full details.

## Contributing

Contributions are welcome! Please open issues or pull requests on GitHub.