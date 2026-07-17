# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Build

### Dependencies

On Nix, `nix develop` provides all dependencies.

- C++23 compiler — Clang 19+ (preferred), GCC 15+, or MSVC
- CMake (3.18+)
- Ninja — build system generator
- SDL3 — found on system, or downloaded automatically by CMake
- glslc (SPIR-V compiler) + Vulkan Validation Layers (debug only):
  - Windows: included in LunarG Vulkan SDK
  - Linux: install glslc via `shaderc` package, validation layers via `vulkan-validation-layers` (or `nix develop`)

#### Preferred (auto-detected, speeds up builds)

- ccache — caches compilation results for instant rebuilds
- mold — faster Linux linker (auto-detected on Linux)

### Compilation

```bash
# Debug — fast build with debug info, prefers Clang
cmake --workflow --preset debug

# Release — optimized, prefers Clang
cmake --workflow --preset release

# Release — explicit compiler for perf comparisons
cmake --workflow --preset release-clang   # Clang only
cmake --workflow --preset release-gcc     # GCC only (Linux)
cmake --workflow --preset release-msvc    # MSVC only (Windows)
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
c++ -Ipath/to/hurdygurdy/include -Lpath/to/lib -lhurdygurdy -lSDL3 your_program.cpp
```

