#include "tests.hpp"
#include "hg/time.hpp"

using namespace hg;

TEST(testClockTickReturnsElapsed)
{
    Clock clock{};
    f64 d = clock.tick();
    ASSERT(d >= 0.0);
    ASSERT(d < 1.0);
}

TEST(testClockTickMonotonic)
{
    Clock clock{};
    f64 total = 0.0;
    for (u32 i = 0; i < 10; ++i)
    {
        f64 d = clock.tick();
        ASSERT(d >= 0.0);
        total += d;
    }
    ASSERT(total >= 0.0);
}

TEST(testClockTickAdvancesAfterSleep)
{
    Clock clock{};
    clock.tick();
    sleep(0.01);
    f64 d = clock.tick();
    ASSERT(d >= 0.01);
}

TEST(testSleep)
{
    Clock clock{};
    sleep(0.01);
    f64 d = clock.tick();
    ASSERT(d >= 0.01);
    ASSERT(d < 1.0);
}

TEST(testProfilerEvent)
{
    Profiler::clear();
    Profiler::event("test_event");
    Profiler::event("test_event");
    Profiler::event("other_event");
    u32 count = 0;
    u32 sum = 0;
    Profiler::forEachCounter([&](const std::string&, u32 v) {
        count++;
        sum += v;
    });
    ASSERT(count == 2);
    ASSERT(sum == 3);
    Profiler::clear();
    count = 0;
    Profiler::forEachCounter([&](const std::string&, u32) {
        count++;
    });
    ASSERT(count == 0);
}

TEST(testProfilerClearRemovesTimers)
{
    Profiler::clear();
    {
        ProfilerScopeTimer timer{"test_timer"};
    }
    u32 count = 0;
    Profiler::forEachTimer([&](const std::string&, f64) {
        count++;
    });
    ASSERT(count == 1);
    Profiler::clear();
    count = 0;
    Profiler::forEachTimer([&](const std::string&, f64) {
        count++;
    });
    ASSERT(count == 0);
}

TEST(testProfilerScopeTimerAccumulates)
{
    Profiler::clear();
    {
        ProfilerScopeTimer timer{"prof_accum"};
    }
    {
        ProfilerScopeTimer timer{"prof_accum"};
    }
    f64 value = -1.0;
    Profiler::forEachTimer([&](const std::string& name, f64 v) {
        ASSERT(name == "prof_accum");
        value = v;
    });
    ASSERT(value >= 0.0);
    Profiler::clear();
}

TEST(testScopeTimerWritesResult)
{
    f64 out = 0.0;
    {
        ScopeTimer timer{&out};
        sleep(0.005);
    }
    ASSERT(out >= 0.005);
}

TEST(testScopeTimerAccumulates)
{
    f64 out = 0.0;
    {
        ScopeTimer timer{&out};
    }
    {
        ScopeTimer timer{&out};
    }
    ASSERT(out >= 0.0);
}

TEST(testScopeTimerLog)
{
    // Exercises the logging destructor; only checks it runs without issue
    {
        ScopeTimerLog timer{"test_scope_log"};
    }
    ASSERT(true);
}
