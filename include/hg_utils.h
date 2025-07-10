#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vulkan/vulkan_enums.hpp>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using iptr = std::intptr_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using uptr = std::uintptr_t;
using usize = std::size_t;

using f32 = float;
using f64 = double;

namespace hg {

template <typename F> struct DeferInternal {
    F f;
    explicit DeferInternal(F f_) : f(f_) {}
    ~DeferInternal() { f(); }
};

template <typename F> DeferInternal<F> defer_function(F f) { return DeferInternal<F>(f); }

#define DEFER_INTERMEDIATE_1(x, y) x##y
#define DEFER_INTERMEDIATE_2(x, y) DEFER_INTERMEDIATE_1(x, y)
#define DEFER_INTERMEDIATE_3(x) DEFER_INTERMEDIATE_2(x, __COUNTER__)
#define defer(code) auto DEFER_INTERMEDIATE_3(_defer_) = defer_function([&] { code; })

thread_local inline std::vector<std::string> g_stack_context{};
constexpr inline void push_stack_context(std::string&& context) { g_stack_context.emplace_back(std::move(context)); }
constexpr inline void pop_stack_context() { g_stack_context.pop_back(); }

[[noreturn]] inline void error_internal(std::string&& message) {
    std::cerr << "Error: " << std::move(message) << "\n";
    for (auto it = g_stack_context.rbegin(); it != g_stack_context.rend(); ++it) {
        std::cerr << "    Trace: " << *it << "\n";
    }
    std::terminate();
}

inline void warn_internal(std::string&& message) {
    std::cerr << "Warning: " << std::move(message) << "\n";
    if (!g_stack_context.empty())
        std::cerr << "    Trace: " << g_stack_context.back() << "\n";
}

inline void info_internal(std::string&& message) { std::cout << "Info: " << std::move(message) << "\n"; }

#define CONTEXT(message, ...) push_stack_context(std::format(message, __VA_ARGS__)); defer(pop_stack_context());
#define CONTEXT_PUSH(message, ...) push_stack_context(std::format(message, __VA_ARGS__));
#define CONTEXT_POP() defer(pop_stack_context());

#define ERROR(message, ...) error_internal(std::format(message, __VA_ARGS__));
#define WARN(message, ...) warn_internal(std::format(message, __VA_ARGS__));
#define INFO(message, ...) info_internal(std::format(message, __VA_ARGS__));

#define ASSERT(condition) { if (!(condition)) [[unlikely]] ERROR("Assertion failed: {}", #condition); }

constexpr i32 to_i32(const u32 val) {
    ASSERT(static_cast<i32>(val) >= 0);
    return static_cast<i32>(val);
}
constexpr u32 to_u32(const i32 val) {
    ASSERT(val >= 0);
    return static_cast<u32>(val);
}
constexpr u32 to_u32(const usize val) {
    ASSERT(val < UINT32_MAX);
    return static_cast<u32>(val);
}

enum class Err : u8 {
    Unknown = 0,

    // Initialization
    VulkanLayerUnavailable,
    VulkanExtensionUnavailable,
    VulkanFeatureUnavailable,
    VulkanIncompatibleDriver,
    NoCompatibleVkPhysicalDevice,
    VkQueueFamilyUnavailable,
    VkQueueUnavailable,

    // Window
    MonitorUnvailable,
    InvalidWindow,
    FrameTimeout,

    // Resources
    OutOfMemory,
    OutOfDescriptorSets,

    // File
    ShaderFileNotFound,
    ShaderFileInvalid,
    ImageFileNotFound,
    ImageFileInvalid,
    GltfFileNotFound,
    GltfFileInvalid,

    // ... add more as needed
};

#define HG_MAKE_ERROR_STRING(c) case Err :: c : return #c
constexpr std::string_view to_string(Err code) {
    switch (code) {
        HG_MAKE_ERROR_STRING(Unknown);

        // Initialization
        HG_MAKE_ERROR_STRING(VulkanLayerUnavailable);
        HG_MAKE_ERROR_STRING(VulkanExtensionUnavailable);
        HG_MAKE_ERROR_STRING(VulkanFeatureUnavailable);
        HG_MAKE_ERROR_STRING(VulkanIncompatibleDriver);
        HG_MAKE_ERROR_STRING(NoCompatibleVkPhysicalDevice);
        HG_MAKE_ERROR_STRING(VkQueueFamilyUnavailable);
        HG_MAKE_ERROR_STRING(VkQueueUnavailable);

        // Window
        HG_MAKE_ERROR_STRING(MonitorUnvailable);
        HG_MAKE_ERROR_STRING(InvalidWindow);
        HG_MAKE_ERROR_STRING(FrameTimeout);

        // Resources
        HG_MAKE_ERROR_STRING(OutOfMemory);
        HG_MAKE_ERROR_STRING(OutOfDescriptorSets);

        // File
        HG_MAKE_ERROR_STRING(ShaderFileNotFound);
        HG_MAKE_ERROR_STRING(ShaderFileInvalid);
        HG_MAKE_ERROR_STRING(ImageFileNotFound);
        HG_MAKE_ERROR_STRING(ImageFileInvalid);
        HG_MAKE_ERROR_STRING(GltfFileNotFound);
        HG_MAKE_ERROR_STRING(GltfFileInvalid);

        // ... add more as needed
    }
    return "Cannot stringify invalid error code";
}
#undef HG_MAKE_ERROR_STRING

template <typename T> class Result {
public:
    constexpr Result(const Err error) : m_result{error} { WARN("{}", to_string(error)); }
    template <typename... Args> constexpr Result(std::in_place_type_t<T>, Args&&... args) : m_result{std::in_place_type_t<T>(), std::forward<Args>(args)...} {}

    constexpr bool has_err() const { return std::holds_alternative<Err>(m_result); }
    constexpr Err err() const {
        ASSERT(has_err());
        return std::get<Err>(m_result);
    }

    constexpr T& val() & {
        ASSERT(!has_err());
        return std::get<T>(m_result);
    }
    constexpr T&& val() && {
        ASSERT(!has_err());
        return std::move(std::get<T>(m_result));
    }
    constexpr T* operator->() {
        ASSERT(!has_err());
        return &val();
    }
    constexpr T& operator*() & {
        ASSERT(!has_err());
        return val();
    }
    constexpr T&& operator*() && {
        ASSERT(!has_err());
        return std::move(val());
    }

    constexpr const T& val() const& {
        ASSERT(!has_err());
        return std::get<T>(m_result);
    }
    constexpr const T&& val() const&& {
        ASSERT(!has_err());
        return std::move(std::get<T>(m_result));
    }
    constexpr const T* operator->() const {
        ASSERT(!has_err());
        return &val();
    }
    constexpr const T& operator*() const& {
        ASSERT(!has_err());
        return val();
    }
    constexpr const T&& operator*() const&& {
        ASSERT(!has_err());
        return val();
    }

    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) const& { return !has_err() ? **this : static_cast<T>(std::forward<U>(default_value)); }
    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) && { return !has_err() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value)); }

private:
    std::variant<T, Err> m_result{};
};

template <> class Result<void> {
public:
    Result(const Err error) : m_err{error} { WARN("{}", to_string(error)); }
    constexpr Result(std::in_place_type_t<void>) {}

    constexpr bool has_err() const { return m_err.has_value(); }
    constexpr Err err() const {
        ASSERT(has_err());
        return *m_err;
    }

private:
    std::optional<Err> m_err{std::nullopt};
};

constexpr Result<void> ok() { return std::in_place_type_t<void>(); }

template <typename T, typename U = std::remove_cvref_t<T>> constexpr Result<U> ok(T&& val)
requires(std::is_trivially_copyable_v<U> || std::is_rvalue_reference_v<T>) {
    return Result<U>{std::in_place_type_t<U>(), std::forward<T>(val)};
}

template <typename T, typename... Args> constexpr Result<T> ok(Args&&... args) {
    return Result<T>{std::in_place_type_t<T>(), std::forward<Args>(args)...};
}

inline void* align_ptr(void* ptr, const usize alignment) {
    return reinterpret_cast<void*>((reinterpret_cast<usize>(ptr) + alignment - 1) & ~(alignment - 1));
}

inline usize align_size(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

struct Terminate { explicit constexpr Terminate() = default; };
struct ReturnNull { explicit constexpr ReturnNull() = default; };

template <typename T> concept FailurePolicyTag = (
    std::same_as<T, Terminate> ||
    std::same_as<T, ReturnNull>
);

template <typename T> concept Allocator = requires(T t) {
    { t.template alloc<T>(std::declval<usize>()) } -> std::same_as<T*>;
    { t.template realloc<T>(std::declval<T*>(), std::declval<usize>(), std::declval<usize>()) } -> std::same_as<T*>;
    { t.template dealloc<T>(std::declval<T*>(), std::declval<usize>()) } -> std::same_as<void>;
};

template <FailurePolicyTag FailurePolicy>
class CAllocator {
public:
    template <typename T> [[nodiscard]] static T* alloc(const usize count) {
        auto ptr = std::malloc(count * sizeof(T));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                ERROR("Malloc returned null");
        return static_cast<T*>(ptr);
    }
    template <typename T> static T* realloc(T* original_ptr, const usize, const usize new_count) {
        auto ptr = std::realloc(original_ptr, new_count * sizeof(T));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                ERROR("Realloc returned null");
        return static_cast<T*>(ptr);
    }
    template <typename T> static void dealloc(T* ptr, const usize) { std::free(ptr); }
};

template <Allocator Parent, FailurePolicyTag FailurePolicy = Terminate>
class LinearAllocator {
public:
    LinearAllocator() = default;
    LinearAllocator(const usize size) requires std::is_empty_v<Parent>
        : m_memory{Parent::template alloc<std::byte>(size), size}
        , m_head{m_memory.data()}
    {}
    void destroy() const requires std::is_empty_v<Parent> { Parent::dealloc(m_memory.data(), m_memory.size()); }

    LinearAllocator(Parent& parent, const usize size)
        : m_memory{parent.template alloc<std::byte>(size), size}
        , m_head{m_memory.data()}
    {}
    void destroy(Parent& parent) const { parent.dealloc(m_memory.data(), m_memory.size()); }

    template <typename T> [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        T* alloc_ptr = reinterpret_cast<T*>(align_ptr(m_head, alignof(T)));
        std::byte* alloc_end = reinterpret_cast<std::byte*>(alloc_ptr + count);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Linear allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                WARN("Linear allocator out of memory");
                return nullptr;
            }
        }
        m_head = alloc_end;

        return alloc_ptr;
    }

    template <typename T> T* realloc(T* original_ptr, const usize original_count, const usize new_count) {
        ASSERT(original_ptr != nullptr);
        ASSERT(original_count > 0);
        ASSERT(new_count > 0);

        if (original_ptr + original_count != m_head) {
            T* alloc_ptr = alloc<T>(new_count);
            std::memmove(alloc_ptr, original_ptr, original_count * sizeof(T));
            return alloc_ptr;
        }

        std::byte* new_end = reinterpret_cast<std::byte*>(align_ptr(original_ptr + new_count, alignof(T)));
        if (new_end > m_memory.data() + m_memory.size())
            ERROR("Linear allocator out of memory");

        m_head = new_end;
        return original_ptr;
    }

    template <typename T> void dealloc(T*, usize) {}

    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory = {};
    std::byte* m_head = nullptr;
};

template <Allocator Parent, FailurePolicyTag FailurePolicy = Terminate>
class StackAllocator {
public:
    StackAllocator() = default;
    StackAllocator(const usize size) requires std::is_empty_v<Parent>
        : m_memory{Parent::template alloc<std::byte>(size), size}
        , m_head{reinterpret_cast<std::byte*>(align_ptr(m_memory.data(), 16))}
    {}
    void destroy() const requires std::is_empty_v<Parent> { Parent::dealloc(m_memory.data(), m_memory.size()); }

    StackAllocator(Parent& parent, const usize size)
        : m_memory{parent.template alloc<std::byte>(size), size}
        , m_head{reinterpret_cast<std::byte*>(align_ptr(m_memory.data(), 16))}
    {}
    void destroy(Parent& parent) const { parent.dealloc(m_memory.data(), m_memory.size()); }

    template <typename T> [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        T* alloc_ptr = reinterpret_cast<T*>(m_head);
        std::byte* alloc_end = reinterpret_cast<std::byte*>(alloc_ptr + count);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Stack allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                WARN("Stack allocator out of memory");
                return nullptr;
            }
        }
        m_head = reinterpret_cast<std::byte*>(align_ptr(alloc_end, 16));

        return alloc_ptr;
    }

    template <typename T> T* realloc(T* original_ptr, const usize original_count, const usize new_count) {
        ASSERT(original_ptr != nullptr);
        ASSERT(original_count > 0);
        ASSERT(new_count > 0);

        if (reinterpret_cast<std::byte*>(align_ptr(original_ptr + original_count, 16)) != m_head) {
            T* alloc_ptr = alloc<T>(new_count);
            std::memmove(alloc_ptr, original_ptr, original_count * sizeof(T));
            return alloc_ptr;
        }

        std::byte* new_end = reinterpret_cast<std::byte*>(align_ptr(original_ptr + new_count, alignof(T)));
        if (new_end > m_memory.data() + m_memory.size())
            ERROR("Stack allocator out of memory");

        m_head = new_end;
        return original_ptr;
    }

    template <typename T> void dealloc(T* ptr, const usize count = 1) {
        ASSERT(count > 0);

        if (reinterpret_cast<std::byte*>(align_ptr(ptr + count, 16)) != m_head) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Deallocation of invalid pointer from stack allocator");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                WARN("Deallocation of invalid pointer from stack allocator");
                return;
            }
        }

        m_head = ptr;
    }

    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory = {};
    std::byte* m_head = nullptr;
};

template <typename T, Allocator Parent, FailurePolicyTag FailurePolicy = Terminate, bool CheckMemoryLeaks = true>
class PoolAllocator {
private:
    union Slot {
        T data;
        usize next;
    };

public:
    PoolAllocator() = default;
    PoolAllocator(const usize size) requires std::is_empty_v<Parent>
        : m_slots{Parent::template alloc<Slot>(size), size}
    {
        for (usize i = 0; i < size; ++i) {
            m_slots[i].next = i + 1;
        }
    }
    void destroy() const requires std::is_empty_v<Parent> {
        if constexpr (CheckMemoryLeaks) {
            if (m_slots.data() == nullptr)
                return;

            usize count = 0;
            usize index = m_next;
            while (index != m_slots.size() && count <= m_slots.size()) {
                index = m_slots[index].next;
                ++count;
            }
            if (count < m_slots.size())
                ERROR("Pool allocator leaked memory");
            if (count > m_slots.size())
                ERROR("Pool allocator had double frees");
        }
        Parent::dealloc(m_slots.data(), m_slots.size());
    }

    PoolAllocator(Parent& parent, const usize size)
        : m_slots{parent.template alloc<Slot>(size), size}
    {
        for (usize i = 0; i < size; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    void destroy(Parent& parent) const {
        if constexpr (CheckMemoryLeaks) {
            if (m_slots.data() == nullptr)
                return;

            usize count = 0;
            usize index = m_next;
            while (index != m_slots.size() && count <= m_slots.size()) {
                index = m_slots[index].next;
                ++count;
            }
            if (count < m_slots.size())
                ERROR("Pool allocator leaked memory");
            if (count > m_slots.size())
                ERROR("Pool allocator had double frees");
        }
        parent.dealloc(m_slots.data(), m_slots.size());
    }

    [[nodiscard]] T* alloc_move(T&& resource)
    requires(std::is_trivially_copyable_v<T> || std::is_rvalue_reference_v<T>) {
        return new (get_next_resource()) T(std::move(resource));
    }

    template <typename... Args> [[nodiscard]] T* alloc_new(Args&&... args) {
        return new (get_next_resource()) T(std::forward<Args>(args)...);
    }

    void dealloc(T* resource) {
        if (resource == nullptr)
            return;

        if constexpr (std::is_destructible_v<T>)
            resource->~T();

        Slot* slot = reinterpret_cast<Slot*>(resource);
        const usize index = static_cast<usize>(slot - m_slots.data());
        slot->next = m_next;
        m_next = index;
    }

private:
    T* get_next_resource() {
        usize index = m_next;
        if (index >= m_slots.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Resource pool out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                WARN("Resource pool out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    std::span<Slot> m_slots{};
    usize m_next = 0;
};

class Clock {
public:
    void update() {
        const auto now = std::chrono::high_resolution_clock::now();
        m_delta = now - m_previous;
        m_previous = now;
    }

    [[nodiscard]] constexpr f64 delta_sec() const { return static_cast<f64>(m_delta.count()) / 1'000'000'000.0; }

private:
    std::chrono::high_resolution_clock::time_point m_previous{std::chrono::high_resolution_clock::now()};
    std::chrono::nanoseconds m_delta{0};
};

class Timer {
public:
    void start() { m_begin = std::chrono::high_resolution_clock::now(); }

    void stop(const std::string_view message = "Timer stopped") const {
        const auto end = std::chrono::high_resolution_clock::now();
        std::cout << std::format("{}: {}ms\n", message, static_cast<f64>((end - m_begin).count()) / 1'000'000.0);
    }

private:
    std::chrono::high_resolution_clock::time_point m_begin{std::chrono::high_resolution_clock::now()};
};

} // namespace hg
