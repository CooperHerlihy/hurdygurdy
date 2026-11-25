#include "tests.hpp"

#include "hg/logging.hpp"
#include "hg/time.hpp"
#include "hg/gpu.hpp"

#include "internal.hpp"

using namespace hg;

static std::unique_ptr<Gpu> gpuOwner;
Gpu* hg::testGpu = nullptr;

static constexpr i32 maxTests = 1 << 16;

struct TestEntry {
    const char* name;
    void (*fn)();
};

static TestEntry testRegistry[maxTests];
static u32 testCount = 0;

void hg::registerTest(const char* name, void (*fn)())
{
    testRegistry[testCount++] = {name, fn};
}

struct TestResult {
    const char* name;
    TestFailure failure;
};

static TestResult failures[maxTests];
static i32 failCount = 0;

int main()
{
    // hg::internal::convertPixelFont();

    gpuOwner = Gpu::create({});
    if (gpuOwner == nullptr)
        panic("Could not initialize GPU: {}\n", std::string{getError()}.c_str());
    testGpu = gpuOwner.get();

    std::printf("HurdyGurdy: Tests begun\n");

    Clock timer{};

    for (u32 i = 0; i < testCount; ++i) {
        try {
            testRegistry[i].fn();
        } catch (const TestFailure& f) {
            failures[failCount++] = {testRegistry[i].name, f};
        }
    }

    f64 elapsed = timer.tick() * 1000.0;

    if (failCount == 0) {
        std::printf("\033[32mHurdyGurdy: All %d tests passed in %fms\033[0m\n", testCount, elapsed);
    } else {
        std::printf("\033[31mHurdyGurdy: %d/%d tests FAILED in %fms:\033[0m\n", failCount, testCount, elapsed);
        for (i32 i = 0; i < failCount; ++i) {
            std::fprintf(stderr, "\033[31m  FAIL %s: %s:%d %s() \"%s\"\033[0m\n",
                failures[i].name,
                failures[i].failure.file,
                failures[i].failure.line,
                failures[i].failure.func,
                failures[i].failure.cond);
        }
    }

    gpuOwner = nullptr;
    return failCount;
}
