# Code Review: Hurdy Gurdy

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

## 6. Test File (`src/test.cpp`)

### High

| Issue | Location | Detail |
|-------|----------|--------|
| **`#undef HG_NO_LOGGING` / `#define HG_LOGGING 1` before including the header** | `test.cpp:1-2` | This works, but is fragile — it relies on the header's macro logic (lines 80-110 in the header) which may change. A `cmake`-level test configuration would be more robust. |

---

## Cross-Cutting Concerns

| Issue | Detail |
|-------|--------|
| **No `constexpr` on many eligible functions** | `Vec2::operator+`, `Vec3::operator*`, `lerp`, `smooth`, `clamp` (if it exists), etc., are defined but not `constexpr`. C++20 allows `constexpr` allocation for containers, but the vector math functions could all be `constexpr`. |

