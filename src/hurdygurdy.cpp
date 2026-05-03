#include "hurdygurdy.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

#include "stb_image.h"
#include "stb_image_write.h"

void hgInit(const HgInit* init)
{
    static const HgInit defaultInit{};

    if (init == nullptr)
    {
        init = &defaultInit;
    }

    hgScratchInit(init->arenaSize);
    HgArena* arena = hgScratch();

    hgPlatformInit(arena, init->maxWindows, init->maxWindowEvents);

    HgGpuInit gpuInit{};
    gpuInit.maxFramesInFlight = init->maxFramesInFlight;
    gpuInit.maxWindows = init->maxWindows;
    hgGpuInit(arena, &gpuInit);

    hgFencesInit(arena, init->maxFences);

    u32 workerCount = (u32)std::max(1, (i32)hgHardwareThreadCount() - 2); // main thread, io thread
    hgThreadsInit(arena, init->threadPoolQueueSize, workerCount);
    hgIoInit(arena, init->ioRequestQueueSize);

    hgAssetInitDefaults(arena, init->maxBinaries, init->maxImages, init->maxTextures, init->maxMeshes, init->maxModels);
}

void hgDeinit()
{
    hgIoDeinit();
    hgThreadsDeinit();
    hgFencesDeinit();

    hgGpuDeinit();
    hgPlatformDeinit();

    hgScratchDeinit();
}

const HgVec2& HgVec2::operator+=(HgVec2 other)
{
    x += other.x;
    y += other.y;
    return* this;
}

const HgVec2& HgVec2::operator-=(HgVec2 other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgVec2& HgVec2::operator*=(HgVec2 other)
{
    x *= other.x;
    y *= other.y;
    return* this;
}

const HgVec2& HgVec2::operator/=(HgVec2 other)
{
    x /= other.x;
    y /= other.y;
    return* this;
}

const HgVec3& HgVec3::operator+=(HgVec3 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgVec3& HgVec3::operator-=(HgVec3 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgVec3& HgVec3::operator*=(HgVec3 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

const HgVec3& HgVec3::operator/=(HgVec3 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

const HgVec4& HgVec4::operator+=(HgVec4 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgVec4& HgVec4::operator-=(HgVec4 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

const HgVec4& HgVec4::operator*=(HgVec4 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

const HgVec4& HgVec4::operator/=(HgVec4 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

const HgMat2& HgMat2::operator+=(const HgMat2& other)
{
    x += other.x;
    y += other.y;
    return* this;
}

const HgMat2& HgMat2::operator-=(const HgMat2& other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgMat3& HgMat3::operator+=(const HgMat3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgMat3& HgMat3::operator-=(const HgMat3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgMat4& HgMat4::operator+=(const HgMat4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgMat4& HgMat4::operator-=(const HgMat4& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

const HgComplex& HgComplex::operator+=(HgComplex other)
{
    r += other.r;
    i += other.i;
    return* this;
}

const HgComplex& HgComplex::operator-=(HgComplex other)
{
    r -= other.r;
    i -= other.i;
    return* this;
}

const HgQuat& HgQuat::operator+=(HgQuat other)
{
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

const HgQuat& HgQuat::operator-=(HgQuat other)
{
    r -= other.r;
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return* this;
}

void hgVecAdd(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] + rhs[i];
    }
}

void hgVecSub(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] - rhs[i];
    }
}

void hgVecMulPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] * rhs[i];
    }
}

void hgVecMulScalar(u32 size, f32* dst, f32 scalar, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = scalar * vec[i];
    }
}

void hgVecDivPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        hgAssert(rhs[i] != 0);
        dst[i] = lhs[i] / rhs[i];
    }
}

void hgVecDivScalar(u32 size, f32* dst, const f32* vec, f32 scalar)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgAssert(scalar != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / scalar;
    }
}

void hgVecDot(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    *dst = 0;
    for (u32 i = 0; i < size; ++i)
    {
        *dst += lhs[i] * rhs[i];
    }
}

void hgVecLen(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgVecDot(size, dst, vec, vec);
    *dst = (f32)sqrt(*dst);
}

f32 hgVecLen2(HgVec2 vec)
{
    return (f32)sqrt(hgVecDot2(vec, vec));
}

f32 hgVecLen3(HgVec3 vec)
{
    return (f32)sqrt(hgVecDot3(vec, vec));
}

f32 hgVecLen4(HgVec4 vec)
{
    return (f32)sqrt(hgVecDot3(vec, vec));
}

void hgVecNorm(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    f32 len;
    hgVecLen(size, &len, vec);
    hgAssert(len != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hgVecNorm2(HgVec2 vec)
{
    f32 len = hgVecLen2(vec);
    hgAssert(len != 0);
    return HgVec2{vec.x / len, vec.y / len};
}

HgVec3 hgVecNorm3(HgVec3 vec)
{
    f32 len = hgVecLen3(vec);
    hgAssert(len != 0);
    return HgVec3{vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hgVecNorm4(HgVec4 vec)
{
    f32 len = hgVecLen4(vec);
    hgAssert(len != 0);
    return HgVec4{vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hgVecCross(f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

HgVec3 hgCross(const HgVec3& lhs, const HgVec3& rhs)
{
    return HgVec3{
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

void hgMatAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatAdd(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatAdd(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatAdd(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgMatSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatSub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatSub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatSub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgMatMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
{
    hgAssert(hr == wl);
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    (void)hr;
    for (u32 i = 0; i < wl; ++i)
    {
        for (u32 j = 0; j < wr; ++j)
        {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k)
            {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatMul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatMul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatMul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

void hgMatMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(mat != nullptr);
    hgAssert(vec != nullptr);
    for (u32 i = 0; i < height; ++i)
    {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j)
        {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

HgVec2 operator*(const HgMat2& lhs, HgVec2 rhs)
{
    HgVec2 result{};
    hgMatMulVec(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec3 operator*(const HgMat3& lhs, HgVec3 rhs)
{
    HgVec3 result{};
    hgMatMulVec(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec4 operator*(const HgMat4& lhs, HgVec4 rhs)
{
    HgVec4 result{};
    hgMatMulVec(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgQuat operator*(HgQuat lhs, HgQuat rhs)
{
    return HgQuat{
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

HgQuat hgQuatAxisAngle(HgVec3 axis, f32 angle)
{
    f32 halfAngle = angle * (f32)0.5;
    f32 sinHalfAngle = (f32)std::sin(halfAngle);
    return HgQuat{
        (f32)std::cos(halfAngle),
        axis.x * sinHalfAngle,
        axis.y * sinHalfAngle,
        axis.z * sinHalfAngle,
    };
}

HgVec3 hgVecRotate(HgQuat lhs, HgVec3 rhs)
{
    HgQuat q = lhs * HgQuat{0, rhs.x, rhs.y, rhs.z} * hgQuatConj(lhs);
    return HgVec3{q.i, q.j, q.k};
}

HgMat3 hgMatRotate(HgQuat lhs, HgMat3 rhs)
{
    return HgMat3{
        hgVecRotate(lhs, rhs.x),
        hgVecRotate(lhs, rhs.y),
        hgVecRotate(lhs, rhs.z),
    };
}

HgMat4 hgMatModel2D(HgVec3 position, HgVec2 scale, f32 rotation)
{
    HgMat2 m2{HgVec2{scale.x, 0.0f}, HgVec2{0.0f, scale.y}};
    f32 rotSin = (f32)std::sin(rotation);
    f32 rotCos = (f32)std::cos(rotation);
    HgMat2 rot{HgVec2{rotCos, rotSin}, HgVec2{-rotSin, rotCos}};
    HgMat4 m4 = HgMat4{rot * m2};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgMatModel3D(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation)
{
    HgMat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hgMatRotate(rotation, m3);
    HgMat4 m4 = HgMat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgMatView(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation)
{
    HgMat4 rot{hgMatRotate(hgQuatConj(rotation), HgMat3{1.0f})};
    HgMat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

HgMat4 hgMatOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return HgMat4{
        HgVec4{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        HgVec4{-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hgMatPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    hgAssert(near > 0.0f);
    hgAssert(far > near);
    f32 scale = 1.0f / (f32)tan(fov * 0.5f);
    return HgMat4{
        HgVec4{scale / aspect, 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, scale, 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, far / (far - near), 1.0f},
        HgVec4{0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

u32 hgNoise(u32 seed, u32 pos)
{
    u32 ret = (pos + 384521713u) * 955740521u;
    ret ^= ret >> 13;
    ret *= seed * 725937977u;
    ret ^= ret >> 7;
    ret *= 358166231u;
    ret ^= ret >> 11;
    return ret;
}

u32 hgNoise2D(u32 seed, u32 x, u32 y)
{
    return hgNoise(seed, x + (y * 425537443u));
}

u32 hgNoise3D(u32 seed, u32 x, u32 y, u32 z)
{
    return hgNoise(seed, x + y * 425537443u + z * 682607u);
}

u32 hgNoise4D(u32 seed, u32 x, u32 y, u32 z, u32 w)
{
    return hgNoise(seed, x + y * 425537443u + z * 682607u + w * 9067);
}

f32 hgNoiseNorm(u32 seed, f32 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise(seed, Convert{pos}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm2D(u32 seed, HgVec2 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise2D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm3D(u32 seed, HgVec3 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise3D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32, Convert{pos.z}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm4D(u32 seed, HgVec4 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise4D(
        seed,
        Convert{pos.x}.asU32,
        Convert{pos.y}.asU32,
        Convert{pos.z}.asU32,
        Convert{pos.w}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseVec1D(u32 seed, f32 pos)
{
    return hgNoiseNorm(seed, pos) * 2.0f - 1.0f;
}

HgVec2 hgNoiseVec2D(u32 seed, HgVec2 pos)
{
    f32 rot = 2.0f * (f32)hgPi * hgNoiseNorm2D(seed, pos);
    return HgVec2(std::cos(rot), std::sin(rot));
}

u32 hgGetMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : (u32)log2((f32)max) + 1;
}

void* hgAlloc(HgArena* arena, u64 size, u64 alignment)
{
    hgAssert(arena != nullptr);
    arena->head = hgAlign((u64)arena->head, alignment) + size;
    hgAssert(arena->head <= arena->capacity);
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* hgRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    hgAssert(arena != nullptr);

    if (allocation >= arena->memory && (uptr)allocation + oldSize <= (uptr)arena->memory + arena->capacity)
    {
        if ((uptr)allocation + oldSize - (uptr)arena->memory == (uptr)arena->head)
        {
            arena->head = (uptr)allocation + newSize - (uptr)arena->memory;
            hgAssert(arena->head <= arena->capacity);
            return allocation;
        }

        if (newSize < oldSize)
            return allocation;
    }

    void* newAllocation = hgAlloc(arena, newSize, alignment);
    if (allocation != nullptr)
        memcpy(newAllocation, allocation, std::min(oldSize, newSize));
    return newAllocation;
}

static constexpr u32 arenaCount = 2;
static thread_local HgArena arenas[arenaCount]{};

void hgScratchInit(u64 size)
{
    for (u32 i = 0; i < arenaCount; ++i)
    {
        if (arenas[i].memory == nullptr)
        {
            u64 arenaSize = size;
            arenas[i] = {malloc(arenaSize), arenaSize, 0};
        }
    }
}

void hgScratchDeinit()
{
    for (u32 i = 0; i < arenaCount; ++i)
    {
        if (arenas[i].memory != nullptr)
        {
            free(arenas[i].memory);
            arenas[i] = {};
        }
    }
}

HgArena* hgScratch(HgArena const* const* conflicts, u32 count)
{
    if (conflicts != nullptr)
        hgAssert(count > 0);

    for (HgArena& arena : arenas)
    {
        hgAssert(arena.memory != nullptr);

        for (u32 i = 0; i < count; ++i)
        {
            if (&arena == conflicts[i])
                goto next;
        }
        return &arena;
next:
        continue;
    }
    hgError("No scratch arena available\n");
}

char* hgCString(HgArena* arena, HgStringView str)
{
    hgAssert(arena != nullptr);
    if (str.length > 0)
        hgAssert(str.chars != nullptr);

    char* cStr = hgAlloc<char>(arena, str.length + 1);
    memcpy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

HgString hgStringCreate(HgArena* arena, u64 capacity)
{
    hgAssert(arena != nullptr);

    HgString str;
    str.chars = hgAlloc<char>(arena, capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString hgStringCopy(HgArena* arena, HgStringView str)
{
    hgAssert(arena != nullptr);

    HgString copy;
    copy.chars = hgAlloc<char>(arena, str.length);
    copy.capacity = str.length;
    copy.length = str.length;
    memcpy(copy.chars, str.chars, str.length);
    return copy;
}

void hgStringReserve(HgArena* arena, HgString* str, u64 newCapacity)
{
    hgAssert(arena != nullptr);

    str->chars = hgRealloc(arena, str->chars, str->capacity, newCapacity);
    str->capacity = newCapacity;
}

void hgStringGrow(HgArena* arena, HgString* str, f64 factor)
{
    hgAssert(arena != nullptr);
    hgAssert(str != nullptr);
    hgAssert(factor > 1.0f);

    hgAssert(str->capacity <= (u64)((f32)SIZE_MAX / factor));
    hgStringReserve(arena, str, str->capacity == 0 ? 1 : (u64)((f64)str->capacity * factor));
}

void hgStringInsert(HgArena* arena, HgString* dst, u64 idx, HgStringView src)
{
    hgAssert(arena != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(idx <= dst->length);
    if (src.length > 0)
        hgAssert(src.chars != nullptr);

    u64 newLength = dst->length + src.length;
    while (dst->capacity < newLength)
    {
        hgStringGrow(arena, dst, 2.0);
    }

    if (idx != dst->length)
        memmove(&dst->chars[idx + src.length], &dst->chars[idx], dst->length - idx);
    memcpy(&dst->chars[idx], src.chars, src.length);
    dst->length = newLength;
}

void hgStringInsertc(HgArena* arena, HgString* dst, u64 idx, char c)
{
    hgAssert(arena != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(idx <= dst->length);

    if (dst->capacity < dst->length + 1)
        hgStringGrow(arena, dst, 2.0);

    if (idx != dst->length)
        memmove(&dst->chars[idx + 1], &dst->chars[idx], dst->length - idx);
    dst->chars[idx] = c;
    ++dst->length;
}

bool hgIsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

bool hgIsNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool hgIsInteger(HgStringView str)
{
    if (str.length == 0)
        return false;

    u64 head = 0;
    if (!hgIsNumeral(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    ++head;
    while (head < str.length)
    {
        if (!hgIsNumeral(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool hgIsFloat(HgStringView str)
{
    if (str.length == 0)
        return false;

    bool hasDecimal = false;
    bool hasExponent = false;

    u64 head = 0;

    if (!hgIsNumeral(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '.')
        hasDecimal = true;

    ++head;
    while (head < str.length)
    {
        if (hgIsNumeral(str[head]))
        {
            ++head;
            continue;
        }

        if (str[head] == '.' && !hasDecimal)
        {
            hasDecimal = true;
            ++head;
            continue;
        }

        if (str[head] == 'e' && !hasExponent)
        {
            hasExponent = true;
            ++head;
            if (hgIsNumeral(str[head]) || str[head] == '+' || str[head] == '-')
            {
                ++head;
                continue;
            }
            return false;
        }

        if (str[head] == 'f' && head == str.length - 1)
            break;

        return false;
    }

    return hasDecimal || hasExponent;
}

i64 hgStringToInteger(HgStringView str)
{
    hgAssert(hgIsInteger(str));

    i64 power = 1;
    i64 ret = 0;

    u64 head = str.length - 1;
    while (head > 0)
    {
        ret += (i64)(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+')
    {
        if (str[head] == '-')
            ret *= -1;
        else
            ret += (i64)(str[head] - '0') * power;
    }

    return ret;
}

f64 hgStringToFloat(HgStringView str)
{
    hgAssert(hgIsFloat(str));

    f64 ret = 0.0;
    u64 head = 0;

    bool isNegative = str[head] == '-';
    if (isNegative || str[head] == '+')
        ++head;

    if (hgIsNumeral(str[head]))
    {
        u64 intPartBegin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e')
        {
            ++head;
        }
        ret += (f64)hgStringToInteger({&str[intPartBegin], &str[head]});
    }

    if (head < str.length && str[head] == '.')
    {
        ++head;

        f64 power = 0.1;
        while (head < str.length && hgIsNumeral(str[head]))
        {
            ret += (f64)(str[head] - '0') * power;
            power *= 0.1;
            ++head;
        }
    }

    if (head < str.length && str[head] == 'e')
    {
        ++head;

        bool expIsNegative = str[head] == '-';
        if (expIsNegative || str[head] == '+')
            ++head;

        u64 expBegin = head;
        while (head < str.length && hgIsNumeral(str[head]))
        {
            ++head;
        }

        i64 exp = hgStringToInteger({&str[expBegin], str.chars + head});
        if (exp != 0)
        {
            if (expIsNegative)
            {
                for (i64 i = 0; i < exp; ++i)
                {
                    ret *= 0.1;
                }
            } else {
                for (i64 i = 0; i < exp; ++i)
                {
                    ret *= 10.0;
                }
            }
        } else {
            ret = 1.0;
        }
    }

    if (isNegative)
        ret *= -1.0;

    return ret;
}

HgString hgIntegerToString(HgArena* arena, i64 num)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    if (num == 0)
        return hgStringCopy(arena, "0");

    bool isNegative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgString reverse = hgStringCreate(scratch, 16);
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        hgStringAppendc(scratch, &reverse, '0' + (char)digit);
    }

    HgString ret = hgStringCreate(arena, reverse.length + (isNegative ? 1 : 0));
    if (isNegative)
        hgStringAppendc(arena, &ret, '-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        hgStringAppendc(arena, &ret, reverse[i]);
    }
    return ret;
}

HgString hgFloatToString(HgArena* arena, f64 num, u32 decimalCount)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    if (num == 0.0)
        return hgStringCopy(arena, "0.0");

    HgString intStr = hgIntegerToString(scratch, (i64)fabs(num));

    HgString decStr = hgStringCreate(scratch, decimalCount + 1);
    hgStringAppendc(scratch, &decStr, '.');

    f64 decPart = fabs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        hgStringAppendc(scratch, &decStr, '0' + (char)((u64)decPart % 10));
    }

    HgString ret{};
    if (num < 0.0)
        hgStringAppendc(arena, &ret, '-');
    hgStringAppend(arena, &ret, intStr);
    hgStringAppend(arena, &ret, decStr);
    return ret;
}

HgString hgStringFormat(HgArena* arena, HgStringView fmt, ...)
{
    (void)arena;
    (void)fmt;
    hgError("hgFormatString not implemented yet : TODO\n");
}

struct JsonParseState {
    HgStringView text;
    u64 head;
    u64 line;
};

static HgJson jsonParseNext(HgArena* arena, JsonParseState* state);
static HgJson jsonParseStruct(HgArena* arena, JsonParseState* state);
static HgJson jsonParseArray(HgArena* arena, JsonParseState* state);
static HgJson jsonParseString(HgArena* arena, JsonParseState* state);
static HgJson jsonParseNumber(HgArena* arena, JsonParseState* state);
static HgJson jsonParseBoolean(HgArena* arena, JsonParseState* state);

static HgJson jsonParseNext(HgArena* arena, JsonParseState* state)
{
    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head >= state->text.length)
        return {};

    switch (state->text[state->head])
    {
        case '{':
            ++state->head;
            return jsonParseStruct(arena, state);
        case '[':
            ++state->head;
            return jsonParseArray(arena, state);
        case '\'': [[fallthrough]];
        case '"':
            ++state->head;
            return jsonParseString(arena, state);
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return jsonParseNumber(arena, state);
        case 't': [[fallthrough]];
        case 'f':
            return jsonParseBoolean(arena, state);
        case '}': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", found unexpected token \"]\"\n");
            return {nullptr, error};
        }
    }
    if (hgIsNumeral(state->text[state->head]))
    {
        return jsonParseNumber(arena, state);
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
    error->next = nullptr;

    u64 begin = state->head;
    while (state->head < state->text.length && !hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    error->msg = HgString{};
    hgStringAppend(arena, &error->msg, "on line ");
    hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &error->msg, ", found unexpected token \"");
    hgStringAppend(arena, &error->msg, {&state->text[begin], &state->text[state->head]});
    hgStringAppend(arena, &error->msg, "\"\n");

    return {nullptr, error};
}

static HgJson jsonParseStruct(HgArena* arena, JsonParseState* state)
{
    HgJson json{};
    json.file = hgAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_struct;
    json.file->jstruct.fields = nullptr;

    HgJsonField* lastField = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", expected struct to terminate\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == ']')
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", struct ends with \"]\" instead of \"}\"\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == '}')
        {
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        HgJson value = jsonParseNext(arena, state);

        if (value.file != nullptr)
        {
            if (value.file->type != HgJsonType::HgJsonType_field)
            {
                HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                error->msg = HgString{};
                hgStringAppend(arena, &error->msg, "on line ");
                hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &error->msg, ", struct has a literal instead of a field\n");
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else if (value.file->field.value == nullptr)
            {
                HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                error->msg = HgString{};
                hgStringAppend(arena, &error->msg, "on line ");
                hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &error->msg, ", struct has a field named \"");
                hgStringAppend(arena, &error->msg, value.file->field.name);
                hgStringAppend(arena, &error->msg, "\" which has no value\n");
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastField == nullptr)
                    json.file->jstruct.fields = &value.file->field;
                else
                    lastField->next = &value.file->field;
                lastField = &value.file->field;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static HgJson jsonParseArray(HgArena* arena, JsonParseState* state)
{
    HgJson json{};
    json.file = hgAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_array;

    HgJsonType type = HgJsonType::HgJsonType_none;
    HgJsonElem* lastElem = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", expected struct to terminate\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == '}')
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgStringAppend(arena, &error->msg, "on line ");
            hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &error->msg, ", array ends with \"}\" instead of \"]\"\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == ']')
        {
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        HgJsonElem* elem = hgAlloc<HgJsonElem>(arena, 1);
        elem->next = nullptr;

        HgJson value = jsonParseNext(arena, state);
        elem->value = value.file;

        if (value.file != nullptr)
        {
            if (type == HgJsonType::HgJsonType_none)
            {
                if (value.file->type != HgJsonType::HgJsonType_field)
                {
                    type = value.file->type;
                } else {
                    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                    error->next = nullptr;
                    error->msg = HgString{};
                    hgStringAppend(arena, &error->msg, "on line ");
                    hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
                    hgStringAppend(arena, &error->msg, ", array has a field as an element\n");
                    if (lastError == nullptr)
                        json.errors = lastError = error;
                    else
                        lastError->next = error;
                    lastError = error;
                }
            }
            if (value.file->type != type)
            {
                HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                error->msg = HgString{};
                hgStringAppend(arena, &error->msg, "on line ");
                hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &error->msg, ", array has element which is not the same type as the first valid element\n");
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastElem == nullptr)
                    json.file->array.elems = elem;
                else
                    lastElem->next = elem;
                lastElem = elem;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static HgJson jsonParseString(HgArena* arena, JsonParseState* state)
{
    u64 begin = state->head;
    while (state->head < state->text.length && state->text[state->head] != '"')
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    u64 end = state->head;
    if (state->head < state->text.length)
    {
        ++state->head;
        HgString str = hgStringCreate(arena, end - begin);
        for (u64 i = begin; i < end; ++i)
        {
            char c = state->text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            hgStringAppendc(arena, &str, c);
        }

        HgJson json{};
        json.file = hgAlloc<HgJsonNode>(arena, 1);

        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ':')
        {
            ++state->head;
            json.file->type = HgJsonType::HgJsonType_field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            HgJson next = jsonParseNext(arena, state);
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = HgJsonType::HgJsonType_string;
            json.file->string = str;
        }
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;
        return json;
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
    error->msg = HgString{};
    hgStringAppend(arena, &error->msg, "on line ");
    hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &error->msg, ", expected string to terminate\n");
    return {nullptr, error};
}

static HgJson jsonParseNumber(HgArena* arena, JsonParseState* state)
{
    bool isFloat = false;
    u64 begin = state->head;
    while (state->head < state->text.length && (
        hgIsNumeral(state->text[state->head]) ||
        state->text[state->head] == '-' ||
        state->text[state->head] == '+' ||
        state->text[state->head] == '.' ||
        state->text[state->head] == 'e'
    ))
    {
        if (state->text[state->head] == '.' || state->text[state->head] == 'e')
            isFloat = true;
        ++state->head;
    }
    HgStringView num{&state->text[begin], &state->text[state->head]};
    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head < state->text.length && state->text[state->head] == ',')
        ++state->head;

    if (isFloat)
    {
        if (hgIsFloat(num))
        {
            HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
            node->type = HgJsonType::HgJsonType_float;
            node->floating = hgStringToFloat(num);
            return {node, nullptr};
        }
    } else {
        if (hgIsInteger(num))
        {
            HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
            node->type = HgJsonType::HgJsonType_integer;
            node->integer = hgStringToInteger(num);
            return {node, nullptr};
        }
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);

    error->msg = HgString{};
    hgStringAppend(arena, &error->msg, "on line ");
    hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &error->msg, ", expected numeral value, found \"");
    hgStringAppend(arena, &error->msg, num);
    hgStringAppend(arena, &error->msg, "\"\n");

    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

static HgJson jsonParseBoolean(HgArena* arena, JsonParseState* state)
{
    if (state->head + 4 < state->text.length && HgStringView{&state->text[state->head], 4} == "true")
    {
        state->head += 4;
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = true;
        return {node, nullptr};
    }
    if (state->head + 5 < state->text.length && HgStringView{&state->text[state->head], 5} == "false")
    {
        state->head += 5;
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = false;
        return {node, nullptr};
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);

    u64 begin = state->head;
    while (state->head < state->text.length && !hgIsWhitespace(state->text[state->head])
        && state->text[state->head] != ','
        && state->text[state->head] != '}'
        && state->text[state->head] != ']'
    )
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    error->msg = HgString{};
    hgStringAppend(arena, &error->msg, "on line ");
    hgStringAppend(arena, &error->msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &error->msg, ", expected boolean value, found \"");
    hgStringAppend(arena, &error->msg, {&state->text[begin], &state->text[state->head]});
    hgStringAppend(arena, &error->msg, "\"\n");

    if (state->text[state->head] == ',')
        ++state->head;

    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson hgParseJson(HgArena* arena, HgStringView text)
{
    hgAssert(arena != nullptr);
    if (text.length > 0)
        hgAssert(text.chars != nullptr);

    JsonParseState parseState{};
    parseState.text = text;
    parseState.head = 0;
    parseState.line = 1;
    return jsonParseNext(arena, &parseState);
}

f64 hgClockTick(HgClock* clock)
{
    hgAssert(clock != nullptr);

    f64 prev = clock->time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    clock->time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
    return clock->time - prev;
}

HgPerf hgPerfCreate(HgArena* arena, u32 count)
{
    hgAssert(arena != nullptr);

    HgPerf perf;
    perf.times = hgAlloc<f64>(arena, count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void hgPerfBegin(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgClockTick(&perf->clock);
}

f64 hgPerfEnd(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgAssert(perf->current < perf->count);

    f64 time = hgClockTick(&perf->clock);
    perf->times[perf->current++] = time;

    return time;
}

HgPerfStats hgPerfAnalyze(const HgPerf* perf)
{
    hgAssert(perf != nullptr);

    HgPerfStats stats;
    stats.avg = 0.0;
    stats.best = INFINITY;
    stats.worst = 0.0;

    for (u32 i = 0; i < perf->current; ++i)
    {
        if (perf->times[i] < stats.best)
            stats.best = perf->times[i];
        if (perf->times[i] > stats.worst)
            stats.worst = perf->times[i];
        stats.avg += perf->times[i];
    }
    stats.avg /= (f64)perf->current;

    return stats;
}

void hgPerfLog(HgStringView title, const HgPerfStats* stats, HgPerfScale scale)
{
    hgAssert(stats != nullptr);
    if (title.length == 0 || title.chars == nullptr)
        title = "Title Missing";

    switch (scale)
    {
        case HgPerfScale_seconds:
            printf("HG Performance - %.*s: avg: %.4fs, best: %.4fs, worst: %.4fs\n",
                (int)title.length, title.chars, stats->avg, stats->best, stats->worst);
            break;
        case HgPerfScale_milli:
            printf("HG Performance - %.*s: avg: %.4fms, best: %.4fms, worst: %.4fms\n",
                (int)title.length, title.chars, stats->avg * 1.e3, stats->best * 1.e3, stats->worst * 1.e3);
            break;
        case HgPerfScale_micro:
            printf("HG Performance - %.*s: avg: %.4fmcs, best: %.4fmcs, worst: %.4fmcs\n",
                (int)title.length, title.chars, stats->avg * 1.e6, stats->best * 1.e6, stats->worst * 1.e6);
            break;
        case HgPerfScale_nano:
            printf("HG Performance - %.*s: avg: %.4fns, best: %.4fns, worst: %.4fns\n",
                (int)title.length, title.chars, stats->avg * 1.e9, stats->best * 1.e9, stats->worst * 1.e9);
            break;
    }
}

u32 hgHardwareThreadCount()
{
    return (u32)std::thread::hardware_concurrency();
}

struct FenceData {
    std::atomic<u32> counter{0};
};

HgPool<FenceData> fencePool{};

void hgFencesInit(HgArena* arena, u32 maxFences)
{
    fencePool = hgPoolCreate<FenceData>(arena, maxFences);
}

void hgFencesDeinit()
{
    fencePool = {};
}

HgFence hgFenceCreate()
{
    HgHandle handle = {hgPoolAlloc(&fencePool)};
    new (hgPoolGet(&fencePool, handle)) FenceData{0};
    return {handle};
}

void hgFenceDestroy(HgFence fence)
{
    hgPoolFree(&fencePool, fence.handle);
}

void hgFenceAttach(HgFence fence, u32 count)
{
    hgPoolGet(&fencePool, fence.handle)->counter.fetch_add(count);
}

void hgFenceSignal(HgFence fence, u32 count)
{
    hgPoolGet(&fencePool, fence.handle)->counter.fetch_sub(count);
}

bool hgFenceIsComplete(HgFence fence)
{
    return hgPoolGet(&fencePool, fence.handle)->counter.load() == 0;
}

bool hgFenceWait(HgFence fence, f64 timeoutSeconds)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeoutSeconds);
    while (!hgFenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        _mm_pause();
    }
    return hgFenceIsComplete(fence);
}

void hgFenceWaitIndefinite(HgFence fence)
{
    while (!hgFenceIsComplete(fence))
    {
        _mm_pause();
    }
}

struct ThreadWork {
    HgFence fence;
    void* data;
    void (*fn)(void*);
};

struct ThreadPoolState {
    std::thread* threads = nullptr;
    u32 threadCount = 0;
    std::atomic_bool shouldClose = false;

    std::mutex mtx{};
    std::condition_variable cv{};

    ThreadWork* work = nullptr;
    std::atomic_bool* hasWork = nullptr;
    u32 workCapacity = 0;

    std::atomic<u32> workCount = 0;
    std::atomic<u32> tail = 0;
    std::atomic<u32> workingTail = 0;
    std::atomic<u32> head = 0;
    std::atomic<u32> workingHead = 0;
};

static ThreadPoolState threadPool{};

static bool poolExecute()
{
    u32 idx = threadPool.workingTail.load();
    do {
        if (idx == threadPool.head.load())
            return false;
    } while (!threadPool.workingTail.compare_exchange_weak(idx, (idx + 1) & (threadPool.workCapacity - 1)));

    ThreadWork work = threadPool.work[idx];
    threadPool.hasWork[idx].store(false);

    u32 t = threadPool.tail.load();
    while (t != threadPool.head.load() && !threadPool.hasWork[t].load())
    {
        u32 next = (t + 1) & (threadPool.workCapacity - 1);
        threadPool.tail.compare_exchange_strong(t, next);
        t = next;
    }

    --threadPool.workCount;

    hgAssert(work.fn != nullptr);
    work.fn(work.data);

    if (!hgHandleIsNull(work.fence.handle))
        hgFenceSignal(work.fence, 1);
    return true;
}

void hgThreadsInit(HgArena* arena, u32 queueSize, u32 threadCount)
{
    hgAssert(arena != nullptr);
    hgAssert(threadCount > 0);
    hgAssert(queueSize > 0 && (queueSize & (queueSize - 1)) == 0);

    threadPool.shouldClose.store(false);
    threadPool.threadCount = threadCount;
    threadPool.threads = hgAlloc<std::thread>(arena, threadCount);

    threadPool.work = hgAlloc<ThreadWork>(arena, queueSize);
    threadPool.hasWork = hgAlloc<std::atomic_bool>(arena, queueSize);
    threadPool.workCapacity = queueSize;

    threadPool.workCount.store(0);
    threadPool.tail.store(0);
    threadPool.workingTail.store(0);
    threadPool.head.store(0);
    threadPool.workingHead.store(0);

    for (u32 i = 0; i < threadPool.workCapacity; ++i)
    {
        threadPool.hasWork[i].store(false);
    }

    for (u32 i = 0; i < threadCount; ++i)
    {
        new (threadPool.threads + i) std::thread([] {
            for (;;)
            {
                std::unique_lock<std::mutex> lock{threadPool.mtx};
                while (threadPool.workCount.load() == 0 && !threadPool.shouldClose.load())
                {
                    threadPool.cv.wait(lock);
                }
                lock.unlock();
                if (threadPool.shouldClose.load())
                    return;

                static constexpr u32 spinCount = 128;
                for (u32 j = 0; j < spinCount; ++j)
                {
                    if (!poolExecute())
                        _mm_pause();
                }
            }
        });
    }
}

void hgThreadsDeinit()
{
    threadPool.mtx.lock();
    threadPool.shouldClose = true;
    threadPool.mtx.unlock();
    threadPool.cv.notify_all();

    for (u32 i = 0; i < threadPool.threadCount; ++i)
    {
        threadPool.threads[i].join();
    }
    for (u32 i = 0; i < threadPool.threadCount; ++i)
    {
        threadPool.threads[i].~thread();
    }
    threadPool.cv.~condition_variable();
    threadPool.mtx.~mutex();
}

bool hgThreadsHelp(HgFence fence, f64 timeout)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!hgFenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        if (!poolExecute())
            _mm_pause();
    }
    return hgFenceIsComplete(fence);
}

void hgThreadsCall(HgFence fence, void* data, void (*fn)(void* data))
{
    hgAssert(fn != nullptr);
    if (!hgHandleIsNull(fence.handle))
        hgFenceAttach(fence, 1);

    u32 idx = threadPool.workingHead.fetch_add(1) & (threadPool.workCapacity - 1);

    threadPool.work[idx].fence = fence;
    threadPool.work[idx].data = data;
    threadPool.work[idx].fn = fn;
    threadPool.hasWork[idx].store(true);

    u32 h = threadPool.head.load();
    while (threadPool.hasWork[h].load())
    {
        u32 next = (h + 1) & (threadPool.workCapacity - 1);
        threadPool.head.compare_exchange_strong(h, next);
        h = next;
    }

    ++threadPool.workCount;
    threadPool.mtx.lock();
    threadPool.mtx.unlock();
    threadPool.cv.notify_one();
}

void hgThreadsFor(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    hgAssert(begin <= end);
    hgAssert(fn != nullptr);

    HgArena* scratch = hgScratch();
    HgArenaScope scratchScope{scratch};

    u64 chunkSize = (u64)std::ceil((f32)(end - begin) / (8.0f * (f32)hgHardwareThreadCount()));

    HgFence fence = hgFenceCreate();
    hgDefer(hgFenceDestroy(fence));

    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data;
            void (*fn)(void* data, u64 idx);
            u64 begin;
            u64 end;
        };

        Capture* capture = hgAlloc<Capture>(scratch, 1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = std::min(i + chunkSize, end);

        hgThreadsCall(fence, capture, [](void* pcapture)
        {
            Capture* capture = (Capture*)pcapture;
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    hgThreadsHelp(fence, INFINITY);
}

struct IORequest {
    HgFence fence;
    void* resource;
    HgStringView path;
    void (*fn)(void* resource, HgStringView path);
};

struct IOThreadState {
    std::thread thread{};
    std::atomic_bool shouldClose = false;

    IORequest* requests = nullptr;
    std::atomic_bool* hasRequest = nullptr;
    u32 requestCapacity = 0;

    std::atomic<u32> tail = 0;
    std::atomic<u32> head = 0;
    std::atomic<u32> workingHead = 0;
};

static IOThreadState ioThread{};

static bool ioPop()
{
    u32 idx = ioThread.tail.load() & (ioThread.requestCapacity - 1);
    if (idx == ioThread.head.load())
        return false;

    IORequest request = ioThread.requests[idx];
    ioThread.hasRequest[idx].store(false);

    ioThread.tail.fetch_add(1);

    hgAssert(request.fn != nullptr);
    request.fn(request.resource, request.path);

    if (!hgHandleIsNull(request.fence.handle))
        hgFenceSignal(request.fence, 1);
    return true;
}

void hgIoInit(HgArena* arena, u32 queueSize)
{
    hgAssert(arena != nullptr);
    hgAssert(queueSize > 1 && (queueSize & (queueSize - 1)) == 0);

    ioThread.requests = hgAlloc<IORequest>(arena, queueSize);
    ioThread.hasRequest = hgAlloc<std::atomic_bool>(arena, queueSize);
    ioThread.requestCapacity = queueSize;

    ioThread.tail.store(0);
    ioThread.head.store(0);
    ioThread.workingHead.store(0);
    for (u32 i = 0; i < ioThread.requestCapacity; ++i)
    {
        ioThread.hasRequest[i].store(false);
    }

    ioThread.shouldClose.store(false);
    ioThread.thread = std::thread([]()
    {
        hgScratchInit(UINT32_MAX);

        while (!ioThread.shouldClose.load())
        {
            if (!ioPop())
                poolExecute();
        }

        hgScratchDeinit();
    });
}

void hgIoDeinit()
{
    ioThread.shouldClose.store(true);
    ioThread.thread.join();
}

void hgIoRequest(HgFence fence, void* data, HgStringView path, void (*fn)(void* data, HgStringView path))
{
    hgAssert(fn != nullptr);
    if (!hgHandleIsNull(fence.handle))
        hgFenceAttach(fence, 1);

    u32 idx = ioThread.workingHead.fetch_add(1) & (ioThread.requestCapacity - 1);

    ioThread.requests[idx].fence = fence;
    ioThread.requests[idx].resource = data;
    ioThread.requests[idx].path = path;
    ioThread.requests[idx].fn = fn;
    ioThread.hasRequest[idx].store(true);

    u32 h = ioThread.head.load();
    while (ioThread.hasRequest[h].load())
    {
        u32 next = (h + 1) & (ioThread.requestCapacity - 1);
        ioThread.head.compare_exchange_strong(h, next);
        h = next;
    }
}

void hgAssetInitDefaults(
    HgArena* arena,
    u32 maxBinaries,
    u32 maxImages,
    u32 maxTextures,
    u32 maxMeshes,
    u32 maxModels)
{
    hgAssetInit<HgBinary>(arena, maxBinaries);
    hgAssetInit<HgImage>(arena, maxImages);
    hgAssetInit<HgTexture>(arena, maxTextures);
    hgAssetInit<HgMesh>(arena, maxMeshes);
    hgAssetInit<HgModel>(arena, maxModels);
}

template<>
void hgAssetLoadImpl<HgBinary>(HgAssetData<HgBinary>* data)
{
    hgIoRequest(data->isLoaded, &data->asset, data->path, [](void* pbin, HgStringView path)
    {
        HgBinary* bin = (HgBinary*)pbin;

        HgArena* scratch = hgScratch();
        HgArenaScope scratchScope{scratch};

        char* cpath = hgCString(scratch, path);

        FILE* fileHandle = fopen(cpath, "rb");
        if (fileHandle == nullptr)
        {
        hgWarn("Could not find file to read binary: %s\n", cpath);
        return;
        }
        hgDefer(fclose(fileHandle));

        if (fseek(fileHandle, 0, SEEK_END) != 0)
        {
            hgWarn("Failed to read binary from file: %s\n", cpath);
            return;
        }

        bin->size = (u32)ftell(fileHandle);
        bin->data = malloc(bin->size);

        rewind(fileHandle);
        if (fread(bin->data, 1, bin->size, fileHandle) != bin->size)
        {
            free(bin->data);
            *bin = {};
            hgWarn("Failed to read binary from file: %s\n", cpath);
            return;
        }
    });
}

template<>
void hgAssetUnloadImpl<HgBinary>(HgAssetData<HgBinary>* data)
{
    hgFenceWaitIndefinite(data->isLoaded);
    free(data->asset.data);
}

void hgBinaryStore(HgBinary* bin, HgStringView path, HgFence fence)
{
    hgIoRequest(fence, bin, path, [](void* pbin, HgStringView path)
    {
        HgBinary* bin = (HgBinary*)pbin;

        HgArena* scratch = hgScratch();
        HgArenaScope scratchScope{scratch};

        char* cpath = hgCString(scratch, path);

        FILE* fileHandle = fopen(cpath, "wb");
        if (fileHandle == nullptr)
        {
            hgWarn("Failed to create file to write binary: %s\n", cpath);
            return;
        }
        hgDefer(fclose(fileHandle));

        if (fwrite(bin->data, 1, bin->size, fileHandle) != bin->size)
        {
            hgWarn("Failed to write binary data to file: %s\n", cpath);
        }
    });
}

HgBinary hgBinaryResize(HgArena* arena, const HgBinary* bin, u64 newSize)
{
    HgBinary newBin{};
    newBin.data = hgRealloc(arena, bin->data, bin->size, newSize, 1);
    newBin.size = newSize;
    return newBin;
}

void hgBinaryRead(const HgBinary* bin, u64 idx, void* dst, u64 len)
{
    hgAssert(idx + len <= bin->size);
    memcpy(dst, (u8*)bin->data + idx, len);
}

void hgBinaryOverwrite(HgBinary* bin, u64 idx, const void* src, u64 len)
{
    hgAssert(idx + len <= bin->size);
    memcpy((u8*)bin->data + idx, src, len);
}

template<>
void hgAssetLoadImpl<HgImage>(HgAssetData<HgImage>* data)
{
    hgIoRequest(data->isLoaded, &data->asset, data->path, [](void* pimage, HgStringView path)
    {
        HgImage* image = (HgImage*)pimage;

        HgArena* scratch = hgScratch();
        HgArenaScope scratchScope{scratch};
        char* cpath = hgCString(scratch, path);

        int x, y, channels;
        image->pixels = stbi_load(cpath, &x, &y, &channels, 4);
        if (image->pixels == nullptr)
        {
            hgWarn("Could not load image: %s\n", cpath);
            return;
        }
        image->width = (u32)x;
        image->height = (u32)y;
        image->depth = 1;
        image->format = HgFormat_r8g8b8a8_srgb;
    });
}

template<>
void hgAssetUnloadImpl<HgImage>(HgAssetData<HgImage>* data)
{
    hgFenceWaitIndefinite(data->isLoaded);
    free(data->asset.pixels);
}

void hgImageStorePng(HgImage* image, HgStringView path, HgFence fence)
{
    hgIoRequest(fence, image, path, [](void* pimage, HgStringView fpath)
    {
        HgArena* scratch = hgScratch();
        HgArenaScope scratchScope{scratch};
        char* cpath = hgCString(scratch, fpath);

        HgImage* image = (HgImage*)pimage;
        stbi_write_png(cpath, (int)image->width, (int)image->height, 4, image->pixels, (int)(image->width * sizeof(u32)));
    });
}

template<>
void hgAssetLoadImpl<HgTexture>(HgAssetData<HgTexture>* data)
{
    HgImageHandle imageHandle = hgAssetLoad<HgImage>(data->path);
    hgDefer(hgAssetUnload(imageHandle));

    HgImage* image = hgAssetGet(imageHandle); 
    if (image->pixels == nullptr)
    {
        hgWarn("Could not load image: %.*s\n", (int)data->path.length, data->path.chars);
    }

    HgGpuImageCreateEx imageInfo{};
    imageInfo.format = image->format;
    imageInfo.width = image->width;
    imageInfo.height = image->height;
    imageInfo.depth = image->depth;
    imageInfo.usage = HgGpuImageUsage_transferDst | HgGpuImageUsage_sampled;

    data->asset.image = hgGpuImageCreateEx(&imageInfo);
    data->asset.view = hgGpuViewCreate(data->asset.image, 0, 1, 0, 1, HgGpuAspect_color);

    hgGpuImageWrite(data->asset.view, image->pixels);

    data->asset.sampler = hgGpuSamplerCreate(HgGpuFilter_linear);

    data->asset.descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo descInfo = {data->asset.sampler, data->asset.view, HgGpuLayout_shaderReadOnly};
    hgGpuDescriptorUpdate(data->asset.descriptor, nullptr, &descInfo);
}

template<>
void hgAssetUnloadImpl<HgTexture>(HgAssetData<HgTexture>* data)
{
    hgGpuDescriptorDestroy(data->asset.descriptor);
    hgGpuSamplerDestroy(data->asset.sampler);
    hgGpuViewDestroy(data->asset.view);
    hgGpuImageDestroy(data->asset.image);
}

template<>
void hgAssetLoadImpl<HgMesh>(HgAssetData<HgMesh>* data)
{
    (void)data;
    hgError("load gltf file : TODO\n");
}

template<>
void hgAssetUnloadImpl<HgMesh>(HgAssetData<HgMesh>* data)
{
    hgFenceWaitIndefinite(data->isLoaded);
    free(data->asset.indices);
    free(data->asset.vertices);
}

void hgMeshStoreGltf(HgMesh* data, HgStringView path, HgFence fence)
{
    (void)data;
    (void)path;
    (void)fence;
    hgError("store gltf file : TODO\n");
}

template<>
void hgAssetLoadImpl<HgModel>(HgAssetData<HgModel>* data)
{
    HgMeshHandle meshHandle = hgAssetLoad<HgMesh>(data->path);
    hgDefer(hgAssetUnload(meshHandle));

    HgMesh* mesh = hgAssetGet(meshHandle);
    if (mesh->vertices == nullptr || mesh->indices == nullptr)
    {
        hgWarn("Could not load model: %.*s\n", (int)data->path.length, data->path.chars);
        return;
    }

    data->asset.vertexCount = mesh->vertexCount;
    data->asset.vertexWidth = mesh->vertexWidth;
    data->asset.indexCount = mesh->indexCount;

    data->asset.vertexBuffer = hgGpuBufferCreate(
        data->asset.vertexCount * data->asset.vertexWidth,
        HgGpuBufferUsage_vertexBuffer | HgGpuBufferUsage_transferDst);

    data->asset.indexBuffer = hgGpuBufferCreate(
        data->asset.indexCount * sizeof(u32),
        HgGpuBufferUsage_indexBuffer | HgGpuBufferUsage_transferDst);

    hgGpuBufferWrite(
        data->asset.vertexBuffer,
        0,
        mesh->vertices,
        data->asset.vertexCount * data->asset.vertexWidth);

    hgGpuBufferWrite(
        data->asset.indexBuffer,
        0,
        mesh->indices,
        data->asset.indexCount * sizeof(u32));
}

template<>
void hgAssetUnloadImpl<HgModel>(HgAssetData<HgModel>* data)
{
    hgGpuBufferDestroy(data->asset.vertexBuffer);
    hgGpuBufferDestroy(data->asset.indexBuffer);
}

namespace {
    struct ComponentArr {
        u32* widths;
        u32 capacity;
        u32 count;

        ComponentArr(u32 initCount)
        {
            widths = (u32*)malloc(initCount * sizeof(u32));
            capacity = initCount;
            count = 0;
        }

        ~ComponentArr() {
            free(widths);
        }

        void push(u32 width)
        {
            if (capacity == count)
            {
                u32 newCapacity = capacity == 0 ? 1 : capacity * 2;
                widths = (u32*)realloc(widths, newCapacity);
                capacity = newCapacity;
            }
            widths[count++] = width;
        }
    };
}

ComponentArr& componentArr()
{
    static ComponentArr components{1024};
    return components;
}

static u32 componentWidth(u32 id)
{
    return componentArr().widths[id];
}

u32 hgCreateComponentID(u32 width)
{
    u32 id = componentArr().count;
    componentArr().push(width);
    return id;
}

HgEcs HgEcs::create(HgArena* arena, u32 maxEntities, u32 maxComponentTypes)
{
    HgEcs ecs{};

    ecs.entityPool = hgPoolCreate<void>(arena, maxEntities);
    ecs.systems = hgMapCreate<u32, System>(arena, maxComponentTypes + 1);
    ecs.reset();

    return ecs;
}

void HgEcs::createComponent(HgArena* arena, u32 maxCount, u32 width, u32 align, u32 componentID)
{
    hgAssert(hgMapGet(&systems, componentID) == nullptr);
    System* system = hgMapAdd(&systems, componentID, {});

    system->indices = hgAlloc<u32>(arena, entityPool.capacity);
    system->entities = hgAlloc<HgEntity>(arena, maxCount);
    system->components = hgAlloc(arena, maxCount * width, align);
    system->count = 0;
    system->capacity = maxCount;

    memset(system->indices, -1, entityPool.capacity * sizeof(*system->indices));
}

void HgEcs::reset()
{
    for (u32 i = 0; i < systems.capacity; ++i)
    {
        if (systems.hasVal[i])
        {
            memset(systems.vals[i].indices, -1, entityPool.capacity * sizeof(*systems.vals[i].indices));
            systems.vals[i].count = 0;
        }
    }

    hgPoolReset(&entityPool);
}

HgEntity HgEcs::spawn()
{
    return {hgPoolAlloc(&entityPool)};
}

void HgEcs::despawn(HgEntity e)
{
    hgAssert(alive(e));

    for (u32 i = 0; i < systems.capacity; ++i)
    {
        if (systems.hasVal[i] && has(e, systems.keys[i]))
            remove(e, systems.keys[i]);
    }
    hgPoolFree(&entityPool, e.handle);
}

bool HgEcs::alive(HgEntity e)
{
    return hgPoolAlive(&entityPool, e.handle);
}

void* HgEcs::add(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));
    hgAssert(!has(e, componentID));

    System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    system->indices[hgHandleIdx(e.handle)] = system->count;
    system->entities[system->count] = e;
    void* c = (u8*)system->components + componentWidth(componentID) * system->count;
    ++system->count;
    return c;
}

void HgEcs::remove(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));
    hgAssert(has(e, componentID));

    System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    HgEntity last = system->entities[system->count - 1];
    system->entities[system->count - 1] = HgEntity{};
    if (e.handle.id != last.handle.id)
    {
        u32 idx = system->indices[hgHandleIdx(e.handle)];
        system->entities[idx] = last;
        system->indices[hgHandleIdx(last.handle)] = idx;
        memcpy(
            (u8*)system->components + componentWidth(componentID) * idx,
            (u8*)system->components + componentWidth(componentID) * (system->count - 1),
            componentWidth(componentID));
    }
    system->indices[hgHandleIdx(e.handle)] = (u32)-1;
    --system->count;
}

bool HgEcs::has(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));

    System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    return system->indices[hgHandleIdx(e.handle)] < system->count;
}

void* HgEcs::get(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));

    System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    return (u8*)system->components + componentWidth(componentID) * system->indices[hgHandleIdx(e.handle)];
}

HgEntity HgEcs::get(const void* component, u32 componentID)
{
    hgAssert(component != nullptr);

    System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    return system->entities[(u32)((uptr)component - (uptr)system->components) / componentWidth(componentID)];
}

u32 HgEcs::findSmallest(u32* ids, u32 idCount)
{
    u32 smallestCount = (u32)-1;
    u32 smallest = ids[0];

    for (u32 i = 1; i < idCount; ++i)
    {
        System* system = hgMapGet(&systems, ids[i]);
        hgAssert(system != nullptr);

        if (system->count < smallestCount)
        {
            smallestCount = system->count;
            smallest = ids[i];
        }
    }
    return smallest;
}

static void swapIdxLocation(HgEcs* ecs, u32 lhs, u32 rhs, u32 componentID)
{
    HgEcs::System* system = hgMapGet(&ecs->systems, componentID);
    hgAssert(system != nullptr);

    hgAssert(lhs < system->count);
    hgAssert(rhs < system->count);

    HgEntity lhsEntity = system->entities[lhs];
    HgEntity rhsEntity = system->entities[rhs];

    hgAssert(ecs->alive(lhsEntity));
    hgAssert(ecs->alive(rhsEntity));
    hgAssert(ecs->has(lhsEntity, componentID));
    hgAssert(ecs->has(rhsEntity, componentID));

    HgArena* scratch = hgScratch();
    HgArenaScope scratchScope{scratch};

    system->entities[lhs] = rhsEntity;
    system->entities[rhs] = lhsEntity;
    system->indices[hgHandleIdx(lhsEntity.handle)] = rhs;
    system->indices[hgHandleIdx(rhsEntity.handle)] = lhs;

    u32 width = componentWidth(componentID);
    void* temp = hgAlloc(scratch, componentWidth(componentID), 1);
    memcpy(temp, (u8*)system->components + width * lhs, width);
    memcpy((u8*)system->components + width * lhs, (u8*)system->components + width * rhs, width);
    memcpy((u8*)system->components + width * rhs, temp, width);
}

namespace {
    struct QuicksortData {
        HgEcs* ecs;
        HgEcs::System* system;
        u32 comp;
        void* data;
        bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs);

        u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
        {
            while (inc != dec)
            {
                while (!compare(data, ecs, system->entities[dec], system->entities[pivot]))
                {
                    --dec;
                    if (dec == inc)
                        goto finish;
                }
                while (!compare(data, ecs, system->entities[pivot], system->entities[inc]))
                {
                    ++inc;
                    if (inc == dec)
                        goto finish;
                }
                swapIdxLocation(ecs, inc, dec, comp);
            }

        finish:
            if (compare(data, ecs, system->entities[inc], system->entities[pivot]))
                swapIdxLocation(ecs, pivot, inc, comp);

            return inc;
        }

        void quicksort(u32 begin, u32 end)
        {
            if (begin + 1 >= end)
                return;

            u32 middle = quicksortInter(begin, begin + 1, end - 1);
            quicksort(begin, middle);
            quicksort(middle, end);
        }
    };
}

void HgEcs::sort(
    u32 componentID,
    void* data,
    bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs))
{
    hgAssert(compare != nullptr);

    HgEcs::System* system = hgMapGet(&systems, componentID);
    hgAssert(system != nullptr);

    QuicksortData q{this, system, componentID, data, compare};
    q.quicksort(0, system->count);
}

void hgAddChildEntity(HgEcs* ecs, HgEntity parent, HgEntity child)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(parent);
    HgHierarchy& oldFirst = ecs->get<HgHierarchy>(node.firstChild);
    HgHierarchy& newFirst = ecs->get<HgHierarchy>(child);

    hgAssert(newFirst.parent.handle.id == HgEntity{}.handle.id);
    hgAssert(newFirst.prevSibling.handle.id == HgEntity{}.handle.id);
    hgAssert(newFirst.nextSibling.handle.id == HgEntity{}.handle.id);

    newFirst.parent = parent;
    newFirst.nextSibling = node.firstChild;

    oldFirst.prevSibling = child;
    node.firstChild = child;
}

void hgDetachEntity(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(e);
    if (node.parent.handle.id != HgEntity{}.handle.id)
    {
        if (node.prevSibling.handle.id == HgEntity{}.handle.id)
            ecs->get<HgHierarchy>(node.parent).firstChild = node.nextSibling;
        else
            ecs->get<HgHierarchy>(node.prevSibling).nextSibling = node.nextSibling;
        ecs->get<HgHierarchy>(node.nextSibling).prevSibling = node.prevSibling;

        HgEntity child = node.firstChild;
        while (child.handle.id != HgEntity{}.handle.id)
        {
            HgHierarchy& tf = ecs->get<HgHierarchy>(child);
            HgEntity next = tf.nextSibling;
            tf.parent = HgEntity{};
            tf.nextSibling = HgEntity{};
            tf.prevSibling = HgEntity{};
            hgAddChildEntity(ecs, node.parent, child);
            child = next;
        }
    } else {
        hgAssert(node.prevSibling.handle.id == HgEntity{}.handle.id);
        hgAssert(node.nextSibling.handle.id == HgEntity{}.handle.id);
        HgEntity child = node.firstChild;
        while (child.handle.id != HgEntity{}.handle.id)
        {
            HgHierarchy& tf = ecs->get<HgHierarchy>(child);
            child = tf.nextSibling;
            tf.parent = HgEntity{};
            tf.nextSibling = HgEntity{};
            tf.prevSibling = HgEntity{};
        }
    }
    node = {};
}

void hgDestroyEntity(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(e);
    HgEntity child = node.firstChild;
    while (child.handle.id != HgEntity{}.handle.id)
    {
        HgHierarchy& tf = ecs->get<HgHierarchy>(child);
        HgEntity next = tf.nextSibling;
        tf.parent = HgEntity{};
        tf.prevSibling = HgEntity{};
        tf.nextSibling = HgEntity{};
        hgDestroyEntity(ecs, child);
        child = next;
    }
    if (node.parent.handle.id != HgEntity{}.handle.id)
    {
        if (node.prevSibling.handle.id != HgEntity{}.handle.id)
            ecs->get<HgHierarchy>(node.prevSibling).nextSibling = node.nextSibling;
        else
            ecs->get<HgHierarchy>(node.parent).firstChild = node.nextSibling;
        if (node.nextSibling.handle.id != HgEntity{}.handle.id)
            ecs->get<HgHierarchy>(node.nextSibling).prevSibling = HgEntity{};
    }
    ecs->despawn(e);
}

void hgSetEntity(HgEcs* ecs, HgEntity e, const HgVec3& pos, const HgVec3& scale, const HgQuat& rot)
{
    hgAssert(ecs != nullptr);

    HgTransform& tf = ecs->get<HgTransform>(e);
    if (ecs->has<HgHierarchy>(e))
    {
        HgHierarchy& node = ecs->get<HgHierarchy>(e);
        HgEntity child = node.firstChild;
        while (child.handle.id != HgEntity{}.handle.id)
        {
            HgHierarchy& cNode = ecs->get<HgHierarchy>(child);
            HgTransform& cTf = ecs->get<HgTransform>(child);
            HgTransform rel;
            rel.position = cTf.position - tf.position;
            rel.scale = cTf.scale / tf.scale;
            rel.rotation = hgQuatConj(tf.rotation) * cTf.rotation;
            // hgSetEntity(ecs, child, // : TODO
            //     hgRotate(r, (cTf.position - tf.position) * s / cTf.scale + p),
            //     s * cTf.scale / tf.scale,
            //     r);
            child = cNode.nextSibling;
        }
    }
    tf.position = pos;
    tf.scale = scale;
    tf.rotation = rot;
}

void hgMoveEntity(HgEcs* ecs, HgEntity e, const HgVec3& dpos, const HgVec3& dscale, const HgQuat& drot)
{
    hgAssert(ecs != nullptr);

    HgTransform& tf = ecs->get<HgTransform>(e);
    hgSetEntity(ecs, e, tf.position + dpos, tf.scale * dscale, drot * tf.rotation);
}

struct Pipeline2DVPUniform {
    HgMat4 proj;
    HgMat4 view;
};

struct Pipeline2DPush {
    HgMat4 model;
    HgVec2 uvPos;
    HgVec2 uvSize;
    u32 vpIdx;
    u32 texIdx;
};

struct Pipeline2DState {
    HgGpuPipeline* pipeline;
    HgGpuBuffer* vpBuffer;
    HgGpuDescriptor vpDesc;
    HgTexture defaultTex;
};

static Pipeline2DState pipeline2D{};

void hgInitPipeline2D(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgBinaryHandle spriteVertSpvHandle = hgAssetLoad<HgBinary>("build/sprite.vert.spv");
    HgBinaryHandle spriteFragSpvHandle = hgAssetLoad<HgBinary>("build/sprite.frag.spv");
    hgDefer(hgAssetUnload(spriteVertSpvHandle));
    hgDefer(hgAssetUnload(spriteFragSpvHandle));

    HgBinary* spriteVertSpv = hgAssetGet(spriteVertSpvHandle);
    HgBinary* spriteFragSpv = hgAssetGet(spriteFragSpvHandle);

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = (u8*)spriteVertSpv->data;
    pipelineConfig.vertexShaderSize = spriteVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)spriteFragSpv->data;
    pipelineConfig.fragmentShaderSize = spriteFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    HgGpuPushRange push{0, sizeof(Pipeline2DPush)};
    pipelineConfig.pushRanges = &push;
    pipelineConfig.pushRangeCount = 1;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    pipeline2D.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    pipeline2D.vpBuffer = hgGpuBufferCreate(
        sizeof(Pipeline2DVPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    Pipeline2DVPUniform vpData{};
    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    hgGpuBufferWrite(pipeline2D.vpBuffer, 0, &vpData, sizeof(vpData));

    pipeline2D.vpDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_uniformBuffer);

    HgGpuBufferDescriptorInfo bufferInfo{pipeline2D.vpBuffer, 0, sizeof(Pipeline2DVPUniform)};
    hgGpuDescriptorUpdate(pipeline2D.vpDesc, &bufferInfo, nullptr);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    pipeline2D.defaultTex.image = hgGpuImageCreate(2, 2, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    pipeline2D.defaultTex.view = hgGpuViewCreate(pipeline2D.defaultTex.image, 0, 1, 0, 1, HgGpuAspect_color);

    hgGpuImageWrite(pipeline2D.defaultTex.view, defaultColors);

    pipeline2D.defaultTex.sampler = hgGpuSamplerCreate(HgGpuFilter_nearest);

    pipeline2D.defaultTex.descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo imageInfo{};
    imageInfo.sampler = pipeline2D.defaultTex.sampler;
    imageInfo.imageView = pipeline2D.defaultTex.view;
    imageInfo.imageLayout = HgGpuLayout_shaderReadOnly;

    hgGpuDescriptorUpdate(pipeline2D.defaultTex.descriptor, nullptr, &imageInfo);
}

void hgDeinitPipeline2D()
{
    hgGpuPipelineDestroy(pipeline2D.pipeline);

    hgGpuDescriptorDestroy(pipeline2D.defaultTex.descriptor);
    hgGpuSamplerDestroy(pipeline2D.defaultTex.sampler);
    hgGpuViewDestroy(pipeline2D.defaultTex.view);
    hgGpuImageDestroy(pipeline2D.defaultTex.image);

    hgGpuDescriptorDestroy(pipeline2D.vpDesc);
    hgGpuBufferDestroy(pipeline2D.vpBuffer);
}

void hgUpdateProjection2D(const HgMat4* projection)
{
    hgAssert(projection != nullptr);
    hgGpuBufferWrite(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, proj), projection, sizeof(*projection));
}

void hgUpdateView2D(const HgMat4* view)
{
    hgAssert(view != nullptr);
    hgGpuBufferWrite(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, view), view, sizeof(*view));
}

void hgDraw2D(HgEcs* ecs, HgGpuCmd* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    // ecs->sort<HgSprite2D>(nullptr, [](void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs) -> bool {
    //     hgAssert(ecs->has<HgTransform>(lhs));
    //     hgAssert(ecs->has<HgTransform>(rhs));
    //     return ecs->get<HgTransform>(lhs).position.z > ecs->get<HgTransform>(rhs).position.z;
    // });

    hgGpuBindPipeline(cmd, pipeline2D.pipeline);

    ecs->forEach<HgSprite2D, HgTransform>([&](HgEntity, HgSprite2D* sprite, HgTransform* transform)
    {
        HgTexture* texture = hgHandleIsNull(sprite->texture.handle)
            ? &pipeline2D.defaultTex
            : hgAssetGet(sprite->texture);

        Pipeline2DPush push{};
        push.model = hgMatModel3D(transform->position, transform->scale, transform->rotation);
        push.uvPos = sprite->uvPos;
        push.uvSize = sprite->uvSize;
        push.vpIdx = hgGpuDescriptorIdx(pipeline2D.vpDesc);
        push.texIdx = hgGpuDescriptorIdx(texture->descriptor);

        hgGpuPushConstants(cmd, pipeline2D.pipeline, 0, &push, sizeof(push));

        hgGpuDraw(cmd, 0, 6, 0, 1);
    });
}

struct Pipeline3DVPUniform {
    HgMat4 proj;
    HgMat4 view;
};

struct Pipeline3DDirLightData {
    HgVec4 dir;
    HgVec4 color;
};

struct Pipeline3DPointLightData {
    HgVec4 pos;
    HgVec4 color;
};

struct Pipeline3DPush {
    HgMat4 model;
    u32 vpIdx;
    u32 dirLightIdx;
    u32 dirLightCount;
    u32 pointLightIdx;
    u32 pointLightCount;
    u32 colorMapIdx;
    u32 normalMapIdx;
};

struct Pipeline3DState {
    HgGpuPipeline* pipeline;

    HgGpuBuffer* vpBuffer;
    Pipeline3DVPUniform vpData;
    HgGpuDescriptor vpDesc;

    HgGpuBuffer* dirLightBuffer;
    u32 dirLightCapacity;
    HgGpuDescriptor dirLightDesc;

    HgGpuBuffer* pointLightBuffer;
    u32 pointLightCapacity;
    HgGpuDescriptor pointLightDesc;

    HgModel defaultModel;
    HgGpuSampler* defaultMapSampler;
    HgTexture defaultColorMap;
    HgTexture defaultNormalMap;
};

static Pipeline3DState pipeline3D{};

void hgInitPipeline3D(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgBinaryHandle modelVertSpvHandle = hgAssetLoad<HgBinary>("build/model.vert.spv");
    HgBinaryHandle modelFragSpvHandle = hgAssetLoad<HgBinary>("build/model.frag.spv");
    hgDefer(hgAssetUnload(modelVertSpvHandle));
    hgDefer(hgAssetUnload(modelFragSpvHandle));

    HgBinary* modelVertSpv = hgAssetGet(modelVertSpvHandle);
    HgBinary* modelFragSpv = hgAssetGet(modelFragSpvHandle);

    HgGpuVertexBinding vertexBindings[]{
        {0, sizeof(HgMeshVertex), false},
    };
    HgGpuVertexAttribute vertexAttributes[]{
        {0, 0, HgFormat_r32g32b32_sfloat, offsetof(HgMeshVertex, pos)},
        {1, 0, HgFormat_r32g32b32_sfloat, offsetof(HgMeshVertex, norm)},
        {2, 0, HgFormat_r32g32b32a32_sfloat, offsetof(HgMeshVertex, tan)},
        {3, 0, HgFormat_r32g32_sfloat, offsetof(HgMeshVertex, uv)},
    };
    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = (u8*)modelVertSpv->data;
    pipelineConfig.vertexShaderSize = modelVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)modelFragSpv->data;
    pipelineConfig.fragmentShaderSize = modelFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    HgGpuPushRange push{0, sizeof(Pipeline3DPush)};
    pipelineConfig.pushRanges = &push;
    pipelineConfig.pushRangeCount = 1;
    pipelineConfig.vertexBindings = vertexBindings;
    pipelineConfig.vertexBindingCount = sizeof(vertexBindings) / sizeof(*vertexBindings);
    pipelineConfig.vertexAttributes = vertexAttributes;
    pipelineConfig.vertexAttributeCount = sizeof(vertexAttributes) / sizeof(*vertexAttributes);
    pipelineConfig.cullMode = HgGpuCull_back;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;

    pipeline3D.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    pipeline3D.vpData.proj = HgMat4{1.0f};
    pipeline3D.vpData.view = HgMat4{1.0f};

    pipeline3D.vpBuffer = hgGpuBufferCreate(
        sizeof(Pipeline3DVPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    hgGpuBufferWrite(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    pipeline3D.vpDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_uniformBuffer);

    HgGpuBufferDescriptorInfo bufferInfo{pipeline3D.vpBuffer, 0, sizeof(Pipeline3DVPUniform)};
    hgGpuDescriptorUpdate(pipeline3D.vpDesc, &bufferInfo, nullptr);

    pipeline3D.dirLightCapacity = 4;
    pipeline3D.dirLightBuffer = hgGpuBufferCreate(
        sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    pipeline3D.pointLightCapacity = 4;
    pipeline3D.pointLightBuffer = hgGpuBufferCreate(
        sizeof(Pipeline3DPointLightData) * pipeline3D.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    pipeline3D.dirLightDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_storageBuffer);
    pipeline3D.pointLightDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_storageBuffer);

    HgGpuBufferDescriptorInfo dirLightBufferInfo{pipeline3D.dirLightBuffer, 0, UINT64_MAX};
    hgGpuDescriptorUpdate(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);

    HgGpuBufferDescriptorInfo pointLightBufferInfo{pipeline3D.pointLightBuffer, 0, UINT64_MAX};
    hgGpuDescriptorUpdate(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);

    static const HgMeshVertex cubeVertices[]{
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,1}},
    };

    static u32 cubeIndices[]{
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    pipeline3D.defaultModel.vertexBuffer = hgGpuBufferCreate(
        sizeof(cubeVertices),
        HgGpuBufferUsage_vertexBuffer | HgGpuBufferUsage_transferDst);

    pipeline3D.defaultModel.indexBuffer = hgGpuBufferCreate(
        sizeof(cubeIndices),
        HgGpuBufferUsage_indexBuffer | HgGpuBufferUsage_transferDst);

    hgGpuBufferWrite(pipeline3D.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgGpuBufferWrite(pipeline3D.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    pipeline3D.defaultModel.vertexCount = sizeof(cubeVertices) / sizeof(*cubeVertices);
    pipeline3D.defaultModel.vertexWidth = sizeof(HgMeshVertex);
    pipeline3D.defaultModel.indexCount = sizeof(cubeIndices) / sizeof(*cubeIndices);

    pipeline3D.defaultMapSampler = hgGpuSamplerCreate(HgGpuFilter_nearest);

    pipeline3D.defaultColorMap.sampler = pipeline3D.defaultMapSampler;
    pipeline3D.defaultNormalMap.sampler = pipeline3D.defaultMapSampler;

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    static const HgVec4 defaultNormals[]{
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
    };

    pipeline3D.defaultColorMap.image = hgGpuImageCreate(3, 3, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);
    pipeline3D.defaultNormalMap.image = hgGpuImageCreate(2, 2, HgFormat_r32g32b32a32_sfloat,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    pipeline3D.defaultColorMap.view = hgGpuViewCreate(
        pipeline3D.defaultColorMap.image, 0, 1, 0, 1, HgGpuAspect_color);
    pipeline3D.defaultNormalMap.view = hgGpuViewCreate(
        pipeline3D.defaultNormalMap.image, 0, 1, 0, 1, HgGpuAspect_color);

    hgGpuImageWrite(pipeline3D.defaultColorMap.view, defaultColors);
    hgGpuImageWrite(pipeline3D.defaultNormalMap.view, defaultNormals);

    pipeline3D.defaultColorMap.descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);
    pipeline3D.defaultNormalMap.descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo colorInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultColorMap.view, HgGpuLayout_shaderReadOnly};
    hgGpuDescriptorUpdate(pipeline3D.defaultColorMap.descriptor, nullptr, &colorInfo);

    HgGpuImageDescriptorInfo normalInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultNormalMap.view, HgGpuLayout_shaderReadOnly};
    hgGpuDescriptorUpdate(pipeline3D.defaultNormalMap.descriptor, nullptr, &normalInfo);
}

void hgDeinitPipeline3D()
{
    hgGpuPipelineDestroy(pipeline3D.pipeline);

    hgGpuDescriptorDestroy(pipeline3D.defaultNormalMap.descriptor);
    hgGpuViewDestroy(pipeline3D.defaultNormalMap.view);
    hgGpuImageDestroy(pipeline3D.defaultNormalMap.image);

    hgGpuDescriptorDestroy(pipeline3D.defaultColorMap.descriptor);
    hgGpuViewDestroy(pipeline3D.defaultColorMap.view);
    hgGpuImageDestroy(pipeline3D.defaultColorMap.image);

    hgGpuSamplerDestroy(pipeline3D.defaultMapSampler);

    hgGpuBufferDestroy(pipeline3D.defaultModel.indexBuffer);
    hgGpuBufferDestroy(pipeline3D.defaultModel.vertexBuffer);

    hgGpuDescriptorDestroy(pipeline3D.pointLightDesc);
    hgGpuDescriptorDestroy(pipeline3D.dirLightDesc);
    hgGpuBufferDestroy(pipeline3D.pointLightBuffer);
    hgGpuBufferDestroy(pipeline3D.dirLightBuffer);

    hgGpuDescriptorDestroy(pipeline3D.vpDesc);
    hgGpuBufferDestroy(pipeline3D.vpBuffer);
}

void hgUpdateProjection3D(const HgMat4* projection)
{
    hgAssert(projection != nullptr);
    pipeline3D.vpData.proj = *projection;
}

void hgUpdateView3D(const HgMat4* view)
{
    hgAssert(view != nullptr);
    pipeline3D.vpData.view = *view;
}

void hgDraw3D(HgEcs* ecs, HgGpuCmd* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgScratch();
    HgArenaScope scratchScope{scratch};

    hgGpuBufferWrite(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    u32 dirLightCount = ecs->count<HgDirLight3D>();
    u32 pointLightCount = ecs->count<HgPointLight3D>();

    if (dirLightCount > pipeline3D.dirLightCapacity)
    {
        hgGpuWaitIdle();

        pipeline3D.dirLightCapacity = pipeline3D.dirLightCapacity == 0 ? 1 : pipeline3D.dirLightCapacity * 2;
        while (pipeline3D.dirLightCapacity < dirLightCount)
        {
            pipeline3D.dirLightCapacity *= 2;
        }

        hgGpuBufferDestroy(pipeline3D.dirLightBuffer);
        pipeline3D.dirLightBuffer = hgGpuBufferCreate(
            sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);

        HgGpuBufferDescriptorInfo dirLightBufferInfo{pipeline3D.dirLightBuffer, 0, UINT64_MAX};
        hgGpuDescriptorUpdate(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);
    }

    if (pointLightCount > pipeline3D.pointLightCapacity)
    {
        hgGpuWaitIdle();

        pipeline3D.pointLightCapacity = pipeline3D.pointLightCapacity == 0 ? 1 : pipeline3D.pointLightCapacity * 2;
        while (pipeline3D.pointLightCapacity < pointLightCount)
        {
            pipeline3D.pointLightCapacity *= 2;
        }

        hgGpuBufferDestroy(pipeline3D.pointLightBuffer);
        pipeline3D.pointLightBuffer = hgGpuBufferCreate(
            sizeof(Pipeline3DPointLightData) * pipeline3D.pointLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);

        HgGpuBufferDescriptorInfo pointLightBufferInfo{pipeline3D.pointLightBuffer, 0, UINT64_MAX};
        hgGpuDescriptorUpdate(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);
    }

    Pipeline3DDirLightData* dirLights = hgAlloc<Pipeline3DDirLightData>(scratch, dirLightCount);
    Pipeline3DPointLightData* pointLights = hgAlloc<Pipeline3DPointLightData>(scratch, pointLightCount);

    u32 i = 0;
    ecs->forEach<HgDirLight3D>([&](HgEntity, HgDirLight3D* light)
    {
        dirLights[i].dir = HgVec4{HgMat3{pipeline3D.vpData.view} * light->dir, 0.0};
        dirLights[i].color = light->color;
        ++i;
    });

    i = 0;
    ecs->forEach<HgPointLight3D, HgTransform>([&](HgEntity, HgPointLight3D* light, HgTransform* transform)
    {
        pointLights[i].pos = pipeline3D.vpData.view * HgVec4{transform->position, 1.0};
        pointLights[i].color = light->color;
        ++i;
    });

    if (dirLightCount > 0)
        hgGpuBufferWrite(pipeline3D.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        hgGpuBufferWrite(pipeline3D.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    hgGpuBindPipeline(cmd, pipeline3D.pipeline);

    ecs->forEach<HgModel3D, HgTransform>([&](HgEntity, HgModel3D* model, HgTransform* transform)
    {
        HgTexture* colorMap = hgHandleIsNull(model->colorMap.handle)
            ? &pipeline3D.defaultColorMap
            : hgAssetGet(model->colorMap);

        HgTexture* normalMap = hgHandleIsNull(model->normalMap.handle)
            ? &pipeline3D.defaultNormalMap
            : hgAssetGet(model->normalMap);

        Pipeline3DPush push{};
        push.model = hgMatModel3D(transform->position, transform->scale, transform->rotation);
        push.dirLightIdx = hgGpuDescriptorIdx(pipeline3D.dirLightDesc);
        push.dirLightCount = dirLightCount;
        push.pointLightIdx = hgGpuDescriptorIdx(pipeline3D.pointLightDesc);
        push.pointLightCount = pointLightCount;
        push.vpIdx = hgGpuDescriptorIdx(pipeline3D.vpDesc);
        push.colorMapIdx = hgGpuDescriptorIdx(colorMap->descriptor);
        push.normalMapIdx = hgGpuDescriptorIdx(normalMap->descriptor);

        HgModel* gpuModel = hgHandleIsNull(model->model.handle)
            ? &pipeline3D.defaultModel
            : hgAssetGet(model->model);

        hgGpuBindIndexBuffer(cmd, gpuModel->indexBuffer);

        HgGpuBuffer* buffers[]{gpuModel->vertexBuffer};
        u64 offsets[]{0};
        hgGpuBindVertexBuffers(cmd, 0, buffers, offsets, 1);

        hgGpuPushConstants(cmd, pipeline3D.pipeline, 0, &push, sizeof(push));

        hgGpuDrawIndexed(cmd, 0, 0, gpuModel->indexCount, 0, 1);
    });
}
