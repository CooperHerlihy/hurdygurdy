# AGENTS.md -- Hurdy Gurdy Game Engine

## Agent Rules

1. Never make changes without consulting the user first.
2. Never make changes without also writing and running tests.
3. Everything should be as simple as possible. If it gets too complex, ask the user.

## Identity

**Hurdy Gurdy**, a game engine written in C++23 using Vulkan.

## Project Structure

```
hurdygurdy/
  include/           -- Public headers (.hpp + .glsl)
  src/               -- Source files + shaders (.vert/.frag/.comp)
  vendor/            -- stb_image, stb_image_write, vk_mem_alloc, imgui
  build/             -- CMake build output (generated, ignored)
  CMakeLists.txt     -- CMake 3.16+, C++23
  flake.nix          -- Nix flake for dev shell + build
  CODING_GUIDELINES.md -- Full coding conventions
```

## Build

**All build & run commands MUST be inside `nix develop` (Vulkan unavailable otherwise).**

```
nix develop
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Targets: `editor`, `noise`, `minimal` (exes), `hurdygurdy` (static lib). Also `nix build` for release.

## Dependencies

- C++23 compiler (GCC 14+/Clang 18+), CMake 3.16+
- SDL3 (auto-downloaded), Vulkan SDK, shaderc (glslc)
- No exceptions, no RTTI, no STL containers
- `-Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror`

## Coding Conventions

See `CODING_GUIDELINES.md` for full rules. Quick summary:

- All public code in `hg` namespace
- `hg::PascalCase` for types, `hg::camelCase()` for functions, `HG_SCREAMING_SNAKE` for macros
- `hg_` prefix on filenames (`hg_memory.hpp`); umbrella header `hg.hpp`
- No `enum class` — C-style enums in `hg` namespace: `hg::RenderPassType_Color`
- No `class` — everything `struct`
- RAII for resource types: default ctor, move, destructor, delete copy
- Move-only for owning types, `clone()` where copies make sense
- Custom containers: `hg::Array`, `hg::ArrayTemp`, `hg::ArrayView` and friends
- No constructors except default + move on RAII types. Free create functions preferred.
- Thread-local error strings, no exceptions, no `Expected`/`Result`
- C++23 concepts for interface constraints
- `constexpr` on small pure functions
- `HG_ASSERT()`, `HG_DEFER()`, no C-style casts

## Architecture (init order)

| Subsystem | Header | Init flag | Note |
|---|---|---|---|
| Memory | hg_memory.hpp | Subsystem_Memory | Scratch arenas (2 x 16MB per thread), GPA |
| Concurrency | hg_concurrency.hpp | Subsystem_Concurrency | Thread pool, mutex, fence |
| GPU | hg_gpu.hpp | Subsystem_Gpu | Vulkan wrapper: buffers, images, pipelines, cmds |
| Assets | hg_assets.hpp | Subsystem_Assets | Ref-counted asset manager, hot reload |
| Windowing | hg_window.hpp | Subsystem_Windowing | SDL3 windows, input, swapchain |
| Audio | hg_audio.hpp | Subsystem_Audio | Streams, player, ECS source component |
| Platform | hg_platform.hpp | (internal) | SDL3 vulkan extensions, platform init |

Call `hg::init(flags)` with flag combinations or `Subsystem_All` (default).

## Key Headers

- **hg_core.hpp** — Typedefs (u8/u32/f32 etc.), String, StringView, logging, init/deinit
- **hg_memory.hpp** — Arena, gpaAlloc/gpaFree (malloc wrapper), scratch arenas
- **hg_containers.hpp** — Array, ArrayTemp, String, StringTemp, Map, Set, Queue, Pool
- **hg_math.hpp** — Vec2/3/4, Mat2/3/4, Quat, Complex, collision, noise, RNG
- **hg_gpu.hpp** — Full Vulkan wrapper: buffers, images, pipelines, render passes, barriers, descriptors
- **hg_window.hpp** — Window, input enums, swapchain frame begin/end
- **hg_rendering.hpp** — Camera, 2D layers, sprites/tilemaps, skybox, 3D models, lights, ImGui
- **hg_ecs.hpp** — ECS with hierarchy nodes, transforms, component registration, iteration
- **hg_assets.hpp** — Asset<T> with ref counting, pool, path cache, load/unload/reload
- **hg_audio.hpp** — Audio streams, AudioPlayer (music+SFX), ECS audio source
- **hg_concurrency.hpp** — Thread pool, spinlock mutex, fence
- **hg_serialization.hpp** — Tree serializer (Serialiser), binary + JSON I/O
- **hg_strings.hpp** — Formatting, number parsing, binary I/O helpers
- **hg_time.hpp** — Clock (high-res), sleep, perf measurement
- **hg_library.hpp** — Dynamic library loading (dlopen/LoadLibrary)

## Key Patterns

- **Shaders:** .vert/.frag/.comp in src/, compiled via glslc to SPIR-V, embedded as C headers via `embed` tool, `#include "name.spv.h"`
- **Arenas:** `HG_ARENA_SCOPE(arena)` for temp allocs auto-freed at scope end
- **Scratch:** `hg::scratch(conflicts, count)` returns thread-local arena
- **Defer:** `HG_DEFER(code)` runs at scope exit
- **Serialization:** `hg::serialise(s, &val)` reads or writes based on s->writing
- **ECS iteration:** `hg::ecsForEach<Component>(ecs, [](Entity e, Component* c) { ... })`
- **Assets:** `hg::assetLoad<T>(path)` returns `Asset<T>*`; `assetUnload()` decrements ref

## Testing

`hg::test()` runs from `src/test.cpp` — covers arenas, strings, containers, threading, serialisation, JSON parsing. Run with `./build/test_runner` (inside `nix develop`, no GPU needed).

## CMake Targets

- `hurdygurdy` — Static library (core + vendor)
- `test_runner` — Runs tests, exits
- `editor` — Editor example with ImGui docking, ECS hierarchy, 3D viewport
- `noise` — GPU compute noise with live controls
- `minimal` — Minimal 2D sprite + audio example
- `shaders` — Compile all GLSL shaders
- `embed` — Binary-to-C-header embedding tool

## Vendor

- imgui (ocornut/imgui@2af6dd9) — SDL3 + Vulkan backends
- stb_image.h, stb_image_write.h — Image load/save
- vk_mem_alloc.h — Vulkan memory allocator
