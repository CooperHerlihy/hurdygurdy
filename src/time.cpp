#include "hg/time.hpp"
#include "hg/utility.hpp"

#include <ctime>
#include <chrono>
#include <thread>

namespace hg {

f64 Clock::tick()
{
    f64 prev = time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    time = assertCast<f64>(ts.tv_sec) + assertCast<f64>(ts.tv_nsec) * 1e-9;
    return time - prev;
}

void sleep(f64 time)
{
    std::this_thread::sleep_for(std::chrono::duration<f64>(time));
}

} // namespace hg
