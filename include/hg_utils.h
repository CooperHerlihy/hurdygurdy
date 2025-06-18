#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
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
#define debug_assert(condition)                                                                                                                                                    \
    if (!(condition)) {                                                                                                                                                            \
        std::cerr << std::format("Failed debug assert: {}\n    {}, {}\n", #condition, __FILE__, __LINE__);                                                                         \
        __debugbreak();                                                                                                                                                            \
        std::terminate();                                                                                                                                                          \
    }
#endif

#define critical_assert(condition)                                                                                                                                                 \
    if (!(condition)) {                                                                                                                                                            \
        std::cerr << std::format("Failed critical assert: {}\n    {}, {}\n", #condition, __FILE__, __LINE__);                                                                      \
        __debugbreak();                                                                                                                                                            \
        std::terminate();                                                                                                                                                          \
    }

template <typename F> struct DeferInternal {
    F f;
    explicit DeferInternal(F f_) : f(f_) {}
    ~DeferInternal() { f(); }
};

template <typename F> DeferInternal<F> defer_function(F f) {
    return DeferInternal<F>(f);
}

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

template <typename T> class Vec {
public:
    template <typename... Args> constexpr Vec(Args&&... args) : m_vec{std::forward<Args>(args)...} {}

    constexpr Vec clone() const { return Vec{std::vector{m_vec}}; }

    constexpr ~Vec() = default;
    Vec(const Vec&) = delete;
    Vec& operator=(const Vec&) = delete;
    constexpr Vec(Vec&&) = default;
    constexpr Vec& operator=(Vec&&) = default;

    constexpr std::strong_ordering operator<=>(const Vec&) const = default;

    constexpr bool empty() const { return m_vec.empty(); }
    constexpr usize size() const { return m_vec.size(); }
    constexpr usize capacity() const { return m_vec.capacity(); }
    constexpr usize max_size() const { return m_vec.max_size(); }

    constexpr T& operator[](usize index) {
        debug_assert(index < size());
        return m_vec[index];
    }
    constexpr T& at(usize index) { return m_vec.at(index); }
    constexpr T& front() { return m_vec.front(); }
    constexpr T& back() { return m_vec.back(); }
    constexpr T* data() { return m_vec.data(); }
    constexpr std::vector<T>& get() { return m_vec; }

    constexpr const T& operator[](usize index) const {
        debug_assert(index < size());
        return m_vec[index];
    }
    constexpr const T& at(usize index) const { return m_vec.at(index); }
    constexpr const T& front() const { return m_vec.front(); }
    constexpr const T& back() const { return m_vec.back(); }
    constexpr const T* data() const { return m_vec.data(); }
    constexpr const std::vector<T>& get() const { return m_vec; }

    constexpr void resize(const usize new_size) { m_vec.resize(new_size); }
    constexpr void clear() { m_vec.clear(); }
    constexpr void reserve(const usize new_size) { m_vec.reserve(new_size); }
    constexpr void shrink_to_fit() { m_vec.shrink_to_fit(); }
    constexpr void push_back(const T& val) { m_vec.emplace_back(val); }
    constexpr void push_back(T&& val) { m_vec.emplace_back(std::move(val)); }
    constexpr void pop_back() { m_vec.pop_back(); }
    template <typename... Args> constexpr void emplace_back(Args&&... args) { m_vec.emplace_back(std::forward<Args>(args)...); }

    constexpr auto begin() { return m_vec.begin(); }
    constexpr auto end() { return m_vec.end(); }
    constexpr auto begin() const { return m_vec.begin(); }
    constexpr auto end() const { return m_vec.end(); }

    operator std::span<T>() { return m_vec; }
    operator std::span<const T>() const { return m_vec; }

private:
    std::vector<T> m_vec = {};
};

class Error {
public:
    enum Code {
        Unknown,
        UnknownVulkan,
        OutOfMemory,
        OutOfGpuMemory,
        FileNotFound,
        FileInvalid,
        // etc. add as needed
    };

    Code code = Unknown;
    std::string message = {};

    Error(const Code code, const std::string_view error, const std::string_view context) : code{code}, message{std::format("Error: {}, while {}", error, context)} {}
    Error(Error&& error, const std::string_view context) : code{error.code}, message{std::move(error.message)} { append_context(context); }
    void append_context(const std::string_view context) { message += std::format(", while {}", context); }

    constexpr Error() = default;
    constexpr ~Error() = default;
    Error(const Error&) = delete;
    Error& operator=(const Error&) = delete;
    constexpr Error(Error&&) = default;
    constexpr Error& operator=(Error&&) = default;
};

template <typename T> class Result {
public:
    template <typename... Args> static constexpr Result emplace_ok(Args&&... args) { return {std::in_place_type_t<T>(), std::forward<Args>(args)...}; }
    template <typename... Args> static constexpr Result emplace_err(Args&&... args) { return {std::in_place_type_t<Error>(), std::forward<Args>(args)...}; }

    constexpr ~Result() = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    constexpr Result(Result&&) = default;
    constexpr Result& operator=(Result&&) = default;

    constexpr bool has_err() const { return std::holds_alternative<Error>(m_result); }
    constexpr Error& get_err() & { return std::get<Error>(m_result); }
    constexpr Error&& get_err() && { return std::move(std::get<Error>(m_result)); }
    constexpr const Error& get_err() const& { return std::get<Error>(m_result); }
    constexpr const Error&& get_err() const&& { return std::move(std::get<Error>(m_result)); }

    constexpr bool has_val() const { return std::holds_alternative<T>(m_result); }
    constexpr T& get_val() & { return std::get<T>(m_result); }
    constexpr T&& get_val() && { return std::move(std::get<T>(m_result)); }
    constexpr const T& get_val() const& { return std::get<T>(m_result); }
    constexpr const T&& get_val() const&& { return std::move(std::get<T>(m_result)); }

    constexpr T& operator*() & { return get_val(); }
    constexpr T&& operator*() && { return std::move(get_val()); }
    constexpr const T& operator*() const& { return get_val(); }
    constexpr const T&& operator*() const&& { return get_val(); }

    constexpr T* operator->() { return &get_val(); }
    constexpr const T* operator->() const { return &get_val(); }

    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) const& { return has_val() ? **this : static_cast<T>(std::forward<U>(default_value)); }
    template <typename U = std::remove_cv_t<T>> constexpr T val_or(U&& default_value) && { return has_val() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value)); }

private:
    template <typename... Args> constexpr Result(Args&&... args) : m_result{std::forward<Args>(args)...} {}

    std::variant<T, Error> m_result = {};
};

template <> class Result<void> {
public:
    template <typename... Args> static constexpr Result emplace_ok(Args&&... args) { return {std::in_place_type_t<void>(), std::forward<Args>(args)...}; }
    template <typename... Args> static constexpr Result emplace_err(Args&&... args) { return {std::in_place_type_t<Error>(), std::forward<Args>(args)...}; }

    constexpr ~Result() = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    constexpr Result(Result&&) = default;
    constexpr Result& operator=(Result&&) = default;

    constexpr bool has_err() const { return m_err.has_value(); }
    constexpr Error& get_err() & { return *m_err; }
    constexpr Error&& get_err() && { return std::move(*m_err); }
    constexpr const Error& get_err() const& { return *m_err; }
    constexpr const Error&& get_err() const&& { return std::move(*m_err); }

private:
    template <typename... Args> constexpr Result(std::in_place_type_t<void>, Args&&...) {}
    template <typename... Args> constexpr Result(std::in_place_type_t<Error>, Args&&... args) : m_err{{std::forward<Args>(args)...}} {}

    std::optional<Error> m_err = std::nullopt;
};

template <typename T = void, typename... Args> constexpr Result<T> ok(Args&&... args) {
    return Result<T>::emplace_ok(std::forward<Args>(args)...);
}

template <typename T = void> constexpr Result<T> err(Error&& error) {
    return Result<T>::emplace_err(std::move(error));
}
template <typename T = void> constexpr Result<T> err(Error&& error, const std::string_view context) {
    return Result<T>::emplace_err(std::move(error), context);
}
template <typename T = void> constexpr Result<T> err(const Error::Code code, const std::string_view error, const std::string_view context) {
    return Result<T>::emplace_err(code, error, context);
}

template <typename T> constexpr Result<T> err(Result<T>& error, const std::string_view context) {
    return Result<T>::emplace_err(std::move(error.get_err()), context);
}
template <typename T, typename U>
constexpr Result<T> err(Result<U>& error, const std::string_view context)
    requires(!std::is_same_v<T, U>)
{
    return Result<T>::emplace_err(std::move(error.get_err()), context);
}

#define HG_MAKE_CASE(e)                                                                                                                                                            \
    case Error::e:                                                                                                                                                                 \
        return Result<T>::emplace_err((Error::e), #e " error", context)

template <typename T = void> constexpr Result<T> err(const Error::Code code, const std::string_view context) {
    switch (code) {
        HG_MAKE_CASE(Unknown);
        HG_MAKE_CASE(UnknownVulkan);
        HG_MAKE_CASE(OutOfMemory);
        HG_MAKE_CASE(OutOfGpuMemory);
        HG_MAKE_CASE(FileNotFound);
        HG_MAKE_CASE(FileInvalid);
    }
    debug_assert(false);
}

#undef HG_MAKE_CASE

#define HG_MAKE_CASE_WITH_CODE(e, c)                                                                                                                                               \
    case vk::Result::e:                                                                                                                                                            \
        return Result<T>::emplace_err((Error::c), "Vulkan " #e " error", context)

#define HG_MAKE_CASE(e) HG_MAKE_CASE_WITH_CODE(e, UnknownVulkan)

template <typename T = void> constexpr Result<T> err(const vk::Result result, const std::string_view context) {
    switch (result) {
        HG_MAKE_CASE_WITH_CODE(eErrorOutOfHostMemory, OutOfMemory);
        HG_MAKE_CASE_WITH_CODE(eErrorOutOfDeviceMemory, OutOfGpuMemory);

        HG_MAKE_CASE(eSuccess);
        HG_MAKE_CASE(eNotReady);
        HG_MAKE_CASE(eTimeout);
        HG_MAKE_CASE(eEventSet);
        HG_MAKE_CASE(eEventReset);
        HG_MAKE_CASE(eIncomplete);
        HG_MAKE_CASE(eErrorInitializationFailed);
        HG_MAKE_CASE(eErrorDeviceLost);
        HG_MAKE_CASE(eErrorMemoryMapFailed);
        HG_MAKE_CASE(eErrorLayerNotPresent);
        HG_MAKE_CASE(eErrorExtensionNotPresent);
        HG_MAKE_CASE(eErrorFeatureNotPresent);
        HG_MAKE_CASE(eErrorIncompatibleDriver);
        HG_MAKE_CASE(eErrorTooManyObjects);
        HG_MAKE_CASE(eErrorFormatNotSupported);
        HG_MAKE_CASE(eErrorFragmentedPool);
        HG_MAKE_CASE(eErrorUnknown);
        HG_MAKE_CASE(eErrorOutOfPoolMemory);
        HG_MAKE_CASE(eErrorInvalidExternalHandle);
        HG_MAKE_CASE(eErrorFragmentation);
        HG_MAKE_CASE(eErrorInvalidDeviceAddressEXT);
        HG_MAKE_CASE(eErrorPipelineCompileRequiredEXT);
        HG_MAKE_CASE(eErrorNotPermitted);
        HG_MAKE_CASE(eErrorSurfaceLostKHR);
        HG_MAKE_CASE(eErrorNativeWindowInUseKHR);
        HG_MAKE_CASE(eSuboptimalKHR);
        HG_MAKE_CASE(eErrorOutOfDateKHR);
        HG_MAKE_CASE(eErrorIncompatibleDisplayKHR);
        HG_MAKE_CASE(eErrorValidationFailedEXT);
        HG_MAKE_CASE(eErrorInvalidShaderNV);
        HG_MAKE_CASE(eErrorImageUsageNotSupportedKHR);
        HG_MAKE_CASE(eErrorVideoPictureLayoutNotSupportedKHR);
        HG_MAKE_CASE(eErrorVideoProfileOperationNotSupportedKHR);
        HG_MAKE_CASE(eErrorVideoProfileFormatNotSupportedKHR);
        HG_MAKE_CASE(eErrorVideoProfileCodecNotSupportedKHR);
        HG_MAKE_CASE(eErrorVideoStdVersionNotSupportedKHR);
        HG_MAKE_CASE(eErrorInvalidDrmFormatModifierPlaneLayoutEXT);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        HG_MAKE_CASE(eErrorFullScreenExclusiveModeLostEXT);
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
        HG_MAKE_CASE(eThreadIdleKHR);
        HG_MAKE_CASE(eThreadDoneKHR);
        HG_MAKE_CASE(eOperationDeferredKHR);
        HG_MAKE_CASE(eOperationNotDeferredKHR);
        HG_MAKE_CASE(eErrorInvalidVideoStdParametersKHR);
        HG_MAKE_CASE(eErrorCompressionExhaustedEXT);
        HG_MAKE_CASE(eErrorIncompatibleShaderBinaryEXT);
        HG_MAKE_CASE(ePipelineBinaryMissingKHR);
        HG_MAKE_CASE(eErrorNotEnoughSpaceKHR);
    }
    return Result<T>::emplace_err(Error::UnknownVulkan, "Vulkan unknown vk::Result value", context);
}

#undef HG_MAKE_CASE
#undef HG_MAKE_CASE_WITH_CODE

} // namespace hg
