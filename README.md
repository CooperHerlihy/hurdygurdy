# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Supported platforms

- **Linux** — uses X11 and Pipewire, headers are vendored in
- **SDL3** — A backup so Hurdy Gurdy can run on platforms without a dedicated backend

## Build

### Dependencies

On Nix, `nix develop` provides all dependencies.

- C++23 compiler (Clang 19+, GCC 15+, or MSVC)
- CMake (3.18+)
- SPIR-V compiler (glslc)
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK
- Windows only: Windows SDK (installed with Visual Studio)

#### Preferred (auto-detected, not strictly necessary)

- Ninja = faster build system (and generates compile commands on windows)
- ccache — caches compilation
- mold — faster Linux linker
- Vulkan Validation Layers — for Vulkan debugging
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK

### Compilation

```bash
# Debug — unoptimized with debug info
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release — optimized
cmake -B build -DCMAKE_BUILD_TYPE=Debug
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

