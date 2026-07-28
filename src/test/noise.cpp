#include "tests.hpp"

void testNoise()
{
    // Noise
    {
        // Deterministic
        {
            TEST(noise(42, 100) == noise(42, 100));
            TEST(noise2D(1, 2, 3) == noise2D(1, 2, 3));
            TEST(noise3D(1, 2, 3, 4) == noise3D(1, 2, 3, 4));
            TEST(noise4D(1, 2, 3, 4, 5) == noise4D(1, 2, 3, 4, 5));
        }

        // Likely different for different inputs
        {
            TEST(noise(42, 100) != noise(42, 101));
        }

        // noiseNorm range
        {
            f32 v = noiseNorm(42, 3.14f);
            TEST(v >= 0.0f && v <= 1.0f);
        }

        // noiseVec1D range
        {
            f32 v = noiseVec1D(42, 3.14f);
            TEST(v >= -1.0f && v <= 1.0f);
        }

        // noiseVec2D unit length
        {
            Vec2 v = noiseVec2D(42, {3.14f, 2.72f});
            TEST(std::abs(vecLen2(v) - 1.0f) < FLT_EPSILON);
        }
    }
}

