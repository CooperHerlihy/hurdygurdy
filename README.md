# Hurdy Gurdy

## Description

Hurdy Gurdy is a game engine written in C++ for fun.

## Build

### Dependencies

#### Required

- C++23 compiler — GCC 15+, Clang 19+, or MSVC
- CMake (3.18+)
- SDL3 — found on system, or downloaded automatically by CMake
- glslc (SPIR-V compiler) + Vulkan Validation Layers (debug only):
  - Windows: included in LunarG Vulkan SDK
  - Linux: install glslc via `shaderc` package, validation layers via `vulkan-validation-layers` (or `nix develop`)

#### Preferred (auto-detected, speeds up builds)

- Ninja — faster build system generator (vs Make)
- ccache — caches compilation results for instant rebuilds
- mold — faster Linux linker (auto-detected on Linux)

On Nix, `nix develop` provides all of the above.

### Compilation

```bash
# Fastest — no debug flags or sanitizers
cmake --workflow --preset default

# Debug — full debug info + UBSan
cmake --workflow --preset debug

# Release — optimized
cmake --workflow --preset release
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

