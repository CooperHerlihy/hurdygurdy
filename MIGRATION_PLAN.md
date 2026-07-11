# Migration Plan: C Style to C++23

Target: new conventions in `CODING_GUIDELINES.md`.

## Guiding Principles

- System always builds and tests pass at each step.
- Migrate one subsystem at a time, file by file.
- No big bang rewrites. If halfway through, code still ships.

## Phase Order (dependency-driven)

Bottom-up. Each phase unblocks the next.

```
memory -> containers -> core (string, error, log)
  -> concurrency -> gpu
    -> assets -> windowing -> rendering -> audio
      -> ecs -> serialization
        -> apps (minimal, noise, editor, test_runner)
```

Each phase:
1. Update header (`hg_*.hpp`) to new style
2. Update source (`src/*.cpp`) to match
3. Update all callers in already-migrated subsystems
4. Build and run tests

## Phase 1: Memory Subsystem

`hg_memory.hpp` -> `hg_memory.hpp` (same file, new internals)

Changes:
- Wrap in `namespace hg { ... }`
- `HgArena` -> `Arena`
- `hgGpaAlloc` -> `gpaAlloc`
- `hgGpaFree` -> `gpaFree`
- `hgArenaAlloc` -> `arenaAlloc`
- `hgArenaRealloc` -> `arenaRealloc`
- `hgArenaScope` -> `HG_ARENA_SCOPE` (macro, keep all-caps)
- `hgScratchInit` -> `scratchInit`
- `hgScratchDeinit` -> `scratchDeinit`
- `hgScratch` -> `scratch`

Arena stays a plain struct (no RAII, no resources to own).
gpaAlloc/gpaFree are thin wrappers, not resource owners.

## Phase 2: Containers

New files (not migrated, fresh implementation):

- `hg_containers.hpp` — `Array<T>`, `ArrayTemp<T>`, `ArrayView<T>`
- `hg_strings.hpp` — `String`, `StringTemp`, `StringView`

Design:
- `Array<T>`: owning, GPA, move-only, destructor frees
- `ArrayTemp<T>`: arena-backed, non-owning, trivially destructible
- `ArrayView<T>`: pointer+size, non-owning, trivially copyable
- `String`: owning GPA string, move-only
- `StringTemp`: arena-backed mutable string
- `StringView`: non-owning string view

Old `HgArray<T>`, `HgArrayAny`, `HgMap`, `HgSet`, `HgQueue`, `HgPool`, `HgHandlePool`, `HgStringBuilder` get replaced by these.

## Phase 3: Core Subsystem

`hg_core.hpp` + `src/hurdygurdy.cpp`

Changes:
- Wrap in `namespace hg`
- `HgBinary` -> `Binary`, `HgString` -> `String` (but String is now owning from phase 2)
- `HgString` (view) -> `StringView`
- `hgErrorGet/set` -> `errorGet/set`
- `hgLogInternal` -> `log` (internal, or just `log`)
- `hgWarnInternal` -> `warn`
- `hgPanicInternal` -> `panic`
- `hgInit` -> `init`
- `hgDeinit` -> `deinit`
- `hgTest` -> `test`
- `HG_DEFER` macro stays (macro, all-caps)
- Subsystem enum: `hg::Subsystem`, `hg::Subsystem_Memory`, etc.
- `HgDefer<T>` -> internal detail, maybe unnamed or `DeferGuard`

Init/deinit get RAII wrapper? Or keep free functions. Keep free functions for now (init is global, not a resource you move around).

## Phase 4: Concurrency

`hg_concurrency.hpp` + `src/concurrency.cpp`

- `HgThreadPool` -> `ThreadPool`
- `hgThreadsCall` -> `threadsCall`
- `HgSpinlock` -> `Spinlock`
- Same patterns: RAII for owned resources

## Phase 5: GPU Subsystem

`hg_gpu.hpp` (biggest file). This is where RAII pays off most.

- `HgGpuDevice` -> `GpuDevice`
- `HgGpuBuffer` -> `GpuBuffer` (RAII: destructor frees)
- `HgGpuImage` -> `GpuImage` (RAII: destructor frees)
- `HgGpuPipeline` -> `GpuPipeline`
- `HgGpuCmd` -> `GpuCmd`
- etc.

Create functions return owning handles. No more manual `hgGpuBufferDestroy()`.

## Phase 6-11: Remaining Subsystems

Same pattern for each:
- Assets (`hg_assets.hpp`)
- Windowing (`hg_window.hpp`)
- Rendering (`hg_rendering.hpp`)
- Audio (`hg_audio.hpp`)
- ECS (`hg_ecs.hpp`)
- Serialization (`hg_serialization.hpp`)

## Phase 12: Applications

Each app file (`minimal.cpp`, `editor.cpp`, `noise.cpp`, `test_runner.cpp`):
- Update to use new namespace-qualified names
- Remove `hgDefer(destroy)` patterns where RAII handles it
- Clean up manual cleanup code

## Phase 13: Test Suite

`src/test.cpp` — update all calls to match new names. Expand tests for:
- RAII correctness (move, destructor)
- Container operations
- Arena + GPA integration

## Macro and Type Migration Reference

| Old | New |
|---|---|
| `HgArena` | `hg::Arena` |
| `hgGpaAlloc()` | `hg::gpaAlloc()` |
| `hgGpaFree()` | `hg::gpaFree()` |
| `hgArenaAlloc()` | `hg::arenaAlloc()` |
| `hgArenaScope()` | `HG_ARENA_SCOPE()` |
| `hgScratch()` | `hg::scratch()` |
| `HgDefer<T>` | `hg::DeferGuard<T>` (internal) |
| `hgDefer()` | `HG_DEFER()` |
| `HgArray<T>` | `hg::Array<T>` |
| `HgString` (view) | `hg::StringView` |
| `HgStringBuilder` | `hg::StringTemp` |
| `hgInit()` | `hg::init()` |
| `hgDeinit()` | `hg::deinit()` |
| `hgErrorGet()` | `hg::errorGet()` |
| `hgLog()` | `hg::log()` |
| `hgWarn()` | `hg::warn()` |
| `hgPanic()` | `hg::panic()` |
| `hgAssert()` | `HG_ASSERT()` |
| `HgSubsystem_memory` | `hg::Subsystem_Memory` |
| `HgSubsystemFlags` | `hg::SubsystemFlags` |

## Non-Goals

- No touching vendor code (imgui, stb, vma)
- No exceptions or RTTI (project constraint)
- No STL containers
- No enum class
- No private members or encapsulation
