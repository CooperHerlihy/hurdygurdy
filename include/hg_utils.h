#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

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

enum class LogLevel { Info, Warning, Error };

inline const char* to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Info: return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
        default: return "Invalid log level";
    }
}

#define LOG(level, message) {                                                                                \
    std::fprintf(stderr, "%s: %s : %d %s(): %s\n", to_string(level), __FILE__, __LINE__, __func__, message); \
}
#define LOG_INFO(message) { LOG(LogLevel::Info, message) }
#define LOG_WARN(message) { LOG(LogLevel::Warning, message) }
#define LOG_ERROR(message) { LOG(LogLevel::Error, message) }

#define LOGF(level, message, ...) { LOG(level, std::format(message, __VA_ARGS__).c_str()) }
#define LOGF_INFO(message, ...) { LOGF(LogLevel::Info, message, __VA_ARGS__) }
#define LOGF_WARN(message, ...) { LOGF(LogLevel::Warning, message, __VA_ARGS__) }
#define LOGF_ERROR(message, ...) { LOGF(LogLevel::Error, message, __VA_ARGS__) }

#define ERROR(message, ...) { LOG_ERROR(message); terminate(); }
#define ERRORF(message, ...) { LOGF_ERROR(message, __VA_ARGS__); terminate(); }

#ifdef NDEBUG
#define ASSERT(condition) ((void)0)
#else
#define ASSERT(condition) { if (!(condition)) [[unlikely]] ERROR("Assertion failed: " #condition); }
#endif

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
    constexpr Result(const Err error) : m_err{error} {}
    template <typename... Args> constexpr Result(std::in_place_type_t<T>, Args&&... args)
        : m_ok{std::forward<Args>(args)...}
        , m_is_ok{true}
    {}

    ~Result() {
        if constexpr (std::is_destructible_v<T>)
            if (m_is_ok)
                m_ok.~T();
    }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    constexpr Result(Result&& other) noexcept
        : m_err{other.m_err}
        , m_is_ok{other.m_is_ok}
    {
        if (m_is_ok)
            new (&m_ok) T{std::move(other.m_ok)};
    }
    constexpr Result& operator=(Result&& other) noexcept {
        if (this == &other)
            return *this;
        new (this) Result(std::move(other));
        if (m_is_ok)
            new (&m_ok) T{std::move(other.m_ok)};
        return *this;
    }

    constexpr bool has_err() const { return !m_is_ok; }
    constexpr Err err() const {
        ASSERT(has_err());
        return m_err;
    }

    constexpr T& val() & {
        ASSERT(!has_err());
        return m_ok;
    }
    constexpr T&& val() && {
        ASSERT(!has_err());
        return std::move(m_ok);
    }
    constexpr T* operator->() {
        ASSERT(!has_err());
        return &m_ok;
    }
    constexpr T& operator*() & {
        ASSERT(!has_err());
        return m_ok;
    }
    constexpr T&& operator*() && {
        ASSERT(!has_err());
        return std::move(m_ok);
    }

    constexpr const T& val() const& {
        ASSERT(!has_err());
        return m_ok;
    }
    constexpr const T&& val() const&& {
        ASSERT(!has_err());
        return std::move(m_ok);
    }
    constexpr const T* operator->() const {
        ASSERT(!has_err());
        return &m_ok;
    }
    constexpr const T& operator*() const& {
        ASSERT(!has_err());
        return m_ok;
    }
    constexpr const T&& operator*() const&& {
        ASSERT(!has_err());
        return m_ok;
    }

    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) const& {
        return !has_err() ? **this : static_cast<T>(std::forward<U>(default_value));
    }
    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) && {
        return !has_err() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

private:
    union {
        T m_ok;
        Err m_err = Err::Unknown;
    };
    bool m_is_ok = false;
};

template <> class Result<void> {
public:
    constexpr Result(const Err error) : m_err{error} {}
    constexpr Result(std::in_place_type_t<void>) : m_is_ok{true} {}

    constexpr bool has_err() const { return !m_is_ok; }
    constexpr Err err() const {
        if (!has_err())
            ERROR("Result does not have an error");
        return m_err;
    }

private:
    Err m_err = Err::Unknown;
    bool m_is_ok = false;
};

constexpr Result<void> ok() { return std::in_place_type_t<void>(); }

template <typename T, typename U = std::remove_cvref_t<T>> constexpr Result<U> ok(T&& val)
requires(std::is_trivially_copyable_v<U> || std::is_rvalue_reference_v<T>) {
    return Result<U>{std::in_place_type_t<U>(), std::forward<T>(val)};
}

template <typename T, typename... Args> constexpr Result<T> ok(Args&&... args) {
    return Result<T>{std::in_place_type_t<T>(), std::forward<Args>(args)...};
}

template <typename T> struct Slice {
    T* data;
    usize count;

    T& operator[](const usize index) const { return data[index]; }

    T* begin() const { return data; }
    T* end() const { return data + count; }
};

class Allocator {
public:
    virtual ~Allocator() = default;

    [[nodiscard]] virtual void* alloc_v(usize size, usize alignment) = 0;
    [[nodiscard]] virtual void* realloc_v(void* original, usize original_size, usize new_size, usize alignment) = 0;
    virtual void dealloc_v(void* ptr, usize size, usize alignment) = 0;

    template <typename T> [[nodiscard]] T* alloc() {
        return static_cast<T*>(alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        ASSERT(count > 0);
        return {static_cast<T*>(alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] Slice<T> realloc(Slice<T> original, usize new_count) {
        return {static_cast<T*>(realloc_v(original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T))), new_count};
    }
    template <typename T> void dealloc(T* ptr) {
        dealloc_v(ptr, sizeof(T), alignof(T));
    }
    template <typename T> void dealloc(Slice<T> slice) {
        dealloc_v(slice.data, slice.count * sizeof(T), alignof(T));
    }
};

class AllocContext {
public:
    Allocator* persistent_storage{};
    Allocator* stack_storage{};
};

struct Terminate { explicit constexpr Terminate() = default; };
struct ReturnNull { explicit constexpr ReturnNull() = default; };

template <typename T> concept FailurePolicyTag = (
    std::same_as<T, Terminate> ||
    std::same_as<T, ReturnNull>
);

inline constexpr usize align_size(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
inline std::byte* align_ptr(void* ptr, const usize alignment) {
    return reinterpret_cast<std::byte*>(align_size(reinterpret_cast<usize>(ptr), alignment));
}
template <typename T> T* align_ptr(T* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_size(reinterpret_cast<usize>(ptr), alignof(T)));
}

template <FailurePolicyTag FailurePolicy = Terminate>
class CAllocator : public Allocator {
public:
    void* alloc_v(usize size, usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        auto ptr = std::malloc(align_size(size, alignment));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                ERROR("Malloc returned null");
        return ptr;
    }
    void* realloc_v(void* original, usize original_size, usize new_size, usize alignment) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);
        ASSERT(alignment > 0);

        auto ptr = std::realloc(original, align_size(new_size, alignment));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                ERROR("Realloc returned null");
        return ptr;
    }
    void dealloc_v(void* ptr, usize size, usize alignment) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::free(ptr);
    }

    static CAllocator& instance() {
        static CAllocator instance{};
        return instance;
    }

    template <typename T> [[nodiscard]] static T* alloc() {
        return static_cast<T*>(instance().alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] static Slice<T> alloc(usize count) {
        ASSERT(count > 0);
        return {static_cast<T*>(instance().alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] static Slice<T> realloc(Slice<T> original, usize new_count) {
        return {static_cast<T*>(instance().realloc_v(original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T))), new_count};
    }
    template <typename T> static void dealloc(T* ptr) {
        instance().dealloc_v(ptr, sizeof(T), alignof(T));
    }
    template <typename T> static void dealloc(Slice<T> slice) {
        instance().dealloc_v(slice.data, slice.count * sizeof(T), alignof(T));
    }
};

inline CAllocator<>& mallocator() { return CAllocator<>::instance(); }

template <FailurePolicyTag FailurePolicy = Terminate>
class LinearAllocator : public Allocator {
public:
    LinearAllocator() = default;
    LinearAllocator(Slice<std::byte> memory) : m_memory{memory}, m_head{memory.data} {}

    static LinearAllocator create(Allocator& parent, const usize size) { return parent.alloc<std::byte>(size); }
    void destroy(Allocator& parent) const { parent.dealloc(m_memory); }

    [[nodiscard]] void* alloc_v(const usize size, const usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::byte* alloc_ptr = align_ptr(m_head, alignment);
        std::byte* alloc_end = alloc_ptr + align_size(size, alignment);
        if (alloc_end > m_memory.end()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Linear allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Linear allocator out of memory");
                return nullptr;
            }
        }
        m_head = alloc_end;

        return alloc_ptr;
    }

    void* realloc_v(void* original, const usize original_size, const usize new_size, const usize alignment) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);
        ASSERT(alignment > 0);

        std::byte* original_ptr = static_cast<std::byte*>(original);

        if (align_ptr(original_ptr + original_size, alignment) != m_head) {
            void* alloc_ptr = alloc_v(new_size, alignment);
            std::memmove(alloc_ptr, original_ptr, original_size);
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original_ptr + new_size, alignment);
        if (new_end > m_memory.end())
            ERROR("Linear allocator out of memory");

        m_head = new_end;
        return original;
    }

    void dealloc_v(void*, usize, usize) override {}

    void reset() { m_head = m_memory.data; }

private:
    Slice<std::byte> m_memory{};
    std::byte* m_head = nullptr;
};

template <FailurePolicyTag FailurePolicy = Terminate>
class StackAllocator : public Allocator {
public:
    StackAllocator() = default;
    StackAllocator(Slice<std::byte> memory) : m_memory{memory}, m_head{align_ptr(memory.data, 16)} {}

    static StackAllocator create(Allocator& parent, const usize size) { return parent.alloc<std::byte>(size); }
    void destroy(Allocator& parent) const { parent.dealloc(m_memory); }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(size > 0);

        std::byte* alloc_ptr = m_head;
        std::byte* alloc_end = alloc_ptr + align_size(size, 16);
        if (alloc_end > m_memory.end()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Stack allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Stack allocator out of memory");
                return nullptr;
            }
        }
        m_head = align_ptr(alloc_end, 16);

        return alloc_ptr;
    }

    [[nodiscard]] void* realloc_v(void* original, const usize original_size, const usize new_size, const usize) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);

        std::byte* original_ptr = static_cast<std::byte*>(original);

        if (align_ptr(original_ptr + original_size, 16) != m_head) {
            void* alloc_ptr = alloc_v(new_size, 16);
            std::memmove(alloc_ptr, original, align_size(original_size, 16));
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original_ptr + new_size, 16);
        if (new_end > m_memory.end())
            ERROR("Stack allocator out of memory");

        m_head = new_end;
        return original;
    }

    void dealloc_v(void* ptr, const usize size, const usize) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);

        std::byte* ptr_byte = static_cast<std::byte*>(ptr);

        if (align_ptr(ptr_byte + size, 16) != m_head) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Deallocation of invalid pointer from stack allocator");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Deallocation of invalid pointer from stack allocator");
                return;
            }
        }

        m_head = ptr_byte;
    }

    void reset() { m_head = m_memory.data; }

private:
    Slice<std::byte> m_memory{};
    std::byte* m_head = nullptr;
};

template <usize Size, FailurePolicyTag FailurePolicy = Terminate>
class FixedSizeAllocator : public Allocator {
private:
    union Slot {
        std::byte data[align_size(Size, 16)];
        usize next;
    };

public:
    FixedSizeAllocator() = default;
    FixedSizeAllocator(Slice<Slot> slots) : m_slots{slots} {
        for (usize i = 0; i < m_slots.count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static FixedSizeAllocator create(Allocator& parent, const usize count) { return parent.alloc<Slot>(count); }
    void destroy(Allocator& parent) const {
        ASSERT(m_slots.data != nullptr);
        check_leaks();
        parent.dealloc(m_slots);
    }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(align_size(size, 16) <= Size);

        usize index = m_next;
        if (index >= m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Fixed size allocator out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    [[nodiscard]] void* realloc_v(void* original_ptr, const usize, const usize new_size, const usize) override {
        ASSERT(original_ptr != nullptr);
        ASSERT(align_size(new_size, 16) <= Size);
        return original_ptr;
    }

    void dealloc_v(void* ptr, const usize, const usize) override {
        ASSERT(ptr != nullptr);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        const usize index = static_cast<usize>(slot - m_slots.data);
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_slots.count && count <= m_slots.size()) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator leaked memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Fixed size allocator leaked memory");
        }
        if (count > m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator had double frees");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Fixed size allocator had double frees");
        }
#endif
    }

    Slice<Slot> m_slots{};
    usize m_next = 0;
};

template <typename T, FailurePolicyTag FailurePolicy = Terminate>
class PoolAllocator {
private:
    union Slot {
        std::byte data[align_size(sizeof(T), alignof(T))];
        usize next;
    };

public:
    PoolAllocator() = default;
    PoolAllocator(Slice<Slot> slots) : m_slots{slots} {
        for (usize i = 0; i < m_slots.count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static PoolAllocator create(Allocator& parent, const usize count) { return parent.alloc<Slot>(count); }
    void destroy(Allocator& parent) const {
        ASSERT(m_slots.data != nullptr);
        check_leaks();
        parent.dealloc(m_slots);
    }

    [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        usize index = m_next;
        if (index >= m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Resource pool out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Resource pool out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    void dealloc(T* ptr) {
        ASSERT(ptr != nullptr);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        const usize index = static_cast<usize>(slot - m_slots.data);
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_slots.count && count <= m_slots.count) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Pool allocator leaked memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Pool allocator leaked memory");
        }
        if (count > m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Pool allocator had double frees");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Pool allocator had double frees");
        }
#endif
    }

    Slice<Slot> m_slots{};
    usize m_next = 0;
};

} // namespace hg
