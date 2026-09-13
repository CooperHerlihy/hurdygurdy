# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

Supports Linux natively, and any platform supported by SDL3.

## Build

### Dependencies

On Nix, `nix develop` provides all dependencies.

- C++20 compiler (Clang 19+, GCC 15+, or MSVC)
- CMake (3.18+)
- SPIR-V compiler (glslc)
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK
- Windows only: Windows SDK (installed with Visual Studio)

### Compilation

```bash
# Debug:
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Nix, also use `nix build` to compile a standalone release build.

### Integration

Add to CMakeLists.txt:

```cmake
add_subdirectory(path/to/hurdygurdy)
target_link_libraries(your_target hurdygurdy)
```

Or link manually:

```bash
c++ -Ipath/to/hurdygurdy/include -Lpath/to/lib -lhurdygurdy your_program.cpp
```

