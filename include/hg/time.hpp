#pragma once

#include "hg/inttypes.hpp"
#include "hg/macros.hpp"
#include "hg/hash.hpp"
// #include "hg/memory.hpp"

namespace hg {

/**
 * A high precision clock for timers and game deltas
 */
struct Clock {
    /**
     * The begin time
     */
    f64 time = 0.0;

    /**
     * Begin the clock;
     */
    Clock() noexcept
    {
        tick();
    }

    /**
     * Resets the clock and returns the time in seconds since the last tick
     */
    f64 tick();
};

/**
 * Put this thread to sleep
 *
 * Parameters
 * - time The time in seconds to sleep for
 */
void sleep(f64 time);

/**
 * Global registries for profiling
 */
struct Profiler {
    /**
     * The timer registry
     */
    static inline HashMap<std::string, f64> timers{};
    /**
     * The counter registry
     */
    static inline HashMap<std::string, u32> counters{};

    /**
     * Iterate over timers
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const std::string&, f64>
    static void forEachTimer(F fn)
    {
        timers.forEach(fn);
    }

    /**
     * Iterate over timers
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const std::string&, u32>
    static void forEachCounter(F fn)
    {
        counters.forEach(fn);
    }

    /**
     * Clear the global registry
     */
    static void clear()
    {
        timers.clear();
        counters.clear();
    }

    /**
     * Add to a counter
     */
    static void event(std::string_view name)
    {
        u32* counter = counters.get(name);
        if (counter == nullptr)
            counter = counters.add(std::string{name}, 0);

        ++*counter;
    }
};

/**
 * Time a scope, and set the value at the end
 */
struct ScopeTimer {
    /**
     * The timer for the scope
     */
    Clock timer{};
    /**
     * Where to store the result
     */
    f64* out = nullptr;

    /**
     * Begin a scope timer
     */
    ScopeTimer(f64* outVal)
        : timer{}, out{outVal}
    {}

    /**
     * Record the time
     */
    ~ScopeTimer()
    {
        if (out != nullptr)
            *out += timer.tick();
    }

    ScopeTimer(const ScopeTimer& other) = delete;
    ScopeTimer& operator=(const ScopeTimer& other) = delete;
    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer& operator=(ScopeTimer&&) = delete;
};

/**
 * Time a scope and add it to the global registry
 */
struct ProfilerScopeTimer : public ScopeTimer {
    /**
     * Add a new timer to the name in the registry
     */
    ProfilerScopeTimer(std::string_view name)
        : ScopeTimer{Profiler::timers.get(name)}
    {
        if (out == nullptr)
            out = Profiler::timers.add(std::string{name}, 0.0f);
    }
};

/**
 * Time a scope and log to stdout
 */
struct ScopeTimerLog {
    /**
     * The timer for the scope
     */
    Clock timer{};
    /**
     * Where to store the result
     */
    std::string_view name{};

    /**
     * Begin a scope timer
     *
     * Note, the name should be alive for the whole scope
     */
    ScopeTimerLog(std::string_view nameVal)
        : timer{}, name{nameVal}
    {}

    /**
     * Record the time to stdout
     */
    ~ScopeTimerLog()
    {
        HG_LOG("Scope \"{}\": {}ms\n", name, timer.tick() * 1.e3f);
    }

    ScopeTimerLog(const ScopeTimerLog& other) = delete;
    ScopeTimerLog& operator=(const ScopeTimerLog& other) = delete;
    ScopeTimerLog(ScopeTimerLog&&) = delete;
    ScopeTimerLog& operator=(ScopeTimerLog&&) = delete;
};

} // namespace hg

