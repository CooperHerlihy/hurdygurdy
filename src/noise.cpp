#include "hg/noise.hpp"

#include <random>
#include <cmath>

namespace hg {

u32 noise(u32 seed, u32 pos)
{
    u32 ret = (pos + 384521713u) * 955740521u;
    ret ^= ret >> 13;
    ret *= seed * 725937977u;
    ret ^= ret >> 7;
    ret *= 358166231u;
    ret ^= ret >> 11;
    return ret;
}

u32 noise2D(u32 seed, u32 x, u32 y)
{
    return noise(seed, x + (y * 425537443u));
}

u32 noise3D(u32 seed, u32 x, u32 y, u32 z)
{
    return noise(seed, x + y * 425537443u + z * 682607u);
}

u32 noise4D(u32 seed, u32 x, u32 y, u32 z, u32 w)
{
    return noise(seed, x + y * 425537443u + z * 682607u + w * 9067u);
}

f32 noiseNorm(u32 seed, f32 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return static_cast<f32>(noise(seed, Convert{pos}.asU32)) / static_cast<f32>(UINT32_MAX);
}

f32 noiseNorm2D(u32 seed, Vec2 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return static_cast<f32>(noise2D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32)) / static_cast<f32>(UINT32_MAX);
}

f32 noiseNorm3D(u32 seed, Vec3 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return static_cast<f32>(noise3D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32, Convert{pos.z}.asU32)) / static_cast<f32>(UINT32_MAX);
}

f32 noiseNorm4D(u32 seed, Vec4 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return static_cast<f32>(noise4D(
        seed,
        Convert{pos.x}.asU32,
        Convert{pos.y}.asU32,
        Convert{pos.z}.asU32,
        Convert{pos.w}.asU32)) / static_cast<f32>(UINT32_MAX);
}

f32 noiseVec1D(u32 seed, f32 pos)
{
    return noiseNorm(seed, pos) * 2.0f - 1.0f;
}

Vec2 noiseVec2D(u32 seed, Vec2 pos)
{
    f32 rot = 2.0f * pif * noiseNorm2D(seed, pos);
    return Vec2(cosf(rot), sinf(rot));
}

u32 trueRandom()
{
    static std::random_device trueRandom{};
    return trueRandom();
}

u32 Rng::next()
{
    return pos = noise(seed, pos);
}

u64 Rng::next64()
{
    return (static_cast<u64>(next()) << 32) | static_cast<u64>(next());
}

} // namespace hg
