# Coding Guidelines

C++23. No STL containers, no exceptions, no RTTI.

---

## RAII & Move Semantics

Every resource-holding type uses RAII: destructor releases.

**Constructors must never fail** (exceptions are disabled, and allocation failure is
assumed impossible — modern OSes overcommit or kill on OOM). If a higher-level
operation might legitimately fail (bad file, bad data, etc.), put it in a separate
`init()` / `load()` method that returns `bool`.

```cpp
struct Buffer {
    void* data = nullptr;
    u64 size = 0;

    Buffer() = default;
    explicit Buffer(u64 size);   // calls heapAlloc — assumed to succeed
    ~Buffer();

    bool load(const char* path); // fallible — returns false on error

    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};
```

Moved-from state: null/empty (`data = nullptr`, `size = 0`). Fallible factory functions
should signal failure via return type, not throw.

## Containers: Owning vs Temp

| Owning (heap) | Temp (arena) | RAII |
|--------------|-------------|------|
| `Array<T>`   | `TempArray<T>` | Yes |
| `Queue<T>`   | `TempQueue<T>` | Yes |
| `Set<T>`     | `TempSet<T>`   | Yes |
| `Map<K,V>`   | `TempMap<K,V>` | Yes |

Owning variants manage heap memory and support move semantics. Temp variants borrow from an arena with no ownership.

## Naming

- Types: PascalCase. Functions: camelCase. Macros: `HG_UPPER_SNAKE_CASE`.
- Integer types: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`.
- Never use: `int`, `unsigned`, `size_t`, `uint32_t`, `float`, `double`, `std::*` containers.
- Member variables: no prefix/suffix.
- Template params: `typename T`.

## Type Aliases

`using` not `typedef`.

```cpp
using u32 = uint32_t;     // good
typedef uint32_t u32;     // bad
```

## Header Guards

`#pragma once`.

## Casts

C++ named casts only. No C-style casts.

```cpp
static_cast<f32>(x);
reinterpret_cast<u8*>(ptr);
bit_cast<u32>(f);
```

## Enums

Plain unscoped (no `enum class`). Prefix with type name.

```cpp
enum Subsystem : u32 {
    Subsystem_memory = 0x1,
};
```

## Comments

`/** */` Doxygen.

## `auto`

Spell types explicitly. `auto` only in lambdas and unnameable types.

## Range-For

Prefer range-based `for` over index loops where applicable.

## Default Member Init

Use in-class defaults instead of constructor init lists for trivial members.

## `noexcept`

On move operations, `swap`, and trivial accessors only. Not everywhere.

## Custom `string_view`/`span`

Own implementations (`String`, `Span<T>`). Not `std::string_view` or `std::span`.

## Concepts

C++20 `requires` over SFINAE.

```cpp
template<typename T> requires Integral<T>
```

## `if constexpr`

Use for compile-time branching in templates.

## `const`

Left-const: `const Type*`.

## `consteval`

For functions that must run at compile time.

## `= delete`

Explicitly delete copy/move that aren't wanted.

## Structured Bindings

Do not use. Access fields explicitly.

## Pointer Style

`Type* name` (star on type).

## Braces

- Code blocks (`if`, `for`, `while`, function bodies): brace on **next line**.
- Type definitions (`struct`, `class`, `enum`): brace on **same line**.
- Initialization (`{}`): brace on **same line**.

4-space indent, no tabs.

## Braced Init

Prefer `{}` over `()` to avoid most-vexing-parse.

## Namespace

`namespace hg`. Executables: `using namespace hg;`.

## Error Handling

No exceptions. Return `bool` for fallible functions.
- `formatError("...")`, `HG_PANIC("...")`, `HG_ASSERT(cond)`.

## Memory

- `heapAlloc`/`heapRealloc`/`heapFree`, no `malloc`/`new`.
- `getScratch`, `HG_ARENA_SCOPE`, `arenaAlloc` for temp work.
- Arena allocations remain raw (no destructors on reset).

