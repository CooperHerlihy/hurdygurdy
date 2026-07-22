# Hurdy Gurdy

C++20 game engine. Vulkan 1.3, SDL3, Dear ImGui.

## Hard Rules

**Always inform. Never act without approval.**
- No exceptions, RTTI, third-party code, or reformatting
- Never run scripts without backups and tests
- When asked to write tests, load the write-tests skill first

## Build

### Linux / macOS

On NixOS, build and test are aliased as: `debug`, `release`, `san`, and `tsan`

```
cmake --workflow --preset debug
./build/test
san: cmake --workflow --preset san && LSAN_OPTIONS=detect_leaks=0 ./build/san/test
tsan: cmake --workflow --preset tsan && TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/test
valgrind: valgrind --leak-check=full ./build/test
```

Driver library leaks suppressed via LSAN_OPTIONS.
TSan false-positive: lock-order-inversion between SDL3 audio and PipeWire — third-party driver threads, not project code.

### Windows

Open a **Visual Studio 2022 x64 developer shell** first, then:

```
cmake --workflow --preset debug
./build/test
```

## Source Map

Read CONTENTS.md and use grep

```
include/hurdygurdy.hpp   — public API + templates
include/hurdygurdy.glsl  — shared GLSL header
src/internal.hpp         — internal defs
src/hurdygurdy.cpp       — core impl
src/platform.cpp         — Vulkan/SDL/platform
src/test.cpp             — tests (monolithic)
src/editor.cpp           — example editor
src/minimal.cpp          — example minimal
```

## Conventions

- 4-space indent. Braces: next line for code blocks, same line for types/init.
- `hg` namespace, PascalCase types, camelCase fns/vars, `HG_UPPER_CASE` macros.
- Integer types: `u8`..`u64`, `i8`..`i64`, `f32`, `f64`. No `int`/`size_t`/`std::*`.
- RAII, no failing constructors, delete copy ctors, use foo.clone().
- Assert with `HG_ASSERT`. Recoverable errors: `setError()` and `Option<T>`. Unrecoverable: `HG_PANIC`.
- Memory: scratch arena > hg:: containers > heapAlloc/heapFree. Never std:: containers
- Concurrency: forPar() > callPar() > std::*.

