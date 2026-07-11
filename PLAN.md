# C++23 + Move Semantics Migration Plan

## Golden Rule

**Tests pass after every phase.** Not "mostly pass." Not "pass after the
next phase too." Every single commit must compile warning-clean and pass
the full test suite (`./build/test_runner`). If a phase breaks tests,
phase is not done. No exceptions.

`hgTest()` runs ~5000 tests covering arenas, strings, containers,
threading, serialization, JSON. Every migration phase adds coverage
for the newly touched types. By the end, every type with a destructor
or move has explicit test coverage.

---

## Overview

Migrate hurdygurdy from C++17 to C++23. Convert all manual Create/Destroy
resource management to RAII with move semantics. Add `hg::` namespace.
Replace non-nullable pointers with `const&`. Separate type per allocator
strategy: GPA types own memory and have destructors; arena types are
non-owning and destructor-free. Macros become ALL_CAPS. Remove macros
where possible.

No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`) remain.

### C++20 Concepts

Use concepts for trait-like constraints that currently use
`static_assert(false, ...)` or runtime asserts on template args.

```cpp
namespace hg {

template<typename T>
concept Hashable = requires(const T& val) {
    { hash(val) } -> std::convertible_to<u64>;
};

template<typename T, typename S>
concept Serializable = requires(S* s, T* val) {
    serialize(s, val);
};

template<typename T, typename U>
concept ConvertibleTo = std::is_convertible_v<T, U>;

} // namespace hg
```

Used to constrain container templates and give readable errors:

```cpp
template<Hashable V>
struct Set { ... };

template<Hashable K, typename V>
struct Map { ... };
```

The `static_assert(false)` trick in current template stubs goes away.
Replace with `static_assert(!Hashable<T>)` or concept-constrained
specialization.

---

## Naming & Conventions

### Namespace: `hg::`

| Current | New |
|---------|-----|
| `HgFoo` | `hg::Foo` |
| `hgDoFoo` | `hg::doFoo` |
| `hgSubsystem_memory` | `hg::Subsystem_memory` |

Structs, enums, typedefs inside `hg::`. Functions inside `hg::`.
Shaders (`hg_Vertex`, `hg_ModelMatrix`) stay as-is -- different language.

### Qualifier placement: suffix

`hg::ArrayTemp` not `hg::TempArray`. `hg::StringOwned` not `hg::OwnedString`.
Reads left-to-right: base name first, qualifier after.

### Enum style: flat, no `enum class`

`HgSubsystem_memory` becomes `hg::Subsystem_memory` (still flat enum).
The `::` in `hg::Subsystem::memory` costs two chars where `_` costs one.
Not worth it.

### Macromangling

| Current | New | Why macro? |
|---------|-----|------------|
| `hgAssert(cond)` | `HG_ASSERT(cond)` | Needs `__FILE__`/`__LINE__`/`#cond` |
| `hgDefer(...)` | `HG_DEFER(...)` | Needs `__LINE__` for unique var name |
| `hgArenaScope(arena)` | `HG_ARENA_SCOPE(arena)` | Layers on `HG_DEFER`, same reason |
| `hgLog(...)` | `HG_LOG(...)` | Conditional compile-time removal |
| `hgWarn(...)` | `HG_WARN(...)` | Same |
| `hgPanic(...)` | `HG_PANIC(...)` | Same, plus `noreturn` |
| `hgEcsRegisterType(ecs, T)` | `HG_ECS_REGISTER_TYPE(ecs, T)` | Needs `#T` stringification |
| `hgMacroConcat(x, y)` | `HG_CONCAT(x, y)` | Internal helper, stays |
| `hgMacroConcatInternal(x, y)` | `HG_CONCAT_IMPL(x, y)` | Internal helper, stays |

All other `hg` macros become `HG_` prefix. Macros that have function
equivalents but stay for compile-time removal keep the `HG_` prefix.

### Constructor philosophy

C++ forces us to have:

| Member | Required? |
|--------|-----------|
| Default constructor | Yes (trivial, no alloc) |
| Destructor | Yes (frees if owner) |
| Move constructor | Yes (steals state) |
| Move assignment | Yes (steals state) |
| Copy constructor | Explicitly deleted |
| Copy assignment | Explicitly deleted |

**Parameterized constructors that do work: NO.** Actual initialization
stays in free functions, same as current style. Two patterns:

```cpp
// Pattern A: return by value (preferred for simple cases)
Array<T> arr = arrayCreate<T>(0, 1024);

// Pattern B: init with out-param (for types with failure modes)
Ecs ecs;
ecsInit(&ecs);  // returns void since GPA panics on OOM
```

This keeps the allocation story explicit. Constructors exist only to
satisfy the language and enforce lifecycle invariants (no copying,
cleanup on scope exit, safe moving).

### Span / Slice

`hg::Span<T>` is a non-owning `{T* data, u64 count}` view. Lightweight
copyable type for function params that accept any array-like (Array,
ArrayTemp, raw C array, sub-slice).

```cpp
namespace hg {

template<typename T>
struct Span {
    T*  data  = nullptr;
    u64 count = 0;

    Span() = default;
    Span(T* data_, u64 count_);
    Span(T* begin, T* end);       // [begin, end)

    // Implicit from Array, ArrayTemp, raw arrays
    template<typename U> Span(Array<U>& arr);
    template<typename U> Span(ArrayTemp<U>& arr);
    template<u64 N> Span(T (&arr)[N]);

    T&       operator[](u64 idx);
    const T& operator[](u64 idx) const;

    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;
};

} // namespace hg
```

`hg::String` is already a `Span<const char>` in spirit. Could optionally
derive from `Span<const char>` or stay independent. Keep independent for
now (less churn).

Implicit conversions mean a function taking `hg::Span<T>` can be called
with an `hg::Array<T>`, `hg::ArrayTemp<T>`, or `T[]` without the caller
noticing. Good for API boundaries.

### References vs pointers

| Form | When | Example |
|------|------|---------|
| `const T&` | Read-only single object. Default for params. | `hg::serialize(s, val)` |
| `T*` | Mutable out-parameter. NOT null on success. | `hg::ecsAdd(&ecs, entity, componentId)` |
| `const T*` + `u64 count` | Array/range parameter. | `hg::setSomeValues(data, count)` |
| `T*` returned | Pointer to internal data, nullable for "not found". | `hg::mapGet(&map, key)` -> returns `nullptr` if absent |
| `T&` | NEVER allowed. Non-const ref is banned. | Use `T*` instead |
| `hg::Arena*` | Allocator arg, stays pointer. Can be null in scratch conflict patterns. | `hg::arenaAlloc(&arena, size, align)` |

---

## Phase 0: C++23 Baseline

**Files:** `CMakeLists.txt`

- Set `CMAKE_CXX_STANDARD 23`
- Verify strict flags still compile (`-Wall -Wextra -Wconversion
  -Wsign-conversion -pedantic -Werror`)
- C++23 features to rely on:
  - `if constexpr` (already using)
  - Deducting `this` (P0847) -- simplify container members
  - `std::unreachable()` for panic paths
  - `std::byteswap`, `std::to_underlying` -- simplify utils
  - Explicit move-only types well-supported

**No other code changes in this phase.** Verify compile only.

---

## Phase 1: Arena Safety

**Files:** `include/hg_memory.hpp`, `src/arena.cpp` (split from hurdygurdy.cpp)

### Problem

`HgArena` is `{void* memory, u64 capacity, u64 head}`. Copy creates
aliases. Moved-from arena looks alive. Arena memory could be freed by
GPA paths. No guard against use-after-scope.

### Design

Each arena has a `u32 magic` field set at construction and zeroed on
move. All allocation functions assert magic is valid. Moved-from arena
is safely destructible (no-op) but asserts on alloc.

```cpp
namespace hg {

struct Arena {
    void* memory   = nullptr;
    u64   capacity = 0;
    u64   head     = 0;
    u32   magic    = 0;

    Arena() = default;
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept;
    Arena& operator=(Arena&& other) noexcept;

    void reset() { head = 0; }
    bool valid() const;
    u64  remaining() const;

    void* alloc(u64 size, u64 align);
    void* realloc(void* allocation, u64 oldSize, u64 newSize, u64 align);
};

// Init from pre-allocated block
void arenaInit(Arena* arena, void* memory, u64 capacity);

} // namespace hg
```

### API changes

| Before | After |
|--------|-------|
| `hgArenaAlloc(arena, size, align)` | `arena->alloc(size, align)` |
| `hgArenaRealloc(arena, ptr, old, new, align)` | `arena->realloc(ptr, old, new, align)` |
| `hgArenaAlloc<T>(arena, count)` | `arena->alloc<T>(count)` |
| `hgArenaRealloc<T>(arena, ptr, old, new)` | `arena->realloc<T>(ptr, old, new)` |
| `hgScratchInit(count, size)` | `hg::scratchInit(count, size)` |
| `hgScratchDeinit()` | `hg::scratchDeinit()` |
| `hgScratch(conflicts, count)` | `hg::scratch(conflicts, count)` |
| `hgArenaScope(arena)` | `HG_ARENA_SCOPE(arena)` (macro stays, uses guard) |

`HG_ARENA_SCOPE` uses a RAII guard struct internally so it can assert
arena validity on scope exit too.

---

## Phase 2: String Types

**Files:** `include/hg_strings.hpp`, `include/hg_core.hpp`

Design: three types, one per memory strategy. All used for different
use cases. No inheritance between them (keeps them independent).

```cpp
namespace hg {

// Non-owning string view. Freely copyable. No alloc, no dtor.
struct String {
    const char* chars   = nullptr;
    u64         length  = 0;

    String() = default;
    constexpr String(const char* charsVal, u64 lengthVal);
    constexpr String(const char* begin, const char* end);
    constexpr String(const char* cStr);  // implicit from literal, max 4096

    constexpr const char& operator[](u64 idx) const;
};

bool operator==(String lhs, String rhs);
bool operator!=(String lhs, String rhs);

// GPA-owned string. Move-only, destructor frees.
struct StringOwned {
    const char* chars   = nullptr;
    u64         length  = 0;

    StringOwned() = default;
    ~StringOwned();

    StringOwned(const StringOwned&) = delete;
    StringOwned& operator=(const StringOwned&) = delete;

    StringOwned(StringOwned&& other) noexcept;
    StringOwned& operator=(StringOwned&& other) noexcept;

    StringOwned clone() const;          // explicit deep copy
    operator String() const;            // view conversion

    const char& operator[](u64 idx) const;
};

// GPA alloc + copy from src
StringOwned stringOwnedCreate(String src);

// Arena-backed mutable string. Replaces HgStringBuilder.
// No destructor -- arena handles cleanup.
struct StringTemp {
    char* chars   = nullptr;
    u64   length  = 0;
    Arena* arena  = nullptr;            // stored for append/resize

    // append, insert, prepend, format -- all use stored arena
    void append(String src);
    void append(char c);
    void insert(u64 idx, String src);
    void prepend(String src);

    operator String() const;
    char& operator[](u64 idx);
};

// Create empty string temp with arena for future ops
StringTemp stringTempCreate(Arena& a);

// Copy src into arena
StringTemp stringTempCreate(Arena& a, String src);

} // namespace hg
```

### What changes

| Before | After |
|--------|-------|
| `HgString {chars, length}` (non-owning) | `hg::String {chars, length}` |
| `hgStringCreate(src)` | `hg::stringOwnedCreate(src)` |
| `hgStringDestroy(&str)` | destructor fires |
| `HgStringBuilder` | `hg::StringTemp` |
| `hgStringCopy(arena, src)` | `hg::stringTempCreate(arena, src)` |
| `hgStringAppend(arena, &dst, src)` | `dst.append(src)` (arena stored in StringTemp) |
| `HgBinary` | `hg::Binary` (unchanged semantics) |
| `HgBinaryBuilder` | `hg::BinaryBuilder` (unchanged semantics) |
| `hgCString(arena, str)` | `hg::cString(&arena, str)` -- renamed free function |

### Macros removed

- `hgStringFormat` → becomes `hg::StringTemp::format()` or free function
- String comparison operators become `hg::` namespace scope

---

## Phase 3: Containers

**Files:** `include/hg_containers.hpp`, `include/hg_templates.hpp`

### Principle

Two types per container kind. Separate types means:

- No runtime flag. No `_owns` bool. No byte tax.
- GPA type has destructor. Arena type does not.
- Arena type stores an `hg::Arena*` for all mutation operations.
- Arena type cannot accidentally outlive its arena (design choice:
  caller's responsibility, same as current).

### What goes where

Member functions:
- All mutating operations (push, pop, remove, resize, clear)
- Constructors, destructor, move, clone
- operator[]

Stays as free functions:
- Serialization (`hg::serialize`)
- Iteration helpers (`hg::forEach`)

### HgArray\<T\>

```cpp
namespace hg {

template<typename T>
struct Array {
    T*   vals     = nullptr;
    u32  count    = 0;
    u32  capacity = 0;

    Array() = default;
    ~Array();

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    Array(Array&& other) noexcept;
    Array& operator=(Array&& other) noexcept;

    Array clone() const;

    T&       operator[](u64 idx);
    const T& operator[](u64 idx) const;

    T* push();                                 // returns slot, GPA realloc if full
    T  remove(u32 idx);                        // stable shift
    T  removeSwap(u32 idx);                    // swap-with-last
    T  pop();
    void resize(u32 newCount);                 // GPA realloc
};

template<typename T>
Array<T> arrayCreate(u32 count = 0, u32 capacity = 1024);   // GPA alloc
template<typename T>
void     arrayInit(Array<T>* arr, u32 count = 0, u32 capacity = 1024);

template<typename T>
struct ArrayTemp {
    T*    vals     = nullptr;
    u32   count    = 0;
    u32   capacity = 0;
    Arena* arena   = nullptr;

    T&       operator[](u64 idx);
    const T& operator[](u64 idx) const;

    T* push();                                 // arena realloc if full
    T  remove(u32 idx);
    T  removeSwap(u32 idx);
    T  pop();
    void resize(u32 newCount);                 // arena realloc
};

template<typename T>
ArrayTemp<T> arrayTempCreate(Arena& a, u32 count = 0, u32 capacity = 1024);
template<typename T>
void         arrayTempInit(ArrayTemp<T>* arr, Arena& a, u32 count = 0, u32 capacity = 1024);

} // namespace hg
```

### HgArrayAny

Same split: `hg::ArrayAny` (GPA) and `hg::ArrayAnyTemp` (arena).
Type-erased, stores width, align, element dtor callback.

### HgQueue\<T\>

```cpp
namespace hg {

template<typename T>
struct Queue {
    T*   vals     = nullptr;
    u32  front    = 0;
    u32  back     = 0;
    u32  count    = 0;
    u32  capacity = 0;

    Queue() = default;
    ~Queue();

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&& other) noexcept;
    Queue& operator=(Queue&& other) noexcept;
    Queue clone() const;

    template<typename U = T> void pushFront(U val);
    template<typename U = T> void pushBack(U val);
    T popFront();
    T popBack();
};

template<typename T>
Queue<T> queueCreate(u32 capacity = 1024);
template<typename T>
void     queueInit(Queue<T>* q, u32 capacity = 1024);

template<typename T>
struct QueueTemp {
    T*    vals     = nullptr;
    u32   front    = 0;
    u32   back     = 0;
    u32   count    = 0;
    u32   capacity = 0;
    Arena* arena   = nullptr;

    template<typename U = T> void pushFront(U val);
    template<typename U = T> void pushBack(U val);
    T popFront();
    T popBack();
};

template<typename T>
QueueTemp<T> queueTempCreate(Arena& a, u32 capacity = 1024);
template<typename T>
void         queueTempInit(QueueTemp<T>* q, Arena& a, u32 capacity = 1024);

} // namespace hg
```

### HgSet\<V\>

```cpp
namespace hg {

template<Hashable V>
struct Set {
    bool* hasVal   = nullptr;
    V*    vals     = nullptr;
    u32   capacity = 0;
    u32   count    = 0;

    Set() = default;
    ~Set();

    Set(const Set&) = delete;
    Set& operator=(const Set&) = delete;
    Set(Set&& other) noexcept;
    Set& operator=(Set&& other) noexcept;
    Set clone() const;

    void add(const V& val);          // Robin Hood insert
    bool remove(const V& val);       // backward-shift delete, returns found
    bool has(const V& val) const;
    void reset();
    void resize(u32 newSize);

    template<typename F> void forEach(F fn);
};

template<Hashable V>
Set<V> setCreate(u32 slotCount);
template<Hashable V>
void   setInit(Set<V>* set, u32 slotCount);

template<Hashable V>
struct SetTemp {
    bool* hasVal   = nullptr;
    V*    vals     = nullptr;
    u32   capacity = 0;
    u32   count    = 0;
    Arena* arena   = nullptr;

    // same API as Set but all allocs go to arena
};

template<Hashable V>
SetTemp<V> setTempCreate(Arena& a, u32 slotCount);
template<Hashable V>
void       setTempInit(SetTemp<V>* set, Arena& a, u32 slotCount);

} // namespace hg
```

### HgMap\<K,V\>

```cpp
namespace hg {

template<Hashable K, typename V>
struct Map {
    bool* hasVal   = nullptr;
    K*    keys     = nullptr;
    V*    vals     = nullptr;
    u32   capacity = 0;
    u32   count    = 0;

    Map() = default;
    ~Map();

    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;
    Map(Map&& other) noexcept;
    Map& operator=(Map&& other) noexcept;
    Map clone() const;

    template<typename T = K, typename U = V> V* add(const T& key, const U& val);
    template<typename T = K> bool remove(const T& key, V* out = nullptr);
    template<typename T = K> V* get(const T& key) const;
    void reset();
    void resize(u32 newSize);

    template<typename F> void forEach(F fn);
};

template<Hashable K, typename V>
Map<K, V> mapCreate(u32 slotCount);
template<Hashable K, typename V>
void      mapInit(Map<K, V>* map, u32 slotCount);

template<Hashable K, typename V>
struct MapTemp {
    bool* hasVal   = nullptr;
    K*    keys     = nullptr;
    V*    vals     = nullptr;
    u32   capacity = 0;
    u32   count    = 0;
    Arena* arena   = nullptr;

    // same API as Map but allocs from arena
};

template<Hashable K, typename V>
MapTemp<K, V> mapTempCreate(Arena& a, u32 slotCount);
template<Hashable K, typename V>
void          mapTempInit(MapTemp<K, V>* map, Arena& a, u32 slotCount);

} // namespace hg
```

### HgPool

```cpp
namespace hg {

struct Pool {
    Queue<void*> freeList;
    Array<void*> itemStores;
    u32          width  = 0;
    u32          align  = 0;

    Pool() = default;
    ~Pool();

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    Pool(Pool&& other) noexcept;
    Pool& operator=(Pool&& other) noexcept;
    Pool clone() const;

    void* alloc();
    void  free(void* item);
};

Pool poolCreate(u32 width, u32 align);
void poolInit(Pool* pool, u32 width, u32 align);

template<typename T>
void poolInit(Pool* pool)
{
    poolInit(pool, sizeof(T), alignof(T));
}

} // namespace hg
```

Pool has no Temp variant -- it owns its stores regardless.

### HgHandle

```cpp
namespace hg {

struct Handle {
    u32 id = 0;
};

static constexpr Handle nullHandle{0};

bool operator==(Handle lhs, Handle rhs);
bool operator!=(Handle lhs, Handle rhs);

u32 handleIdx(Handle handle);
u32 handleGeneration(Handle handle);
Handle handleNextGeneration(Handle handle);

// HgHandlePool
struct HandlePool {
    Array<Handle> handles;
    Array<Handle> freed;

    HandlePool() = default;
    HandlePool(HandlePool&&) = default;
    HandlePool& operator=(HandlePool&&) = default;

    HandlePool clone() const;

    Handle alloc();
    bool   alive(Handle handle) const;
    void   free(Handle handle);
    void   reset();
};

HandlePool handlePoolCreate();
void       handlePoolInit(HandlePool* pool);

} // namespace hg
```

### Hashing

```cpp
namespace hg {

template<typename T>
constexpr u64 hash(T) {
    static_assert(false, "hash must be specialized for each type");
    return 0;
}

// specializations: u8-u64, i8-i64, f32, f64, void*, String, Handle, etc.

} // namespace hg
```

---

## Phase 4: Resource-Owning Structs

**Files:** All headers in `include/`

### Pattern

Each struct with `Create`/`Destroy` becomes:

- Default constructor (zero-init, valueless)
- Destructor (replaces Destroy)
- Deleted copy + copy assignment
- Move constructor + move assignment (source valueless)
- `clone()` method (deep copy, GPA alloc)
- Free `create()` / `init()` function for initialization (not a ctor)
- Member functions replace free-function API where natural

### Important: reference/pointer rule for methods

Mutating members take `const&` for reads, return `T*` (not `T&`) for write
access to internals. Non-const refs are not allowed.

### Struct table

| New name | Old name | Owned resources | Mutating members |
|----------|----------|----------------|------------------|
| `hg::Ecs` | `HgEcs` | HandlePool entities, Map(u64, Component) components | spawn, despawn, add, remove, reset |
| `hg::Component` | `HgComponent` | StringOwned name, Array(u32) indices, Array(Entity) entities, ArrayAny components | (internal, managed by Ecs) |
| `hg::Entity` | `HgEntity` | Handle handle | (POD) |
| `hg::Camera` | `HgCamera` | GpuBuffer* vpBuffer | (managed by rendering) |
| `hg::Layer2D` | `HgLayer2D` | Array(Render2DInstance) instances, GpuBuffer* instanceBuffer | push, clear, upload |
| `hg::Atlas2D` | `HgAtlas2D` | Array(Rect) sprites, TextureAsset* texture | addSprite |
| `hg::Tilemap2D` | `HgTilemap2D` | u32* tiles (GPA) | setTile, getTile |
| `hg::AudioStream` | `HgAudioStream` | SDL3_AudioStream* | play, stop |
| `hg::AudioPlayer` | `HgAudioPlayer` | Array(Music) music, Array(AudioStream*) sounds | playMusic, playSound, stopAll |
| `hg::Window` | `HgWindow` | SDL3_Window*, VkSwapchainKHR | beginFrame, endFrame |
| `hg::Mutex` | `HgMutex` | atomic_bool* (pool) | lock, unlock |
| `hg::Fence` | `HgFence` | atomic(u32)* (pool) | wait, signal |
| `hg::GpuBuffer` | `HgGpuBuffer` | VkBuffer, VkDeviceMemory | (managed by gpu) |
| `hg::GpuImage` | `HgGpuImage` | VkImage, VkDeviceMemory | (managed by gpu) |
| `hg::GpuView` | `HgGpuView` | VkImageView, VkSampler | (managed by gpu) |
| `hg::GpuPipeline` | `HgGpuPipeline` | VkPipeline, VkPipelineLayout | (managed by gpu) |
| `hg::GpuCmd` | `HgGpuCmd` | VkCommandBuffer | begin, end, bind, draw |
| `hg::Library` | `HgLibrary` | void* dlopen handle | getSymbol |

---

## Phase 5: Asset System

**Files:** `include/hg_assets.hpp`, `include/hg_templates.hpp`, `src/hurdygurdy.cpp`

### Asset types

```cpp
namespace hg {

template<typename T>
struct Asset {
    T           asset;
    u32         refCount = 1;
    StringOwned path;
};

template<typename T>
struct AssetManager {
    Map<String, Asset<T>*> map;
    Pool                   pool;
};

template<typename T>
inline AssetManager<T> assets{};

} // namespace hg
```

### API

| Before | After |
|--------|-------|
| `hgAssetInit<T>()` | `hg::assetInit<T>()` |
| `hgAssetDeinit<T>()` | `hg::assetDeinit<T>()` |
| `hgAssetLoad<T>(path)` | `hg::assetLoad<T>(path)` |
| `hgAssetUnload<T>(asset)` | `hg::assetUnload(asset)` |
| `hgAssetCopy<T>(asset)` | `hg::assetCopy(asset)` |
| `hgAssetCreate<T>()` | `hg::assetCreate<T>()` |
| `hgAssetLoadImpl<T>` | `hg::assetLoadImpl<T>` (template specialization) |
| `hgAssetUnloadImpl<T>` | `hg::assetUnloadImpl<T>` (template specialization) |

Global `hg::assets<T>` has destructor for `map` and `pool`. But order
with Vulkan matters. `hg::assetDeinit<T>()` explicitly destroys map and
pool during `hg::deinit()`, before GPU shutdown.

---

## Phase 6: ECS Component Destructors

**Files:** `include/hg_ecs.hpp`, `src/hurdygurdy.cpp`

ECS stores components type-erased in `ArrayAny`. The registered `dtor`
function pointer is called on remove/reset.

```cpp
namespace hg {

// Default: call real destructor (no-op for trivially destructible)
template<typename T>
void ecsDtor(T* component) {
    component->~T();
}

// Serialization override
template<typename T>
void ecsSerialize(Serializer* s, T* val, EntitySerializer* entities) {
    serialize(s, val);
}

} // namespace hg
```

`HG_ECS_REGISTER_TYPE(ecs, T)` macro stays (needs `#T`) but updates
calls:
- `hgEcsDtor<T>` → `hg::ecsDtor<T>`
- `hgEcsSerialize<T>` → `hg::ecsSerialize<T>`
- struct fill syntax updates for `hg::Component`

---

## Phase 7: Serialization

**Files:** `include/hg_serialization.hpp`, `src/hurdygurdy.cpp`

### Types

| Before | After |
|--------|-------|
| `HgSerializer` | `hg::Serializer` |
| `HgSerialNode` | `hg::SerialNode` |
| `HgSerialType` | `hg::SerialType` |

### New serialize API

All `T*` val params stay as pointers (mutable out-param per ref/ptr rule).

```cpp
namespace hg {

// primitives
void serialize(Serializer* s, u8* val);

// containers
template<typename T>
void serialize(Serializer* s, Array<T>* arr);

// deserialization: move-assign from newly created container
template<typename T>
void serialize(Serializer* s, Array<T>* arr) {
    serializeBegin(s);
    serialize(s, &arr->count);
    serialize(s, &arr->capacity);
    if (!s->writing) {
        *arr = arrayCreate<T>(0, arr->capacity);  // move assign
    }
    for (u32 i = 0; i < arr->count; ++i) {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

} // namespace hg
```

Deserialization of `Array` and `ArrayTemp` differs: `Array` gets
move-assigned from a new GPA-backed instance; `ArrayTemp` would need
an arena to deserialize into (or we treat it as a build-only type).

**Decision:** serialize only supports GPA containers. Arena containers
are for temporary scratch data that never needs serialization.

---

## Phase 8: All Callers

**Files:** Everything under `src/`

### Mechanical transformations

| Before | After |
|--------|-------|
| `HgArena` | `hg::Arena` |
| `hgArenaAlloc(&arena, size, align)` | `arena.alloc(size, align)` |
| `hgArenaAlloc<T>(&arena, count)` | `arena.alloc<T>(count)` |
| `hgGpaAlloc<T>(count)` | `hg::gpaAlloc<T>(count)` |
| `hgGpaRealloc(ptr, old, new)` | `hg::gpaRealloc(ptr, old, new)` |
| `hgGpaFree(ptr, count)` | `hg::gpaFree(ptr, count)` |
| `hgScratch()` | `hg::scratch()` |
| `hgScratch(conflicts, count)` | `hg::scratch(conflicts, count)` |
| `HG_ARENA_SCOPE(scratch)` | `HG_ARENA_SCOPE(scratch)` (macro stays) |

**Caller patterns that change:**

```cpp
// Before
HgArray<u32> indices = hgArrayCreate<u32>(0, 1024);
hgDefer(hgArrayDestroy(&indices));
indices.count = 5;

// After
hg::Array<u32> indices = hg::arrayCreate<u32>(0, 1024);
indices.count = 5;
// destructor fires at scope end, no defer needed.
```

**Tricky: direct field access.** Current code does `arr.count = n;`
and `arr.vals[i] = x;` directly. With member functions, direct field
access is still possible (fields are public), or we add setters.

Decision: fields stay public (matching current style). Members like
`push()`/`resize()` are convenience, not enforcement. This keeps
migration mechanical: old code like `indices.count = 5` just works.

**Defer removal:** Any `HG_DEFER(hgFooDestroy(&x))` where x now has
a destructor gets deleted. Raw GPA allocs still need `HG_DEFER`.

```cpp
// Before
u8* temp = (u8*)hgGpaAlloc(size, align);
hgDefer(hgGpaFree(temp, size));

// After
u8* temp = (u8*)hg::gpaAlloc(size, align);
HG_DEFER(hg::gpaFree(temp, size));
```

---

## Phase 9: Global State & Static Singletons

**Files:** `src/hurdygurdy.cpp`, `src/concurrency.cpp`

Static state like `render2D`, mutex/fence pools, GPU resource arrays
need explicit lifecycle in `hg::init()` / `hg::deinit()`.

Strategy:

```cpp
// Internal state behind an anonymized handle
// No inline globals with complex destructors
namespace hg {
namespace detail {

static Pool* mutexPool = nullptr;
static Pool* fencePool = nullptr;
static Array<GpuBuffer>* tempBuffers = nullptr;

} // namespace detail

void init() {
    detail::mutexPool = gpaAlloc<Pool>(1);
    *detail::mutexPool = Pool(sizeof(MutexData), alignof(MutexData));

    detail::fencePool = gpaAlloc<Pool>(1);
    *detail::fencePool = Pool(sizeof(FenceData), alignof(FenceData));

    detail::tempBuffers = gpaAlloc<Array<GpuBuffer>>(1);
    *detail::tempBuffers = Array<GpuBuffer>(128);
}

void deinit() {
    // Explicit destruction in reverse order
    detail::tempBuffers->~Array();
    gpaFree(detail::tempBuffers, 1);

    detail::fencePool->~Pool();
    gpaFree(detail::fencePool, 1);

    detail::mutexPool->~Pool();
    gpaFree(detail::mutexPool, 1);
}

} // namespace hg
```

This avoids static destruction-order issues. Full control over when
destroy runs (before GPU shutdown).

---

## Phase 10: Testing

**Files:** `src/test.cpp`

New tests per type:

- Default construction (valueless state, safe to destroy)
- Free-function create / init (allocates, expected capacity)
- Move construction (source values zeroed, dest owns)
- Move assignment (old dest freed, source values zeroed)
- Clone produces independent deep copy
- Arena-type create (allocates from arena, no-op dtor)
- Destructor frees GPA memory (ASan/valgrind doors)
- Moved-from source is safely destructible
- Array/Set/Map: push/add/get/remove/forEach all work as members

---

## Migration Order (Dependency Chain)

```
Phase 0: C++23 standard bump
   |
Phase 1: Arena safety + Arena members
   |
Phase 2: String types (String, StringOwned, StringTemp)
   |
Phase 3: Containers (GPA + Arena, member API)
   |
Phase 4: Resource-owning structs
   |
Phase 5: Asset system
   |
Phase 6: ECS component destructors
   |
Phase 7: Serialization
   |
Phase 8: All callers + namespace migration
   |
Phase 9: Global state lifecycle
   |
Phase 10: Testing
```

Each phase is a commit. System compiles and tests pass at each step.
Do not skip phases.

---

## Summary of Design Decisions

| Decision | Choice | Why |
|----------|--------|-----|
| Allocator separation | Two types per container (GPA + Arena) | No runtime flag, no byte tax |
| Naming | `hg::` namespace, suffix qualifier (`ArrayTemp`) | Base name first, qualifier after |
| References | `const T&` default, `T*` for mut out, `T&` banned | Non-const refs are confusing |
| Macros | ALL_CAPS, `HG_ASSERT`, `HG_DEFER`, etc. | Visible as macros, distinct from functions |
| Enum | Flat (`hg::Subsystem_memory`) not `enum class` | `::` costs 2 chars where `_` costs 1 |
| Concepts | `Hashable`, `Serializable`, `ConvertibleTo` | Replace static_assert(false) traps |
| Constructors | Default + dtor + move only. No allocating ctors | Init stays in free functions |
| Init pattern | `arrayCreate<T>()` returns value OR `arrayInit(&x)` out-param | No parameterized constructors |
| Span / Slice | `hg::Span<T>` — lightweight, non-owning view | Formalize `{T*, count}` ad-hoc pattern, implicit from Array/ArrayTemp |
| Moves | Source left valueless (nullptr/0) | Safe to destroy, safe to re-assign |
| Clone | Named method, not copy ctor | Explicit at call site |
| Fields | Public (direct access still works) | Mechanical migration, no enforcement debt |
| Serialization | GPA containers only, Arena never serialized | Arena = scratch, not persistent |
| Component dtors | `ecsDtor<T>` calls `component->~T()` | Real destructors for real types |
| Global state | Raw ptr + manual ctor/dtor in init/deinit | No static destruction order problems |
| Shaders | Left as-is (`hg_Vertex`, etc.) | Different language, not worth touching |
