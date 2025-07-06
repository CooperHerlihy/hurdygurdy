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
#define CONTEXT_POP(message, ...) defer(pop_stack_context());

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

    // Vulkan
    CouldNotAllocateVkDescriptorSets,

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
        HG_MAKE_ERROR_STRING(MonitorUnvailable);

        HG_MAKE_ERROR_STRING(InvalidWindow);
        HG_MAKE_ERROR_STRING(FrameTimeout);

        HG_MAKE_ERROR_STRING(CouldNotAllocateVkDescriptorSets);

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

constexpr Result<void> vk_success_or_error(const vk::Result result) {
    switch (result) {
    case vk::Result::eSuccess:
        return ok();
    case vk::Result::eNotReady:
        ERROR("Vulkan not ready");
    case vk::Result::eTimeout:
        ERROR("Vulkan timeout");
    case vk::Result::eEventSet:
        ERROR("Vulkan event set");
    case vk::Result::eEventReset:
        ERROR("Vulkan event reset");
    case vk::Result::eIncomplete:
        ERROR("Vulkan incomplete");
    case vk::Result::eErrorOutOfHostMemory:
        ERROR("Vulkan ran out of host memory");
    case vk::Result::eErrorOutOfDeviceMemory:
        ERROR("Vulkan ran out of device memory");
    case vk::Result::eErrorInitializationFailed:
        ERROR("Vulkan initialization failed");
    case vk::Result::eErrorDeviceLost:
        ERROR("Vulkan device lost");
    case vk::Result::eErrorMemoryMapFailed:
        ERROR("Vulkan memory map failed");
    case vk::Result::eErrorLayerNotPresent:
        ERROR("Vulkan layer not present");
    case vk::Result::eErrorExtensionNotPresent:
        ERROR("Vulkan extension not present");
    case vk::Result::eErrorFeatureNotPresent:
        ERROR("Vulkan feature not present");
    case vk::Result::eErrorIncompatibleDriver:
        ERROR("Vulkan incompatible driver");
    case vk::Result::eErrorTooManyObjects:
        ERROR("Vulkan too many objects");
    case vk::Result::eErrorFormatNotSupported:
        ERROR("Vulkan format not supported");
    case vk::Result::eErrorFragmentedPool:
        ERROR("Vulkan fragmented pool");
    case vk::Result::eErrorUnknown:
        ERROR("Vulkan unknown");
    case vk::Result::eErrorOutOfPoolMemory:
        ERROR("Vulkan out of pool memory");
    case vk::Result::eErrorInvalidExternalHandle:
        ERROR("Vulkan invalid external handle");
    case vk::Result::eErrorFragmentation:
        ERROR("Vulkan fragmentation");
    case vk::Result::eErrorInvalidDeviceAddressEXT:
        ERROR("Vulkan invalid device address");
    case vk::Result::eErrorPipelineCompileRequiredEXT:
        ERROR("Vulkan pipeline compile required");
    case vk::Result::eErrorNotPermitted:
        ERROR("Vulkan not permitted");
    case vk::Result::eErrorSurfaceLostKHR:
        ERROR("Vulkan surface lost");
    case vk::Result::eErrorNativeWindowInUseKHR:
        ERROR("Vulkan native window in use");
    case vk::Result::eSuboptimalKHR:
        ERROR("Vulkan suboptimal");
    case vk::Result::eErrorOutOfDateKHR:
        ERROR("Vulkan out of date");
    case vk::Result::eErrorIncompatibleDisplayKHR:
        ERROR("Vulkan incompatible display");
    case vk::Result::eErrorValidationFailedEXT:
        ERROR("Vulkan validation failed");
    case vk::Result::eErrorInvalidShaderNV:
        ERROR("Vulkan invalid shader");
    case vk::Result::eErrorImageUsageNotSupportedKHR:
        ERROR("Vulkan image usage not supported");
    case vk::Result::eErrorVideoPictureLayoutNotSupportedKHR:
        ERROR("Vulkan video picture layout not supported");
    case vk::Result::eErrorVideoProfileOperationNotSupportedKHR:
        ERROR("Vulkan video profile operation not supported");
    case vk::Result::eErrorVideoProfileFormatNotSupportedKHR:
        ERROR("Vulkan video profile format not supported");
    case vk::Result::eErrorVideoProfileCodecNotSupportedKHR:
        ERROR("Vulkan video profile codec not supported");
    case vk::Result::eErrorVideoStdVersionNotSupportedKHR:
        ERROR("Vulkan video std version not supported");
    case vk::Result::eErrorInvalidDrmFormatModifierPlaneLayoutEXT:
        ERROR("Vulkan invalid drm format modifier plane layout");
    case vk::Result::eThreadIdleKHR:
        ERROR("Vulkan thread idle");
    case vk::Result::eThreadDoneKHR:
        ERROR("Vulkan thread done");
    case vk::Result::eOperationDeferredKHR:
        ERROR("Vulkan operation deferred");
    case vk::Result::eOperationNotDeferredKHR:
        ERROR("Vulkan operation not deferred");
    case vk::Result::eErrorInvalidVideoStdParametersKHR:
        ERROR("Vulkan invalid video std parameters");
    case vk::Result::eErrorCompressionExhaustedEXT:
        ERROR("Vulkan compression exhausted");
    case vk::Result::eErrorIncompatibleShaderBinaryEXT:
        ERROR("Vulkan incompatible shader binary");
    case vk::Result::ePipelineBinaryMissingKHR:
        ERROR("Vulkan pipeline binary missing");
    case vk::Result::eErrorNotEnoughSpaceKHR:
        ERROR("Vulkan not enough space");
    default:
        ERROR("Unknown Vulkan error");
    }
}

} // namespace hg
