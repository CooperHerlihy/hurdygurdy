# Hurdy Gurdy

Modern C++23 game engine. Vulkan 1.3, SDL3, Dear ImGui.
No STL containers, no exceptions, no RTTI.

## Hard Rules

**I make all decisions. Never act without approval.**

- Ask before structural changes, destructive ops, or anything ambiguous.
- No STL, exceptions, RTTI, third-party code, or reformatting without instruction.
- Never run scripts without backups and tests

## Build & Verify

```
build: cmake --build build -j$(nproc)
test:  ./build/test
```

Fix all warnings (`-Werror` is on). Run both after every change.

## Structure

```
include/hurdygurdy.hpp    — public API + templates (all declarations)
include/hurdygurdy.glsl   — shared GLSL header
src/hurdygurdy.cpp        — core engine impl
src/platform.cpp          — Vulkan/SDL/platform impl
src/test.cpp              — all tests (monolithic)
src/editor.cpp            — example editor app
src/minimal.cpp           — example minimal app
build/                    — CMake build output
```

## Sections

hurdygurdy.hpp is large, grep for these sections

- Configuration Macros
- Core Types (forward declarations)
- Error Handling
- Initialization
- Utility Macros
- Diagnostics
- Core Types
- Utility Functions
- Memory
- Concurrency
- GPU
    - GPU Init
    - Pixel Formats
    - Pipeline Stages & Access
    - Buffers
    - Images & Views
    - Pipelines
    - Command Buffer & Draw
    - Barriers
    - Compute Pass
    - Render Pass
- Math
    - Constants & Helpers
    - Vector Types
    - Matrix Types
    - Complex & Quaternion Types
    - Comparison Operators
    - Vector Functions
    - Matrix Functions
    - Complex Functions
    - Quaternion Functions
    - Transform Matrices
    - Geometry 2D
    - Geometry 3D
    - Noise & RNG
- Binary Builder
- String Utilities
- Serialization
- Containers
- Asset System
- Timing
- Dynamic Library
- Windowing
- Input
- Audio
- Camera
- Texture Assets
- 2D Renderer
- 3D Mesh
- ImGui Integration
- ECS

## Conventions

- 4-space indent, no tabs.
- Braces:
    - Code blocks (`if`, `for`, `while`, function bodies): brace on **next line**.
    - Type definitions (`struct`, `class`, `enum`): brace on **same line**.
    - Initialization (`{}`): brace on **same line**.
- Names:
    - namespace hg
    - Types: PascalCase
    - Functions/Variables: camelCase
    - Macros: `HG_UPPER_SNAKE_CASE`.
- Types:
    - Integer types: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`.
    - No `int`, `unsigned`, `size_t`, `std::*` containers, exceptions, RTTI.
- Resources:
    - RAII
    - Constructors must never fail, otherwise use static create function
    - Delete copy constructors, use clone function if needed
- Errors:
    - Assert all preconditions with HG_ASSERT
    - Recoverable: Option<Foo> foo(), then getError()
    - Unrecoverable: HG_PANIC("Error")
- Memory: prefer scratch arena, then containers, then heapAlloc and heapFree
- Concurrency: prefer forPar(), then callPar(), then std::* primitives

## Writing Comments

Header doc comments on every function, type, variable, etc:

```cpp
/**
 * Short punchy description
 *
 * Parameters
 * - param1 The parameter description
 *
 * Returns
 * - What the function returns
 */
 ```

Tests comment extensively
Cpp files only comment confusing code

