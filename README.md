# Nodex

A visual node-based graph editor for digital signal processing.

![Nodex GUI Preview](https://github.com/user-attachments/assets/0784dfbe-ab51-48c7-81fd-736d5df205d6)

## Features

- **Visual node editor**: Intuitive interface for creating signal processing
 graphs
- **Node management**: Create, delete, and configure signal processing nodes
- **Graph persistence**: Save and load node graphs for reproducible workflows
 Chebyshev, etc.)
- **Built-in nodes**: Common signal processing operations (filters, mixers,
 viewers)
- **Python bindings**: Access to core functionality via Python

### Dependencies

The project uses the following external libraries:

- **Eigen**: Linear algebra library
- **ImGui**: GUI framework
- **ImPlot**: Plotting library
- **JSON (nlohmann)**: JSON serialization
- **pybind11**: C++-Python bindings

## Building

```bash
mkdir -p build
cd build
cmake .. cmake --build .
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

```bash
./build/bin/nodex_gui
```

- Right-click for context menu
- Create nodes from the menu
- Drag connections between node ports
- Save/load graphs via File menu

### Python Scripting

Example scripts are located in `examples/`:

```bashpython
python examples/[example_script].py
```

To use the Python bindings:

```python
import pynodex
```

Ensure the `pynodex` module is accessible in your `PYTHONPATH`.

## License

See [LICENSE](LICENSE) for details.
