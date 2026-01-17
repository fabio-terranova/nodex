# Nodex

WORK IN PROGRESS...

<img width="1458" height="1030" alt="image" src="https://github.com/user-attachments/assets/127a147c-de88-4d09-b501-47e0aed7e80a" />

## Building

This project uses CMake. To build:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Project Structure

```
├── app/          # Main application code
├── core/         # Core library
├── bindings/     # Language bindings (python with pybind11 for now)
├── external/     # Third-party dependencies
├── examples/     # Example python scripts
└── tests/        # Test files
```

## Usage

Run the application from the build directory:

```bash
./bin/nodex_gui
```

## Examples

Example python scripts are located in the `examples/` directory. To run an example:

```bashpython
python examples/example_script.py
```

Ensure you have the necessary Python dependencies installed and the `nodex_py` module is accessible in your `PYTHONPATH`.

## License

See [LICENSE](LICENSE) for details.
