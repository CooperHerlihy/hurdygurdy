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
    for (auto it = g_stack_context.rbegin(); it != g_stack_context.rend(); ++it) {
        std::cerr << "    Trace: " << *it << "\n";
    }
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

template <typename T>
inline T* align_ptr(void* ptr) {
    return reinterpret_cast<T*>((reinterpret_cast<usize>(ptr) + alignof(T) - 1) & ~(alignof(T) - 1));
}

template <typename T> concept Allocator = requires(T t) {
    { t.get() } -> std::convertible_to<void*>;
    { t.alloc(std::declval<usize>()) } -> std::same_as<std::span<T>>;
    { t.dealloc(std::declval<void*>()) } -> std::same_as<void>;
};

struct ReturnNull { explicit constexpr ReturnNull() = default; };
struct FatalError { explicit constexpr FatalError() = default; };

template <typename FailurePolicy = FatalError> class LinearAllocator {
    static_assert(std::is_same_v<FailurePolicy, FatalError> || std::is_same_v<FailurePolicy, ReturnNull>);
public:
    void* get() const { return m_memory.data(); }

    LinearAllocator() = default;
    LinearAllocator(std::byte* memory, usize size) : m_memory(memory, size), m_head(memory) {}
    ~LinearAllocator() noexcept = default;

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&& other) noexcept : m_memory(other.m_memory), m_head(other.m_head) {
        other.m_memory = {};
        other.m_head = nullptr;
    }
    LinearAllocator& operator=(LinearAllocator&& other) noexcept {
        if (this == &other)
            return *this;
        this->~LinearAllocator();
        new (this) LinearAllocator(std::move(other));
        return *this;
    }

    template <typename T> [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        T* ptr = align_ptr<T>(m_head);
        std::byte* alloc_end = reinterpret_cast<std::byte*>(ptr + count) + sizeof(std::byte*);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::is_same_v<FailurePolicy, FatalError>) {
                ERROR("Linear allocator out of memory");
            } else if constexpr (std::is_same_v<FailurePolicy, ReturnNull>) {
                WARN("Linear allocator out of memory");
                return nullptr;
            }
        }
        m_head = ptr + count;
        return ptr;
    }

    void dealloc(void*) {}
    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory;
    void* m_head = nullptr;
};

template <typename FailurePolicy = FatalError> class StackAllocator {
    static_assert(std::is_same_v<FailurePolicy, FatalError> || std::is_same_v<FailurePolicy, ReturnNull>);
public:
    void* get() const { return m_memory.data(); }

    StackAllocator() = default;
    StackAllocator(std::byte* memory, usize size) : m_memory(memory, size), m_head(memory) {}
    ~StackAllocator() noexcept = default;

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&& other) noexcept : m_memory(other.m_memory), m_head(other.m_head) {
        other.m_memory = {};
        other.m_head = nullptr;
    }
    StackAllocator& operator=(StackAllocator&& other) noexcept {
        if (this == &other)
            return *this;
        this->~StackAllocator();
        new (this) StackAllocator(std::move(other));
        return *this;
    }

    template <typename T> [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        T* result_ptr = align_ptr<T>(m_head);
        std::byte** metadata_ptr = align_ptr<std::byte*>(result_ptr + count);

        std::byte* alloc_end = reinterpret_cast<std::byte*>(metadata_ptr + 1);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::is_same_v<FailurePolicy, FatalError>) {
                ERROR("Stack allocator out of memory");
            } else if constexpr (std::is_same_v<FailurePolicy, ReturnNull>) {
                WARN("Stack allocator out of memory");
                return nullptr;
            }
        }

        *metadata_ptr = reinterpret_cast<std::byte*>(m_head);
        m_head = alloc_end;

        return result_ptr;
    }

    void dealloc(void*) {
        if (m_head > m_memory.data())
            m_head = *(reinterpret_cast<std::byte**>(m_head) - 1);
    }
    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory;
    std::byte* m_head = nullptr;
};

template <typename T, bool CheckMemoryLeaks = true> class PoolAllocator {
private:
    union Slot {
        T data;
        usize next;
    };

public:
    void* get() const { return m_slots.data(); }

    PoolAllocator() = default;
    PoolAllocator(T* memory, usize size) requires(sizeof(T) >= sizeof(Slot)) :
        m_slots(reinterpret_cast<Slot*>(memory), size)
    {
        for (usize i = 0; i < size; ++i) {
            m_slots[i].next = i + 1;
        }
    }
    PoolAllocator(usize* memory, usize size) requires(sizeof(T) < sizeof(Slot)) :
        m_slots(reinterpret_cast<Slot*>(memory), size)
    {
        for (usize i = 0; i < size; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    ~PoolAllocator() noexcept requires(!CheckMemoryLeaks) = default;
    ~PoolAllocator() noexcept requires CheckMemoryLeaks {
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

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&& other) noexcept :
        m_slots(other.m_slots),
        m_next(other.m_next)
    {
        other.m_slots = {};
        other.m_next = 0;
    }
    PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this == &other)
            return *this;
        this->~PoolAllocator();
        new (this) PoolAllocator(std::move(other));
        return *this;
    }

    [[nodiscard]] T* create(T&& resource)
    requires(std::is_trivially_copyable_v<T> || std::is_rvalue_reference_v<T>) {
        return new (get_next_resource()) T(std::move(resource));
    }

    template <typename... Args> [[nodiscard]] T* create(Args&&... args) {
        return new (get_next_resource()) T(std::forward<Args>(args)...);
    }

    void destroy(T* resource) {
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
            WARN("Resource pool out of memory");
            return nullptr;
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    std::span<Slot> m_slots{};
    usize m_next = 0;
};

} // namespace hg
