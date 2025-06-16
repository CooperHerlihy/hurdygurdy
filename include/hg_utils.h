#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using iptr = intptr_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using uptr = uintptr_t;
using usize = size_t;

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

} // namespace hg
