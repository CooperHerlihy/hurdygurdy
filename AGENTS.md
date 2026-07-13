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
  include/           -- Public headers (hurdygurdy.hpp umbrella + .glsl)
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
- `hg_` prefix on filenames (`hurdygurdy.hpp` umbrella); all public API in `hurdygurdy.hpp` except `hg_templates.hpp` (template definitions)
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

| Subsystem | Init flag | Note |
|---|---|---|
| Memory | Subsystem_Memory | Scratch arenas (2 x 16MB per thread), GPA |
| Concurrency | Subsystem_Concurrency | Thread pool, mutex, fence |
| GPU | Subsystem_Gpu | Vulkan wrapper: buffers, images, pipelines, cmds |
| Assets | Subsystem_Assets | Ref-counted asset manager, hot reload |
| Windowing | Subsystem_Windowing | SDL3 windows, input, swapchain |
| Audio | Subsystem_Audio | Streams, player, ECS source component |
| Platform | (internal) | SDL3 vulkan extensions, platform init |

Call `hg::init(flags)` with flag combinations or `Subsystem_All` (default).

## Key Headers

All public API is in **hurdygurdy.hpp** (except template implementations in hg_templates.hpp). The umbrella provides (in order):

- **Core** — Typedefs (u8/u32/f32 etc.), String, StringView, logging, init/deinit
- **Utils** — min/max/clamp, align, endian, mem ops
- **Memory** — Arena, gpaAlloc/gpaFree (malloc wrapper), scratch arenas
- **Concurrency** — Thread pool, spinlock mutex, fence
- **GPU** — Full Vulkan wrapper: buffers, images, pipelines, render passes, barriers, descriptors
- **Strings** — Formatting, number parsing, binary I/O helpers
- **Serialization** — Tree serializer (Serialiser), binary + JSON I/O
- **Containers** — Array, ArrayTemp, String, StringTemp, Map, Set, Queue, Pool
- **Math** — Vec2/3/4, Mat2/3/4, Quat, Complex, collision, noise, RNG
- **Assets** — Asset<T> with ref counting, pool, path cache, load/unload/reload
- **Time** — Clock (high-res), sleep, perf measurement
- **Library** — Dynamic library loading (dlopen/LoadLibrary)
- **Windowing** — Window, input enums, swapchain frame begin/end
- **Audio** — Audio streams, AudioPlayer (music+SFX), ECS audio source
- **Rendering** — Camera, 2D layers, sprites/tilemaps, skybox, 3D models, lights, ImGui
- **ECS** — ECS with hierarchy nodes, transforms, component registration, iteration

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
