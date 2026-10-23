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
- `hg/serialization.hpp` - Serializer, binary format, JSON (JsonValue/JsonObject)
- `hg/time.hpp` - Clock, Perf
- `hg/dynlib.hpp` - Library dynamic loading
- `hg/filesystem.hpp` - FilePath builder and filesystem ops
- `hg/gpu.hpp` - Format, GpuBuffer, GpuImage, GpuPipeline, GpuCmd
- `hg/window.hpp` - Button, Window, input/event types
- `hg/audio.hpp` - AudioStream, Sound, AudioPlayer
- `hg/render2d.hpp` - Camera, Texture, Renderer2D
- `hg/render3d.hpp` - Mesh, Renderer3D
- `hg/imgui.hpp` - ImGui impl for HurdyGurdy
- `hg/ecs.hpp` - Entity component system

`src/` - implementation:
- `internal.hpp` - internal declarations (platform Vulkan exts, gpu/render2d/imgui init hooks)
- `platform_test.cpp` - manual test app: windows, input, audio, ImGui, profiler
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
- `render3d.cpp`
- `imgui.cpp` - ImGui backend (delegates to window/gpu internal init)
- `dynlib.cpp`
- `filesystem.cpp`
- `example2d.cpp` - standalone 2D demo app
- `example3d.cpp` - standalone 3D demo app
- `embed.c` - host build tool: converts a file into a C byte array
- `stb.c` - stb library impl
- `vk_mem_alloc.cpp` - Vulkan Memory Allocator impl
- `pixel_font.h` - embedded pixel font data

`src/vulkan/` - Vulkan implementation:
- `vulkan_internal.hpp` - internal header, Gpu*Data structs, VulkanState, Frame, inline enum converters
- `vulkan.cpp` - internal infrastructure: rglobals VulkanState, init/deinit
- `loader.cpp` - VulkanFuncs struct, dynamic library loading, vk* wrapper thunks
- `gpu.cpp` - public API impl: GpuBuffer/Image/View/Pipeline, cmds, barriers, render/compute passes, frame begin/end

`src/sdl/` - SDL implementation:
- `sdl_internal.hpp` - internal header: window/audio init declarations
- `sdl.cpp` - init/deinit
- `sdl_platform.hpp` - SDL platform abstractions
- `window.cpp` - Window create, processEvents, surface, input/gamepad/clipboard
- `audio.cpp` - SDL audio device/stream open, bridges to user callback

`src/pipewire/` - PipeWire audio implementation:
- `pipewire_platform.hpp` - internal header, audio callback interface
- `pipewire.cpp` - PipeWire audio init/deinit, stream management

`src/win32/` - Windows (Win32) window/audio implementation:
- `win32_platform.hpp` - internal header, mirrors sdl_platform.hpp
- `win32.cpp` - init/deinit, Vulkan extension list
- `window.cpp` - window create, WndProc event pump, Vulkan surface, input/clipboard
- `audio.cpp` - WASAPI shared-mode playback, bridges to user callback

`src/x11/` - X11 window implementation:
- `x11_platform.hpp` - internal header, window/Vulkan extension interface
- `x11.cpp` - X11 window management, input handling

`src/wayland/` - Wayland window implementation:
- `wayland_platform.hpp` - internal header, window/Vulkan extension interface
- `wayland.cpp` - Wayland window management, input handling
- `wayland-protocol.h` - hand-written Wayland protocol types, listener structs, interface externs
- `wayland-protocol.c` - hand-written Wayland protocol interface data tables
- `wayland-version.h` - generated version header (committed, do not regenerate)

Note: the protocol functions in `wayland.cpp` call through the `wlFuncs` function pointer struct
(see top of `wayland.cpp`). The `wl_*_interface` data globals come from `wayland-protocol.c`.

`src/shaders/` - GLSL shaders:
- `render2d.vert`/`render2d.frag` - 2D sprite rendering
- `render3d.vert`/`render3d.frag` - 3D model rendering
- `skybox.vert`/`skybox.frag` - skybox rendering

`src/test/` - tests:
- `tests.hpp` - test framework: TEST/ASSERT registration macros, LifecycleT helper
- `tests.cpp` - runs all tests
- `render.cpp` - TODO stubs, no actual tests yet
- `array_temp.cpp`/`ecs_foreach.cpp`/`ecs_serialize.cpp` - tests without matching headers
- `*.cpp` - remaining tests, named like headers
- `*.vert`/`*.frag`/`*.comp` - test shaders

