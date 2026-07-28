#pragma once

#include "hg/inttypes.hpp"
#include "hg/math.hpp"

namespace hg {

/**
 * Generate white noise
 */
u32 noise(u32 seed, u32 pos);

/**
 * Generate white noise
 */
u32 noise2D(u32 seed, u32 x, u32 y);

/**
 * Generate white noise
 */
u32 noise3D(u32 seed, u32 x, u32 y, u32 z);

/**
 * Generate white noise
 */
u32 noise4D(u32 seed, u32 x, u32 y, u32 z, u32 w);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm(u32 seed, f32 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm2D(u32 seed, Vec2 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm3D(u32 seed, Vec3 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm4D(u32 seed, Vec4 pos);

/**
 * Generate white noise unit vector
 */
f32 noiseVec1D(u32 seed, f32 pos);

/**
 * Generate white noise unit vector
 */
Vec2 noiseVec2D(u32 seed, Vec2 pos);

// value and gradient noise : TODO

/**
 * Get a true random number from hardware
 */
u32 trueRandom();

/**
 * A pseudo random number generator
 */
struct Rng {
    u32 seed = 0;
    u32 pos = 0;
};

/**
 * Set the rng seed
 */
void rngSeed(Rng* rng, u32 seed);

/**
 * Get the next random value
 */
u32 rngNext(Rng* rng);

/**
 * Get the next 64 bit random value
 */
u64 rngNext64(Rng* rng);

} // namespace hg

