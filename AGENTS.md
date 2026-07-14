# Hurdy Gurdy

Modern C++23 game engine. Vulkan 1.3, SDL3, Dear ImGui.
No STL containers, no exceptions, no RTTI.

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
    - Use RAII
    - Constructors must never fail, otherwise use static create function
    - Delete copy constructors, use clone function if needed
- Errors:
    - Assert all preconditions with HG_ASSERT
    - Recoverable: Option<Foo> foo(), then getError()
    - Unrecoverable: HG_PANIC("Error")
- Memory:
    - Scratch memory: getScratch(conflicts), HG_ARENA_SCOPE(scratch), and Temp containers
    - Heap memory: prefer containers, then heapAlloc(), heapRealloc, heapFree()
    - Common containers: Array, Queue, Set, Map, String
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

Implementation files only comment confusing code.
Tests comment extensively as usage documentation.

## Reading the Codebase

- grep before reading big files (hurdygurdy.hpp is large).
- Read `test.cpp` for test patterns, `minimal.cpp` for a simple usage example.

