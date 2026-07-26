#pragma once

void logError();

#define HG_MACRO_CONCAT_INTERNAL(x, y) x##y

#define HG_MACRO_CONCAT(x, y) HG_MACRO_CONCAT_INTERNAL(x, y)

template<typename F> requires std::is_invocable_r_v<void, F>
struct Defer {
    F fn;
    Defer(F fnVal) : fn(fnVal) {}
    ~Defer()
    {
        fn();
    }
};

#define HG_DEFER(...) [[maybe_unused]] hg::Defer HG_MACRO_CONCAT(defer_, __LINE__){[&]{ __VA_ARGS__ ;}};

#ifdef HG_LOGGING

#define HG_LOG(...) do { std::fprintf(stderr, "HurdyGurdy Log: " __VA_ARGS__); } while(0)

#define HG_WARN(...) do { std::fprintf(stderr, "HurdyGurdy Warn: " __VA_ARGS__); } while(0)

#define HG_PANIC(...) do { logError(); std::fprintf(stderr, "HurdyGurdy Panic: " __VA_ARGS__); std::abort(); \
} while(0)

#else

#define HG_LOG(...) do {} while(0)
#define HG_WARN(...) do {} while(0)
#define HG_PANIC(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        HG_PANIC("Assertion failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif