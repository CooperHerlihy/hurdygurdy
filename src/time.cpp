#include "hg/time.hpp"

#include <chrono>
#include <thread>

namespace hg {

u64 Clock::nowNanos()
{
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

f64 Clock::tick()
{
    u64 prev = time;
    time = nowNanos();
    return static_cast<f64>(time - prev) / 1.0e9;
}

void sleep(f64 time)
{
    std::this_thread::sleep_for(std::chrono::duration<f64>(time));
}

} // namespace hg
