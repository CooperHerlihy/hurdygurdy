#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <chrono>

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

using byte = std::byte;

using f32 = std::float_t;
using f64 = std::double_t;

namespace hg {

template <typename Func> class DeferInternal {
public:
    explicit DeferInternal(Func func) : m_func(func) {}
    ~DeferInternal() { m_func(); }
private:
    Func m_func;
};

template <typename Func> DeferInternal<Func> defer_function(Func f) { return DeferInternal<Func>(f); }

#define DEFER_INTERMEDIATE_1(x, y) x##y
#define DEFER_INTERMEDIATE_2(x, y) DEFER_INTERMEDIATE_1(x, y)
#define DEFER_INTERMEDIATE_3(x) DEFER_INTERMEDIATE_2(x, __COUNTER__)
#define defer(code) auto DEFER_INTERMEDIATE_3(_defer_) = defer_function([&] { code; })

enum class LogLevel { Info, Warning, Error };

inline std::string_view to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Info: return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
        default: return "Invalid log level";
    }
}

#define LOG(level, message) {                                                                                       \
    std::fprintf(stderr, "%s: %s : %d %s(): %s\n", to_string(level).data(), __FILE__, __LINE__, __func__, message); \
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
    return "Invalid error code";
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
    T* data = nullptr;
    usize count = 0;

    T& operator[](const usize index) const {
        ASSERT(index < count);
        return data[index];
    }
};

template <typename T> T* begin(const Slice<T> slice) { return slice.data; }
template <typename T> T* end(const Slice<T> slice) { return slice.data + slice.count; }

inline constexpr usize align_up(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
inline constexpr usize align_down(const usize size, const usize alignment) {
    return size & ~(alignment - 1);
}

template <typename T> class Pool {
public:
    struct Handle { usize index = SIZE_MAX; };

    union Block {
        T data;
        usize next;
    };

    Pool() = default;
    Pool(Slice<Block> memory) : m_blocks{memory} {
        for (usize i = 0; i < m_blocks.count; ++i) {
            m_blocks[i].next = i + 1;
        }
    }
    Slice<Block> release() {
        Slice<Block> memory{m_blocks};
        m_blocks = Slice<Block>{};
        m_next = 0;
        return memory;
    }

    [[nodiscard]] static Pool create(const usize count) {
        Slice<Block> memory{reinterpret_cast<Block*>(std::malloc(sizeof(Block) * count)), count};
        if (memory.data == nullptr)
            ERROR("Could not allocate memory");
        return Pool{memory};
    }
    void destroy() { std::free(m_blocks.data); }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    Pool(Pool&& other) noexcept
        : m_blocks{other.m_blocks}
        , m_next{other.m_next}
    {
        other.m_blocks = Slice<Block>{};
        other.m_next = 0;
    }
    Pool& operator=(Pool&& other) noexcept {
        if (this == &other)
            return *this;
        if (m_blocks.data != nullptr)
            ERROR("Occupied pool cannot be moved into");
        new (this) Pool(std::move(other));
        return *this;
    }

    [[nodiscard]] T& operator[](Handle handle) {
        ASSERT(handle.index < m_blocks.count);
        return m_blocks[handle.index].data;
    }

    [[nodiscard]] Handle alloc() {
        usize index = m_next;
        if (index >= m_blocks.count)
            ERROR("Pool out of memory");

        m_next = m_blocks[index].next;
        return {index};
    }

    void dealloc(Handle handle) {
        if (handle.index >= m_blocks.count)
            ERROR("Pool cannot deallocate block outside of pool");

        m_blocks[handle.index].next = m_next;
        m_next = handle.index;
    }

    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_blocks.count && count <= m_blocks.count) {
            index = m_blocks[index].next;
            ++count;
        }
        if (count < m_blocks.count)
            ERROR("Pool leaked memory");
        if (count > m_blocks.count)
            ERROR("Pool had double frees");
#endif
    }

private:
    Slice<Block> m_blocks{};
    usize m_next = 0;
};

class Arena {
public:
    Arena() = default;
    Arena(Slice<byte> memory) : m_memory{memory}, m_head{0} {}
    [[nodiscard]] Slice<byte> release() {
        Slice<byte> memory{m_memory};
        m_memory = Slice<byte>{};
        m_head = 0;
        return memory;
    }

    [[nodiscard]] static Arena create(const usize count) {
        Slice<byte> memory{reinterpret_cast<byte*>(std::malloc(count)), count};
        if (memory.data == nullptr)
            ERROR("Could not allocate memory");
        return Arena{memory};
    }
    void destroy() { std::free(m_memory.data); }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept
        : m_memory{other.m_memory}
        , m_head{other.m_head}
    {
        other.m_memory = Slice<byte>{};
        other.m_head = 0;
    }
    Arena& operator=(Arena&& other) noexcept {
        if (this == &other)
            return *this;
        if (m_memory.data != nullptr)
            ERROR("Occupied arena cannot be moved into");
        new (this) Arena(std::move(other));
        return *this;
    }

    [[nodiscard]] Slice<byte> alloc(usize size) {
        Slice<byte> allocation{m_memory.data + m_head, align_up(size, 16)};
        if (end(allocation) > end(m_memory))
            ERROR("Arena out of memory");
        m_head = end(allocation) - m_memory.data;

        return allocation;
    }

    template <typename T> [[nodiscard]] T* alloc() {
        return reinterpret_cast<T*>(alloc(sizeof(T)).data);
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        return {reinterpret_cast<T*>(alloc(count * sizeof(T)).data), count};
    }

    [[nodiscard]] Slice<byte> realloc(Slice<byte> original, usize new_size) {
        ASSERT(original.data != nullptr);

        if (end(original) != m_memory.data + m_head)
            ERROR("Arena can only reallocate top allocation");

        Slice<byte> allocation{original.data, align_up(new_size, 16)};
        if (end(allocation) > end(m_memory))
            ERROR("Arena out of memory");
        m_head = end(allocation) - m_memory.data;

        return allocation;
    }

    template <typename T> [[nodiscard]] Slice<T> realloc(Slice<T> original, usize new_count) {
        return {reinterpret_cast<T*>(realloc(
            Slice<byte>{reinterpret_cast<byte*>(original.data), align_up(original.count * sizeof(T), 16)},
            new_count * sizeof(T)
        ).data), new_count};
    }

    void dealloc(Slice<byte> allocation) {
        ASSERT(allocation.data != nullptr);

        if (end(allocation) != m_memory.data + m_head)
            ERROR("Arena can only deallocate top allocation");

        m_head = allocation.data - m_memory.data;
    }

    template <typename T> void dealloc(T* ptr) {
        dealloc({reinterpret_cast<byte*>(ptr), align_up(sizeof(T), 16)});
    }
    template <typename T> void dealloc(Slice<T> slice) {
        dealloc({reinterpret_cast<byte*>(slice.data), align_up(slice.count * sizeof(T), 16)});
    }

private:
    Slice<byte> m_memory{};
    usize m_head = 0;
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
        std::printf("%s: %fms\n", message.data(), static_cast<f64>((end - m_begin).count()) / 1'000'000.0);
    }

private:
    std::chrono::high_resolution_clock::time_point m_begin{std::chrono::high_resolution_clock::now()};
};

} // namespace hg
