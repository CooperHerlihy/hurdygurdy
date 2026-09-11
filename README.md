# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Supported platforms

- **SDL3** — A backup so Hurdy Gurdy can run on platforms without a dedicated backend
- **Linux** — uses X11 and Pipewire, headers are vendored in

## Build

### Dependencies

On Nix, `nix develop` provides all dependencies.

- C++23 compiler — Clang 19+, GCC 15+, or MSVC
- CMake (3.18+)
- Ninja
- glslc (SPIR-V compiler):
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK

#### Preferred (auto-detected, speeds up builds)

- ccache — caches compilation results for instant rebuilds
- mold — faster Linux linker (auto-detected on Linux)
- Vulkan Validation Layers — better Vulkan debugging
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK

### Compilation

```bash
# Debug — unoptimized with debug info
cmake --workflow --preset debug

# Release — optimized
cmake --workflow --preset release
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

