# Hurdy Gurdy

Modern C++23 game engine. Vulkan 1.3, SDL3, Dear ImGui.
No STL containers, no exceptions, no RTTI.

## Hard Rules

**Always inform me. Never act without express approval.**

- All new information must go through me before making decisions.
- No STL, exceptions, RTTI, third-party code, or reformatting without instruction.
- Never run scripts without backups and tests

## Build & Verify

### Linux / macOS

```
build:    cmake --workflow --preset debug
test:     ./build/test
san:      cmake --workflow --preset san && LSAN_OPTIONS=detect_leaks=0 ./build/san/test
tsan:     cmake --workflow --preset tsan && TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/test
valgrind: valgrind --leak-check=full ./build/test
```

LSAN_OPTIONS=detect_leaks=0 suppresses known driver library leaks (Vulkan/wayland).
TSan shows a false-positive lock-order-inversion between SDL3 audio (`SDLAudioP*`) and PipeWire (`libspa`) — these are third-party driver threads, not project code.

### Windows (x64)

Open a **Visual Studio 2022 x64 developer shell** first:

```
"%VS2022INSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"
```

Or from PowerShell / cmd:
```
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

Then:

```
build:   cmake --workflow --preset debug
test:    ./build/test
```

The VS dev shell provides `cmake`, `ninja`, and the MSVC compiler toolchain.

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

