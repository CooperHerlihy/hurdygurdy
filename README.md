# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Build

### Dependencies

#### Linux

- C++23 compiler (GCC or Clang)
- GLSL compiler (glslc)
- CMake (3.16+)
- Vulkan Validation Layers (only required in debug mode)
- SDL3 (found on system, or downloaded automatically)

On Nix, use `nix develop` to create a shell with all dependencies

#### Windows

- C++23 compiler (MSVC)
- CMake (3.16+)
- LunarG Vulkan SDK
- SDL3 (downloaded automatically)

### Compilation

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Release mode:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

On Nix, also use `nix build` to compile a standalone release build

### Integration

Add to CMakeLists.txt:

```cmake
add_subdirectory(path/to/hurdygurdy)
target_link_libraries(your_target hurdygurdy)
```

Or link manually:

```bash
c++ -Ipath/to/hurdygurdy/include -Lpath/to/lib -lhurdygurdy -lSDL3 your_program.cpp
```

