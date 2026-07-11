# Hurdy Gurdy C++23 Coding Guidelines

## C++ Standard

Target C++23. No exceptions (`-fno-exceptions`). No RTTI (`-fno-rtti`).

## Namespace and Naming

All public hurdygurdy code lives in the `hg` namespace. The `hg` prefix on names moves into the namespace.

| thing | rule | example |
|---|---|---|
| types | PascalCase | `hg::Array`, `hg::Arena`, `hg::String` |
| functions | camelCase | `hg::gpaAlloc()`, `hg::arenaAlloc()` |
| variables | camelCase | `scratchArena`, `tempBuffer` |
| macros | SCREAMING_SNAKE | `HG_ASSERT`, `HG_DEFER` |

File names keep the `hg_` prefix: `hg_memory.hpp`, `hg_gpu.hpp`. Umbrella header is `hg.hpp`.

## Structs, Not Classes

Everything is `struct`. No `private:`, no `protected:`, no encapsulation. Data is public. Member functions are fine. Free functions on a type are also fine.

Vendor code (imgui, stb, vma) follows vendor conventions.

## RAII and Resource Management

Every type that owns a resource gets:

1. default constructor -- leaves in empty/moved-from state
2. move constructor -- steals resource, leaves source empty
3. move assignment -- steals resource, leaves source empty
4. destructor -- releases resource
5. delete copy constructor and copy assignment
6. `clone()` method if copies make sense (rare)

```cpp
struct Buffer {
    void* data = nullptr;
    u64 size = 0;

    Buffer() = default;
    Buffer(Buffer&& other) noexcept : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (data) gpaFree(data, size);
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
    ~Buffer() { if (data) gpaFree(data, size); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer* clone() const; // optional
};
```

Free create functions remain the primary way to construct:
```cpp
Buffer buffer = hg::bufferCreate(1024);
// destructor auto-cleans
```

## Move Semantics

- Resource-owning types: move-only, copy deleted.
- POD types (math structs, config, small data): trivially copyable, no special members.
- Containers: move-only by default, `clone()` where needed.

## Error Handling

No exceptions. No `Expected`, no `Result`, no `Optional` for error propagation.

- Functions return `bool` or `nullptr` on failure.
- Thread-local error string via `hg::errorGet()` / `hg::errorSet()`.
- Fatal errors call `hg::panic()` (noreturn).

## Enums

Keep C-style enums with u32 underlying type and prefix:

```cpp
namespace hg {
enum RenderPassType : u32 {
    RenderPassType_Color = 0,
    RenderPassType_Depth = 1,
};
typedef u32 RenderPassTypeFlags;
}
```

No `enum class`. Enums live inside the `hg` namespace. Type name omits `Hg` prefix since namespace covers it. Enum values are prefixed with type name + PascalCase value: `hg::RenderPassType_Color`.

## Containers

Two container families:

| owning (GPA) | temp (arena) | view (non-owning) |
|---|---|---|
| `hg::Array<T>` | `hg::ArrayTemp<T>` | `hg::ArrayView<T>` |
| `hg::String` | `hg::StringTemp` | `hg::StringView` |
| `hg::Map<K,V>` | `hg::MapTemp<K,V>` | -- |
| `hg::Set<K>` | `hg::SetTemp<K>` | -- |

- Owning containers allocate from GPA, free in destructor.
- Temp containers accept an arena in constructor, do not own memory. No destructor needed.
- Views are non-owning pointer+size, trivially copyable.

## Templates and Concepts

- `template<typename T>` style for template parameters.
- Use C++23 concepts to clarify intent on public interfaces:

```cpp
template<typename T>
concept HgAllocator = requires(T& a, u64 size, u64 align) {
    { a.alloc(size, align) } -> std::convertible_to<void*>;
};

template<HgAllocator A>
struct Array { ... };
```

## constexpr

Small pure functions should be `constexpr` (math, string comparisons, utilities). Containers are not constexpr (heap allocation). No formal rule beyond "if it fits in 5 lines and has no side effects, constexpr it."

## Constructors

Prefer no user-declared constructors. Keep types aggregate-initializable where possible.

- Math types: remain aggregates (designated init). Exception for very useful convenience constructors.
- RAII types: default constructor, move constructor, move assignment, destructor. That is the full set.
- Free create functions are preferred over multi-argument constructors.

## Headers and Includes

- Every header is self-contained. It includes what it needs.
- `hg.hpp` is the umbrella header including all public headers.
- Use `#pragma once` for header guards.

## Style Misc

| thing | rule |
|---|---|
| assertions | `HG_ASSERT(cond)` macro, not `assert()` |
| defer | `HG_DEFER(code)` macro for scope-exit cleanup |
| logging | `hg::log()`, `hg::warn()`, `hg::panic()` in `hg` namespace |
| STL | never use `std::` containers. custom containers only. |
| integer types | `u8/u16/u32/u64`, `i8/i16/i32/i64` from `hg.hpp` |
| floating types | `f32` (float), `f64` (double) |
| casts | use C++ casts (`static_cast`, `reinterpret_cast`), never C-style casts |
| nullptr | use `nullptr`, never `NULL` or `0` for pointers |
