#include "tests.hpp"

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    std::printf("HurdyGurdy: Tests begun\n");

    Clock timer{};

    testTypes();
    testError();
    testUtils();
    testMemory();
    testConcurrency();
    testMath();
    testGeometry2D();
    testGeometry3D();
    testNoise();
    testStrings();
    testContainers();
    testAssets();
    testSerialization();
    testGpu();
    testRender2D();
    testEcs();

    std::printf("HurdyGurdy: All tests passed in %fms\n", timer.tick() * 1000.0f);
}

