# Noddy

Node-based DSP application written in C++.

<img width="1500" height="1146" alt="image" src="https://github.com/user-attachments/assets/9abd5ad5-e2f2-4a72-a957-968c4bfd9334" />

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
