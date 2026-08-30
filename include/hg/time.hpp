#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"
#include "hg/map.hpp"

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
    static inline Map<String, f64> timers{};
    /**
     * The counter registry
     */
    static inline Map<String, u32> counters{};

    /**
     * Iterate over timers
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const String&, f64>
    static void forEachTimer(F fn)
    {
        timers.forEach(fn);
    }

    /**
     * Iterate over timers
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const String&, u32>
    static void forEachCounter(F fn)
    {
        counters.forEach(fn);
    }

    /**
     * Clear the global registry
     */
    static void clear()
    {
        timers.reset();
        counters.reset();
    }

    /**
     * Add to a counter
     */
    static void event(StringView name)
    {
        u32* counter = counters.get(name);
        if (counter == nullptr)
            counter = counters.add(String::create(name), 0);

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
    ProfilerScopeTimer(StringView name)
        : ScopeTimer{Profiler::timers.get(name)}
    {
        if (out == nullptr)
            out = Profiler::timers.add(String::create(name), 0.0f);
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
    StringView name{};

    /**
     * Begin a scope timer
     *
     * Note, the name should be alive for the whole scope
     */
    ScopeTimerLog(StringView nameVal)
        : timer{}, name{nameVal}
    {}

    /**
     * Record the time to stdout
     */
    ~ScopeTimerLog()
    {
        HG_LOG("Scope \"%.*s\": %fms\n", (int)name.length, name.chars, timer.tick() * 1.e3f);
    }

    ScopeTimerLog(const ScopeTimerLog& other) = delete;
    ScopeTimerLog& operator=(const ScopeTimerLog& other) = delete;
    ScopeTimerLog(ScopeTimerLog&&) = delete;
    ScopeTimerLog& operator=(ScopeTimerLog&&) = delete;
};

/**
 * A simple performance measurement tool
 */
struct Perf {
    /**
     * The clock to keep track of each time
     */
    Clock clock = {};
    /**
     * The measured time for each iteration
     */
    f64* times = nullptr;
    /**
     * The max number of measurements
     */
    u32 count = 0;
    /**
     * The current measurement
     */
    u32 current = 0;
};

/**
 * Create a performance measurer
 */
Perf perfCreate(Arena* arena, u32 count);

/**
 * Begin the timer for a measurement
 */
void perfBegin(Perf* perf);

/**
 * End the timer for a measurement
 *
 * Returns
 * - The time this measurement took
 */
f64 perfEnd(Perf* perf);

/**
 * A set of statistics from performance measurements
 */
struct PerfStats {
    /**
     * The average time of all measurements
     */
    f64 avg = 0.0;
    /**
     * The best case (the shortest time)
     */
    f64 best = 0.0;
    /**
     * The worst case (the longest time)
     */
    f64 worst = 0.0;
};

/**
 * Analyzes the performance measurements for statistics
 */
PerfStats perfAnalyze(const Perf* perf);

/**
 * The scale to log performance metrics at
 */
enum PerfScale : u32 {
    PerfScale_seconds,
    PerfScale_milli,
    PerfScale_micro,
    PerfScale_nano,
};

/**
 * Logs performance statistics to stdout
 */
void perfLog(StringView title, const PerfStats* stats, PerfScale scale);

} // namespace hg

