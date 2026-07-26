#pragma once

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
