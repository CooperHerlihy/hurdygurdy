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

#ifdef NDEBUG
#define debug_assert(condition) ((void)0)
#else
#define debug_assert(condition)                                                                        \
if (!(condition)) {                                                                                    \
    std::cerr << std::format("Failed debug assert: {}\n    {}, {}\n", #condition, __FILE__, __LINE__); \
    std::terminate();                                                                                  \
}
#endif

#define critical_assert(condition)                                                                        \
if (!(condition)) {                                                                                       \
    std::cerr << std::format("Failed critical assert: {}\n    {}, {}\n", #condition, __FILE__, __LINE__); \
    std::terminate();                                                                                     \
}

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
    std::chrono::high_resolution_clock::time_point m_previous = std::chrono::high_resolution_clock::now();
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
    std::chrono::high_resolution_clock::time_point m_begin = std::chrono::high_resolution_clock::now();
};

enum class Err : u8 {
    Unknown = 0,

    ImageFileNotFound,
    ImageFileInvalid,
    GltfFileNotFound,
    GltfFileInvalid,

    VkInstanceExtensionsUnavailable,
    VkInstanceExtensionPropertiesUnavailable,
    CouldNotCreateVkInstance,
    CouldNotCreateVkDebugUtilsMessenger,
    VkPhysicalDevicesUnavailable,
    VkPhysicalDevicesUnsuitable,
    VkPhysicalDevicePropertiesUnavailable,
    CouldNotCreateVkDevice,
    VkQueueFamilyUnavailable,
    VkQueueUnavailable,
    CouldNotCreateVmaAllocator,
    CouldNotCreateVkCommandPool,

    CouldNotCreateGlfwWindow,
    CouldNotCreateGlfwVkSurface,
    CouldNotAllocateVkCommandBuffers,
    CouldNotCreateVkFence,
    CouldNotCreateVkSemaphore,
    VkSurfaceCapabilitiesUnavailable,
    VkSurfaceInvalidSize,
    VkSurfacePresentModesUnavailable,
    CouldNotCreateVkSwapchain,
    VkSwapchainImagesUnavailable,
    CouldNotCreateSwapchainImageView,

    // ... add more as needed
};

#define HG_MAKE_ERROR_STRING(c) case Err :: c : return #c
constexpr std::string_view err_to_string(Err code) {
    switch (code) {
        HG_MAKE_ERROR_STRING(Unknown);

        HG_MAKE_ERROR_STRING(ImageFileNotFound);
        HG_MAKE_ERROR_STRING(ImageFileInvalid);
        HG_MAKE_ERROR_STRING(GltfFileNotFound);
        HG_MAKE_ERROR_STRING(GltfFileInvalid);

        HG_MAKE_ERROR_STRING(VkInstanceExtensionsUnavailable);
        HG_MAKE_ERROR_STRING(VkInstanceExtensionPropertiesUnavailable);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkInstance);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDebugUtilsMessenger);
        HG_MAKE_ERROR_STRING(VkPhysicalDevicesUnavailable);
        HG_MAKE_ERROR_STRING(VkPhysicalDevicesUnsuitable);
        HG_MAKE_ERROR_STRING(VkPhysicalDevicePropertiesUnavailable);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDevice);
        HG_MAKE_ERROR_STRING(VkQueueFamilyUnavailable);
        HG_MAKE_ERROR_STRING(VkQueueUnavailable);
        HG_MAKE_ERROR_STRING(CouldNotCreateVmaAllocator);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkCommandPool);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkFence);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkSemaphore);

        HG_MAKE_ERROR_STRING(CouldNotCreateGlfwWindow);
        HG_MAKE_ERROR_STRING(CouldNotCreateGlfwVkSurface);
        HG_MAKE_ERROR_STRING(CouldNotAllocateVkCommandBuffers);
        HG_MAKE_ERROR_STRING(VkSurfaceCapabilitiesUnavailable);
        HG_MAKE_ERROR_STRING(VkSurfaceInvalidSize);
        HG_MAKE_ERROR_STRING(VkSurfacePresentModesUnavailable);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkSwapchain);
        HG_MAKE_ERROR_STRING(VkSwapchainImagesUnavailable);
        HG_MAKE_ERROR_STRING(CouldNotCreateSwapchainImageView);
    }
    debug_assert(false);
}
#undef HG_MAKE_ERROR_STRING

template <typename T> class Result {
public:
    constexpr Result(const Err error) : m_result{error} {}
    template <typename... Args> constexpr Result(std::in_place_type_t<T>, Args&&... args) : m_result{std::in_place_type_t<T>(), std::forward<Args>(args)...} {}

    constexpr bool has_err() const { return std::holds_alternative<Err>(m_result); }
    constexpr Err err() const { return std::get<Err>(m_result); }

    constexpr T& val() & { return std::get<T>(m_result); }
    constexpr T&& val() && { return std::move(std::get<T>(m_result)); }
    constexpr T* operator->() { return &val(); }
    constexpr T& operator*() & { return val(); }
    constexpr T&& operator*() && { return std::move(val()); }

    constexpr const T& val() const& { return std::get<T>(m_result); }
    constexpr const T&& val() const&& { return std::move(std::get<T>(m_result)); }
    constexpr const T* operator->() const { return &val(); }
    constexpr const T& operator*() const& { return val(); }
    constexpr const T&& operator*() const&& { return val(); }

    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) const& { return !has_err() ? **this : static_cast<T>(std::forward<U>(default_value)); }
    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) && { return !has_err() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value)); }

private:
    std::variant<T, Err> m_result = {};
};

template <> class Result<void> {
public:
    constexpr Result(const Err error) : m_err{error} {}
    constexpr Result(std::in_place_type_t<void>) {}

    constexpr bool has_err() const { return m_err.has_value(); }
    constexpr Err err() const { return *m_err; }

private:
    std::optional<Err> m_err = std::nullopt;
};

constexpr Result<void> ok() { return std::in_place_type_t<void>(); }

template <typename T, typename U = std::remove_cvref_t<T>> constexpr Result<U> ok(T&& val)
requires(std::is_trivially_copyable_v<U> || std::is_rvalue_reference_v<T>) {
    return Result<U>{std::in_place_type_t<U>(), std::forward<T>(val)};
}

template <typename T, typename... Args> constexpr Result<T> ok(Args&&... args) {
    return Result<T>{std::in_place_type_t<T>(), std::forward<Args>(args)...};
}

} // namespace hg
