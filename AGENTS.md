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
debug:
  cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build
  ./build/tests

release:
  cmake -G Ninja -B build/release -DCMAKE_BUILD_TYPE=Release
  cmake --build build/release
  ./build/release/tests

san:
  cmake -G Ninja -B build/san -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cmake --build build/san
  LSAN_OPTIONS=detect_leaks=0 ./build/san/tests

tsan:
  cmake -G Ninja -B build/tsan -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
    -DCMAKE_C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
  cmake --build build/tsan
  TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/tests

valgrind:
  valgrind --leak-check=full ./build/tests
```

Driver library leaks suppressed via LSAN_OPTIONS.
TSan false-positive: lock-order-inversion between SDL3 audio and PipeWire - third-party driver threads, not project code.

### Windows

Open a **Visual Studio 2022 x64 developer shell** first, then cmake

## Conventions

- 4-space indent. Braces: next line for code blocks, same line for types/init.
- `hg` namespace, PascalCase types, camelCase fns/vars, `HG_UPPER_CASE` macros.
- Integer types: `u8`..`u64`, `i8`..`i64`, `f32`, `f64`. No `int`/`size_t`/`std::*`.
- RAII, no failing constructors, use static create(), delete copy ctors, use foo.clone().
- Assert with `HG_ASSERT`. Recoverable errors: `setError()` and `Maybe<T>`. Unrecoverable: `HG_PANIC`.
- Memory: hg:: temp containers > scratch arena > hg:: containers > heapAlloc/heapFree. Never std:: containers
- Concurrency: forPar() > callPar() > std:: primitives.

## Contents

`include/` - public headers:
- `hurdygurdy.hpp` - umbrella, includes all sub-headers
- `hurdygurdy.glsl` - shared GLSL header
- `hg/macros.hpp` - config/detection macros, HG_DEFER, HG_LOG, HG_WARN, HG_PANIC, HG_ASSERT
- `hg/inttypes.hpp` - u8..f64
- `hg/span.hpp`
- `hg/product.hpp`
- `hg/sum.hpp`
- `hg/maybe.hpp`
- `hg/error.hpp` - getError, setError, logError
- `hg/init.hpp`
- `hg/utility.hpp` - size, isPowerOf2, align, endianReverse
- `hg/memory.hpp` - Arena, ArenaScope, heapAlloc/Free, scratch
- `hg/concurrency.hpp` - SpinLock, Fence, forPar, callPar
- `hg/math.hpp` - Vec2/3/4, Mat2/3/4, Complex, Quat
- `hg/geometry2d.hpp` - Circle, Rect, Ray2D, Line2D
- `hg/geometry3d.hpp` - Sphere, Box, Tri, Plane, Ray3D, Line3D
- `hg/noise.hpp` - Rng, noise functions
- `hg/strings.hpp` - StringView, StringBuilder, String
- `hg/binary.hpp` - BinaryView, BinaryBuilder, Binary
- `hg/smart_ptr.hpp` - UniquePtr, makeUnique, SharedPtr, makeShared
- `hg/array.hpp` - Array, ArrayTemp
- `hg/queue.hpp` - Queue, QueueTemp
- `hg/hash.hpp` - hgHash template declaration
- `hg/set.hpp` - Set, SetTemp
- `hg/map.hpp` - Map, MapTemp
- `hg/pool.hpp` - Pool, HandlePool
- `hg/assets.hpp` - AssetT, AssetManagerT, load/reload
- `hg/serialization.hpp` - Serializer, binary format
- `hg/time.hpp` - Clock, Perf
- `hg/dynlib.hpp` - Library dynamic loading
- `hg/gpu.hpp` - Format, GpuBuffer, GpuImage, GpuPipeline, GpuCmd
- `hg/window.hpp` - Button, Window, input/event types
- `hg/audio.hpp` - AudioStream, Sound, AudioPlayer
- `hg/render2d.hpp` - Texture, Mesh, Camera, Sprite2D, Atlas2D, Layer2D
- `hg/imgui.hpp` - ImGui impl for HurdyGurdy
- `hg/ecs.hpp` - Entity component system

`src/` - implementation:
- `internal.hpp` - internal header (platform/gpu/audio init)
- `platform_test.cpp` - platform test harness
- `error.cpp`
- `init.cpp`
- `memory.cpp`
- `concurrency.cpp`
- `math.cpp`
- `geometry2d.cpp`
- `geometry3d.cpp`
- `noise.cpp`
- `strings.cpp`
- `binary.cpp`
- `pool.cpp`
- `assets.cpp`
- `serialization.cpp`
- `time.cpp`
- `audio.cpp`
- `render2d.cpp`
- `imgui.cpp` - ImGui backend (delegates to window/gpu internal init)
- `dynlib.cpp`
- `minimal.cpp` - minimal example app
- `embed.c` - embedded resources
- `stb.c` - stb library impl
- `vk_mem_alloc.cpp` - Vulkan Memory Allocator impl
- `pixel_font.h` - embedded pixel font data

`src/vulkan/` - Vulkan implementation:
- `vulkan_internal.hpp` - internal header, data structs, VulkanFuncs, inline helpers
- `vulkan.cpp` - internal infrastructure: VulkanState, init/deinit, format tables, samplers, descriptors
- `loader.cpp` - dynamic Vulkan library loading, function pointer population
- `gpu.cpp` - public API impl: GpuBuffer/Image/View/Pipeline, cmds, barriers, render/compute passes

`src/sdl/` - SDL implementation:
- `sdl_internal.hpp` - internal header, SdlFuncs struct, extern libsdl/sdlFuncs
- `sdl.cpp` - init/deinit
- `sdl_platform.hpp` - SDL platform abstractions
- `window.cpp` - Window create, processEvents, swapchain, gpuFrameBegin/End
- `audio.cpp` - AudioStream/AudioPlayer impl

`src/pipewire/` - PipeWire audio implementation:
- `pipewire_platform.hpp` - internal header, audio callback interface
- `pipewire.cpp` - PipeWire audio init/deinit, stream management

`src/x11/` - X11 window implementation:
- `x11_platform.hpp` - internal header, window/Vulkan extension interface
- `x11.cpp` - X11 window management, input handling

`src/shaders/` - GLSL shaders:
- `render2d.vert`/`render2d.frag` - 2D rendering
- `sprite.vert`/`sprite.frag` - sprite rendering
- `model.vert`/`model.frag` - 3D model rendering
- `skybox.vert`/`skybox.frag` - skybox rendering
- `debug2d.frag` - 2D debug drawing

`src/test/` - tests:
- `tests.hpp` - internal header
- `tests.cpp` - runs all tests
- `*.cpp` - tests, named like headers
- `*.vert`/`*.frag`/`*.comp` - test shaders

