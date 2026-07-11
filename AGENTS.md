# AGENTS.md -- Hurdy Gurdy Game Engine

## Identity

This is **Hurdy Gurdy**, a game engine written in C++17 by Cooper Herlihy.
MIT license, for fun, no big corporate nonsense.

## Project Structure

```
hurdygurdy/
  include/           -- All public headers (.hpp + .glsl)
  src/               -- Source files + shaders (.vert/.frag/.comp)
  vendor/            -- Third party (stb_image, stb_image_write, vk_mem_alloc, imgui/)
  build/             -- CMake build output (generated, ignored)
  CMakeLists.txt     -- CMake 3.16+, C++17
  flake.nix          -- Nix flake for dev shell + build
```

## Build (MUST USE nix develop)

**IMPORTANT: All build & run commands MUST be run inside `nix develop` or they will crash (Vulkan libraries unavailable outside the shell).**

```
nix develop
cmake -B build -S .
cmake --build build -j$(nproc)
```

Targets: `editor`, `noise`, `minimal` (executables), `hurdygurdy` (static lib).
Also available: `nix build` for release build.

## Dependencies

- C++17 compiler (GCC/Clang/MSVC), CMake 3.16+
- SDL3 (auto-downloaded if missing), Vulkan SDK, shaderc (glslc)
- No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`)
- Strict warnings: `-Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror`

## Code Conventions

- **Prefix:** `hg` for functions/types, `Hg` for structs, `HG_` for defines
- **No exceptions, no RTTI, no std:: containers** -- custom containers only
- **Memory:** arena allocator for temp, GPA (malloc wrapper) for persistent
- **No RAII** -- `hgDefer()` macro for cleanup, manual destroy calls
- **Thread-local error messages** via `hgErrorGet()/hgErrorSet()`
- **Serialization:** template specialization of `hgSerialize()` for each type
- **ECS:** component type registration via `hgEcsRegisterType(ecs, T)` macro
- **Naming:** enums use `HgXxx_name` pattern; flags are `typedef u32 HgXxxFlags`

## Architecture (subsystems in init order)

| Subsystem | Header | Init flag | Note |
|-----------|--------|-----------|------|
| Memory    | hg_memory.hpp | HgSubsystem_memory | Scratch arenas (2 x 16MB per thread), GPA |
| Concurrency | hg_concurrency.hpp | HgSubsystem_concurrency | Thread pool, mutex, fence |
| GPU       | hg_gpu.hpp | HgSubsystem_gpu | Vulkan wrapper: buffers, images, pipelines, cmd buffers |
| Assets    | hg_assets.hpp | HgSubsystem_assets | Ref-counted asset manager, hot reload |
| Windowing | hg_window.hpp | HgSubsystem_windowing | SDL3-based windows, input, swapchain |
| Audio     | hg_audio.hpp | HgSubsystem_audio | Audio streams, player, ECS source |
| Platform  | hg_platform.hpp | (internal) | SDL3 vulkan extensions, platform init |

Call `hgInit()` with flag combinations or `HgSubsystem_all` (default).

## Key Headers Overview

- **hg_core.hpp** -- Core typedefs (u8/u32/f32 etc.), HgBinary, HgString, logging, init/deinit
- **hg_gpu.hpp** -- Full Vulkan wrapper (1267 lines). Buffers, images, views, pipelines, render passes, barriers, descriptors
- **hg_math.hpp** -- Vec2/3/4, Mat2/3/4, Quat, Complex, collision (circle/rect/sphere/box/ray/line/tri/plane), noise, RNG
- **hg_window.hpp** -- Window create/destroy, input (keyboard/mouse enum), swapchain frame begin/end
- **hg_rendering.hpp** -- Camera (persp/ortho), 2D layers (rects/sprites/atlas/tilemap), skybox, 3D models, lights, ImGui integration
- **hg_ecs.hpp** -- ECS with hierarchy (HgNode parent/child/sibling), transforms, component registration, iteration
- **hg_assets.hpp** -- Asset system: HgAsset<T> with ref counting, pool, path-based cache, load/unload/reload
- **hg_audio.hpp** -- Audio streams, HgAudioPlayer (music + sound effects), ECS audio source component
- **hg_concurrency.hpp** -- Thread pool (hgThreadsCall/For), spinlock mutex, fence
- **hg_containers.hpp** -- HgArray<T>, HgArrayAny (type-erased), HgQueue<T> (ring buffer), HgSet<K,V>, HgMap<K,V>, HgPool, HgHandlePool
- **hg_memory.hpp** -- hgGpaAlloc/Free (malloc wrapper), HgArena (bump allocator), scratch arenas
- **hg_serialization.hpp** -- Tree-based serializer (HgSerializer), binary + JSON I/O
- **hg_strings.hpp** -- HgStringBuilder (arena-backed), formatting, number parsing, binary I/O
- **hg_templates.hpp** -- Template implementations for containers, assets, ECS iteration (forEach/forPar)
- **hg_utils.hpp** -- min/max/clamp, align, endian swap, memcpy/memset/memcmp
- **hg_time.hpp** -- HgClock (high-res), hgSleep, HgPerf measurement
- **hg_library.hpp** -- Dynamic library loading (dlopen/LoadLibrary)

## Examples (source apps)

- **editor** -- Full editor with ImGui docking, ECS hierarchy editor, 3D viewport, camera controls, component inspector
- **minimal** -- Minimal 2D example with sprite, audio player, basic movement
- **noise** -- GPU compute shader noise generation with live slider controls
- **embed** -- CLI tool to embed binary files as C headers (used in build for shaders)

## Key Patterns

- **Shaders:** written as .vert/.frag/.comp in src/, compiled via glslc to SPIR-V, embedded as C headers via `embed` tool, included as `#include "name.spv.h"`
- **Arenas:** use `hgArenaScope(arena)` for temp allocations that auto-free at scope end
- **Scratch arenas:** `hgScratch()` returns a thread-local arena; pass conflict arenas to avoid overlap
- **Defer:** `hgDefer(code)` runs code at scope exit via destructor trick
- **Serialization:** `hgSerialize(s, &val)` reads or writes based on s->writing flag
- **ECS iteration:** `hgEcsForEach<ComponentType>(ecs, [](HgEntity e, ComponentType* c) { ... })`
- **Asset system:** `hgAssetLoad<T>(path)` returns ref-counted `HgAsset<T>*`; `hgAssetUnload()` decrements

## Testing

`hgTest()` runs from `src/test.cpp` -- ~5000 lines of tests covering arenas, strings, containers, threading, serialization, JSON parsing.
Run with `./build/test_runner` (must be inside `nix develop`).

## CMake Targets

- `hurdygurdy` -- Static library (core + vendor)
- `test_runner` -- Runs hgTest() and exits (fast, no GPU/window needed, but still needs `nix develop` for Vulkan libs)
- `editor` -- Editor example
- `noise` -- Noise compute shader example
- `minimal` -- Minimal 2D example
- `shaders` -- Custom target to compile all GLSL shaders
- `embed` -- Helper tool for shader embedding

## Vendor

- imgui (ocornut/imgui@2af6dd9) -- SDL3 + Vulkan backends
- stb_image.h, stb_image_write.h -- Image loading/saving
- vk_mem_alloc.h -- Vulkan memory allocator

