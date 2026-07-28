# Hurdy Gurdy

C++20 game engine. Vulkan 1.3, SDL3, Dear ImGui.

## Hard Rules

**Always inform. Never act without approval.**
- No exceptions, RTTI, third-party code, or reformatting
- Never run scripts without backups and tests
- When asked to write tests, load the write-tests skill first

## Build

### Linux / macOS

```
debug: cmake --workflow --preset debug && ./build/test
san: cmake --workflow --preset san && LSAN_OPTIONS=detect_leaks=0 ./build/san/test
tsan: cmake --workflow --preset tsan && TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/test
valgrind: valgrind --leak-check=full ./build/test
```

Driver library leaks suppressed via LSAN_OPTIONS.
TSan false-positive: lock-order-inversion between SDL3 audio and PipeWire - third-party driver threads, not project code.

### Windows

Open a **Visual Studio 2022 x64 developer shell** first, then:

```
cmake --workflow --preset debug && .\build\test
```

## Contents

Root files:
- `README.md` - build instructions (update when build changes)
- `AGENTS.md` - agent instructions (update when structure changes)
- `flake.nix` - development environment
- `CMakeLists.txt` - build system

`include/` - public headers:
- `hurdygurdy.hpp` - umbrella, includes all sub-headers
- `hurdygurdy.glsl` - shared GLSL header
- `hg_macros.hpp` - config/detection macros, HG_DEFER, HG_LOG, HG_WARN, HG_PANIC, HG_ASSERT
- `hg_types.hpp` - u8..f64, StringView, Span, Product, Sum, Maybe
- `hg_error.hpp` - getError, setError, logError
- `hg_init.hpp` - HurdyGurdy scope guard
- `hg_utility.hpp` - isPowerOf2, align, endianReverse
- `hg_memory.hpp` - Arena, ArenaScope, heapAlloc/Free, scratch
- `hg_concurrency.hpp` - SpinLock, Fence, forPar, callPar
- `hg_math.hpp` - Vec2/3/4, Mat2/3/4, Complex, Quat
- `hg_geometry2d.hpp` - Circle, Rect, Ray2D, Line2D
- `hg_geometry3d.hpp` - Sphere, Box, Tri, Plane, Ray3D, Line3D
- `hg_noise.hpp` - Rng, noise functions
- `hg_strings.hpp` - String, StringBuilder, parsing
- `hg_containers.hpp` - Array, Queue, Set, Map, Pool, HandlePool
- `hg_assets.hpp` - AssetT, AssetManagerT, load/reload
- `hg_serialization.hpp` - Serializer, binary format
- `hg_timing.hpp` - Clock, Perf
- `hg_dynlib.hpp` - Library dynamic loading
- `hg_gpu.hpp` - Format, GpuBuffer, GpuImage, GpuPipeline, GpuCmd
- `hg_window.hpp` - Button, Window, input/event types
- `hg_audio.hpp` - AudioStream, Sound, AudioPlayer
- `hg_render2d.hpp` - Texture, Mesh, Camera, Sprite2D, Atlas2D, Layer2D
- `hg_imgui.hpp` - ImGui impl for HurdyGurdy
- `hg_ecs.hpp` - ECS (commented out)

`src/` - implementation:
- `internal.hpp` - internal declarations (platform/gpu/audio init)
- `error.cpp` - getError, setError, logError
- `init.cpp` - init(), HurdyGurdy ctor/dtor
- `memory.cpp` - heapAlloc/Free, Arena, scratch
- `concurrency.cpp` - SpinLock, Fence, thread pool, forPar
- `math.cpp` - Vec/Mat/Complex/Quat operations, camera matrices
- `geometry_2d.cpp` - Circle/Rect/Ray2D/Line2D intersection
- `geometry_3d.cpp` - Sphere/Box/Tri/Plane/Ray3D/Line3D intersection
- `noise.cpp` - noise functions, Rng, trueRandom
- `strings.cpp` - StringBuilder, String, parsing
- `containers.cpp` - BinaryView, BinaryBuilder, Binary, HandlePool
- `serialization.cpp` - Serializer, binary serial
- `timing.cpp` - Clock, Perf
- `audio.cpp` - Sound asset, AudioPlayer
- `asset_io.cpp` - Texture/Mesh/Binary file I/O
- `camera.cpp` - Camera create/update
- `render2d.cpp` - Renderer 2D, Atlas, Tilemap, Layer
- `platform.cpp` - SDL init/deinit, platformGetVulkanExtensions
- `dynlib.cpp` - Library::load/findFunction
- `vk_mem_alloc.cpp` - VMA vendor source
- `test.cpp` - tests (monolithic)
- `editor.cpp` - example editor
- `minimal.cpp` - example minimal

`src/vulkan/` - Vulkan implementation:
- `backend.hpp` - shared internal header, data structs, VulkanFuncs, inline helpers
- `vulkan.cpp` - internal infrastructure: VulkanState, init/deinit, format tables, samplers, descriptors
- `loader.cpp` - dynamic Vulkan library loading, function pointer population, vulkanFuncs/libvulkan storage
- `gpu.cpp` - public API impl: GpuBuffer/Image/View/Pipeline, cmds, barriers, render/compute passes

`src/sdl/` - SDL implementation:
- `window.cpp` - Window create, processEvents, ImGui integration, swapchain, gpuFrameBegin/End
- `audio_stream.cpp` - AudioStream primitive

## Conventions

- 4-space indent. Braces: next line for code blocks, same line for types/init.
- `hg` namespace, PascalCase types, camelCase fns/vars, `HG_UPPER_CASE` macros.
- Integer types: `u8`..`u64`, `i8`..`i64`, `f32`, `f64`. No `int`/`size_t`/`std::*`.
- RAII, no failing constructors, delete copy ctors, use foo.clone().
- Assert with `HG_ASSERT`. Recoverable errors: `setError()` and `Option<T>`. Unrecoverable: `HG_PANIC`.
- Memory: scratch arena > hg:: containers > heapAlloc/heapFree. Never std:: containers
- Concurrency: forPar() > callPar() > std::*.

