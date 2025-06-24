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

#ifdef NDEBUG

inline void hg_assert_internal(const std::string_view message) {
    std::cerr << std::format("Failed assertion: {}\n", message);
    std::terminate();
}

#define ASSERT(condition) { if (!(condition)) [[unlikely]] hg_assert_internal(#condition); }

#else

inline void hg_assert_internal(const std::string_view message, const std::string_view file, const int line) {
    std::cerr << std::format("Failed assertion: {}\n    File: {}\n    Line: {}\n", message, file, line);
    std::terminate();
}

#define ASSERT(condition) { if (!(condition)) [[unlikely]] hg_assert_internal(#condition, __FILE__, __LINE__); }

#endif

inline void hg_error_internal(const std::string_view message, const std::string_view file, const int line) {
    std::cerr << std::format("Critical error: {}\n    File: {}\n    Line: {}\n", message, file, line);
    std::terminate();
}

#define ERROR(message) { hg_error_internal(message, __FILE__, __LINE__); }

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

    // Initialization
    GlfwFailure,
    CouldNotInitializeGlfw,
    VulkanFailure,
    VulkanExtensionsUnavailable,
    VkPhysicalDevicesUnavailable,
    VkPhysicalDevicesUnsuitable,
    VkQueueFamilyUnavailable,
    VkQueueUnavailable,
    VkSwapchainImagesUnavailable,
    InvalidWindowSize,

    // Vulkan
    CouldNotCreateVkInstance,
    CouldNotCreateVkDebugUtilsMessenger,
    CouldNotCreateVkDevice,
    CouldNotCreateVmaAllocator,
    CouldNotCreateVkCommandPool,
    CouldNotAllocateVkCommandBuffers,
    CouldNotCreateVkFence,
    CouldNotCreateVkSemaphore,
    CouldNotCreateVkSwapchain,
    CouldNotCreateVkDescriptorSetLayout,
    CouldNotCreateVkPipelineLayout,
    CouldNotCreateVkShader,
    CouldNotCreateVkDescriptorPool,
    CouldNotAllocateVkDescriptorSets,
    CouldNotCreateVkSampler,

    CouldNotCreateGpuBuffer,
    CouldNotWriteGpuBuffer,
    CouldNotCreateGpuImage,
    CouldNotCreateGpuImageView,
    CouldNotWriteGpuImage,
    CouldNotGenerateMipmaps,

    CouldNotBeginVkCommandBuffer,
    CouldNotEndVkCommandBuffer,
    CouldNotSubmitVkCommandBuffer,
    CouldNotAcquireVkSwapchainImage,
    CouldNotPresentVkSwapchainImage,
    CouldNotWaitForVkFence,
    CouldNotWaitForVkQueue,
    CouldNotWaitForVkDevice,

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
        HG_MAKE_ERROR_STRING(GlfwFailure);
        HG_MAKE_ERROR_STRING(CouldNotInitializeGlfw);
        HG_MAKE_ERROR_STRING(VulkanFailure);
        HG_MAKE_ERROR_STRING(VulkanExtensionsUnavailable);
        HG_MAKE_ERROR_STRING(VkPhysicalDevicesUnavailable);
        HG_MAKE_ERROR_STRING(VkPhysicalDevicesUnsuitable);
        HG_MAKE_ERROR_STRING(VkQueueFamilyUnavailable);
        HG_MAKE_ERROR_STRING(VkQueueUnavailable);
        HG_MAKE_ERROR_STRING(VkSwapchainImagesUnavailable);
        HG_MAKE_ERROR_STRING(InvalidWindowSize);

        // Vulkan
        HG_MAKE_ERROR_STRING(CouldNotCreateVkInstance);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDebugUtilsMessenger);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDevice);
        HG_MAKE_ERROR_STRING(CouldNotCreateVmaAllocator);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkCommandPool);
        HG_MAKE_ERROR_STRING(CouldNotAllocateVkCommandBuffers);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkFence);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkSemaphore);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkSwapchain);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDescriptorSetLayout);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkPipelineLayout);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkShader);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkDescriptorPool);
        HG_MAKE_ERROR_STRING(CouldNotAllocateVkDescriptorSets);
        HG_MAKE_ERROR_STRING(CouldNotCreateVkSampler);

        HG_MAKE_ERROR_STRING(CouldNotCreateGpuBuffer);
        HG_MAKE_ERROR_STRING(CouldNotWriteGpuBuffer);
        HG_MAKE_ERROR_STRING(CouldNotCreateGpuImage);
        HG_MAKE_ERROR_STRING(CouldNotCreateGpuImageView);
        HG_MAKE_ERROR_STRING(CouldNotWriteGpuImage);
        HG_MAKE_ERROR_STRING(CouldNotGenerateMipmaps);

        HG_MAKE_ERROR_STRING(CouldNotBeginVkCommandBuffer);
        HG_MAKE_ERROR_STRING(CouldNotEndVkCommandBuffer);
        HG_MAKE_ERROR_STRING(CouldNotSubmitVkCommandBuffer);
        HG_MAKE_ERROR_STRING(CouldNotAcquireVkSwapchainImage);
        HG_MAKE_ERROR_STRING(CouldNotPresentVkSwapchainImage);
        HG_MAKE_ERROR_STRING(CouldNotWaitForVkFence);
        HG_MAKE_ERROR_STRING(CouldNotWaitForVkQueue);
        HG_MAKE_ERROR_STRING(CouldNotWaitForVkDevice);

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
    constexpr Result(const Err error) : m_result{error} {}
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
    std::variant<T, Err> m_result = {};
};

template <> class Result<void> {
public:
    constexpr Result(const Err error) : m_err{error} {}
    constexpr Result(std::in_place_type_t<void>) {}

    constexpr bool has_err() const { return m_err.has_value(); }
    constexpr Err err() const {
        ASSERT(has_err());
        return *m_err;
    }

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
