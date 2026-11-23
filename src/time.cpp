#include "hg/time.hpp"

#include <chrono>
#include <thread>

namespace hg {

f64 Clock::tick()
{
    auto prev = time;
    time = std::chrono::steady_clock::now();
    return std::chrono::duration<f64>(time - prev).count();
}

void sleep(f64 time)
{
    std::this_thread::sleep_for(std::chrono::duration<f64>(time));
}

} // namespace hg
