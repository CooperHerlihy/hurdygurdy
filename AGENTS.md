# Hurdy Gurdy

C++20 game development library

## Hard Rules

**Always inform. Never act without approval.**
- No exceptions, RTTI, third-party code, or reformatting
- Never run scripts without backups and tests

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

### Windows

Open a **Visual Studio 2022 x64 developer shell** first, then cmake

## Conventions

- 4-space indent
- Braces: next line for code blocks, same line for struct/initialization
- `hg` namespace, PascalCase types, camelCase fns/vars, `HG_UPPER_CASE` macros
- Integer types: `u8`..`u64`, `i8`..`i64`, `f32`, `f64`. No `int`/`size_t`/`std::*`
- RAII, no failing constructors, use static create() with Maybe, generally delete copy ctors
- Assertions: `HG_ASSERT`, Recoverable errors: `setError()` and `Maybe<T>`, Unrecoverable: `HG_PANIC`
- Memory: hg temp containers > scratch arena > hg containers > heapAlloc/heapFree; Never std containers
- Concurrency: forPar() > callPar() > std:: primitives

## Contents

`include/` - public headers:
- `hurdygurdy.hpp`
- `hurdygurdy.glsl`
- `hg/macros.hpp` - HG_DEFER, HG_LOG, HG_WARN
- `hg/inttypes.hpp`
- `hg/span.hpp`
- `hg/product.hpp`
- `hg/sum.hpp`
- `hg/maybe.hpp`
- `hg/error.hpp` - setError
- `hg/init.hpp`
- `hg/utility.hpp`
- `hg/memory.hpp` - Arena, ArenaScope
- `hg/concurrency.hpp`
- `hg/math.hpp`
- `hg/geometry2d.hpp`
- `hg/geometry3d.hpp`
- `hg/noise.hpp`
- `hg/strings.hpp`
- `hg/binary.hpp`
- `hg/smart_ptr.hpp`
- `hg/array.hpp`
- `hg/queue.hpp`
- `hg/hash.hpp`
- `hg/set.hpp`
- `hg/map.hpp`
- `hg/pool.hpp`
- `hg/assets.hpp`
- `hg/serialization.hpp`
- `hg/json.hpp`
- `hg/time.hpp`
- `hg/dynlib.hpp`
- `hg/filesystem.hpp`
- `hg/gpu.hpp`
- `hg/window.hpp`
- `hg/audio.hpp`
- `hg/render2d.hpp`
- `hg/render3d.hpp`
- `hg/imgui.hpp` - ImGui impl for HurdyGurdy
- `hg/ecs.hpp`

`src/` - implementation:
- `stb.c` - stb library impl
- `vk_mem_alloc.cpp` - Vulkan Memory Allocator impl
- `embed.c` - build tool: converts a file into a C byte array
- `internal.hpp` - internal declarations
- `platform_test.cpp` - manual test app: windows, input, audio, ImGui, profiler
- `example2d.cpp` - standalone 2D demo app
- `example3d.cpp` - standalone 3D demo app
- `*.cpp` - remaining implementations, named like headers

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
- `*.cpp` - remaining tests, named like headers
- `shaders/` - `.vert`/`.frag`/`.comp` test shaders

