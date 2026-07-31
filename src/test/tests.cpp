#include "tests.hpp"
#include "hg/init.hpp"
#include "hg/time.hpp"

// #include "hg/assets.hpp"
#include "hg/render2d.hpp"

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    std::printf("HurdyGurdy: Tests begun\n");

    Clock timer{};

    testSpan();
    testProduct();
    testSum();
    testMaybe();
    testError();
    testUtils();
    testMemory();
    testConcurrency();
    testMath();
    testGeometry2D();
    testGeometry3D();
    testNoise();
    testStrings();
    testBinary();
    testSmartPtr();
    testArray();
    testQueue();
    testSet();
    testMap();
    testPool();
    testAssets();
    testSerialization();
    testGpu();
    testRender2D();
    testEcs();

    // Asset<TextureData> pixelFont = load<TextureData>("pixel-font.png");
    //
    // ArenaScope scratch = getScratch();
    //
    // Serializer s = serialWriter(scratch);
    // serializeBegin(&s);
    // serializeObject(&s, &pixelFont->width, &pixelFont->height, &pixelFont->format);
    // serializeVoid(&s, {pixelFont->pixels, pixelFont->width * pixelFont->height * formatToSize(pixelFont->format)});
    // serializeEnd(&s);
    //
    // BinaryView bin = writeSerialBinary(scratch, &s);
    // binaryStore(bin, "pixel-font.hg");

    std::printf("HurdyGurdy: All tests passed in %fms\n", timer.tick() * 1000.0f);
}

