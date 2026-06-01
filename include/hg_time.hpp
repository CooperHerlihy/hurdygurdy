/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_TIME_HPP
#define HG_TIME_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_containers.hpp"

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock {
    /**
     * The begin time
     */
    f64 time;
};

/**
 * Resets the clock
 *
 * Returns
 * - The time in seconds since the last tick
 */
f64 hgClockTick(HgClock* clock);

/**
 * A simple performance measurement tool
 */
struct HgPerf {
    /**
     * The clock to keep track of each time
     */
    HgClock clock;
    /**
     * The measured time for each iteration
     */
    f64* times;
    /**
     * The max number of measurements
     */
    u32 count;
    /**
     * The current measurement
     */
    u32 current;
};

/**
 * Create a performance measurer
 */
HgPerf hgPerfCreate(HgArena* arena, u32 count);

/**
 * Begin the timer for a measurement
 */
void hgPerfBegin(HgPerf* perf);

/**
 * End the timer for a measurement
 *
 * Returns
 * - The time this measurement took
 */
f64 hgPerfEnd(HgPerf* perf);

/**
 * A set of statistics from performance measurements
 */
struct HgPerfStats {
    /**
     * The average time of all measurements
     */
    f64 avg;
    /**
     * The best case (the shortest time)
     */
    f64 best;
    /**
     * The worst case (the longest time)
     */
    f64 worst;
};

/**
 * Analyzes the performance measurements for statistics
 */
HgPerfStats hgPerfAnalyze(const HgPerf* perf);

/**
 * The scale to log performance metrics at
 */
enum HgPerfScale : u32 {
    HgPerfScale_seconds,
    HgPerfScale_milli,
    HgPerfScale_micro,
    HgPerfScale_nano,
};

/**
 * Logs performance statistics to stdout
 */
void hgPerfLog(HgStringView title, const HgPerfStats* stats, HgPerfScale scale);

#endif // HG_TIME_HPP
