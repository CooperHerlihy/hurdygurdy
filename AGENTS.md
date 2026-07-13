# Hurdy Gurdy

C++23 game engine. Vulkan 1.3, SDL3, Dear ImGui.
No STL containers, no exceptions, no RTTI.

See [CODING_GUIDELINES.md](CODING_GUIDELINES.md) for all coding conventions (RAII, move semantics, modern C++ style).

## Hard Rules

**I make all decisions. Never act without approval.**

- Ask before structural changes, destructive ops, or anything ambiguous.
- No STL, exceptions, RTTI, third-party code, or reformatting without instruction.
- Use proper git workflow for rollback safety

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

## Quick Conventions

- Types: PascalCase. Functions: camelCase. Macros: `HG_UPPER_SNAKE_CASE`.
- Integer types: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`.
- No `int`, `unsigned`, `size_t`, `std::*` containers, exceptions, RTTI.
- `#include "hurdygurdy.hpp"` first in .cpp files. `using namespace hg;` in executables.

## Reading the Codebase

- grep before reading big files (hurdygurdy.hpp is large).
- Read `test.cpp` for test patterns, `minimal.cpp` for a simple usage example.

## Git Workflow

```
1. git switch main && git pull --rebase
2. git switch -c hg/<short-description>
3. Make atomic commits as you work
4. git switch main && git merge --squash <branch>
5. git branch -D <branch>
```

