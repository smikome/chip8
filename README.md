# chip8
Chip-8 emulator written in C++

# Prerequisites
- [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)
- [CMake](https://cmake.org/download/) 3.20+
- Visual Studio 2022 (Windows) / Xcode Command Line Tools (macOS)

# Build
```bash
vcpkg install
cmake --preset default
cmake --build build
```

# Run
## Windows
```bash
.\build\Release\chip8.exe
```

## macOS
```bash
./build/chip8
```
