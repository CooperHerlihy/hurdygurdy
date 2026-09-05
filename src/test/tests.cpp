#include "tests.hpp"
#include "hg/init.hpp"
#include "hg/time.hpp"

// #include "hg/assets.hpp"
// #include "hg/render2d.hpp"

struct TestResult {
    const char* name;
    TestFailure failure;
};

static TestResult failures[256];
static i32 failCount = 0;

#define RUN_TEST(name) \
do { \
    try { \
        name(); \
    } catch (const TestFailure& f) { \
        failures[failCount++] = {#name, f}; \
    } \
} while(0)

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    std::printf("HurdyGurdy: Tests begun\n");

    Clock timer{};

    RUN_TEST(testSpan);
    RUN_TEST(testProduct);
    RUN_TEST(testSum);
    RUN_TEST(testMaybe);
    RUN_TEST(testError);
    RUN_TEST(testUtils);
    RUN_TEST(testMemory);
    RUN_TEST(testConcurrency);
    RUN_TEST(testMath);
    RUN_TEST(testGeometry2D);
    RUN_TEST(testGeometry3D);
    RUN_TEST(testNoise);
    RUN_TEST(testStrings);
    RUN_TEST(testHash);
    RUN_TEST(testBinary);
    RUN_TEST(testSmartPtr);
    RUN_TEST(testArray);
    RUN_TEST(testQueue);
    RUN_TEST(testSet);
    RUN_TEST(testMap);
    RUN_TEST(testPool);
    RUN_TEST(testAssets);
    RUN_TEST(testSerialization);
    RUN_TEST(testGpu);
    RUN_TEST(testRender2D);
    RUN_TEST(testEcs);

    constexpr i32 totalTests = 26;

    f64 elapsed = timer.tick() * 1000.0;

    if (failCount == 0) {
        std::printf("\033[32mHurdyGurdy: All %d tests passed in %fms\033[0m\n", totalTests, elapsed);
    } else {
        std::printf("\033[31mHurdyGurdy: %d/%d tests FAILED in %fms:\033[0m\n", failCount, totalTests, elapsed);
        for (i32 i = 0; i < failCount; ++i) {
            std::fprintf(stderr, "\033[31m  FAIL %s: %s:%d %s() \"%s\"\033[0m\n",
                failures[i].name,
                failures[i].failure.file,
                failures[i].failure.line,
                failures[i].failure.func,
                failures[i].failure.cond);
        }
    }

    return failCount;
}

