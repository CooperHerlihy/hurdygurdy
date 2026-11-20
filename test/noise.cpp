#include "tests.hpp"
#include "hg/noise.hpp"
#include "hg/gpu.hpp"

#include <cfloat>

#include "shaders/noise.comp.spv.h"

using namespace hg;

TEST(testNoiseDeterministic)
{
    ASSERT(noise(42, 100) == noise(42, 100));
    ASSERT(noise2D(1, 2, 3) == noise2D(1, 2, 3));
    ASSERT(noise3D(1, 2, 3, 4) == noise3D(1, 2, 3, 4));
    ASSERT(noise4D(1, 2, 3, 4, 5) == noise4D(1, 2, 3, 4, 5));
}

TEST(testNoiseDifferentInputs)
{
    ASSERT(noise(42, 100) != noise(42, 101));
}

TEST(testNoiseNormRange)
{
    f32 v = noiseNorm(42, 3.14f);
    ASSERT(v >= 0.0f && v <= 1.0f);
}

TEST(testNoiseVec1DRange)
{
    f32 v = noiseVec1D(42, 3.14f);
    ASSERT(v >= -1.0f && v <= 1.0f);
}

TEST(testNoiseVec2DUnitLength)
{
    Vec2 v = noiseVec2D(42, {3.14f, 2.72f});
    ASSERT(std::abs(length(v) - 1.0f) < FLT_EPSILON);
}

TEST(testNoiseNorm2DRange)
{
    for (u32 seed = 0; seed < 5; ++seed)
    {
        for (f32 x = 0.0f; x < 10.0f; x += 0.5f)
        {
            for (f32 y = 0.0f; y < 10.0f; y += 0.5f)
            {
                f32 v = noiseNorm2D(seed, {x, y});
                ASSERT(v >= 0.0f && v <= 1.0f);
            }
        }
    }
}

TEST(testNoiseNorm3DRange)
{
    for (u32 seed = 0; seed < 5; ++seed)
    {
        for (f32 x = 0.0f; x < 10.0f; x += 1.0f)
        {
            for (f32 y = 0.0f; y < 10.0f; y += 1.0f)
            {
                for (f32 z = 0.0f; z < 10.0f; z += 1.0f)
                {
                    f32 v = noiseNorm3D(seed, {x, y, z});
                    ASSERT(v >= 0.0f && v <= 1.0f);
                }
            }
        }
    }
}

TEST(testNoiseNorm4DRange)
{
    for (u32 seed = 0; seed < 3; ++seed)
    {
        for (f32 x = 0.0f; x < 8.0f; x += 2.0f)
        {
            for (f32 y = 0.0f; y < 8.0f; y += 2.0f)
            {
                for (f32 z = 0.0f; z < 8.0f; z += 2.0f)
                {
                    for (f32 w = 0.0f; w < 8.0f; w += 2.0f)
                    {
                        f32 v = noiseNorm4D(seed, {x, y, z, w});
                        ASSERT(v >= 0.0f && v <= 1.0f);
                    }
                }
            }
        }
    }
}

TEST(testNoiseNorm1DRange)
{
    for (u32 seed = 0; seed < 5; ++seed)
    {
        for (f32 x = -5.0f; x < 5.0f; x += 0.5f)
        {
            f32 v = noiseNorm(seed, x);
            ASSERT(v >= 0.0f && v <= 1.0f);
        }
    }
}

TEST(testNoiseVec1DRangeMulti)
{
    for (u32 seed = 0; seed < 5; ++seed)
    {
        for (f32 x = -5.0f; x < 5.0f; x += 0.5f)
        {
            f32 v = noiseVec1D(seed, x);
            ASSERT(v >= -1.0f && v <= 1.0f);
        }
    }
}

TEST(testNoiseMultiDDifferentInputs)
{
    for (u32 seed = 0; seed < 3; ++seed)
    {
        ASSERT(noise2D(seed, 1, 2) != noise2D(seed, 2, 2));
        ASSERT(noise3D(seed, 1, 2, 3) != noise3D(seed, 1, 3, 3));
        ASSERT(noise4D(seed, 1, 2, 3, 4) != noise4D(seed, 1, 2, 4, 4));
    }
}

TEST(testNoiseVec2DDeterministic)
{
    Vec2 a = noiseVec2D(7, {1.5f, -2.5f});
    Vec2 b = noiseVec2D(7, {1.5f, -2.5f});
    ASSERT(a == b);
    Vec2 c = noiseVec2D(7, {1.5f, -2.4f});
    ASSERT(a != c);
}

TEST(testRngSeeded)
{
    Rng rng(12345);
    u32 a = rng.next();
    u32 b = rng.next();
    ASSERT(a != b);
    u64 c = rng.next64();
    u64 d = rng.next64();
    ASSERT(c != d);
}

TEST(testRngNextDeterministic)
{
    Rng a(42);
    Rng b(42);
    for (u32 i = 0; i < 10; ++i)
    {
        ASSERT(a.next() == b.next());
    }
}

TEST(testRngNext64)
{
    Rng rng(99);
    u64 v = rng.next64();
    ASSERT(v != 0);
}

TEST(testRngSequence)
{
    Rng rng(7);
    u32 a = rng.next();
    u32 b = rng.next();
    u32 c = rng.next();
    ASSERT(a != b);
    ASSERT(b != c);
    ASSERT(a != c);
}

TEST(testRngDefaultConstruct)
{
    Rng a;
    Rng b;
    u32 av = a.next();
    u32 bv = b.next();
    ASSERT(av == bv);
    ASSERT(a.next64() == b.next64());
}

TEST(testTrueRandomCallable)
{
    // Hardware RNG: only check it runs and yields a usable seed
    u32 seed = trueRandom();
    ASSERT(noise(seed, 0) == noise(seed, 0));
}

TEST(testNoiseGLSL)
{
    // Tests the GLSL math in hurdygurdy.glsl with an offscreen compute dispatch:
    // src/test/noise.comp writes one pass/fail pixel per row.

    static constexpr u32 imgWidth = 1;
    static constexpr u32 imgHeight = 11;

    std::unique_ptr<GpuImage> img = testGpu->createImage(imgWidth, imgHeight, Format::rgba8_unorm,
        GpuImageUsage::storage | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> imgView = img->createView();

    struct Push {
        u32 imgIdx;
    };
    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{noise_comp_spv}),
        sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuImageBarrier ib{};
    ib.image = imgView.get();
    ib.nextAccess = GpuAccess::storageImageCompute;
    ib.nextLayout = GpuLayout::general;
    cmd->memoryBarrier({}, {&ib, 1});

    {
        GpuPass pass{};
        GpuView* storageImages[] = {imgView.get()};
        pass.storageImages = storageImages;
        cmd->computePass(pass);

        Push push{imgView->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(imgWidth, imgHeight, 1);
    }

    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&ib, 1});

    testGpu->endCommands(std::move(cmd));

    u32 result[imgWidth * imgHeight] = {};
    img->readWhole(std::as_writable_bytes(std::span{result}));

    const u32 passPixel = 0xFF00FF00; // green
    for (u32 row = 0; row < imgHeight; ++row)
    {
        ASSERT(result[row] == passPixel);
    }
}
