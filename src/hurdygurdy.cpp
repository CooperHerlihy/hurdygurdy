#include "hurdygurdy.hpp"

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

    hgInitScratchMemory(init->arenaSize);
    HgArena* arena = hgGetScratch();

    hgInitPlatform(arena, init->maxWindows, init->maxWindowEvents);
    hgInitGraphics(arena);

    u32 workerCount = (u32)std::max(1, (i32)hgHardwareThreadCount() - 2); // main thread, io thread
    hgInitThreadPool(arena, init->threadPoolQueueSize, workerCount);
    hgInitIOThread(arena, init->ioRequestQueueSize);
    hgInitResources(arena, init->maxResources);
    hgInitGpuResources(arena, init->maxTextures, init->maxModels);
}

void hgDeinit(void)
{
    hgDeinitGpuResources();
    hgDeinitResource();
    hgDeinitIOThread();
    hgDeinitThreadPool();

    hgDeinitGraphics();
    hgDeinitPlatform();

    hgDeinitScratchMemory();
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

void hgAddVec(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] + rhs[i];
    }
}

void hgSubVec(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] - rhs[i];
    }
}

void hgPairwiseMulVec(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] * rhs[i];
    }
}

void hgMulVecScalar(u32 size, f32* dst, f32 scalar, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = scalar * vec[i];
    }
}

void hgPairwiseDivVec(u32 size, f32* dst, const f32* lhs, const f32* rhs)
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

void hgDivVecScalar(u32 size, f32* dst, const f32* vec, f32 scalar)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgAssert(scalar != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / scalar;
    }
}

void hgDot(u32 size, f32* dst, const f32* lhs, const f32* rhs)
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

void hgLen(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgDot(size, dst, vec, vec);
    *dst = (f32)sqrt(*dst);
}

f32 hgLen(HgVec2 vec)
{
    return (f32)sqrt(hgDot(vec, vec));
}

f32 hgLen(HgVec3 vec)
{
    return (f32)sqrt(hgDot(vec, vec));
}

f32 hgLen(HgVec4 vec)
{
    return (f32)sqrt(hgDot(vec, vec));
}

void hgNorm(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    f32 len;
    hgLen(size, &len, vec);
    hgAssert(len != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hgNorm(HgVec2 vec)
{
    f32 len = hgLen(vec);
    hgAssert(len != 0);
    return HgVec2{vec.x / len, vec.y / len};
}

HgVec3 hgNorm(HgVec3 vec)
{
    f32 len = hgLen(vec);
    hgAssert(len != 0);
    return HgVec3{vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hgNorm(HgVec4 vec)
{
    f32 len = hgLen(vec);
    hgAssert(len != 0);
    return HgVec4{vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hgCross(f32* dst, const f32* lhs, const f32* rhs)
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

void hgAddMat(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
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
    hgAddMat(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgAddMat(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgAddMat(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgSubMat(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
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
    hgSubMat(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgSubMat(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgSubMat(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgMulMat(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
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
    hgMulMat(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMulMat(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMulMat(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

void hgMulMatVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
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
    hgMulMatVec(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec3 operator*(const HgMat3& lhs, HgVec3 rhs)
{
    HgVec3 result{};
    hgMulMatVec(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec4 operator*(const HgMat4& lhs, HgVec4 rhs)
{
    HgVec4 result{};
    hgMulMatVec(4, 4, &result.x, &lhs.x.x, &rhs.x);
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

HgQuat hgAxisAngle(HgVec3 axis, f32 angle)
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

HgVec3 hgRotate(HgQuat lhs, HgVec3 rhs)
{
    HgQuat q = lhs * HgQuat{0, rhs.x, rhs.y, rhs.z} * hgConj(lhs);
    return HgVec3{q.i, q.j, q.k};
}

HgMat3 hgRotate(HgQuat lhs, HgMat3 rhs)
{
    return HgMat3{
        hgRotate(lhs, rhs.x),
        hgRotate(lhs, rhs.y),
        hgRotate(lhs, rhs.z),
    };
}

HgMat4 hgModelMatrix2D(HgVec3 position, HgVec2 scale, f32 rotation)
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

HgMat4 hgModelMatrix3D(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation)
{
    HgMat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hgRotate(rotation, m3);
    HgMat4 m4 = HgMat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgViewMatrix(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation)
{
    HgMat4 rot{hgRotate(hgConj(rotation), HgMat3{1.0f})};
    HgMat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

HgMat4 hgOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return HgMat4{
        HgVec4{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        HgVec4{-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hgPerspective(f32 fov, f32 aspect, f32 near, f32 far)
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

u32 hgNoise(u32 seed, u32 x, u32 y)
{
    return hgNoise(seed, x + (y * 425537443u));
}

u32 hgNoise(u32 seed, u32 x, u32 y, u32 z)
{
    return hgNoise(seed, x + y * 425537443u + z * 682607u);
}

u32 hgNoise(u32 seed, u32 x, u32 y, u32 z, u32 w)
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

f32 hgNoiseNorm(u32 seed, HgVec2 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm(u32 seed, HgVec3 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32, Convert{pos.z}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm(u32 seed, HgVec4 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise(
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
    f32 rot = 2.0f * (f32)hgPi * hgNoiseNorm(seed, pos);
    return HgVec2(std::cos(rot), std::sin(rot));
}

u32 hgMaxMipmaps(u32 width, u32 height, u32 depth)
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

void hgInitScratchMemory(u64 size)
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

void hgDeinitScratchMemory()
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

HgArena* hgGetScratch(HgArena const* const* conflicts, u32 count)
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

HgString hgCreateString(HgArena* arena, u64 capacity)
{
    hgAssert(arena != nullptr);

    HgString str;
    str.chars = hgAlloc<char>(arena, capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString hgCopyString(HgArena* arena, HgStringView str)
{
    hgAssert(arena != nullptr);

    HgString copy;
    copy.chars = hgAlloc<char>(arena, str.length);
    copy.capacity = str.length;
    copy.length = str.length;
    memcpy(copy.chars, str.chars, str.length);
    return copy;
}

void hgReserveString(HgArena* arena, HgString* str, u64 newCapacity)
{
    hgAssert(arena != nullptr);

    str->chars = hgRealloc(arena, str->chars, str->capacity, newCapacity);
    str->capacity = newCapacity;
}

void hgGrowString(HgArena* arena, HgString* str, f32 factor)
{
    hgAssert(arena != nullptr);
    hgAssert(str != nullptr);
    hgAssert(factor > 1.0f);

    hgAssert(str->capacity <= (u64)((f32)SIZE_MAX / factor));
    hgReserveString(arena, str, str->capacity == 0 ? 1 : (u64)((f32)str->capacity * factor));
}

void hgInsertString(HgArena* arena, HgString* dst, u64 idx, HgStringView src)
{
    hgAssert(arena != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(idx <= dst->length);
    if (src.length > 0)
        hgAssert(src.chars != nullptr);

    u64 newLength = dst->length + src.length;
    while (dst->capacity < newLength)
    {
        hgGrowString(arena, dst);
    }

    if (idx != dst->length)
        memmove(&dst->chars[idx + src.length], &dst->chars[idx], dst->length - idx);
    memcpy(&dst->chars[idx], src.chars, src.length);
    dst->length = newLength;
}

void hgInsertString(HgArena* arena, HgString* dst, u64 idx, char c)
{
    hgAssert(arena != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(idx <= dst->length);

    if (dst->capacity < dst->length + 1)
        hgGrowString(arena, dst);

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

bool hgIsIntenger(HgStringView str)
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

i64 hgStrToInt(HgStringView str)
{
    hgAssert(hgIsIntenger(str));

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

f64 hgStrToFloat(HgStringView str)
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
        ret += (f64)hgStrToInt({&str[intPartBegin], &str[head]});
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

        i64 exp = hgStrToInt({&str[expBegin], str.chars + head});
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

HgString hgIntToStr(HgArena* arena, i64 num)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgGetScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    if (num == 0)
        return hgCopyString(arena, "0");

    bool isNegative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgString reverse = hgCreateString(scratch, 16);
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        hgAppendString(scratch, &reverse, '0' + (char)digit);
    }

    HgString ret = hgCreateString(arena, reverse.length + (isNegative ? 1 : 0));
    if (isNegative)
        hgAppendString(arena, &ret, '-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        hgAppendString(arena, &ret, reverse[i]);
    }
    return ret;
}

HgString hgFloatToStr(HgArena* arena, f64 num, u32 decimalCount)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgGetScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    if (num == 0.0)
        return hgCopyString(arena, "0.0");

    HgString intStr = hgIntToStr(scratch, (i64)fabs(num));

    HgString decStr = hgCreateString(scratch, decimalCount + 1);
    hgAppendString(scratch, &decStr, '.');

    f64 decPart = fabs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        hgAppendString(scratch, &decStr, '0' + (char)((u64)decPart % 10));
    }

    HgString ret{};
    if (num < 0.0)
        hgAppendString(arena, &ret, '-');
    hgAppendString(arena, &ret, intStr);
    hgAppendString(arena, &ret, decStr);
    return ret;
}

HgString hgFormatString(HgArena* arena, HgStringView fmt, ...)
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
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", found unexpected token \"]\"\n");
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
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
    hgAppendString(arena, &error->msg, ", found unexpected token \"");
    hgAppendString(arena, &error->msg, {&state->text[begin], &state->text[state->head]});
    hgAppendString(arena, &error->msg, "\"\n");

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
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", expected struct to terminate\n");
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
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", struct ends with \"]\" instead of \"}\"\n");
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
                hgAppendString(arena, &error->msg, "on line ");
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
                hgAppendString(arena, &error->msg, ", struct has a literal instead of a field\n");
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
                hgAppendString(arena, &error->msg, "on line ");
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
                hgAppendString(arena, &error->msg, ", struct has a field named \"");
                hgAppendString(arena, &error->msg, value.file->field.name);
                hgAppendString(arena, &error->msg, "\" which has no value\n");
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
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", expected struct to terminate\n");
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
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
            hgAppendString(arena, &error->msg, ", array ends with \"}\" instead of \"]\"\n");
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
                    hgAppendString(arena, &error->msg, "on line ");
                    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
                    hgAppendString(arena, &error->msg, ", array has a field as an element\n");
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
                hgAppendString(arena, &error->msg, "on line ");
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
                hgAppendString(arena, &error->msg, ", array has element which is not the same type as the first valid element\n");
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
        HgString str = hgCreateString(arena, end - begin);
        for (u64 i = begin; i < end; ++i)
        {
            char c = state->text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            hgAppendString(arena, &str, c);
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
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
    hgAppendString(arena, &error->msg, ", expected string to terminate\n");
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
            node->floating = hgStrToFloat(num);
            return {node, nullptr};
        }
    } else {
        if (hgIsIntenger(num))
        {
            HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
            node->type = HgJsonType::HgJsonType_integer;
            node->integer = hgStrToInt(num);
            return {node, nullptr};
        }
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);

    error->msg = HgString{};
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
    hgAppendString(arena, &error->msg, ", expected numeral value, found \"");
    hgAppendString(arena, &error->msg, num);
    hgAppendString(arena, &error->msg, "\"\n");

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
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)state->line));
    hgAppendString(arena, &error->msg, ", expected boolean value, found \"");
    hgAppendString(arena, &error->msg, {&state->text[begin], &state->text[state->head]});
    hgAppendString(arena, &error->msg, "\"\n");

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

HgClock::HgClock()
{
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
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

HgPerf hgCreatePerf(HgArena* arena, u32 count)
{
    hgAssert(arena != nullptr);

    HgPerf perf;
    perf.times = hgAlloc<f64>(arena, count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void hgBeginPerf(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgClockTick(&perf->clock);
}

f64 hgEndPerf(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgAssert(perf->current < perf->count);

    f64 time = hgClockTick(&perf->clock);
    perf->times[perf->current++] = time;

    return time;
}

HgPerfStats hgAnalyzePerf(const HgPerf* perf)
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

void hgLogPerf(HgStringView title, const HgPerfStats* stats, HgPerfScale scale)
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

void hgAttachFence(HgFence* fence, u32 count)
{
    hgAssert(fence != nullptr);
    fence->counter.fetch_add(count);
}

void hgSignalFence(HgFence* fence, u32 count)
{
    hgAssert(fence != nullptr);
    fence->counter.fetch_sub(count);
}

bool hgIsFenceComplete(const HgFence* fence)
{
    hgAssert(fence != nullptr);
    return fence->counter.load() == 0;
}

void hgWaitForFenceIndefinite(const HgFence* fence)
{
    hgAssert(fence != nullptr);
    while (!hgIsFenceComplete(fence))
    {
        _mm_pause();
    }
}

bool hgWaitForFenceTimeout(const HgFence* fence, f64 timeoutSeconds)
{
    hgAssert(fence != nullptr);
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeoutSeconds);
    while (!hgIsFenceComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        _mm_pause();
    }
    return hgIsFenceComplete(fence);
}

struct ThreadWork {
    HgFence* fences;
    u32 fenceCount;
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

    for (u32 i = 0; i < work.fenceCount; ++i)
    {
        hgSignalFence(&work.fences[i], 1);
    }
    return true;
}

void hgInitThreadPool(HgArena* arena, u32 queueSize, u32 threadCount)
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

void hgDeinitThreadPool()
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

bool hgHelpThreadPool(const HgFence* fence, f64 timeout)
{
    hgAssert(fence != nullptr);

    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!hgIsFenceComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        if (!poolExecute())
            _mm_pause();
    }
    return hgIsFenceComplete(fence);
}

void hgCallPar(HgFence* fences, u32 fenceCount, void* data, void (*fn)(void*))
{
    if (fenceCount > 0)
        hgAssert(fences != nullptr);

    hgAssert(fn != nullptr);
    for (u32 i = 0; i < fenceCount; ++i)
    {
        hgAttachFence(&fences[i], 1);
    }

    u32 idx = threadPool.workingHead.fetch_add(1) & (threadPool.workCapacity - 1);

    threadPool.work[idx].fences = fences;
    threadPool.work[idx].fenceCount = fenceCount;
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

void hgForPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    hgAssert(begin <= end);
    hgAssert(fn != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u64 chunkSize = (u64)std::ceil((f32)(end - begin) / (8.0f * (f32)hgHardwareThreadCount()));

    HgFence fence;
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

        hgCallPar(&fence, 1, capture, [](void* pcapture)
        {
            Capture* capture = (Capture*)pcapture;
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    hgHelpThreadPool(&fence, INFINITY);
}

struct IORequest {
    HgFence* fences;
    u32 fenceCount;
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

    for (u32 i = 0; i < request.fenceCount; ++i)
    {
        hgSignalFence(&request.fences[i], 1);
    }
    return true;
}

void hgInitIOThread(HgArena* arena, u32 queueSize)
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
        hgInitScratchMemory(UINT32_MAX);
        hgDefer(hgDeinitScratchMemory());

        for (;;)
        {
            if (ioThread.shouldClose.load())
                return;

            if (!ioPop())
                poolExecute();
        }
    });
}

void hgDeinitIOThread()
{
    ioThread.shouldClose.store(true);
    ioThread.thread.join();
}

void hgRequestIO(
    HgFence* fences,
    u32 fenceCount,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path))
{
    if (fenceCount > 0)
        hgAssert(fences != nullptr);
    hgAssert(fn != nullptr);
    for (u32 i = 0; i < fenceCount; ++i)
    {
        hgAttachFence(&fences[i], 1);
    }

    u32 idx = ioThread.workingHead.fetch_add(1) & (ioThread.requestCapacity - 1);

    ioThread.requests[idx].fences = fences;
    ioThread.requests[idx].fenceCount = fenceCount;
    ioThread.requests[idx].resource = resource;
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

HgResourceManager HgResourceManager::create(HgArena* arena, u32 width, u32 align, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(width > 0);
    hgAssert(align > 0);
    hgAssert(capacity > 0);

    HgResourceManager rm;
    rm.refCounts = hgAlloc<u32>(arena, capacity);
    rm.ids = hgAlloc<HgResource>(arena, capacity);
    rm.resources = hgAlloc(arena, capacity * width, align);
    rm.width = width;
    rm.capacity = capacity;
    rm.reset();
    return rm;
}

void HgResourceManager::reset()
{
    for (u32 i = 0; i < capacity; ++i)
    {
        refCounts[i] = (u32)-1;
    }
    count = 0;
}

u32 HgResourceManager::add(HgResource id)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    void* resource = hgAlloc(scratch, width, 16);
    void* resourceTmp = hgAlloc(scratch, width, 16);

    u32 idx = id % capacity;
    for (u32 dist = 0; refCounts[idx] != (u32)-1; ++dist)
    {
        u32 otherDist = ids[idx] % capacity - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            HgResource tmpID = ids[idx];
            ids[idx] = id;
            id = tmpID;
            memcpy(resourceTmp, (u8*)resources + idx * width, width);
            memcpy((u8*)resources + idx * width, resource, width);
            memcpy(resourceTmp, resource, width);
            dist = otherDist;
        }

        if (ids[idx] == id)
            return idx;
        idx = (idx + 1) % capacity;
    }

    refCounts[idx] = 0;
    ids[idx] = id;
    ++count;

    return idx;
}

void HgResourceManager::remove(HgResource id)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    if (refCounts[idx] == (u32)-1)
        return;

    u32 next = (idx + 1) % capacity;
    while (refCounts[next] % capacity != (u32)-1)
    {
        if (ids[next] % capacity != next)
        {
            refCounts[idx] = refCounts[next];
            ids[idx] = ids[next];
            memcpy(
                (u8*)resources + idx * width,
                (u8*)resources + next * width,
                width);
            idx = next;
        }
        next = (next + 1) % capacity;
    }
    refCounts[idx] = (u32)-1;
    --count;
}

bool HgResourceManager::load(HgResource id)
{
    u32 idx = add(id);
    return refCounts[idx]++ == 0;
}

bool HgResourceManager::unload(HgResource id, void* resource)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    if (refCounts[idx] == (u32)-1 || refCounts[idx] == 0)
        return false;
    if (--refCounts[idx] != 0)
        return false;
    memcpy(resource, (u8*)resources + idx * width, width);
    return true;
}

void* HgResourceManager::get(HgResource id)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    return refCounts[idx] == (u32)-1 || refCounts[idx] == 0
        ? nullptr
        : (u8*)resources + idx * width;
}

HgBinary hgLoadBinary(HgArena* arena, HgStringView path)
{
    HgArena* scratch = hgGetScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    HgBinary bin;

    char* cpath = hgCString(scratch, path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        hgWarn("Could not find file to read binary: %s\n", cpath);
        return {};
    }
    hgDefer(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    bin.resize(arena, (u32)ftell(fileHandle));
    rewind(fileHandle);

    if (fread(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    return bin;
}

void hgStoreBinary(HgBinary bin, HgStringView path)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    char* cpath = hgCString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        hgWarn("Failed to create file to write binary: %s\n", cpath);
        return;
    }
    hgDefer(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        hgWarn("Failed to write binary data to file: %s\n", cpath);
        return;
    }
}

static HgResourceManager binResources{};

void hgInitResources(HgArena* arena, u32 maxResources)
{
    binResources = binResources.create(
        arena,
        sizeof(HgBinary*),
        alignof(HgBinary*),
        maxResources);
}

void hgDeinitResource()
{
    binResources.forEach([](HgResource, void* pres)
    {
        HgBinary* bin = (HgBinary*)pres;
        free(bin->data);
        free(bin);
    });
}

void hgLoadEmptyResource(HgResource id)
{
    if (binResources.load(id))
    {
        HgBinary** bin = (HgBinary**)binResources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};
    }
}

void hgLoadResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    if (binResources.load(id))
    {
        HgBinary** bin = (HgBinary**)binResources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};

        hgRequestIO(fences, fenceCount, *bin, path, [](void* pres, HgStringView fpath)
        {
            HgBinary& bin = *(HgBinary*)pres;

            HgArena* scratch = hgGetScratch();
            HgArenaScope scratchScope{scratch};
            char* cpath = hgCString(scratch, fpath);

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
            bin.size = (u32)ftell(fileHandle);
            bin.data = malloc(bin.size);
            rewind(fileHandle);

            if (fread(bin.data, 1, bin.size, fileHandle) != bin.size)
            {
                hgWarn("Failed to read binary from file: %s\n", cpath);
                free(bin.data);
                bin.size = 0;
                bin.data = nullptr;
                return;
            }
        });
    }
}

void hgUnloadResource(HgFence* fences, u32 fenceCount, HgResource id)
{
    HgBinary* bin;
    if (binResources.unload(id, &bin))
    {
        hgRequestIO(fences, fenceCount, bin, {}, [](void* pres, HgStringView)
        {
            HgBinary& bin = *(HgBinary*)pres;
            free(bin.data);
            bin = {};
        });
    }
}

void hgStoreResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    HgBinary** res = (HgBinary**)binResources.get(id);
    if (res == nullptr || *res == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not store resource to \"%.*s\" because the resource does not exist\n",
            (int)path.length, path.chars);
        return;
    }

    hgRequestIO(fences, fenceCount, *res, path, [](void* pres, HgStringView fpath)
    {
        hgStoreBinary((*(HgBinary*)pres), fpath);
    });
}

HgBinary* hgGetResource(HgResource id)
{
    HgBinary** res = (HgBinary**)binResources.get(id);
    return res == nullptr ? nullptr : *res;
}

bool HgImageData::getInfo(HgFormat* format, u32* width, u32* height, u32* depth)
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, imageIdentifier, sizeof(imageIdentifier)) == 0)
    {
        bin.read(offsetof(Info, format), format, sizeof(*format));
        bin.read(offsetof(Info, width), width, sizeof(*width));
        bin.read(offsetof(Info, height), height, sizeof(*height));
        bin.read(offsetof(Info, depth), depth, sizeof(*depth));
        return true;
    }
    return false;
}

void* HgImageData::getPixels()
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, imageIdentifier, sizeof(imageIdentifier)) == 0)
    {
        return (u8*)bin.data + bin.read<u64>(offsetof(Info, pixelsBegin));
    }
    return bin.data;
}

void hgImportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    hgLoadResource(fences, fenceCount, id, path);

    hgRequestIO(fences, fenceCount, hgGetResource(id), path, [](void* pbin, HgStringView fpath)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgBinary& bin = *(HgBinary*)pbin;

        int width, height, channels;
        u8* pixels = stbi_load_from_memory((u8*)bin.data, (i32)bin.size, &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            hgWarn("Failed to decode image file: %.*s\n", (int)fpath.length, fpath.chars);
            return;
        }
        free(bin.data);
        hgDefer(free(pixels));

        bin.size = sizeof(HgImageData::Info) + (u32)width * (u32)height * sizeof(u32);
        bin.data = malloc(bin.size);

        HgImageData::Info info;
        memcpy(info.identifier, HgImageData::imageIdentifier, sizeof(HgImageData::imageIdentifier));
        info.format = HgFormat_r8g8b8a8_srgb;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;
        info.pixelsBegin = sizeof(info);

        bin.overwrite(0, info);
        bin.overwrite(sizeof(info), pixels, (u32)width * (u32)height * sizeof(u32));
    });
}

void hgExportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    HgBinary* resource = hgGetResource(id);
    if (resource == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not export png resource to \"%.*s\" because the resource does not exist\n",
            (int)path.length, path.chars);
        return;
    }

    hgRequestIO(fences, fenceCount, resource, path, [](void* ptex, HgStringView fpath)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        char* cpath = hgCString(scratch, fpath);

        HgImageData tex = *(HgBinary*)ptex;
        void* pixels = tex.getPixels();
        if (pixels == nullptr)
        {
            hgWarn("Cannot export empty image %s\n", cpath);
            return;
        }

        HgFormat format;
        u32 width, height, depth;
        if (!tex.getInfo(&format, &width, &height, &depth))
        {
            hgWarn("Could not get info from image %s to export\n", cpath);
            return;
        }
        if (depth > 1)
            hgWarn("Cannot export 3d image %s, exporting only the first layer\n", cpath);
        stbi_write_png(cpath, (int)width, (int)height, 4, pixels, (int)(width * sizeof(u32)));
    });
}

bool HgModelData::getInfo(u32* vertexCount, u32* vertexWidth, u32* indexCount, HgGpuTopology* topology)
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        file.read(offsetof(Info, vertexCount), vertexCount, sizeof(vertexCount));
        file.read(offsetof(Info, vertexWidth), vertexWidth, sizeof(vertexWidth));
        file.read(offsetof(Info, indexCount), indexCount, sizeof(indexCount));
        file.read(offsetof(Info, topology), topology, sizeof(topology));
        return true;
    }
    return false;
}

void* HgModelData::getVertexData()
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        return (u8*)file.data + file.read<u64>(offsetof(Info, verticesBegin));
    }
    return file.data;
}

void* HgModelData::getIndexData()
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        return (u8*)file.data + file.read<u64>(offsetof(Info, indicesBegin));
    }
    return nullptr;
}

void hgImportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgImportGltf not implemented yet : TODO\n");
}

void hgExportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgExportGltf not implemented yet : TODO\n");
}

void hgInitGpuResources(HgArena* arena, u32 maxTextures, u32 maxModels)
{
    HgInitTextures(arena, maxTextures);
    hgInitModels(arena, maxModels);
}

void hgDeinitGpuResources()
{
    hgDeinitModels();
    hgDeinitTextures();
}

static HgResourceManager gpuTextures{};

void HgInitTextures(HgArena* arena, u32 maxTextures)
{
    gpuTextures = gpuTextures.create(
        arena,
        sizeof(HgTextureResource),
        alignof(HgTextureResource),
        maxTextures);
}

void hgDeinitTextures()
{
    gpuTextures.forEach([](HgResource, void* ptex)
    {
        HgTextureResource& tex = *(HgTextureResource*)ptex;
        hgDestroyGpuView(tex.view);
        hgDestroyGpuImage(tex.image);
    });
}

void hgLoadEmptyTexture(HgResource id)
{
    gpuTextures.load(id);
}

void hgLoadTexture(HgResource id, HgGpuSampler* sampler)
{
    if (gpuTextures.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgImageData data = *hgGetResource(id);

        HgFormat format;
        u32 width, height, depth;
        if (!data.getInfo(&format, &width, &height, &depth))
        {
            hgWarn("Could not get info to load texture\n");
            return;
        }
        hgAssert(format != 0);
        hgAssert(width != 0);
        hgAssert(height != 0);
        hgAssert(depth != 0);

        HgTextureResource& tex = *(HgTextureResource*)gpuTextures.get(id);;

        HgCreateGpuImageEx imageInfo{};
        imageInfo.format = format;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.depth = depth;
        imageInfo.usage = HgGpuImageUsage_transferDst | HgGpuImageUsage_sampled;

        tex.image = hgCreateGpuImageEx(&imageInfo);
        tex.view = hgCreateGpuView(tex.image, HgGpuAspect_color, 0, 1, 0, 1);

        hgWriteGpuImage(tex.view, data.getPixels());

        hgAssert(sampler != nullptr);
        tex.sampler = sampler;

        tex.descriptor = hgCreateGpuDescriptor(HgGpuDescriptorType_combinedImageSampler);

        HgGpuImageDescriptorInfo descInfo = {tex.sampler, tex.view, HgGpuLayout_shaderReadOnly};
        hgUpdateGpuDescriptor(tex.descriptor, nullptr, &descInfo);
    }
}

void hgUnloadTexture(HgResource id)
{
    HgTextureResource tex;
    if (gpuTextures.unload(id, &tex))
    {
        hgDestroyGpuDescriptor(tex.descriptor);
        hgDestroyGpuView(tex.view);
        hgDestroyGpuImage(tex.image);
        tex = {};
    }
}

HgTextureResource* hgGetTexture(HgResource id)
{
    return (HgTextureResource*)gpuTextures.get(id);
}

static HgResourceManager gpuModels{};

void hgInitModels(HgArena* arena, u32 maxModels)
{
    gpuModels = gpuModels.create(
        arena,
        sizeof(HgModelResource),
        alignof(HgModelResource),
        maxModels);
}

void hgDeinitModels()
{
    gpuModels.forEach([](HgResource, void* pmodel)
    {
        HgModelResource& model = *(HgModelResource*)pmodel;
        hgDestroyGpuBuffer(model.vertexBuffer);
        hgDestroyGpuBuffer(model.indexBuffer);
    });
}

void hgLoadModel(HgResource id)
{
    if (gpuModels.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgModelData data = *hgGetResource(id);

        HgGpuTopology topology;
        HgModelResource& model = *(HgModelResource*)gpuModels.get(id);
        if (!data.getInfo(
            &model.vertexCount,
            &model.vertexWidth,
            &model.indexCount,
            &topology))
        {
            hgWarn("Could not get info to load model\n");
            return;
        }

        model.vertexBuffer = hgCreateGpuBuffer(
            model.vertexCount * model.vertexWidth,
            HgGpuBufferUsage_vertexBuffer | HgGpuBufferUsage_transferDst);

        model.indexBuffer = hgCreateGpuBuffer(
            model.indexCount * sizeof(u32),
            HgGpuBufferUsage_indexBuffer | HgGpuBufferUsage_transferDst);

        hgWriteGpuBuffer(
            model.vertexBuffer,
            0,
            data.getVertexData(),
            model.vertexCount * model.vertexWidth);

        hgWriteGpuBuffer(
            model.indexBuffer,
            0,
            data.getIndexData(),
            model.indexCount * sizeof(u32));
    }
}

void hgUnloadModel(HgResource id)
{
    HgModelResource model;
    if (gpuModels.unload(id, &model))
    {
        hgDestroyGpuBuffer(model.vertexBuffer);
        hgDestroyGpuBuffer(model.indexBuffer);
    }
}

HgModelResource* hgGetModel(HgResource id)
{
    return (HgModelResource*)gpuModels.get(id);
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

HgECS HgECS::create(HgArena* arena, u32 maxEntities, u32 maxComponentTypes)
{
    HgECS ecs{};

    ecs.pool = hgAlloc<HgEntity>(arena, maxEntities);
    ecs.poolSize = maxEntities;
    ecs.systems = ecs.systems.create(arena, maxComponentTypes + 1);
    ecs.reset();

    return ecs;
}

void HgECS::createComponent(HgArena* arena, u32 maxCount, u32 width, u32 align, u32 componentID)
{
    hgAssert(systems.get(componentID) == nullptr);
    System* system = systems.add(componentID);

    system->indices = hgAlloc<u32>(arena, poolSize);
    system->entities = hgAlloc<HgEntity>(arena, maxCount);
    system->components = hgAlloc(arena, maxCount * width, align);
    system->count = 0;
    system->capacity = maxCount;

    memset(system->indices, -1, poolSize * sizeof(*system->indices));
}

void HgECS::reset()
{
    systems.forEach([&] (const u32*, System* system)
    {
        memset(system->indices, -1, poolSize * sizeof(*system->indices));
        system->count = 0;
    });

    for (u32 i = 0; i < poolSize; ++i)
    {
        pool[i] = {i + 1};
    }
    next = {0};
}

HgEntity HgECS::spawn()
{
    hgAssert(next.idx() < poolSize);

    HgEntity entity = next;
    next = pool[entity.idx()];
    pool[entity.idx()] = entity;
    return entity;
}

void HgECS::despawn(HgEntity e)
{
    hgAssert(alive(e));

    systems.forEach([&] (const u32* componentID, System*)
    {
        if (has(e, *componentID))
            remove(e, *componentID);
    });
    pool[e.idx()] = next;
    next = e;
    next.incrementGeneration();
}

bool HgECS::alive(HgEntity e)
{
    return e.idx() < poolSize && pool[e.idx()] == e;
}

void* HgECS::add(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));
    hgAssert(!has(e, componentID));

    System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    system->indices[e.idx()] = system->count;
    system->entities[system->count] = e;
    void* c = (u8*)system->components + componentWidth(componentID) * system->count;
    ++system->count;
    return c;
}

void HgECS::remove(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));
    hgAssert(has(e, componentID));

    System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    HgEntity last = system->entities[system->count - 1];
    system->entities[system->count - 1] = HgEntity{};
    if (e != last)
    {
        u32 idx = system->indices[e.idx()];
        system->entities[idx] = last;
        system->indices[last.idx()] = idx;
        memcpy(
            (u8*)system->components + componentWidth(componentID) * idx,
            (u8*)system->components + componentWidth(componentID) * (system->count - 1),
            componentWidth(componentID));
    }
    system->indices[e.idx()] = (u32)-1;
    --system->count;
}

bool HgECS::has(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));

    System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    return system->indices[e.idx()] < system->count;
}

void* HgECS::get(HgEntity e, u32 componentID)
{
    hgAssert(alive(e));

    System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    return (u8*)system->components + componentWidth(componentID) * system->indices[e.idx()];
}

HgEntity HgECS::get(const void* component, u32 componentID)
{
    hgAssert(component != nullptr);

    System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    return system->entities[(u32)((uptr)component - (uptr)system->components) / componentWidth(componentID)];
}

u32 HgECS::findSmallest(u32* ids, u32 idCount)
{
    u32 smallestCount = (u32)-1;
    u32 smallest = ids[0];

    for (u32 i = 1; i < idCount; ++i)
    {
        System* system = systems.get(ids[i]);
        hgAssert(system != nullptr);

        if (system->count < smallestCount)
        {
            smallestCount = system->count;
            smallest = ids[i];
        }
    }
    return smallest;
}

static void swapIdxLocation(HgECS* ecs, u32 lhs, u32 rhs, u32 componentID)
{
    HgECS::System* system = ecs->systems.get(componentID);
    hgAssert(system != nullptr);

    hgAssert(lhs < system->count);
    hgAssert(rhs < system->count);

    HgEntity lhsEntity = system->entities[lhs];
    HgEntity rhsEntity = system->entities[rhs];

    hgAssert(ecs->alive(lhsEntity));
    hgAssert(ecs->alive(rhsEntity));
    hgAssert(ecs->has(lhsEntity, componentID));
    hgAssert(ecs->has(rhsEntity, componentID));

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    system->entities[lhs] = rhsEntity;
    system->entities[rhs] = lhsEntity;
    system->indices[lhsEntity.id] = rhs;
    system->indices[rhsEntity.id] = lhs;

    u32 width = componentWidth(componentID);
    void* temp = hgAlloc(scratch, componentWidth(componentID), 1);
    memcpy(temp, (u8*)system->components + width * lhs, width);
    memcpy((u8*)system->components + width * lhs, (u8*)system->components + width * rhs, width);
    memcpy((u8*)system->components + width * rhs, temp, width);
}

namespace {
    struct QuicksortData {
        HgECS* ecs;
        HgECS::System* system;
        u32 comp;
        void* data;
        bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs);

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

void HgECS::sort(
    u32 componentID,
    void* data,
    bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs))
{
    hgAssert(compare != nullptr);

    HgECS::System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    QuicksortData q{this, system, componentID, data, compare};
    q.quicksort(0, system->count);
}

void hgAddChildEntity(HgECS* ecs, HgEntity parent, HgEntity child)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(parent);
    HgHierarchy& oldFirst = ecs->get<HgHierarchy>(node.firstChild);
    HgHierarchy& newFirst = ecs->get<HgHierarchy>(child);

    hgAssert(newFirst.parent == HgEntity{});
    hgAssert(newFirst.prevSibling == HgEntity{});
    hgAssert(newFirst.nextSibling == HgEntity{});

    newFirst.parent = parent;
    newFirst.nextSibling = node.firstChild;

    oldFirst.prevSibling = child;
    node.firstChild = child;
}

void hgDetachEntity(HgECS* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(e);
    if (node.parent != HgEntity{})
    {
        if (node.prevSibling == HgEntity{})
            ecs->get<HgHierarchy>(node.parent).firstChild = node.nextSibling;
        else
            ecs->get<HgHierarchy>(node.prevSibling).nextSibling = node.nextSibling;
        ecs->get<HgHierarchy>(node.nextSibling).prevSibling = node.prevSibling;

        HgEntity child = node.firstChild;
        while (child != HgEntity{})
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
        hgAssert(node.prevSibling == HgEntity{});
        hgAssert(node.nextSibling == HgEntity{});
        HgEntity child = node.firstChild;
        while (child != HgEntity{})
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

void hgDestroyEntity(HgECS* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);

    HgHierarchy& node = ecs->get<HgHierarchy>(e);
    HgEntity child = node.firstChild;
    while (child != HgEntity{})
    {
        HgHierarchy& tf = ecs->get<HgHierarchy>(child);
        HgEntity next = tf.nextSibling;
        tf.parent = HgEntity{};
        tf.prevSibling = HgEntity{};
        tf.nextSibling = HgEntity{};
        hgDestroyEntity(ecs, child);
        child = next;
    }
    if (node.parent != HgEntity{})
    {
        if (node.prevSibling != HgEntity{})
            ecs->get<HgHierarchy>(node.prevSibling).nextSibling = node.nextSibling;
        else
            ecs->get<HgHierarchy>(node.parent).firstChild = node.nextSibling;
        if (node.nextSibling != HgEntity{})
            ecs->get<HgHierarchy>(node.nextSibling).prevSibling = HgEntity{};
    }
    ecs->despawn(e);
}

void hgSetEntity(HgECS* ecs, HgEntity e, const HgVec3& pos, const HgVec3& scale, const HgQuat& rot)
{
    hgAssert(ecs != nullptr);

    HgTransform& tf = ecs->get<HgTransform>(e);
    if (ecs->has<HgHierarchy>(e))
    {
        HgHierarchy& node = ecs->get<HgHierarchy>(e);
        HgEntity child = node.firstChild;
        while (child != HgEntity{})
        {
            HgHierarchy& cNode = ecs->get<HgHierarchy>(child);
            HgTransform& cTf = ecs->get<HgTransform>(child);
            HgTransform rel;
            rel.position = cTf.position - tf.position;
            rel.scale = cTf.scale / tf.scale;
            rel.rotation = hgConj(tf.rotation) * cTf.rotation;
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

void hgMoveEntity(HgECS* ecs, HgEntity e, const HgVec3& dpos, const HgVec3& dscale, const HgQuat& drot)
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
    HgTextureResource defaultTex;
};

static Pipeline2DState pipeline2D{};

void hgInitPipeline2D(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);

    HgFence fence;
    const char* spriteVertSpvName = "build/sprite.vert.spv";
    const char* spriteFragSpvName = "build/sprite.frag.spv";
    HgResource spriteVertSpvID = hgResourceID(spriteVertSpvName);
    HgResource spriteFragSpvID = hgResourceID(spriteFragSpvName);
    hgLoadResource(&fence, 1, spriteVertSpvID, spriteVertSpvName);
    hgLoadResource(&fence, 1, spriteFragSpvID, spriteFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, spriteVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, spriteFragSpvID));

    hgWaitForFenceIndefinite(&fence);
    HgBinary* spriteVertSpv = hgGetResource(spriteVertSpvID);
    HgBinary* spriteFragSpv = hgGetResource(spriteFragSpvID);

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
    pipelineConfig.enableDepthRead = depthFormat != HgFormat_undefined;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    pipeline2D.pipeline = hgCreateGpuGraphicsPipeline(&pipelineConfig);

    pipeline2D.vpBuffer = hgCreateGpuBuffer(
        sizeof(Pipeline2DVPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    Pipeline2DVPUniform vpData{};
    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    hgWriteGpuBuffer(pipeline2D.vpBuffer, 0, &vpData, sizeof(vpData));

    pipeline2D.vpDesc = hgCreateGpuDescriptor(HgGpuDescriptorType_uniformBuffer);

    HgGpuBufferDescriptorInfo bufferInfo{pipeline2D.vpBuffer, 0, sizeof(Pipeline2DVPUniform)};
    hgUpdateGpuDescriptor(pipeline2D.vpDesc, &bufferInfo, nullptr);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    pipeline2D.defaultTex.image = hgCreateGpuImage(2, 2, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    pipeline2D.defaultTex.view = hgCreateGpuView(pipeline2D.defaultTex.image, HgGpuAspect_color, 0, 1, 0, 1);

    hgWriteGpuImage(pipeline2D.defaultTex.view, defaultColors);

    pipeline2D.defaultTex.sampler = hgCreateGpuSampler(HgGpuFilter_nearest);

    pipeline2D.defaultTex.descriptor = hgCreateGpuDescriptor(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo imageInfo{};
    imageInfo.sampler = pipeline2D.defaultTex.sampler;
    imageInfo.imageView = pipeline2D.defaultTex.view;
    imageInfo.imageLayout = HgGpuLayout_shaderReadOnly;

    hgUpdateGpuDescriptor(pipeline2D.defaultTex.descriptor, nullptr, &imageInfo);
}

void hgDeinitPipeline2D()
{
    hgDestroyGpuPipeline(pipeline2D.pipeline);

    hgDestroyGpuDescriptor(pipeline2D.defaultTex.descriptor);
    hgDestroyGpuSampler(pipeline2D.defaultTex.sampler);
    hgDestroyGpuView(pipeline2D.defaultTex.view);
    hgDestroyGpuImage(pipeline2D.defaultTex.image);

    hgDestroyGpuDescriptor(pipeline2D.vpDesc);
    hgDestroyGpuBuffer(pipeline2D.vpBuffer);
}

void hgUpdateProjection2D(const HgMat4* projection)
{
    hgAssert(projection != nullptr);
    hgWriteGpuBuffer(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, proj), projection, sizeof(*projection));
}

void hgUpdateView2D(const HgMat4* view)
{
    hgAssert(view != nullptr);
    hgWriteGpuBuffer(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, view), view, sizeof(*view));
}

void hgDraw2D(HgECS* ecs, HgGpuCommands* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    ecs->sort<HgSprite2D>(nullptr, [](void*, HgECS* ecs, HgEntity lhs, HgEntity rhs) -> bool {
        hgAssert(ecs->has<HgTransform>(lhs));
        hgAssert(ecs->has<HgTransform>(rhs));
        return ecs->get<HgTransform>(lhs).position.z > ecs->get<HgTransform>(rhs).position.z;
    });

    hgBindGpuPipeline(cmd, pipeline2D.pipeline);

    ecs->forEach<HgSprite2D, HgTransform>([&](HgEntity, HgSprite2D* sprite, HgTransform* transform) 
    {
        HgTextureResource* texture = hgGetTexture(sprite->texture);
        if (texture == nullptr)
            texture = &pipeline2D.defaultTex;

        Pipeline2DPush push{};
        push.model = hgModelMatrix3D(transform->position, transform->scale, transform->rotation);
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

    HgModelResource defaultModel;
    HgGpuSampler* defaultMapSampler;
    HgTextureResource defaultColorMap;
    HgTextureResource defaultNormalMap;
};

static Pipeline3DState pipeline3D{};

void hgInitPipeline3D(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgFence fence;
    const char* modelVertSpvName = "build/model.vert.spv";
    const char* modelFragSpvName = "build/model.frag.spv";
    HgResource modelVertSpvID = hgResourceID(modelVertSpvName);
    HgResource modelFragSpvID = hgResourceID(modelFragSpvName);
    hgLoadResource(&fence, 1, modelVertSpvID, modelVertSpvName);
    hgLoadResource(&fence, 1, modelFragSpvID, modelFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, modelVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, modelFragSpvID));

    hgWaitForFenceIndefinite(&fence);
    HgBinary* modelVertSpv = hgGetResource(modelVertSpvID);
    HgBinary* modelFragSpv = hgGetResource(modelFragSpvID);

    HgGpuVertexBinding vertexBindings[]{
        {0, sizeof(HgModelVertex), false},
    };
    HgGpuVertexAttribute vertexAttributes[]{
        {0, 0, HgFormat_r32g32b32_sfloat, offsetof(HgModelVertex, pos)},
        {1, 0, HgFormat_r32g32b32_sfloat, offsetof(HgModelVertex, norm)},
        {2, 0, HgFormat_r32g32b32a32_sfloat, offsetof(HgModelVertex, tan)},
        {3, 0, HgFormat_r32g32_sfloat, offsetof(HgModelVertex, uv)},
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

    pipeline3D.pipeline = hgCreateGpuGraphicsPipeline(&pipelineConfig);

    pipeline3D.vpData.proj = HgMat4{1.0f};
    pipeline3D.vpData.view = HgMat4{1.0f};

    pipeline3D.vpBuffer = hgCreateGpuBuffer(
        sizeof(Pipeline3DVPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    hgWriteGpuBuffer(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    pipeline3D.vpDesc = hgCreateGpuDescriptor(HgGpuDescriptorType_uniformBuffer);

    HgGpuBufferDescriptorInfo bufferInfo{pipeline3D.vpBuffer, 0, sizeof(Pipeline3DVPUniform)};
    hgUpdateGpuDescriptor(pipeline3D.vpDesc, &bufferInfo, nullptr);

    pipeline3D.dirLightCapacity = 4;
    pipeline3D.dirLightBuffer = hgCreateGpuBuffer(
        sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    pipeline3D.pointLightCapacity = 4;
    pipeline3D.pointLightBuffer = hgCreateGpuBuffer(
        sizeof(Pipeline3DPointLightData) * pipeline3D.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    pipeline3D.dirLightDesc = hgCreateGpuDescriptor(HgGpuDescriptorType_storageBuffer);
    pipeline3D.pointLightDesc = hgCreateGpuDescriptor(HgGpuDescriptorType_storageBuffer);

    HgGpuBufferDescriptorInfo dirLightBufferInfo{pipeline3D.dirLightBuffer, 0, UINT64_MAX};
    hgUpdateGpuDescriptor(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);

    HgGpuBufferDescriptorInfo pointLightBufferInfo{pipeline3D.pointLightBuffer, 0, UINT64_MAX};
    hgUpdateGpuDescriptor(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);

    static const HgModelVertex cubeVertices[]{
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

    pipeline3D.defaultModel.vertexBuffer = hgCreateGpuBuffer(
        sizeof(cubeVertices),
        HgGpuBufferUsage_vertexBuffer | HgGpuBufferUsage_transferDst);

    pipeline3D.defaultModel.indexBuffer = hgCreateGpuBuffer(
        sizeof(cubeIndices),
        HgGpuBufferUsage_indexBuffer | HgGpuBufferUsage_transferDst);

    hgWriteGpuBuffer(pipeline3D.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgWriteGpuBuffer(pipeline3D.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    pipeline3D.defaultModel.vertexCount = sizeof(cubeVertices) / sizeof(*cubeVertices);
    pipeline3D.defaultModel.vertexWidth = sizeof(HgModelVertex);
    pipeline3D.defaultModel.indexCount = sizeof(cubeIndices) / sizeof(*cubeIndices);

    pipeline3D.defaultMapSampler = hgCreateGpuSampler(HgGpuFilter_nearest);

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

    pipeline3D.defaultColorMap.image = hgCreateGpuImage(3, 3, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);
    pipeline3D.defaultNormalMap.image = hgCreateGpuImage(2, 2, HgFormat_r32g32b32a32_sfloat,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    pipeline3D.defaultColorMap.view = hgCreateGpuView(
        pipeline3D.defaultColorMap.image, HgGpuAspect_color, 0, 1, 0, 1);
    pipeline3D.defaultNormalMap.view = hgCreateGpuView(
        pipeline3D.defaultNormalMap.image, HgGpuAspect_color, 0, 1, 0, 1);

    hgWriteGpuImage(pipeline3D.defaultColorMap.view, defaultColors);
    hgWriteGpuImage(pipeline3D.defaultNormalMap.view, defaultNormals);

    pipeline3D.defaultColorMap.descriptor = hgCreateGpuDescriptor(HgGpuDescriptorType_combinedImageSampler);
    pipeline3D.defaultNormalMap.descriptor = hgCreateGpuDescriptor(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo colorInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultColorMap.view, HgGpuLayout_shaderReadOnly};
    hgUpdateGpuDescriptor(pipeline3D.defaultColorMap.descriptor, nullptr, &colorInfo);

    HgGpuImageDescriptorInfo normalInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultNormalMap.view, HgGpuLayout_shaderReadOnly};
    hgUpdateGpuDescriptor(pipeline3D.defaultNormalMap.descriptor, nullptr, &normalInfo);
}

void hgDeinitPipeline3D()
{
    hgDestroyGpuPipeline(pipeline3D.pipeline);

    hgDestroyGpuDescriptor(pipeline3D.defaultNormalMap.descriptor);
    hgDestroyGpuView(pipeline3D.defaultNormalMap.view);
    hgDestroyGpuImage(pipeline3D.defaultNormalMap.image);

    hgDestroyGpuDescriptor(pipeline3D.defaultColorMap.descriptor);
    hgDestroyGpuView(pipeline3D.defaultColorMap.view);
    hgDestroyGpuImage(pipeline3D.defaultColorMap.image);

    hgDestroyGpuSampler(pipeline3D.defaultMapSampler);

    hgDestroyGpuBuffer(pipeline3D.defaultModel.indexBuffer);
    hgDestroyGpuBuffer(pipeline3D.defaultModel.vertexBuffer);

    hgDestroyGpuDescriptor(pipeline3D.pointLightDesc);
    hgDestroyGpuDescriptor(pipeline3D.dirLightDesc);
    hgDestroyGpuBuffer(pipeline3D.pointLightBuffer);
    hgDestroyGpuBuffer(pipeline3D.dirLightBuffer);

    hgDestroyGpuDescriptor(pipeline3D.vpDesc);
    hgDestroyGpuBuffer(pipeline3D.vpBuffer);
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

void hgDraw3D(HgECS* ecs, HgGpuCommands* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    hgWriteGpuBuffer(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    u32 dirLightCount = ecs->count<HgDirLight3D>();
    u32 pointLightCount = ecs->count<HgPointLight3D>();

    if (dirLightCount > pipeline3D.dirLightCapacity)
    {
        hgGraphicsWaitIdle();

        pipeline3D.dirLightCapacity = pipeline3D.dirLightCapacity == 0 ? 1 : pipeline3D.dirLightCapacity * 2;
        while (pipeline3D.dirLightCapacity < dirLightCount)
        {
            pipeline3D.dirLightCapacity *= 2;
        }

        hgDestroyGpuBuffer(pipeline3D.dirLightBuffer);
        pipeline3D.dirLightBuffer = hgCreateGpuBuffer(
            sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);

        HgGpuBufferDescriptorInfo dirLightBufferInfo{pipeline3D.dirLightBuffer, 0, UINT64_MAX};
        hgUpdateGpuDescriptor(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);
    }

    if (pointLightCount > pipeline3D.pointLightCapacity)
    {
        hgGraphicsWaitIdle();

        pipeline3D.pointLightCapacity = pipeline3D.pointLightCapacity == 0 ? 1 : pipeline3D.pointLightCapacity * 2;
        while (pipeline3D.pointLightCapacity < pointLightCount)
        {
            pipeline3D.pointLightCapacity *= 2;
        }

        hgDestroyGpuBuffer(pipeline3D.pointLightBuffer);
        pipeline3D.pointLightBuffer = hgCreateGpuBuffer(
            sizeof(Pipeline3DPointLightData) * pipeline3D.pointLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);

        HgGpuBufferDescriptorInfo pointLightBufferInfo{pipeline3D.pointLightBuffer, 0, UINT64_MAX};
        hgUpdateGpuDescriptor(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);
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
        hgWriteGpuBuffer(pipeline3D.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        hgWriteGpuBuffer(pipeline3D.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    hgBindGpuPipeline(cmd, pipeline3D.pipeline);

    ecs->forEach<HgModel3D, HgTransform>([&](HgEntity, HgModel3D* model, HgTransform* transform)
    {
        HgTextureResource* colorMap = hgGetTexture(model->colorMap);
        if (colorMap == nullptr)
            colorMap = &pipeline3D.defaultColorMap;

        HgTextureResource* normalMap = hgGetTexture(model->normalMap);
        if (normalMap == nullptr)
            normalMap = &pipeline3D.defaultNormalMap;

        Pipeline3DPush push{};
        push.model = hgModelMatrix3D(transform->position, transform->scale, transform->rotation);
        push.dirLightIdx = hgGpuDescriptorIdx(pipeline3D.dirLightDesc);
        push.dirLightCount = dirLightCount;
        push.pointLightIdx = hgGpuDescriptorIdx(pipeline3D.pointLightDesc);
        push.pointLightCount = pointLightCount;
        push.vpIdx = hgGpuDescriptorIdx(pipeline3D.vpDesc);
        push.colorMapIdx = hgGpuDescriptorIdx(colorMap->descriptor);
        push.normalMapIdx = hgGpuDescriptorIdx(normalMap->descriptor);

        HgModelResource* gpuModel = hgGetModel(model->modelResource);
        if (gpuModel == nullptr)
            gpuModel = &pipeline3D.defaultModel;

        hgBindGpuIndexBuffer(cmd, gpuModel->indexBuffer);

        HgGpuBuffer* buffers[]{gpuModel->vertexBuffer};
        u64 offsets[]{0};
        hgBindGpuVertexBuffers(cmd, 0, buffers, offsets, 1);

        hgGpuPushConstants(cmd, pipeline3D.pipeline, 0, &push, sizeof(push));

        hgGpuDrawIndexed(cmd, 0, 0, gpuModel->indexCount, 0, 1);
    });
}

void hgInternalInitImGuiPlatform(HgWindow* window);
void hgInternalInitImGuiRenderer(
    HgWindow* window,
    u32 colorAttachmentCount,
    const HgFormat* colorFormats,
    HgFormat depthFormat,
    HgFormat stencilFormat);

void hgInternalDeinitImGuiPlatform();
void hgInternalDeinitImGuiRenderer();

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const HgFormat* colorFormats,
    HgFormat depthFormat,
    HgFormat stencilFormat)
{
    hgInternalInitImGuiPlatform(window);
    hgInternalInitImGuiRenderer(window, colorAttachmentCount, colorFormats, depthFormat, stencilFormat);
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    hgInternalDeinitImGuiRenderer();
    hgInternalDeinitImGuiPlatform();
}

