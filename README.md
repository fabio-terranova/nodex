# Noddy

WORK IN PROGRESS...

Node-based DSP application written in C++.

<img width="1436" height="1020" alt="image" src="https://github.com/user-attachments/assets/faf4414b-4269-4f29-a556-506c0539dcfd" />

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
