#include "tests.hpp"
#include "hg/noise.hpp"

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
    ASSERT(std::abs(vecLen2(v) - 1.0f) < FLT_EPSILON);
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

TEST(testRngDefault)
{
    Rng rng;
    ASSERT(rng.seed == 0);
    ASSERT(rng.pos == 0);
}

TEST(testRngSeeded)
{
    Rng rng(12345);
    ASSERT(rng.seed == 12345);
    ASSERT(rng.pos == 0);
    u32 v = rng.next();
    ASSERT(v != 0);
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
