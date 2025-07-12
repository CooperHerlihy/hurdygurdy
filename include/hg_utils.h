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

enum class LogLevel {
    Info,
    Warning,
    Error,
};

inline const char* to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Info: return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
    }
    return "Invalid log level";
}

struct LogDetails {
    const char* file = nullptr;
    usize line = 0;
    const char* function = nullptr;
    LogLevel level = LogLevel::Info;
};

inline void log(const LogDetails& details, const char* message) {
    std::fprintf(
        stderr,
        "%s: %s : %llu %s(): %s\n",
        to_string(details.level),
        details.file,
        details.line,
        details.function,
        message
    );
}


#define LOG(level, message) { log({ __FILE__, __LINE__, __func__, level }, message); }
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

class Allocator {
public:
    virtual ~Allocator() = default;

    [[nodiscard]] virtual void* alloc_v(usize size, usize alignment) = 0;
    [[nodiscard]] virtual void* realloc_v(void* original, usize original_size, usize new_size, usize alignment) = 0;
    virtual void dealloc_v(void* ptr, usize size, usize alignment) = 0;

    template <typename T> [[nodiscard]] T* alloc(usize count = 1) {
        return static_cast<T*>(alloc_v(count * sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] T* realloc(T* original, usize original_count, usize new_count) {
        return static_cast<T*>(realloc_v(original, original_count * sizeof(T), new_count * sizeof(T), alignof(T)));
    }
    template <typename T> void dealloc(T* ptr, usize count = 1) {
        dealloc_v(ptr, count * sizeof(T), alignof(T));
    }
};

class AllocatorContext {
public:
    Allocator* persistent_storage{};
    Allocator* stack_storage{};
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
    constexpr Result(const Err error) : m_result{error} { LOGF_WARN("{}", to_string(error)); }
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
    Result(const Err error) : m_err{error} { LOGF_WARN("{}", to_string(error)); }
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
