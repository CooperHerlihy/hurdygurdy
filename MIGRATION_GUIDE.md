# Migration Guide — C Style to Modern C++

This guide explains the transition from the old C-style C++ patterns to the conventions
in [CODING_GUIDELINES.md](CODING_GUIDELINES.md). Read that first for the rules; this
document shows how to apply them in practice.

---

## 1. RAII and Move Semantics

This is the biggest change. Every resource-holding type should own its resources through
RAII so that cleanup is automatic and leak-proof.

### Old Pattern (manual create/destroy)

```cpp
Window* win = windowCreate("Title", 800, 600, nullptr);
// ... use win ...
windowDestroy(win);
```

### New Pattern (RAII)

```cpp
Window win{"Title", 800, 600, nullptr};
// ... use win ...
// destroyed automatically at end of scope
```

### Important: constructors must never fail

Since exceptions are disabled, a constructor cannot signal failure. Additionally,
heap allocations are assumed to always succeed — modern OSes overcommit or kill on
OOM, so null-checking every allocation just adds noise.

Therefore:

- **Constructors freely allocate resources** — allocation failure is not handled
  (the process will die on OOM regardless).
- **Fallible operations** (file I/O, shader compilation, data validation) must live
  in a separate method or factory that returns `bool` or a pointer.

```cpp
// Good — constructor assumes allocation succeeds (simplifies callers)
Buffer::Buffer(u64 size) : data{heapAlloc(size, 16)}, size{size}
{
}

// Good — fallible init as separate method
struct Texture {
    Image image;
    bool load(const char* path);   // returns false on error
};

// Good — fallible static factory returning pointer
struct Shader {
    static Shader* create(const char* vertSrc, const char* fragSrc);
};

// Bad — constructor that might fail with no way to report it
Shader::Shader(const char* vertSrc, const char* fragSrc) // can't fail
{
    // what if compilation fails? can't tell caller
}
```

### How to write an RAII type

```cpp
struct Window {
    SDL_Window* handle = nullptr;

    Window() = default;

    // Constructor: allocation is assumed to succeed
    Window(const char* title, u32 width, u32 height, const WindowConfig* config)
    {
        handle = SDL_CreateWindow(title, width, height, 0);
    }

    ~Window()
    {
        if (handle != nullptr)
            SDL_DestroyWindow(handle);
    }

    // Move — transfer ownership, source becomes empty
    Window(Window&& other) noexcept
        : handle{other.handle}
    {
        other.handle = nullptr;
    }

    Window& operator=(Window&& other) noexcept
    {
        if (this != &other)
        {
            if (handle != nullptr)
                SDL_DestroyWindow(handle);
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    // No copying (resources aren't shared)
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};
```

### Key points

- Default constructor creates an empty (null) object.
- Parameterized constructor does the old `Create` work but **never fails** — on
  allocation failure it stays in empty state.
- Destructor does the old `Destroy` work (null-safe).
- Move steals the resource and nulls the source.
- Copy is deleted (add deep copy if genuinely needed).
- `noexcept` on moves (good for performance).

### Factory functions

Return by value for infallible, return `bool` or pointer for fallible.

```cpp
// Good — infallible (allocation failure leaves empty state)
Buffer createVertexBuffer(u32 vertexCount)
{
    return Buffer{vertexCount * sizeof(Vertex)};
}

// Good — fallible factory
Texture* createTexture(const char* path)
{
    Texture* tex = heapAlloc<Texture>(1);
    if (tex != nullptr && tex->load(path))
        return tex;
    heapFree(tex);
    return nullptr;
}

// Bad — out parameters or raw owning pointers
void createTexture(Texture* out);
Texture* createTexture();   // who frees this?
```

### HG_DEFER still has a place

`HG_DEFER` remains useful for non-resource cleanup (file descriptors, temporary state
changes) where creating a whole RAII wrapper would be overkill. But for any type that
has a clear acquire/release lifecycle, prefer a proper RAII type.

---

## 2. Engine Class

### Old

```cpp
if (!init())
    HG_PANIC("Could not initialize\n");
HG_DEFER(deinit());
```

### New

```cpp
Engine engine{};
// all subsystems ready
// destroyed automatically at end of scope
```

The `Engine` class wraps all subsystem initialization:

```cpp
struct Engine {
    SubsystemFlags active = 0;

    Engine(SubsystemFlags flags = Subsystem_all)
    {
        if (!init(flags))
            HG_PANIC("Could not initialize\n");
        active = flags;
    }

    ~Engine()
    {
        deinit();
        active = 0;
    }
};
```

---

## 3. Cast Migration

Use this table to replace C-style casts.

| C-style | C++ replacement |
|---------|----------------|
| `(f32)x` | `static_cast<f32>(x)` |
| `(u8*)ptr` | `reinterpret_cast<u8*>(ptr)` |
| `(uptr)(end - begin)` | `static_cast<uptr>(end - begin)` |
| `(u32)value` | `static_cast<u32>(value)` |
| `(int*)addr` | `reinterpret_cast<int*>(addr)` |
| `(double)f` | `static_cast<double>(f)` |

For numeric truncation warnings, use `static_cast` — the explicit cast signals intent.

For bit-punning floats to ints etc., use `bit_cast<T>(val)` (C++20) instead of
`*(T*)&val` (which is UB).

### Old

```cpp
f32 x = (f32)i / (f32)count;
u8* bytes = (u8*)data;
uptr aligned = (uptr)(addr + align - 1) & ~(align - 1);
```

### New

```cpp
f32 x = static_cast<f32>(i) / static_cast<f32>(count);
u8* bytes = reinterpret_cast<u8*>(data);
uptr aligned = (reinterpret_cast<uptr>(addr) + align - 1) & ~(align - 1);
```

---

## 4. Header Guards

### Old

```cpp
#ifndef HG_HURDYGURDY_HPP
#define HG_HURDYGURDY_HPP
// ...
#endif
```

### New

```cpp
#pragma once
// ...
```

---

## 5. Type Aliases

### Old

```cpp
typedef uint8_t u8;
typedef uint32_t u32;
typedef float f32;
```

### New

```cpp
using u8 = uint8_t;
using u32 = uint32_t;
using f32 = float;
```

---

## 6. Container Migration

The old containers (`Array<T>`, etc.) allocate from arena by default. The new split
gives you two choices depending on lifetime.

### Old (all arena-based)

```cpp
Arena* scratch = getScratch();
Array<u32> nums = array<u32>(scratch, 16);
// push, pop, etc. — all arena memory
```

### New — owning (long-lived data)

```cpp
Array<u32> nums;
nums.reserve(16);
nums.append(42);
// freed automatically when `nums` goes out of scope
```

### New — temp (scratch/arena)

```cpp
Arena* scratch = getScratch();
TempArray<u32> nums = tempArray<u32>(scratch, 16);
nums.append(42);
// no free needed — arena resets on scope exit via HG_ARENA_SCOPE
```

### Migration table

| Old call | Owning replacement | Temp replacement |
|----------|-------------------|-----------------|
| `array<T>(arena, cap)` | `Array<T>{}` then `.reserve(cap)` | `tempArray<T>(arena, cap)` |
| `queue<T>(arena, cap)` | `Queue<T>{}` then `.reserve(cap)` | `tempQueue<T>(arena, cap)` |
| `set<T>(arena, cap)` | `Set<T>{}` then `.reserve(cap)` | `tempSet<T>(arena, cap)` |
| `map<K,V>(arena, cap)` | `Map<K,V>{}` then `.reserve(cap)` | `tempMap<K,V>(arena, cap)` |

---

## 7. Default Member Initializers

### Old

```cpp
struct Arena {
    void* memory;
    u64 capacity;
    u64 head;

    Arena() : memory(nullptr), capacity(0), head(0) {}
};
```

### New

```cpp
struct Arena {
    void* memory = nullptr;
    u64 capacity = 0;
    u64 head = 0;
};
```

This also applies to aggregate initialization — many structs can drop their constructor
entirely and just use default member initializers + aggregate init.

---

## 8. Range-Based For

### Old

```cpp
for (u32 i = 0; i < eventCount; ++i)
{
    WindowEvent& ev = events[i];
    // ...
}
```

### New

```cpp
for (const WindowEvent& ev : Span{events, eventCount})
{
    // ...
}
```

When the container already supports range-for (custom `Array<T>`, etc.):

```cpp
for (const Entity& e : entities)
{
    // ...
}
```

Index loops are still fine when you actually need the index.

---

## 9. Concepts (SFINAE → requires)

### Old

```cpp
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
T twice(T val)
{
    return val * 2;
}
```

### New

```cpp
template<typename T> requires std::is_integral_v<T>
T twice(T val)
{
    return val * 2;
}
```

Or with a named concept:

```cpp
template<typename T>
concept Integral = std::is_integral_v<T>;

template<Integral T>
T twice(T val)
{
    return val * 2;
}
```

---

## 10. `if constexpr`

### Old

```cpp
template<typename T>
void serialize(const T& val)
{
    // overloads or SFINAE to handle different cases
}
```

### New

```cpp
template<typename T>
void serialize(const T& val)
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        memCopy(&val, dst, sizeof(T));
    }
    else
    {
        val.serialize(dst);
    }
}
```

---

## 11. `= delete` for Special Members

### Old

```cpp
struct UniqueResource {
    // implicitly generated copy/move would double-free
    // rely on convention: "don't copy this"
};
```

### New

```cpp
struct UniqueResource {
    UniqueResource() = default;
    ~UniqueResource();
    UniqueResource(UniqueResource&&) noexcept;
    UniqueResource& operator=(UniqueResource&&) noexcept;
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator=(const UniqueResource&) = delete;
};
```

The compiler enforces the rule instead of relying on documentation.

---

## 12. `const` Placement

Left-const everywhere.

```cpp
const char* chars;       // pointer to const char
char* const chars;       // const pointer to char (rare)
const char* const chars; // const pointer to const char
```

---

## 13. `consteval`

Use for functions that must produce compile-time constants and would be UB or
meaningless at runtime.

```cpp
consteval u32 compileTimeHash(const char* str)
{
    u32 h = 5381;
    while (*str)
        h = ((h << 5) + h) + static_cast<u8>(*str++);
    return h;
}
```

---

## 14. Braces

### Old (Allman everywhere)

```cpp
struct Foo          // struct brace on next line
{
    int x;
};

void func()          // code block brace on next line
{
    if (cond)
    {
    }
}
```

### New (Allman for code blocks, same-line for types)

```cpp
struct Foo {         // type — same line
    int x;
};

void func()          // code block — next line
{
    if (cond)        // code block — next line
    {
    }
}
```

The rationale: type definitions are declarations, not control flow — the brace position
distinguishes them visually from code blocks.

---

## 15. Braced Initialization

Prefer `{}` to `()` for construction. This avoids the most-vexing-parse and prevents
narrowing conversions.

```cpp
Vec2 pos{1.0f, 2.0f};  // good
Vec2 pos(1.0f, 2.0f);  // could be a function declaration in some contexts
u32 x{};                // zero-initialized
```

---

