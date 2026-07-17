# Hurdy Gurdy

Modern C++23 game engine. Vulkan 1.3, SDL3, Dear ImGui.
No STL containers, no exceptions, no RTTI.

## Hard Rules

**Always inform me. Never act without express approval.**

- All new information must go through me before making decisions.
- No STL, exceptions, RTTI, third-party code, or reformatting without instruction.
- Never run scripts without backups and tests

## Build & Verify

```
build: cmake --workflow --preset default
test:  ./build/test
```

Fix all warnings (`-Werror` is on). Run both after every change.

## Navigation

```
include/hurdygurdy.hpp      — public API + templates (all declarations)
include/hurdygurdy.glsl     — shared GLSL header
src/internal.hpp            — internal definitions
src/hurdygurdy.cpp          — core engine impl
src/platform.cpp            — Vulkan/SDL/platform impl
src/test.cpp                — all tests (monolithic)
src/editor.cpp              — example editor app
src/minimal.cpp             — example minimal app
```

hurdygurdy.hpp is very large, read CONTENTS.md then grep the code

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
    - Constructors must never fail, otherwise use create function
    - Delete copy constructors, use clone function if needed
- Errors:
    - Assert all preconditions with HG_ASSERT
    - Recoverable: Option<Foo> foo(), then getError()
    - Unrecoverable: HG_PANIC("Error")
- Memory: prefer scratch arena, then containers, then heapAlloc and heapFree
- Concurrency: prefer forPar(), then callPar(), then std::* primitives

### Writing Comments

Header doc comments on every function, type, variable, etc:

```cpp
/**
 * Short punchy description
 *
 * Parameters
 * - param1 What the parameter means
 *
 * Returns
 * - What the function returns
 */
 ```

Tests comment extensively
Cpp files only comment confusing code

