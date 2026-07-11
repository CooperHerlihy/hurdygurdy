#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_time.hpp"

#include <thread>
#include <chrono>

#include <cstdio>
#include <ctime>

namespace hg {

f64 clockTick(Clock* clock)
{
    HG_ASSERT(clock != nullptr);

    f64 prev = clock->time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    clock->time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
    return clock->time - prev;
}

void sleep(f64 time)
{
    std::this_thread::sleep_for(std::chrono::duration<f64>(time));
}

Perf perfCreate(Arena* arena, u32 count)
{
    HG_ASSERT(arena != nullptr);

    Perf perf;
    perf.times = arenaAlloc<f64>(arena, count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void perfBegin(Perf* perf)
{
    HG_ASSERT(perf != nullptr);
    clockTick(&perf->clock);
}

f64 perfEnd(Perf* perf)
{
    HG_ASSERT(perf != nullptr);
    HG_ASSERT(perf->current < perf->count);

    f64 time = clockTick(&perf->clock);
    perf->times[perf->current++] = time;

    return time;
}

PerfStats perfAnalyze(const Perf* perf)
{
    HG_ASSERT(perf != nullptr);

    PerfStats stats;
    stats.avg = 0.0;
    stats.best = INFINITY;
    stats.worst = 0.0;

    for (u32 i = 0; i < perf->current; ++i)
    {
        if (perf->times[i] < stats.best)
            stats.best = perf->times[i];
        if (perf->times[i] > stats.worst)
            stats.worst = perf->times[i];
        stats.avg += perf->times[i];
    }
    stats.avg /= (f64)perf->current;

    return stats;
}

void perfLog(String title, const PerfStats* stats, PerfScale scale)
{
    HG_ASSERT(stats != nullptr);
    if (title.length == 0 || title.chars == nullptr)
        title = "Title Missing";

    switch (scale)
    {
        case PerfScale_seconds:
            printf("HG Performance - %.*s: avg: %.4fs, best: %.4fs, worst: %.4fs\n",
                (int)title.length, title.chars, stats->avg, stats->best, stats->worst);
            break;
        case PerfScale_milli:
            printf("HG Performance - %.*s: avg: %.4fms, best: %.4fms, worst: %.4fms\n",
                (int)title.length, title.chars, stats->avg * 1.e3, stats->best * 1.e3, stats->worst * 1.e3);
            break;
        case PerfScale_micro:
            printf("HG Performance - %.*s: avg: %.4fmcs, best: %.4fmcs, worst: %.4fmcs\n",
                (int)title.length, title.chars, stats->avg * 1.e6, stats->best * 1.e6, stats->worst * 1.e6);
            break;
        case PerfScale_nano:
            printf("HG Performance - %.*s: avg: %.4fns, best: %.4fns, worst: %.4fns\n",
                (int)title.length, title.chars, stats->avg * 1.e9, stats->best * 1.e9, stats->worst * 1.e9);
            break;
    }
}

} // namespace hg

