# Code Review: Hurdy Gurdy

Review conducted against modern C++20 guidelines, following the project's own conventions (no exceptions, no RTTI, custom integer types, `hg` namespace).

---

## Table of Contents

1. [Build System & Config](#1-build-system--config)
2. [Public API Header (`include/hurdygurdy.hpp`)](#2-public-api-header-includehurdygurdyhpp)
3. [GLSL Header (`include/hurdygurdy.glsl`)](#3-glsl-header-includehurdygurdyslsl)
4. [Core Implementation (`src/hurdygurdy.cpp`)](#4-core-implementation-srchurdygurdycpp)
5. [Platform Layer (`src/platform.cpp`)](#5-platform-layer-srcplatformcpp)
6. [Test File (`src/test.cpp`)](#6-test-file-srctestcpp)
7. [Examples (`src/editor.cpp`, `src/minimal.cpp`)](#7-examples-srceditorcpp-srcminimalcpp)

---

## 2. Public API Header (`include/hurdygurdy.hpp`)

### Medium

| Issue | Location | Detail |
|-------|----------|--------|
| **`UniquePtr` / `SharedPtr` are hand-rolled** | `hpp` (containers section) | These are implemented with raw `heapAlloc`/`heapFree`. The custom allocator is justified for arena-awareness, but `SharedPtr` lacks weak references and thread-safety guarantees that `std::shared_ptr` provides. This is a project choice, but the lack of weak references means cycles can never be broken. |
| **`Pool<T>::free` uses `goto`** | `hpp:10693` | The `goto found` pattern can be trivially replaced with a `break` out of the for-loop into a `if (found) { ... }` block. |
| **`#ifdef`/`#ifndef` nesting in config macros** | `hpp:80-110` | The logic is: `#ifdef HG_DEBUG_MODE` → set flags unless `HG_NO_*` defined; `#ifdef HG_RELEASE_MODE` → unset flags unless `HG_*` defined. This is confusing and leads to double-negatives. A simpler approach: let the user define `HG_LOGGING`, `HG_ASSERTIONS`, `HG_VK_DEBUG_MESSENGER` explicitly, and default them based on `NDEBUG`. |

---

## 3. GLSL Header (`include/hurdygurdy.glsl`)

### Medium

| Issue | Location | Detail |
|-------|----------|--------|
| **`hgPerlin1D` has a likely bug in the mix** | `glsl:523-526` | `hgNoiseVec1D(seed, pos1) * 1.0 - t` — precedence: `*` binds before `-`, so this is `(hgNoiseVec1D(...) * 1.0) - t`, not `hgNoiseVec1D(...) * (1.0 - t)`. Comparing with the 2D version which correctly uses `dot(grad, offset)`, the 1D version should be `mix(hgNoiseVec1D(seed, pos0) * t, hgNoiseVec1D(seed, pos1) * (t - 1.0), hgSmoothQuintic(t))`. |
| **Redundant GLSL overloads** | `glsl:69-96` | GLSL supports function overloading natively, but the `square` function is essentially `x*x` for all numeric types — a single `genType` template would be half the code. |
| **Macros instead of functions for fractal noise** | `glsl:204-263` | The four `hgFractalNoise*DFunctionDef` macros generate near-identical functions. A single GLSL function could handle all dimensions via a `float` parameter (mapping scalars to 1D, `vec2` to 2D, etc.) using GLSL's overloaded operators. |

---

## 4. Core Implementation (`src/hurdygurdy.cpp`)

### Critical

| Issue | Location | Detail |
|-------|----------|--------|
| **`init()` uses `goto` for cleanup** | `cpp:39-65` | The multi-label `goto` cleanup pattern is error-prone (forgot to call `deinitConcurrency()` on the `gpuFailed` path? It's there, but future maintainers may not be so careful). This is a textbook case for RAII with scope guards, which the project already has (`HG_DEFER`). |
| **`heapAlloc` ignores alignment** | `cpp:111-118` | `heapAlloc(u64 size, u64 alignment)` casts `alignment` to `void` and calls `malloc`. `malloc` only guarantees `alignof(std::max_align_t)` alignment (typically 8 or 16 bytes). Any allocation with a higher alignment (e.g., 256-byte alignment for SIMD) will silently be misaligned. Should use `aligned_alloc` or `_aligned_malloc` on MSVC. |
| **`threadPool.~ThreadPoolState()` called explicitly, then placement-new** | `cpp:283-284` | `deinitConcurrency` explicitly calls the destructor on a static object, then placement-news over it. This is valid in C++ only if the object's lifetime is explicitly managed. But `threadPool` is a namespace-scope static — its lifetime is program lifetime. Calling its destructor manually ends its lifetime, then placement-new begins a new one. This works, but the standard pattern for reinitializing a static is to call `reset()` or `clear()`, not destroy-and-rebuild. Additionally, the explicit destructor call does not restart the thread pool's threads (they have `std::jthread` which auto-joins on destruction) — but then the new pool starts with an empty thread array, so there are no worker threads. Any subsequent `callPar` will deadlock. |
| **`INFINITY` used as timeout in `forPar`** | `cpp:414` | `helpThreads(&fence, INFINITY)` — `INFINITY` is a `float` (or macro expanding to `float`), but `helpThreads` takes `f64`. On MSVC without `<cmath>`, `INFINITY` may not be defined. The `Clock::tick()` comparison `timeout -= c.tick()` will work with floating point, but `INFINITY - finite_value` is still `INFINITY`, so the `while` loop in `helpThreads` never exits — correct for "wait indefinitely". But `INFINITY` is typically defined in `<cmath>` or `<cfloat>`, neither of which are included in `hurdygurdy.cpp` directly (they're in the header, but MSVC differs). Safer: use `f64` constant like `1e100` or add a separate `waitIndefinite` method. |

### High

| Issue | Location | Detail |
|-------|----------|--------|
| **`threadPoolExecute` uses `compare_exchange_weak` in a loop with no memory ordering** | `cpp:217` | The default memory order is `std::memory_order_seq_cst`, which is correct but slow. For a thread pool work-stealing queue, `acquire`/`release` ordering would suffice. More critically, the `hasWork` load on line 223 is not synchronized with the store on line 365 (`callPar`). |

### Medium

| Issue | Location | Detail |
|-------|----------|--------|
| **`stringToDouble` / `integerToString` / `floatToString` are locale-dependent** | `cpp:1923-2076` | These manual parsers assume ASCII digits and `.` as decimal separator. On a system with a non-C locale, `std::fprintf` may expect `,` as decimal separator, but these functions always emit `.`. Consider using `std::from_chars` / `std::to_chars` (C++17) which are locale-independent. |

---

## 6. Test File (`src/test.cpp`)

### High

| Issue | Location | Detail |
|-------|----------|--------|
| **`#undef HG_NO_LOGGING` / `#define HG_LOGGING 1` before including the header** | `test.cpp:1-2` | This works, but is fragile — it relies on the header's macro logic (lines 80-110 in the header) which may change. A `cmake`-level test configuration would be more robust. |

---

## 7. Examples (`src/editor.cpp`, `src/minimal.cpp`)

### High

| Issue | Location | Detail |
|-------|----------|--------|
| **`minimal.cpp` uses `goto quit`** | `minimal.cpp:98` | The `for(;;)` loop exits via `goto quit` when the window closes. A structured approach (using a `bool running` flag or `break` from an inner scope) would be more maintainable. |

---

## Cross-Cutting Concerns

| Issue | Detail |
|-------|--------|
| **No `nodiscard` usage** | Many functions return values that must not be ignored (e.g., `Arena::alloc` returning `nullptr` on OOM, `Maybe<T>::expect`, `Fence::wait`). None are marked `[[nodiscard]]`. C++20 allows `[[nodiscard]]` on constructors, which would catch `Maybe` construction without using the result. |
| **No `constexpr` on many eligible functions** | `Vec2::operator+`, `Vec3::operator*`, `lerp`, `smooth`, `clamp` (if it exists), etc., are defined but not `constexpr`. C++20 allows `constexpr` allocation for containers, but the vector math functions could all be `constexpr`. |
| **No `consteval` where appropriate** | `isPowerOf2`, `align`, `endianReverse*` are `constexpr` and would benefit from `consteval` when called with constant arguments. |
| **Inconsistent `noexcept`** | Many functions that cannot throw (e.g., `Vec2::operator+`, `StringView::begin()`) are missing `noexcept`. Others that allocate (e.g., `ArenaScope::alloc`) are marked `noexcept` but can return `nullptr` (arenas can fail). |
| **Project does not use `std::span` or `std::string_view`** | A deliberate choice (custom arena-aware types), but users familiar with C++20 will expect `std::span` semantics. The custom `Span<T>` lacks `subspan`, `first`, `last`, and other standard methods. |

