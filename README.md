# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Build

### Dependencies

#### Linux

- C++17 compiler (GCC or Clang)
- CMake (3.16+)
- SDL3 (found on system, or downloaded automatically)
- Vulkan Validation Layers (only required in debug mode)
- GLSL compiler (glslc)

On Nix, use `nix develop` to create a shell with all dependencies

#### Windows

- C++17 compiler (MSVC)
- CMake (3.16+)
- SDL3 (downloaded automatically)
- LunarG Vulkan SDK

### Compilation

```bash
cmake -B build -S .
cmake --build build -j8
```

Release mode:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
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

