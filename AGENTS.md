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
include/hurdygurdy.hpp     — umbrella, includes all sub-headers
include/hurdygurdy.glsl    — shared GLSL header
include/hurdygurdy/
  config.hpp               — platform/compiler detection macros
  core.hpp                 — u8..u64, StringView, Span, Product, Sum, Maybe
  error.hpp                — getError, setError, logError
  macros.hpp               — Defer, HG_DEFER, HG_LOG, HG_PANIC, HG_ASSERT
  init.hpp                 — HurdyGurdy scope guard
  utility.hpp              — isPowerOf2, align, endianReverse
  memory.hpp               — Arena, ArenaScope, heapAlloc/Free, scratch
  concurrency.hpp          — SpinLock, Fence, forPar, callPar
  math.hpp                 — Vec2/3/4, Mat2/3/4, Complex, Quat
  geometry_2d.hpp          — Circle, Rect, Ray2D, Line2D
  geometry_3d.hpp          — Sphere, Box, Tri, Plane, Ray3D, Line3D
  noise.hpp                — Rng, noise functions
  strings.hpp              — String, StringBuilder, parsing
  containers.hpp           — Array, Queue, Set, Map, Pool, HandlePool
  asset.hpp                — Asset<T>, AssetManager<T>, load/reload
  serialization.hpp        — Serializer, binary format
  timing.hpp               — Clock, Perf
  dynlib.hpp               — Library dynamic loading
  gpu.hpp                  — Format, GpuBuffer, GpuImage, GpuPipeline, GpuCmd
  window.hpp               — Button, Window, input/event types
  audio.hpp                — AudioStream, Sound, AudioPlayer
  rendering.hpp            — Texture, Mesh, Camera, Sprite2D, Atlas2D, Layer2D
  ecs.hpp                  — ECS (commented out)
  templates.hpp            — out-of-line template method bodies
src/internal.hpp           — internal declarations (VulkanFuncs, platform/gpu/audio init)
src/core.cpp               — error handling, init, heap alloc, Arena, scratch
src/concurrency.cpp        — SpinLock, Fence, thread pool, forPar
src/math.cpp               — math ops, 2D/3D geometry, noise
src/strings.cpp            — StringBuilder, String, parsing
src/containers.cpp         — BinaryBuilder, Binary, HandlePool
src/serialization.cpp      — Serializer, binary serial
src/timing.cpp             — Clock, Perf
src/audio.cpp              — Sound asset loading, AudioPlayer
src/asset_io.cpp           — Texture/Mesh/Binary file I/O
src/camera.cpp             — Camera create/update
src/render2d.cpp           — Renderer 2D, Atlas, Tilemap, Layer
src/platform.cpp           — Vulkan/SDL/platform init, GPU ops, windowing, ImGui
src/vulkan_stubs.cpp       — C-linkage Vulkan PFN stubs
src/dynlib.cpp             — Library::load/findFunction
src/hurdygurdy.cpp         — JSON parser (commented out), ECS (commented out)
src/test.cpp               — tests (monolithic)
src/editor.cpp             — example editor
src/minimal.cpp            — example minimal
```

## Conventions

- 4-space indent. Braces: next line for code blocks, same line for types/init.
- `hg` namespace, PascalCase types, camelCase fns/vars, `HG_UPPER_CASE` macros.
- Integer types: `u8`..`u64`, `i8`..`i64`, `f32`, `f64`. No `int`/`size_t`/`std::*`.
- RAII, no failing constructors, delete copy ctors, use foo.clone().
- Assert with `HG_ASSERT`. Recoverable errors: `setError()` and `Option<T>`. Unrecoverable: `HG_PANIC`.
- Memory: scratch arena > hg:: containers > heapAlloc/heapFree. Never std:: containers
- Concurrency: forPar() > callPar() > std::*.

