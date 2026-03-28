#include "hurdygurdy.hpp"

#include <ctime>

#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

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

    hgInitThreadPool(arena, init->threadPoolQueueSize, hgHardwareThreadCount() - 2); // main thread, io thread
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
    arena->head = hgAlign((u64)arena->head, alignment) + size;
    hgAssert(arena->head <= arena->capacity);
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* hgRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
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
    for (HgArena& arena : arenas)
    {
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
    char* cStr = hgAlloc<char>(arena, str.length + 1);
    memcpy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

HgString hgCreateString(HgArena* arena, u64 capacity)
{
    HgString str;
    str.chars = hgAlloc<char>(arena, capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString hgCopyString(HgArena* arena, HgStringView str)
{
    HgString copy;
    copy.chars = hgAlloc<char>(arena, str.length);
    copy.capacity = str.length;
    copy.length = str.length;
    memcpy(copy.chars, str.chars, str.length);
    return copy;
}

void hgReserveString(HgArena* arena, HgString* str, u64 newCapacity)
{
    str->chars = hgRealloc(arena, str->chars, str->capacity, newCapacity);
    str->capacity = newCapacity;
}

void hgGrowString(HgArena* arena, HgString* str, f32 factor)
{
    hgAssert(factor > 1.0f);
    hgAssert(str->capacity <= (u64)((f32)SIZE_MAX / factor));
    hgReserveString(arena, str, str->capacity == 0 ? 1 : (u64)((f32)str->capacity * factor));
}

void hgInsertString(HgArena* arena, HgString* dst, u64 idx, HgStringView src)
{
    hgAssert(idx <= dst->length);

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

namespace {
    struct HgJsonParser {
        HgArena* arena;
        HgStringView text;
        u64 head;
        u64 line;

        HgJsonParser(HgArena* arenaVal) : arena{arenaVal} {}

        HgJson parseNext();
        HgJson parseStruct();
        HgJson parseArray();
        HgJson parseString();
        HgJson parseNumber();
        HgJson parseBoolean();
    };
}

HgJson HgJsonParser::parseNext()
{
    while (head < text.length && hgIsWhitespace(text[head]))
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head >= text.length)
        return {};

    switch (text[head])
    {
        case '{':
            ++head;
            return parseStruct();
        case '[':
            ++head;
            return parseArray();
        case '\'': [[fallthrough]];
        case '"':
            ++head;
            return parseString();
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return parseNumber();
        case 't': [[fallthrough]];
        case 'f':
            return parseBoolean();
        case '}': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", found unexpected token \"]\"\n");
            return {nullptr, error};
        }
    }
    if (hgIsNumeral(text[head]))
    {
        return parseNumber();
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
    error->next = nullptr;

    u64 begin = head;
    while (head < text.length && !hgIsWhitespace(text[head]))
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{};
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
    hgAppendString(arena, &error->msg, ", found unexpected token \"");
    hgAppendString(arena, &error->msg, {&text[begin], &text[head]});
    hgAppendString(arena, &error->msg, "\"\n");

    return {nullptr, error};
}

HgJson HgJsonParser::parseStruct()
{
    HgJson json{};
    json.file = hgAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_struct;
    json.file->jstruct.fields = nullptr;

    HgJsonField* lastField = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length)
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", expected struct to terminate\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (text[head] == ']')
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", struct ends with \"]\" instead of \"}\"\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++head;
            while (head < text.length && hgIsWhitespace(text[head]))
            {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == '}')
        {
            ++head;
            while (head < text.length && hgIsWhitespace(text[head]))
            {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJson value = parseNext();

        if (value.file != nullptr)
        {
            if (value.file->type != HgJsonType::HgJsonType_field)
            {
                HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                error->msg = HgString{};
                hgAppendString(arena, &error->msg, "on line ");
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
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
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
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

HgJson HgJsonParser::parseArray()
{
    HgJson json{};
    json.file = hgAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_array;

    HgJsonType type = HgJsonType::HgJsonType_none;
    HgJsonElem* lastElem = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length)
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", expected struct to terminate\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (text[head] == '}')
        {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{};
            hgAppendString(arena, &error->msg, "on line ");
            hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
            hgAppendString(arena, &error->msg, ", array ends with \"}\" instead of \"]\"\n");
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++head;
            while (head < text.length && hgIsWhitespace(text[head]))
            {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == ']')
        {
            ++head;
            while (head < text.length && hgIsWhitespace(text[head]))
            {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJsonElem* elem = hgAlloc<HgJsonElem>(arena, 1);
        elem->next = nullptr;

        HgJson value = parseNext();
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
                    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
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
                hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
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

HgJson HgJsonParser::parseString()
{
    u64 begin = head;
    while (head < text.length && text[head] != '"')
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    u64 end = head;
    if (head < text.length)
    {
        ++head;
        HgString str = hgCreateString(arena, end - begin);
        for (u64 i = begin; i < end; ++i)
        {
            char c = text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            hgAppendString(arena, &str, c);
        }

        HgJson json{};
        json.file = hgAlloc<HgJsonNode>(arena, 1);

        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ':')
        {
            ++head;
            json.file->type = HgJsonType::HgJsonType_field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            HgJson next = parseNext();
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = HgJsonType::HgJsonType_string;
            json.file->string = str;
        }
        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;
        return json;
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
    error->msg = HgString{};
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
    hgAppendString(arena, &error->msg, ", expected string to terminate\n");
    return {nullptr, error};
}

HgJson HgJsonParser::parseNumber()
{
    bool isFloat = false;
    u64 begin = head;
    while (head < text.length && (
        hgIsNumeral(text[head]) ||
        text[head] == '-' ||
        text[head] == '+' ||
        text[head] == '.' ||
        text[head] == 'e'
    ))
    {
        if (text[head] == '.' || text[head] == 'e')
            isFloat = true;
        ++head;
    }
    HgStringView num{&text[begin], &text[head]};
    while (head < text.length && hgIsWhitespace(text[head]))
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head < text.length && text[head] == ',')
        ++head;

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
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
    hgAppendString(arena, &error->msg, ", expected numeral value, found \"");
    hgAppendString(arena, &error->msg, num);
    hgAppendString(arena, &error->msg, "\"\n");

    while (head < text.length && hgIsWhitespace(text[head]))
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = parseNext();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson HgJsonParser::parseBoolean()
{
    if (head + 4 < text.length && HgStringView{&text[head], 4} == "true")
    {
        head += 4;
        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = true;
        return {node, nullptr};
    }
    if (head + 5 < text.length && HgStringView{&text[head], 5} == "false")
    {
        head += 5;
        while (head < text.length && hgIsWhitespace(text[head]))
        {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJsonNode* node = hgAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = false;
        return {node, nullptr};
    }

    HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);

    u64 begin = head;
    while (head < text.length && !hgIsWhitespace(text[head])
        && text[head] != ','
        && text[head] != '}'
        && text[head] != ']'
    )
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{};
    hgAppendString(arena, &error->msg, "on line ");
    hgAppendString(arena, &error->msg, hgIntToStr(arena, (i64)line));
    hgAppendString(arena, &error->msg, ", expected boolean value, found \"");
    hgAppendString(arena, &error->msg, {&text[begin], &text[head]});
    hgAppendString(arena, &error->msg, "\"\n");

    if (text[head] == ',')
        ++head;

    while (head < text.length && hgIsWhitespace(text[head]))
    {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = parseNext();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson hgParseJson(HgArena* arena, HgStringView text)
{
    HgJsonParser parser{arena};
    parser.text = text;
    parser.head = 0;
    parser.line = 1;
    return parser.parseNext();
}

HgClock::HgClock()
{
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
}

f64 hgClockTick(HgClock* clock)
{
    f64 prev = clock->time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    clock->time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
    return clock->time - prev;
}

HgPerf hgCreatePerf(HgArena* arena, u32 count)
{
    HgPerf perf;
    perf.times = hgAlloc<f64>(arena, count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void hgBeginPerf(HgPerf* perf)
{
    hgClockTick(&perf->clock);
}

f64 hgEndPerf(HgPerf* perf)
{
    hgAssert(perf->current < perf->count);

    f64 time = hgClockTick(&perf->clock);
    perf->times[perf->current++] = time;

    return time;
}

HgPerfStats hgAnalyzePerf(const HgPerf* perf)
{
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
    fence->counter.fetch_add(count);
}

void hgSignalFence(HgFence* fence, u32 count)
{
    fence->counter.fetch_sub(count);
}

bool hgIsFenceComplete(const HgFence* fence)
{
    return fence->counter.load() == 0;
}

void hgWaitForFenceIndefinite(const HgFence* fence)
{
    while (!hgIsFenceComplete(fence))
    {
        _mm_pause();
    }
}

bool hgWaitForFenceTimeout(const HgFence* fence, f64 timeoutSeconds)
{
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
    hgAssert(threadCount > 1 && (queueSize & (queueSize - 1)) == 0);

    threadPool.shouldClose.store(false);
    threadPool.threadCount = std::min((u32)1, threadCount - 1);
    threadPool.threads = hgAlloc<std::thread>(arena, threadPool.threadCount);

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

    for (u32 i = 0; i < threadPool.threadCount; ++i)
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
    ioThread.shouldClose = true;
    ioThread.thread.join();
}

void hgRequestIO(
    HgFence* fences,
    u32 fenceCount,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path))
{
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

void hgInternalLoadVulkan();
void hgInternalUnloadVulkan();

struct VulkanState {
#ifdef HG_VK_DEBUG_MESSENGER
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif

    VkDescriptorPool bindlessPool = nullptr;
    VkDescriptorSetLayout bindlessLayout = nullptr;
    VkDescriptorSet bindlessSet = nullptr;

    HgDescriptor* samplerPool = nullptr;
    HgDescriptor samplerPoolNext = {0};
    HgDescriptor* combinedImageSamplerPool = nullptr;
    HgDescriptor combinedImageSamplerPoolNext = {0};
    HgDescriptor* sampledImagePool = nullptr;
    HgDescriptor sampledImagePoolNext = {0};
    HgDescriptor* storageImagePool = nullptr;
    HgDescriptor storageImagePoolNext = {0};
    HgDescriptor* uniformTexelBufferPool = nullptr;
    HgDescriptor uniformTexelBufferPoolNext = {0};
    HgDescriptor* storageTexelBufferPool = nullptr;
    HgDescriptor storageTexelBufferPoolNext = {0};
    HgDescriptor* uniformBufferPool = nullptr;
    HgDescriptor uniformBufferPoolNext = {0};
    HgDescriptor* storageBufferPool = nullptr;
    HgDescriptor storageBufferPoolNext = {0};
};

static VulkanState vulkanState;

static HgDescriptor* createBindlessPool(HgArena* arena, HgDescriptorType type, HgDescriptor* next)
{
    HgDescriptor* pool = hgAlloc<HgDescriptor>(arena, UINT16_MAX);

    for (u32 i = 0; i < UINT16_MAX; ++i)
    {
        pool[i] = {i + 1};
        pool[i].id = (pool[i].id & 0x0fffffff) | (type << 28);
    }
    *next = {0};
    next->id = (next->id & 0x0fffffff) | (type << 28);

    return pool;
}

void hgInitGraphics(HgArena* arena)
{
    hgInternalLoadVulkan();

    if (hgVkInstance == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgStringView* exts;
        u32 extCount = hgGetPlatformVulkanExtensions(scratch, &exts);
#ifdef HG_VK_DEBUG_MESSENGER
        exts = hgRealloc(scratch, exts, extCount, extCount + 1);
        exts[extCount++] = "VK_EXT_debug_utils";
#endif
        hgVkInstance = hgCreateVkInstance(exts, extCount);
        hgLoadVulkanInstanceFuncs(hgVkInstance);
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (vulkanState.debugMessenger == nullptr)
        vulkanState.debugMessenger = hgCreateVkDebugUtilsMessenger();
#endif

    if (hgVkPhysicalDevice == nullptr)
    {
        hgVkPhysicalDevice = hgFindVkPhysicalDevice();
        hgFindVkQueueFamily(hgVkPhysicalDevice, &hgVkQueueFamily,
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    if (hgVkDevice == nullptr)
    {
        hgVkDevice = hgCreateVkDevice();
        hgLoadVulkanDeviceFuncs(hgVkDevice);
        vkGetDeviceQueue(hgVkDevice, hgVkQueueFamily, 0, &hgVkQueue);
    }

    if (hgVkVma == nullptr)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = hgVkPhysicalDevice;
        allocatorInfo.device = hgVkDevice;
        allocatorInfo.instance = hgVkInstance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &hgVkVma);
        if (hgVkVma == nullptr)
            hgError("Could note create Vulkan memory allocator: %s\n", hgVkResultToStr(result));
    }

    if (hgVkCmdPool == nullptr)
    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = hgVkQueueFamily;

        VkResult result = vkCreateCommandPool(hgVkDevice, &cmdPoolInfo, nullptr, &hgVkCmdPool);
        if (hgVkCmdPool == nullptr)
            hgError("Could note create Vulkan command pool: %s\n", hgVkResultToStr(result));
    }

    if (vulkanState.bindlessPool == nullptr)
    {
        VkDescriptorPoolSize sizes[]{
            {VK_DESCRIPTOR_TYPE_SAMPLER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, UINT16_MAX},
        };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        info.maxSets = 1;
        info.poolSizeCount = sizeof(sizes) / sizeof(*sizes);
        info.pPoolSizes = sizes;

        VkResult result = vkCreateDescriptorPool(hgVkDevice, &info, nullptr, &vulkanState.bindlessPool);
        if (vulkanState.bindlessPool == nullptr)
            hgError("Could not create bindless VkDescriptorPool: %s\n", hgVkResultToStr(result));
    };

    if (vulkanState.bindlessLayout == nullptr)
    {
        VkDescriptorSetLayoutBinding bindings[HgDescriptorType_count]{};
        VkDescriptorBindingFlags flags[HgDescriptorType_count]{};
        for (u32 i = 0; i < HgDescriptorType_count; ++i)
        {
            bindings[i].descriptorCount = UINT16_MAX;
            bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
            flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        }
        bindings[0].binding = HgDescriptorType_sampler;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[1].binding = HgDescriptorType_combinedImageSampler;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].binding = HgDescriptorType_sampledImage;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[3].binding = HgDescriptorType_storageImage;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[4].binding = HgDescriptorType_uniformTexelBuffer;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bindings[5].binding = HgDescriptorType_storageTexelBuffer;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        bindings[6].binding = HgDescriptorType_uniformBuffer;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[7].binding = HgDescriptorType_storageBuffer;
        bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
        flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsInfo.bindingCount = sizeof(bindings) / sizeof(*bindings);
        flagsInfo.pBindingFlags = flags;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = &flagsInfo;
        info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        info.bindingCount = sizeof(bindings) / sizeof(*bindings);
        info.pBindings = bindings;

        VkResult result = vkCreateDescriptorSetLayout(hgVkDevice, &info, nullptr, &vulkanState.bindlessLayout);
        if (vulkanState.bindlessLayout == nullptr)
            hgError("Could not create bindless VkDescriptorSetLayout: %s\n", hgVkResultToStr(result));
    }

    if (vulkanState.bindlessSet == nullptr)
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vulkanState.bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vulkanState.bindlessLayout;

        VkResult result = vkAllocateDescriptorSets(hgVkDevice, &info, &vulkanState.bindlessSet);
        if (vulkanState.bindlessSet == nullptr)
            hgError("Could not allocate bindless VkDescriptorSet: %s\n", hgVkResultToStr(result));
    }

    if (vulkanState.samplerPool == nullptr)
        vulkanState.samplerPool = createBindlessPool(
            arena, HgDescriptorType_sampler, &vulkanState.samplerPoolNext);

    if (vulkanState.combinedImageSamplerPool == nullptr)
        vulkanState.combinedImageSamplerPool = createBindlessPool(
            arena, HgDescriptorType_combinedImageSampler, &vulkanState.combinedImageSamplerPoolNext);

    if (vulkanState.sampledImagePool == nullptr)
        vulkanState.sampledImagePool = createBindlessPool(
            arena, HgDescriptorType_sampledImage, &vulkanState.sampledImagePoolNext);

    if (vulkanState.storageImagePool == nullptr)
        vulkanState.storageImagePool = createBindlessPool(
            arena, HgDescriptorType_storageImage, &vulkanState.storageImagePoolNext);

    if (vulkanState.uniformTexelBufferPool == nullptr)
        vulkanState.uniformTexelBufferPool = createBindlessPool(
            arena, HgDescriptorType_uniformTexelBuffer, &vulkanState.uniformTexelBufferPoolNext);

    if (vulkanState.storageTexelBufferPool == nullptr)
        vulkanState.storageTexelBufferPool = createBindlessPool(
            arena, HgDescriptorType_storageTexelBuffer, &vulkanState.storageTexelBufferPoolNext);

    if (vulkanState.uniformBufferPool == nullptr)
        vulkanState.uniformBufferPool = createBindlessPool(
            arena, HgDescriptorType_uniformBuffer, &vulkanState.uniformBufferPoolNext);

    if (vulkanState.storageBufferPool == nullptr)
        vulkanState.storageBufferPool = createBindlessPool(
            arena, HgDescriptorType_storageBuffer, &vulkanState.storageBufferPoolNext);
}

void hgDeinitGraphics()
{
    vulkanState.samplerPool = nullptr;
    vulkanState.combinedImageSamplerPool = nullptr;
    vulkanState.sampledImagePool = nullptr;
    vulkanState.storageImagePool = nullptr;
    vulkanState.uniformTexelBufferPool = nullptr;
    vulkanState.storageTexelBufferPool = nullptr;
    vulkanState.uniformBufferPool = nullptr;
    vulkanState.storageBufferPool = nullptr;
    vulkanState.bindlessSet = nullptr;

    if (vulkanState.bindlessLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(hgVkDevice, vulkanState.bindlessLayout, nullptr);
        vulkanState.bindlessLayout = nullptr;
    }

    if (vulkanState.bindlessPool != nullptr)
    {
        vkDestroyDescriptorPool(hgVkDevice, vulkanState.bindlessPool, nullptr);
        vulkanState.bindlessPool = nullptr;
    }

    if (hgVkCmdPool != nullptr)
    {
        vkDestroyCommandPool(hgVkDevice, hgVkCmdPool, nullptr);
        hgVkCmdPool = nullptr;
    }

    if (hgVkVma != nullptr)
    {
        vmaDestroyAllocator(hgVkVma);
        hgVkVma = nullptr;
    }

    if (hgVkDevice != nullptr)
    {
        vkDestroyDevice(hgVkDevice, nullptr);
        hgVkDevice = nullptr;
    }

    if (hgVkPhysicalDevice != nullptr)
    {
        hgVkPhysicalDevice = nullptr;
        hgVkQueueFamily = (u32)-1;
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (vulkanState.debugMessenger != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(hgVkInstance, vulkanState.debugMessenger, nullptr);
        vulkanState.debugMessenger = nullptr;
    }
#endif

    if (hgVkInstance != nullptr)
    {
        vkDestroyInstance(hgVkInstance, nullptr);
        hgVkInstance = nullptr;
    }

    hgInternalUnloadVulkan();
}

const char* hgVkResultToStr(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

u32 hgVkFormatToSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_A8_UNORM:
        case VK_FORMAT_R8_BOOL_ARM:
            return 1;

        case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
            return 2;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;

        case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return 8;

        case VK_FORMAT_G8B8G8R8_422_UNORM:
        case VK_FORMAT_B8G8R8G8_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16B16G16R16_422_UNORM:
        case VK_FORMAT_B16G16R16G16_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return 0;

        default:
            hgWarn("Unrecognized Vulkan format value\n");
            return 0;
    }
}

#ifdef HG_VK_DEBUG_MESSENGER

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    (void)type;
    (void)userData;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage);
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage);
    } else {
        (void)fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugCallback,
    nullptr,
};

#endif

VkInstance hgCreateVkInstance(HgStringView* extensions, u32 extensionCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hurdy Gurdy Application",
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Hurdy Gurdy Engine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef HG_VK_DEBUG_MESSENGER
    instanceInfo.pNext = &debugUtilsMessengerInfo;
#endif
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;

#ifdef HG_VK_DEBUG_MESSENGER
    const char* layers[]{
        "VK_LAYER_KHRONOS_validation",
    };
    instanceInfo.enabledLayerCount = sizeof(layers) / sizeof(*layers);
    instanceInfo.ppEnabledLayerNames = layers;
#endif

    const char** extCStrs = hgAlloc<const char*>(scratch, extensionCount);
    for (u32 i = 0; i < extensionCount; ++i)
    {
        extCStrs[i] = hgCString(scratch, extensions[i]);
    }
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extCStrs;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
        hgError("Failed to create Vulkan instance: %s\n", hgVkResultToStr(result));

    return instance;
}

VkDebugUtilsMessengerEXT hgCreateVkDebugUtilsMessenger()
{
#ifdef HG_VK_DEBUG_MESSENGER
    hgAssert(hgVkInstance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        hgVkInstance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
        hgError("Failed to create Vulkan debug messenger: %s\n", hgVkResultToStr(result));

    return messenger;
#else
    return nullptr;
#endif
}

bool hgFindVkQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    hgAssert(gpu != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = hgAlloc<VkQueueFamilyProperties>(scratch, familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families);

    for (u32 i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & queueFlags)
        {
            *queueFamily = i;
            return true;
        }
    }
    return false;
}

static const char* const deviceExtensions[]{
    "VK_KHR_swapchain",
};

VkPhysicalDevice hgFindVkPhysicalDevice()
{
    hgAssert(hgVkInstance != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 gpuCount;
    vkEnumeratePhysicalDevices(hgVkInstance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = hgAlloc<VkPhysicalDevice>(scratch, gpuCount);
    vkEnumeratePhysicalDevices(hgVkInstance, &gpuCount, gpus);

    VkExtensionProperties* extProps = nullptr;
    u32 extPropCount = 0;

    for (u32 i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 newPropCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &newPropCount, nullptr);
        if (newPropCount > extPropCount)
        {
            extProps = hgRealloc(scratch, extProps, extPropCount, newPropCount);
            extPropCount = newPropCount;
        }
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &newPropCount, extProps);

        for (u32 j = 0; j < sizeof(deviceExtensions) / sizeof(*deviceExtensions); j++)
        {
            for (u32 k = 0; k < newPropCount; k++)
            {
                if (strcmp(deviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!hgFindVkQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    hgWarn("Could not find a suitable gpu\n");
    return nullptr;
}

VkDevice hgCreateVkDevice()
{
    hgAssert(hgVkPhysicalDevice != nullptr);
    hgAssert(hgVkQueueFamily != (u32)-1);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature{};
    descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeature.pNext = nullptr;
    descriptorIndexingFeature.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = &descriptorIndexingFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = hgVkQueueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(*deviceExtensions);
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(hgVkPhysicalDevice, &deviceInfo, nullptr, &device);

    if (device == nullptr)
        hgError("Could not create Vulkan device: %s\n", hgVkResultToStr(result));
    return device;
}

u32 hgFindVkMemoryTypeIndex(
    u32 bitmask,
    VkMemoryPropertyFlags preferredFlags,
    VkMemoryPropertyFlags unpreferredFlags)
{
    hgAssert(hgVkPhysicalDevice != nullptr);
    hgAssert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(hgVkPhysicalDevice, &memProps);

    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0 &&
            (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0 &&
            (memProps.memoryTypes[i].propertyFlags & unpreferredFlags) == 0)
        {
            return i;
        }
    }
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0)
        {
            hgWarn("Could not find Vulkan memory type without undesired flags\n");
            return i;
        }
    }
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0)
        {
            hgWarn("Could not find Vulkan memory type with desired flags\n");
            return i;
        }
    }
    hgError("Could not find Vulkan memory type\n");
}

HgBuffer* hgCreateBuffer(
    u64 size,
    VkBufferUsageFlags usage,
    HgBufferMemoryUsage access)
{
    HgBuffer* buffer = (HgBuffer*)malloc(sizeof(*buffer));
    *buffer = {};

    hgAssert(size > 0);
    hgAssert(usage != 0);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    switch (access)
    {
    case HgBufferMemoryUsage_deviceOnly:
        break;
    case HgBufferMemoryUsage_frequentUpdate:
        allocInfo.flags
            = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        break;
    case HgBufferMemoryUsage_stagingWrite:
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        break;
    case HgBufferMemoryUsage_stagingRead:
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        break;
    }

    VkResult result = vmaCreateBuffer(
        hgVkVma,
        &bufferInfo,
        &allocInfo,
        &buffer->buffer,
        &buffer->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkBuffer: %s", hgVkResultToStr(result));

    buffer->size = size;

    switch (access)
    {
    case HgBufferMemoryUsage_deviceOnly:
        buffer->access = HgBufferMemoryHostAccess_none;
        break;
    case HgBufferMemoryUsage_frequentUpdate:
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(hgVkVma, buffer->alloc, &memPropFlags);
        buffer->access = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? HgBufferMemoryHostAccess_write
            : HgBufferMemoryHostAccess_none;
        break;
    case HgBufferMemoryUsage_stagingWrite:
        buffer->access = HgBufferMemoryHostAccess_write;
        break;
    case HgBufferMemoryUsage_stagingRead:
        buffer->access = HgBufferMemoryHostAccess_read;
        break;
    }

    return buffer;
}

void hgDestroyBuffer(HgBuffer *buffer)
{
    if (buffer != nullptr)
    {
        vmaDestroyBuffer(hgVkVma, buffer->buffer, buffer->alloc);
        free(buffer);
    }
}

void hgWriteBuffer(HgBuffer* dst, u64 offset, const void* src, u64 size)
{
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (dst->access & HgBufferMemoryHostAccess_write)
    {
        VkResult result = vmaCopyMemoryToAllocation(hgVkVma, src, dst->alloc, offset, size);
        if (result != VK_SUCCESS)
            hgError("Could not write gpu buffer: %s", hgVkResultToStr(result));
        return;
    }

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(stage));
    hgWriteBuffer(stage, 0, src, size);

    VkCommandBuffer cmd = hgBeginVkCmd();
    {
        VkBufferCopy region{};
        region.dstOffset = offset;
        region.size = size;

        vkCmdCopyBuffer(cmd, stage->buffer, dst->buffer, 1, &region);
    }
    hgEndVkCmd(cmd);
}

void hgReadBuffer(void* dst, HgBuffer* src, u64 offset, u64 size)
{
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (src->access & HgBufferMemoryHostAccess_read)
    {
        VkResult result = vmaCopyAllocationToMemory(hgVkVma, src->alloc, offset, dst, size);
        if (result != VK_SUCCESS)
            hgError("Could not read gpu buffer: %s", hgVkResultToStr(result));
        return;
    }

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, HgBufferMemoryUsage_stagingRead);
    hgDefer(hgDestroyBuffer(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();
    {
        VkBufferCopy region{};
        region.srcOffset = offset;
        region.size = size;

        vkCmdCopyBuffer(cmd, src->buffer, stage->buffer, 1, &region);
    }
    hgEndVkCmd(cmd);

    hgReadBuffer(dst, stage, 0, size);
}

HgImage* hgCreateImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage)
{
    HgCreateImageEx create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    return hgCreateImageEx(&create);
}

HgImage* hgCreateImageEx(const HgCreateImageEx* create)
{
    hgAssert(create->format != VK_FORMAT_UNDEFINED);
    hgAssert(create->usage != 0);

    HgImage* image = (HgImage*)malloc(sizeof(*image));
    *image = {};

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = create->type;
    imageInfo.format = create->format;
    imageInfo.extent = {create->width, create->height, create->depth};
    imageInfo.mipLevels = create->mipLevels;
    imageInfo.arrayLayers = create->arrayLayers;
    imageInfo.samples = create->msaaSamples;
    imageInfo.usage = create->usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        hgVkVma,
        &imageInfo,
        &allocInfo,
        &image->image,
        &image->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkImage: %s", hgVkResultToStr(result));

    image->type = create->type;
    image->format = create->format;
    image->width = create->width;
    image->height = create->height;
    image->depth = create->depth;
    image->mipLevels = create->mipLevels;
    image->arrayLayers = create->arrayLayers;
    image->msaaSamples = create->msaaSamples;

    return image;
}

void hgDestroyImage(HgImage* image)
{
    if (image != nullptr)
    {
        vmaDestroyImage(hgVkVma, image->image, image->alloc);
        free(image);
    }
}

HgImageView* hgCreateImageView(
    const HgImage* image,
    VkImageSubresourceRange subresource,
    VkImageViewType type)
{
    hgAssert(image != nullptr);
    hgAssert(subresource.aspectMask != 0);

    HgImageView* view = (HgImageView*)malloc(sizeof(*view));
    *view = {};

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image->image;
    info.viewType = type;
    info.format = image->format;
    info.subresourceRange = subresource;

    VkResult result = vkCreateImageView(hgVkDevice, &info, nullptr, &view->view);
    if (view == nullptr)
        hgError("Could not create VkImageView: %s\n", hgVkResultToStr(result));

    view->image = image;
    view->type = type;
    view->aspectFlags = subresource.aspectMask;
    view->baseMipLevel = subresource.baseMipLevel;
    view->levelCount = subresource.levelCount;
    view->baseArrayLayer = subresource.baseArrayLayer;
    view->layerCount = subresource.layerCount;

    return view;
}

void hgDestroyImageView(HgImageView* view)
{
    if (view != nullptr)
    {
        vkDestroyImageView(hgVkDevice, view->view, nullptr);
        free(view);
    }
}

void hgWriteImage(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout)
{
    hgAssert(dst!= nullptr);
    hgAssert(src!= nullptr);

    u64 size = dst->width * dst->height * dst->depth * hgVkFormatToSize(dst->format);

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(stage));
    hgWriteBuffer(stage, 0, src, size);

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = dst->image;
    transferBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = subresource;
    region.imageExtent = {dst->width, dst->height, dst->depth};

    vkCmdCopyBufferToImage(cmd, stage->buffer, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = dst->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);
}

void hgWriteImageCubemap(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout)
{
    hgAssert(dst != nullptr);
    hgAssert(subresource.baseArrayLayer == 0);
    hgAssert(subresource.layerCount == 6);
    hgAssert(src != nullptr);
    hgAssert(dst->depth == 1);

    u64 size = dst->width * dst->height * hgVkFormatToSize(dst->format);

    HgBuffer* buffer = hgCreateBuffer(
        size * 4 * 3,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(buffer));
    hgWriteBuffer(buffer, 0, src, size);

    HgImage* stage = hgCreateImage(
        dst->width * 4,
        dst->height * 3,
        dst->format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    hgDefer(hgDestroyImage(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = dst->image;
    stageBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    stageBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    stageBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(cmd, &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = subresource;
    stageRegion.imageExtent = {dst->width, dst->height, 1};

    vkCmdCopyBufferToImage(cmd, buffer->buffer, stage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &stageRegion);

    VkImageMemoryBarrier2 transferBarriers[2]{};

    transferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarriers[0].image = stage->image;
    transferBarriers[0].subresourceRange.aspectMask = subresource.aspectMask;
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = dst->image;
    transferBarriers[1].subresourceRange.aspectMask = subresource.aspectMask;
    transferBarriers[1].subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarriers[1].subresourceRange.levelCount = 1;
    transferBarriers[1].subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = sizeof(transferBarriers) / sizeof(*transferBarriers);
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[0].srcOffset = {(int)dst->width * 2, (int)dst->height * 1, 0};
    regions[0].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {dst->width, dst->height, 1};

    regions[1].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[1].srcOffset = {(int)dst->width * 0, (int)dst->height * 1, 0};
    regions[1].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {dst->width, dst->height, 1};

    regions[2].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[2].srcOffset = {(int)dst->width * 1, (int)dst->height * 0, 0};
    regions[2].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {dst->width, dst->height, 1};

    regions[3].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[3].srcOffset = {(int)dst->width * 1, (int)dst->height * 2, 0};
    regions[3].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {dst->width, dst->height, 1};

    regions[4].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[4].srcOffset = {(int)dst->width * 1, (int)dst->height * 1, 0};
    regions[4].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {dst->width, dst->height, 1};

    regions[5].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[5].srcOffset = {(int)dst->width * 3, (int)dst->height * 1, 0};
    regions[5].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {dst->width, dst->height, 1};

    vkCmdCopyImage(
        cmd,
        stage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        sizeof(regions) / sizeof(*regions),
        regions);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = dst->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);
}

void hgReadImage(
    void* dst,
    const HgImage* src,
    VkImageSubresourceLayers subresource,
    VkImageLayout layout)
{
    hgAssert(src != nullptr);
    hgAssert(layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(dst != nullptr);

    u64 size = src->width * src->height * src->depth * hgVkFormatToSize(src->format);

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        HgBufferMemoryUsage_stagingRead);
    hgDefer(hgDestroyBuffer(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = layout;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = src->image;
    transferBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = subresource;
    region.imageExtent = {src->width, src->height, src->depth};

    vkCmdCopyImageToBuffer(cmd, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage->buffer, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = src->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);

    hgReadBuffer(dst, stage, 0, size);
}

void hgGenerateMipmaps(
    HgImage* image,
    VkImageAspectFlags aspectFlags,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    hgAssert(image != nullptr);
    hgAssert(oldLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    if (image->mipLevels == 1)
        return;

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkOffset3D mipOffset{};
    mipOffset.x = (i32)image->width;
    mipOffset.y = (i32)image->height;
    mipOffset.z = (i32)image->depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = image->image;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    for (u32 level = 0; level < image->mipLevels - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = aspectFlags;
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = aspectFlags;
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(cmd,
            image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);
    }

    barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.dstAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = newLayout;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image->mipLevels;

    vkCmdPipelineBarrier2(cmd, &dep);

    hgEndVkCmd(cmd);
}

VkSampler hgCreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBorderColor borderColor)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.mipmapMode = filter == VK_FILTER_LINEAR
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = addressMode;
    info.addressModeV = addressMode;
    info.addressModeW = addressMode;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = borderColor;

    VkSampler sampler = nullptr;
    VkResult result = vkCreateSampler(hgVkDevice, &info, nullptr, &sampler);
    if (sampler == nullptr)
        hgError("Could not create VkSampler: %s", hgVkResultToStr(result));

    return sampler;
}

static HgDescriptor allocDescriptor(HgDescriptor* pool, HgDescriptor* next)
{
    HgDescriptor desc = *next;

    u32 idx = hgDescriptorIdx(*next);
    *next = pool[idx];

    pool[hgDescriptorIdx(desc)] = desc;
    return desc;
}

static void deallocDescriptor(HgDescriptor* pool, HgDescriptor* next, HgDescriptor desc)
{
    u32 idx = hgDescriptorIdx(desc);
    pool[idx] = *next;

    desc.id += 1 << 16;
    *next = desc;
}

HgDescriptor hgCreateDescriptor(HgDescriptorType type)
{
    switch (type)
    {
        case HgDescriptorType_sampler:
            hgAssert(hgDescriptorIdx(vulkanState.samplerPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.samplerPool, &vulkanState.samplerPoolNext);
            break;
        case HgDescriptorType_combinedImageSampler:
            hgAssert(hgDescriptorIdx(vulkanState.combinedImageSamplerPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.combinedImageSamplerPool, &vulkanState.combinedImageSamplerPoolNext);
            break;
        case HgDescriptorType_sampledImage:
            hgAssert(hgDescriptorIdx(vulkanState.sampledImagePoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.sampledImagePool, &vulkanState.sampledImagePoolNext);
            break;
        case HgDescriptorType_storageImage:
            hgAssert(hgDescriptorIdx(vulkanState.storageImagePoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.storageImagePool, &vulkanState.storageImagePoolNext);
            break;
        case HgDescriptorType_uniformTexelBuffer:
            hgAssert(hgDescriptorIdx(vulkanState.uniformTexelBufferPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.uniformTexelBufferPool, &vulkanState.uniformTexelBufferPoolNext);
            break;
        case HgDescriptorType_storageTexelBuffer:
            hgAssert(hgDescriptorIdx(vulkanState.storageTexelBufferPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.storageTexelBufferPool, &vulkanState.storageTexelBufferPoolNext);
            break;
        case HgDescriptorType_uniformBuffer:
            hgAssert(hgDescriptorIdx(vulkanState.uniformBufferPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.uniformBufferPool, &vulkanState.uniformBufferPoolNext);
            break;
        case HgDescriptorType_storageBuffer:
            hgAssert(hgDescriptorIdx(vulkanState.storageBufferPoolNext) != UINT16_MAX);
            return allocDescriptor(vulkanState.storageBufferPool, &vulkanState.storageBufferPoolNext);
            break;
        default:
            hgAssert(false);
            break;
    }
}

void hgDestroyDescriptor(HgDescriptor desc)
{
    if (desc.id == HgDescriptor{}.id)
        return;

    switch (hgDescriptorType(desc))
    {
        case HgDescriptorType_sampler:
            hgAssert(desc.id == vulkanState.samplerPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.samplerPool, &vulkanState.samplerPoolNext, desc);
            break;
        case HgDescriptorType_combinedImageSampler:
            hgAssert(desc.id == vulkanState.combinedImageSamplerPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.combinedImageSamplerPool, &vulkanState.combinedImageSamplerPoolNext, desc);
            break;
        case HgDescriptorType_sampledImage:
            hgAssert(desc.id == vulkanState.sampledImagePool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.sampledImagePool, &vulkanState.sampledImagePoolNext, desc);
            break;
        case HgDescriptorType_storageImage:
            hgAssert(desc.id == vulkanState.storageImagePool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.storageImagePool, &vulkanState.storageImagePoolNext, desc);
            break;
        case HgDescriptorType_uniformTexelBuffer:
            hgAssert(desc.id == vulkanState.uniformTexelBufferPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.uniformTexelBufferPool, &vulkanState.uniformTexelBufferPoolNext, desc);
            break;
        case HgDescriptorType_storageTexelBuffer:
            hgAssert(desc.id == vulkanState.storageTexelBufferPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.storageTexelBufferPool, &vulkanState.storageTexelBufferPoolNext, desc);
            break;
        case HgDescriptorType_uniformBuffer:
            hgAssert(desc.id == vulkanState.uniformBufferPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.uniformBufferPool, &vulkanState.uniformBufferPoolNext, desc);
            break;
        case HgDescriptorType_storageBuffer:
            hgAssert(desc.id == vulkanState.storageBufferPool[hgDescriptorIdx(desc)].id);
            deallocDescriptor(vulkanState.storageBufferPool, &vulkanState.storageBufferPoolNext, desc);
            break;
        default:
            hgAssert(false);
            break;
    }
}

VkDescriptorType hgDescriptorTypeToVk(HgDescriptorType type)
{
    switch (type)
    {
        case HgDescriptorType_sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case HgDescriptorType_combinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        case HgDescriptorType_sampledImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case HgDescriptorType_storageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case HgDescriptorType_uniformTexelBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case HgDescriptorType_storageTexelBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        case HgDescriptorType_uniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case HgDescriptorType_storageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        default:
            break;
    }
    hgAssert(false);
}

void hgUpdateDescriptor(
    HgDescriptor descriptor,
    const VkDescriptorBufferInfo* bufferInfo,
    const VkDescriptorImageInfo* imageInfo,
    const VkBufferView* texelInfo)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vulkanState.bindlessSet;
    write.dstBinding = hgDescriptorType(descriptor);
    write.dstArrayElement = hgDescriptorIdx(descriptor);
    write.descriptorCount = 1;
    write.descriptorType = hgDescriptorTypeToVk(hgDescriptorType(descriptor));
    write.pBufferInfo = bufferInfo;
    write.pImageInfo = imageInfo;
    write.pTexelBufferView = texelInfo;

    vkUpdateDescriptorSets(hgVkDevice, 1, &write, 0, nullptr);
}

VkPipelineLayout hgCreatePipelineLayout(const VkPushConstantRange* push)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &vulkanState.bindlessLayout;
    info.pushConstantRangeCount = 1;
    info.pPushConstantRanges = push;

    VkPipelineLayout layout = nullptr;
    VkResult result = vkCreatePipelineLayout(hgVkDevice, &info, nullptr, &layout);
    if (layout == nullptr)
        hgError("Could not create bindless VkPipelineLayout: %s\n", hgVkResultToStr(result));

    return layout;
}

static VkShaderModule createShaderModule(const u8* spirvCode, u64 codeSize)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = (const u32*)spirvCode;

    VkShaderModule shader = nullptr;
    VkResult result = vkCreateShaderModule(hgVkDevice, &info, nullptr, &shader);
    if (shader == nullptr)
        hgError("Could not create VkShaderModule: %s\n", hgVkResultToStr(result));

    return shader;
}

VkPipeline hgCreateGraphicsPipeline(const HgCreateGraphicsPipeline* config)
{
    hgAssert(config->layout != nullptr);
    hgAssert(config->vertexShader != nullptr);
    hgAssert(config->fragmentShader != nullptr);
    if (config->colorAttachmentCount > 0)
        hgAssert(config->colorAttachmentFormats != nullptr);
    if (config->vertexBindingCount > 0)
        hgAssert(config->vertexBindings != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkShaderModule vertexShader = createShaderModule(config->vertexShader, config->vertexShaderSize);
    VkShaderModule fragmentShader = createShaderModule(config->fragmentShader, config->fragmentShaderSize);
    hgDefer(vkDestroyShaderModule(hgVkDevice, vertexShader, nullptr));
    hgDefer(vkDestroyShaderModule(hgVkDevice, fragmentShader, nullptr));

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = (u32)config->vertexBindingCount;
    vertexInputState.pVertexBindingDescriptions = config->vertexBindings;
    vertexInputState.vertexAttributeDescriptionCount = (u32)config->vertexAttributeCount;
    vertexInputState.pVertexAttributeDescriptions = config->vertexAttributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = config->topology;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellationState{};
    tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationState.patchControlPoints = config->tesselationPatchControlPoints;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = config->polygonMode;
    rasterizationState.cullMode = config->cullMode;
    rasterizationState.frontFace = config->frontFace;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = config->multisampleCount;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = config->enableDepthRead;
    depthStencilState.depthWriteEnable = config->enableDepthWrite;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = config->enableDepthRead;
    depthStencilState.stencilTestEnable = VK_FALSE, // : TODO
    depthStencilState.front = {}; // : TODO
    depthStencilState.back = {}; // : TODO
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState* colorBlendAttachments
        = hgAlloc<VkPipelineColorBlendAttachmentState>(scratch, config->colorAttachmentCount);

    for (u32 i = 0; i < config->colorAttachmentCount; ++i)
    {
        colorBlendAttachments[i].blendEnable = config->colorBlendEnables != nullptr
            ? config->colorBlendEnables[i]
            : VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].colorWriteMask
            = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = config->colorAttachmentCount;
    colorBlendState.pAttachments = colorBlendAttachments;
    for (float& blendConstant : colorBlendState.blendConstants)
    {
        blendConstant = 1.0f;
    }

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(*dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = (u32)config->colorAttachmentCount;
    renderingInfo.pColorAttachmentFormats = config->colorAttachmentFormats;
    renderingInfo.depthAttachmentFormat = config->depthAttachmentFormat;
    renderingInfo.stencilAttachmentFormat = config->stencilAttachmentFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(*shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pTessellationState = &tessellationState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = config->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateGraphicsPipelines(hgVkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan graphics pipeline: %s\n", hgVkResultToStr(result));

    return pipeline;
}

void hgBindGraphicsPipeline(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &vulkanState.bindlessSet, 0, nullptr);
}

VkPipeline hgCreateComputePipeline(VkPipelineLayout layout, const u8* shaderCode, u64 shaderCodeSize)
{
    hgAssert(layout != nullptr);
    hgAssert(shaderCode != nullptr);
    hgAssert(shaderCodeSize > 0);

    VkShaderModule computeShader = createShaderModule(shaderCode, shaderCodeSize);
    hgDefer(vkDestroyShaderModule(hgVkDevice, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateComputePipelines(hgVkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan compute pipeline: %s\n", hgVkResultToStr(result));

    return pipeline;
}

void hgBindComputePipeline(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &vulkanState.bindlessSet, 0, nullptr);
}

VkCommandBuffer hgBeginVkCmd()
{
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = hgVkCmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hgVkDevice, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void hgEndVkCmd(VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(hgVkDevice, &fenceInfo, nullptr, &fence);
    hgDefer(vkDestroyFence(hgVkDevice, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(hgVkQueue, 1, &submit, fence);
    vkWaitForFences(hgVkDevice, 1, &fence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, 1, &cmd);
}

HgRenderer HgRenderer::create(HgArena* arena, u32 maxBuffers, u32 maxImages)
{
    HgRenderer renderer{};
    renderer.buffers = renderer.buffers.create(arena, maxBuffers);
    renderer.images = renderer.images.create(arena, maxImages);
    return renderer;
}

void HgRenderer::reset()
{
    buffers.empty();
    images.empty();
}

void HgRenderer::setBuffer(HgBuffer* buffer, VkPipelineStageFlags lastStage, VkAccessFlags lastAccess)
{
    buffers.add(buffer, {lastStage, lastAccess});
}

void HgRenderer::setImage(
    HgImageView* image,
    VkPipelineStageFlags lastStage,
    VkAccessFlags lastAccess,
    VkImageLayout lastLayout)
{
    images.add(image, {lastStage, lastAccess, lastLayout});
}

void HgRenderer::barrier(
    VkCommandBuffer cmd,
    const HgBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const HgImageBarrier* imageBarriers,
    u32 imageBarrierCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkBufferMemoryBarrier2* vkBufferBarriers = hgAlloc<VkBufferMemoryBarrier2>(scratch, imageBarrierCount);
    VkImageMemoryBarrier2* vkImageBarriers = hgAlloc<VkImageMemoryBarrier2>(scratch, imageBarrierCount);

    for (u32 i = 0; i < bufferBarrierCount; ++i)
    {
        BufferState* buffer = buffers.get(bufferBarriers[i].buffer);
        if (buffer == nullptr)
            buffer = buffers.add(bufferBarriers[i].buffer);

        vkBufferBarriers[i] = {};
        vkBufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBufferBarriers[i].srcStageMask = buffer->lastStage;
        vkBufferBarriers[i].srcAccessMask = buffer->lastAccess;
        vkBufferBarriers[i].dstStageMask = bufferBarriers[i].nextStage;
        vkBufferBarriers[i].dstAccessMask = bufferBarriers[i].nextAccess;
        vkBufferBarriers[i].buffer = bufferBarriers[i].buffer->buffer;
        vkBufferBarriers[i].size = bufferBarriers[i].buffer->size;

        buffer->lastStage = bufferBarriers[i].nextStage;
        buffer->lastAccess = bufferBarriers[i].nextAccess;
    }

    for (u32 i = 0; i < imageBarrierCount; ++i)
    {
        ImageState* image = images.get(imageBarriers[i].image);
        if (image == nullptr)
            image = images.add(imageBarriers[i].image);

        vkImageBarriers[i] = {};
        vkImageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkImageBarriers[i].srcStageMask = image->lastStage;
        vkImageBarriers[i].srcAccessMask = image->lastAccess;
        vkImageBarriers[i].dstStageMask = imageBarriers[i].nextStage;
        vkImageBarriers[i].dstAccessMask = imageBarriers[i].nextAccess;
        vkImageBarriers[i].oldLayout = image->lastLayout;
        vkImageBarriers[i].newLayout = imageBarriers[i].nextLayout;
        vkImageBarriers[i].image = imageBarriers[i].image->image->image;
        vkImageBarriers[i].subresourceRange = {
            imageBarriers[i].image->aspectFlags,
            imageBarriers[i].image->baseMipLevel,
            imageBarriers[i].image->levelCount,
            imageBarriers[i].image->baseArrayLayer,
            imageBarriers[i].image->layerCount,
        };

        image->lastStage = imageBarriers[i].nextStage;
        image->lastAccess = imageBarriers[i].nextAccess;
        image->lastLayout = imageBarriers[i].nextLayout;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = vkBufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = vkImageBarriers;

    vkCmdPipelineBarrier2(cmd, &dep);
}

void HgRenderer::prepareResources(VkCommandBuffer cmd, const HgRenderPass* pass)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkBufferMemoryBarrier2* bufferBarriers = nullptr;
    u32 bufferBarrierCount = 0;
    VkImageMemoryBarrier2* imageBarriers = nullptr;
    u32 imageBarrierCount = 0;

    bufferBarriers = hgRealloc<VkBufferMemoryBarrier2>(scratch, 
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->uniformBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->uniformBufferCount; ++i)
    {
        BufferState* buffer = buffers.get(pass->uniformBuffers[i - bufferBarrierCount]);
        if (buffer == nullptr)
            buffer = buffers.add(pass->uniformBuffers[i - bufferBarrierCount]);

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = buffer->lastStage;
        bufferBarriers[i].srcAccessMask = buffer->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                       | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        bufferBarriers[i].buffer = pass->uniformBuffers[i - bufferBarrierCount]->buffer;
        bufferBarriers[i].size = pass->uniformBuffers[i - bufferBarrierCount]->size;

        buffer->lastStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                 | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        buffer->lastAccess = VK_ACCESS_UNIFORM_READ_BIT;
    }

    bufferBarrierCount += pass->uniformBufferCount;

    bufferBarriers = hgRealloc<VkBufferMemoryBarrier2>(scratch, 
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->storageBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->storageBufferCount; ++i)
    {
        BufferState* buffer = buffers.get(pass->storageBuffers[i - bufferBarrierCount]);
        if (buffer == nullptr)
            buffer = buffers.add(pass->storageBuffers[i - bufferBarrierCount]);

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = buffer->lastStage;
        bufferBarriers[i].srcAccessMask = buffer->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                       | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                        | VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarriers[i].buffer = pass->uniformBuffers[i - bufferBarrierCount]->buffer;
        bufferBarriers[i].size = pass->uniformBuffers[i - bufferBarrierCount]->size;

        buffer->lastStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                  | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        buffer->lastAccess = VK_ACCESS_SHADER_READ_BIT
                           | VK_ACCESS_SHADER_WRITE_BIT;
    }

    bufferBarrierCount += pass->storageBufferCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->sampledImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->sampledImageCount; ++i)
    {
        ImageState* image = images.get(pass->sampledImages[i - imageBarrierCount]);
        if (image == nullptr)
            image = images.add(pass->sampledImages[i - imageBarrierCount]);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarriers[i].image = pass->sampledImages[i - imageBarrierCount]->image->image;
        imageBarriers[i].subresourceRange = {
            pass->sampledImages[i - imageBarrierCount]->aspectFlags,
            pass->sampledImages[i - imageBarrierCount]->baseMipLevel,
            pass->sampledImages[i - imageBarrierCount]->levelCount,
            pass->sampledImages[i - imageBarrierCount]->baseArrayLayer,
            pass->sampledImages[i - imageBarrierCount]->layerCount,
        };

        image->lastStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        image->lastAccess = VK_ACCESS_SHADER_READ_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    imageBarrierCount += pass->sampledImageCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->storageImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->storageImageCount; ++i)
    {
        ImageState* image = images.get(pass->storageImages[i - imageBarrierCount]);
        if (image == nullptr)
            image = images.add(pass->storageImages[i - imageBarrierCount]);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                       | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarriers[i].image = pass->storageImages[i - imageBarrierCount]->image->image;
        imageBarriers[i].subresourceRange = {
            pass->storageImages[i - imageBarrierCount]->aspectFlags,
            pass->storageImages[i - imageBarrierCount]->baseMipLevel,
            pass->storageImages[i - imageBarrierCount]->levelCount,
            pass->storageImages[i - imageBarrierCount]->baseArrayLayer,
            pass->storageImages[i - imageBarrierCount]->layerCount,
        };

        image->lastStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        image->lastAccess = VK_ACCESS_SHADER_READ_BIT
                          | VK_ACCESS_SHADER_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    imageBarrierCount += pass->storageImageCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->colorAttachmentCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->colorAttachmentCount; ++i)
    {
        ImageState* image = images.get(pass->colorAttachments[i - imageBarrierCount].image);
        if (image == nullptr)
            image = images.add(pass->colorAttachments[i - imageBarrierCount].image);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageBarriers[i].image = pass->colorAttachments[i - imageBarrierCount].image->image->image;
        imageBarriers[i].subresourceRange = {
            pass->colorAttachments[i - imageBarrierCount].image->aspectFlags,
            pass->colorAttachments[i - imageBarrierCount].image->baseMipLevel,
            pass->colorAttachments[i - imageBarrierCount].image->levelCount,
            pass->colorAttachments[i - imageBarrierCount].image->baseArrayLayer,
            pass->colorAttachments[i - imageBarrierCount].image->layerCount,
        };

        image->lastStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        image->lastAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    imageBarrierCount += pass->colorAttachmentCount;

    if (pass->depthAttachment != nullptr)
    {
        ImageState* image = images.get(pass->depthAttachment->image);
        if (image == nullptr)
            image = images.add(pass->depthAttachment->image);

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = image->lastStage;
        imageBarriers[idx].srcAccessMask = image->lastAccess;
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageBarriers[idx].oldLayout = image->lastLayout;
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = pass->depthAttachment->image->image->image;
        imageBarriers[idx].subresourceRange = {
            pass->depthAttachment->image->aspectFlags,
            pass->depthAttachment->image->baseMipLevel,
            pass->depthAttachment->image->levelCount,
            pass->depthAttachment->image->baseArrayLayer,
            pass->depthAttachment->image->layerCount,
        };

        image->lastStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        image->lastAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        ++imageBarrierCount;
    }

    if (pass->stencilAttachment != nullptr)
    {
        ImageState* image = images.get(pass->stencilAttachment->image);
        if (image == nullptr)
            image = images.add(pass->stencilAttachment->image);

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = image->lastStage;
        imageBarriers[idx].srcAccessMask = image->lastAccess;
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageBarriers[idx].oldLayout = image->lastLayout;
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = pass->stencilAttachment->image->image->image;
        imageBarriers[idx].subresourceRange = {
            pass->stencilAttachment->image->aspectFlags,
            pass->stencilAttachment->image->baseMipLevel,
            pass->stencilAttachment->image->levelCount,
            pass->stencilAttachment->image->baseArrayLayer,
            pass->stencilAttachment->image->layerCount,
        };

        image->lastStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        image->lastAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

        ++imageBarrierCount;
    }
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = bufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = imageBarriers;

    vkCmdPipelineBarrier2(cmd, &dep);
}

void HgRenderer::beginPass(VkCommandBuffer cmd, u32 width, u32 height, const HgRenderPass* pass)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    prepareResources(cmd, pass);

    VkRenderingAttachmentInfo* colorAttachments
        = hgAlloc<VkRenderingAttachmentInfo>(scratch, pass->colorAttachmentCount);

    for (u32 i = 0; i < pass->colorAttachmentCount; ++i)
    {
        colorAttachments[i] = {};
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = pass->colorAttachments[i].image->view;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = pass->colorAttachments[i].loadOp;
        colorAttachments[i].storeOp = pass->colorAttachments[i].storeOp;
        colorAttachments[i].clearValue = pass->colorAttachments[i].clearValue;
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass->depthAttachment != nullptr)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = pass->depthAttachment->image->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = pass->depthAttachment->loadOp;
        depthAttachment.storeOp = pass->depthAttachment->storeOp;
        depthAttachment.clearValue = pass->depthAttachment->clearValue;
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass->stencilAttachment != nullptr)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = pass->stencilAttachment->image->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = pass->stencilAttachment->loadOp;
        stencilAttachment.storeOp = pass->stencilAttachment->storeOp;
        stencilAttachment.clearValue = pass->stencilAttachment->clearValue;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass->layerCount;
    renderingInfo.colorAttachmentCount = pass->colorAttachmentCount;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = pass->depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass->stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{0.0f, 0.0f, (f32)width, (f32)height, 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, {width, height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void HgRenderer::endPass(VkCommandBuffer cmd)
{
    vkCmdEndRendering(cmd);
}

void hgInternalResizeWindowSwapchain(HgWindow* window)
{
    if (window->width == 0 || window->height == 0)
        return;

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    vkQueueWaitIdle(hgVkQueue);

    if (window->cmds != nullptr)
        vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i].view, nullptr);
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hgVkPhysicalDevice, window->surface, &surfaceCapabilities);

    if (surfaceCapabilities.currentExtent.width != (u32)-1)
        window->width = surfaceCapabilities.currentExtent.width;
    if (surfaceCapabilities.currentExtent.height != (u32)-1)
        window->height = surfaceCapabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = window->surface;
    swapchainInfo.minImageCount
        = std::min(surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount - 1) + 1;
    swapchainInfo.imageFormat = window->format;
    swapchainInfo.imageExtent = {window->width, window->height};
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = window->imageUsage;
    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = window->presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = window->swapchain;

    VkResult result = vkCreateSwapchainKHR(hgVkDevice, &swapchainInfo, nullptr, &window->swapchain);
    if (window->swapchain == nullptr)
        hgError("Failed to create swapchain: %s\n", hgVkResultToStr(result));

    u32 swapImageCount;
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &swapImageCount, nullptr);
    VkImage* swapImages = hgAlloc<VkImage>(scratch, swapImageCount);
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &swapImageCount, swapImages);

    if (window->imageCount != swapImageCount)
    {
        window->images = (HgImage*)realloc(window->images, sizeof(HgImage) * swapImageCount);
        window->views = (HgImageView*)realloc(window->views, sizeof(HgImageView) * swapImageCount);

        window->cmds = (VkCommandBuffer*)realloc(window->cmds, sizeof(VkCommandBuffer) * swapImageCount);
        window->frameFinished = (VkFence*)realloc(window->frameFinished, sizeof(VkFence) * swapImageCount);
        window->imageAvailable = (VkSemaphore*)realloc(window->imageAvailable, sizeof(VkSemaphore) * swapImageCount);
        window->readyToPresent = (VkSemaphore*)realloc(window->readyToPresent, sizeof(VkSemaphore) * swapImageCount);

        window->imageCount = swapImageCount;
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        window->images[i].image = swapImages[i];
        window->images[i].type = VK_IMAGE_TYPE_2D;
        window->images[i].format = window->format;
        window->images[i].width = window->width;
        window->images[i].height = window->height;
        window->images[i].depth = 1;
        window->images[i].mipLevels = 1;
        window->images[i].arrayLayers = 1;
        window->images[i].msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = window->format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkResult viewResult = vkCreateImageView(hgVkDevice, &viewInfo, nullptr, &window->views[i].view);
        if (window->views[i].view == nullptr)
            hgError("Could not create VkImageView: %s\n", hgVkResultToStr(viewResult));

        window->views[i].image = &window->images[i];
        window->views[i].type = VK_IMAGE_VIEW_TYPE_2D;
        window->views[i].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        window->views[i].baseMipLevel = 0;
        window->views[i].levelCount = 1;
        window->views[i].baseArrayLayer = 0;
        window->views[i].layerCount = 1;
    }

    if (window->imageCount > 0) {
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = hgVkCmdPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = window->imageCount;

        vkAllocateCommandBuffers(hgVkDevice, &cmdAllocInfo, window->cmds);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hgVkDevice, &info, nullptr, &window->frameFinished[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->imageAvailable[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->readyToPresent[i]);
    }

    vkDestroySwapchainKHR(hgVkDevice, swapchainInfo.oldSwapchain, nullptr);
}

static VkFormat findSwapchainFormat(VkSurfaceKHR surface)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = hgAlloc<VkSurfaceFormatKHR>(scratch, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hgError("No supported swapchain formats\n");
}

static VkPresentModeKHR findSwapchainPresentMode(
    VkSurfaceKHR surface,
    VkPresentModeKHR desiredMode)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    if (desiredMode == VK_PRESENT_MODE_FIFO_KHR)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = hgAlloc<VkPresentModeKHR>(scratch, modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == desiredMode)
            return desiredMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgCreateWindow* config)
{
    window->format = findSwapchainFormat(window->surface);
    window->presentMode = findSwapchainPresentMode(window->surface, config->preferredPresentMode);
    window->imageUsage = config->imageUsage;
    hgInternalResizeWindowSwapchain(window);
}

void hgInternalDestroyWindowSwapchain(HgWindow* window)
{
    vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);
    free(window->cmds);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    free(window->frameFinished);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    free(window->imageAvailable);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    free(window->readyToPresent);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i].view, nullptr);
    }
    free(window->views);
    free(window->images);

    vkDestroySwapchainKHR(hgVkDevice, window->swapchain, nullptr);
}

VkCommandBuffer HgWindow::beginRecording()
{
    hgAssert(hgVkDevice != nullptr);
    if (width == 0 || height == 0)
        return nullptr;

retry:
    currentFrame = (currentFrame + 1) % imageCount;
    vkWaitForFences(hgVkDevice, 1, &frameFinished[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(hgVkDevice, 1, &frameFinished[currentFrame]);

    VkResult result = vkAcquireNextImageKHR(
        hgVkDevice, swapchain, UINT64_MAX, imageAvailable[currentFrame], nullptr, &currentImage);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        hgInternalResizeWindowSwapchain(this);
        goto retry;
    }
    if (result != VK_SUCCESS)
        hgError("Could not acquire next image: %s", hgVkResultToStr(result));

    VkCommandBuffer cmd = cmds[currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void HgWindow::endAndPresent(VkCommandBuffer cmd)
{
    hgAssert(cmd == cmds[currentFrame]);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable[currentFrame];
    VkPipelineStageFlags stageFlags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit.pWaitDstStageMask = &stageFlags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &readyToPresent[currentFrame];

    vkQueueSubmit(hgVkQueue, 1, &submit, frameFinished[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &readyToPresent[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImage;

    vkQueuePresentKHR(hgVkQueue, &presentInfo);
}

struct PlatformState {
    bool imguiInitialized = false;

    u32 windowPoolCount = 0;

    u32* windowFreeList = nullptr;
    u32 windowNext = 0;

    u32 windowWidth = 0;
    HgWindow* windowPool = nullptr;
    HgHashMap<SDL_WindowID, HgWindow*> windows;

    bool wasQuit = false;
    bool isKeyDown[HgKey_count]{};
    f32 mouseDeltaX = 0.0f;
    f32 mouseDeltaY = 0.0f;
};

static PlatformState platform;

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgCreateWindow* config);
void hgInternalDestroyWindowSwapchain(HgWindow* window);
void hgInternalResizeWindowSwapchain(HgWindow* window);

void hgInitPlatform(HgArena* arena, u32 maxWindows, u32 maxEvents)
{
    SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS);

    platform.windowPoolCount = maxWindows;
    platform.windowFreeList = hgAlloc<u32>(arena, maxWindows);
    platform.windowWidth = (u32)hgAlign(sizeof(HgWindow) + sizeof(HgKeyEvent) * (maxEvents - 1), alignof(HgWindow));
    platform.windowPool = (HgWindow*)hgAlloc(arena, platform.windowWidth * maxWindows, alignof(HgWindow));

    for (u32 i = 0; i < maxWindows; ++i)
    {
        platform.windowFreeList[i] = i + 1;
    }
    platform.windowNext = 0;

    platform.windows = platform.windows.create(arena, maxWindows * 2);

    SDL_GetMouseState(&platform.mouseDeltaX, &platform.mouseDeltaY);
}

void hgDeinitPlatform()
{
    SDL_Quit();
}

u32 hgGetPlatformVulkanExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    *extBuffer = hgAlloc<HgStringView>(arena, extCount);
    for (u32 i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}

static HgKey sdlKeycodeToHgKey(u32 key)
{
    switch (key)
    {
        case SDLK_0:
            return HgKey_k0;
        case SDLK_1:
            return HgKey_k1;
        case SDLK_2:
            return HgKey_k2;
        case SDLK_3:
            return HgKey_k3;
        case SDLK_4:
            return HgKey_k4;
        case SDLK_5:
            return HgKey_k5;
        case SDLK_6:
            return HgKey_k6;
        case SDLK_7:
            return HgKey_k7;
        case SDLK_8:
            return HgKey_k8;
        case SDLK_9:
            return HgKey_k9;

        case SDLK_Q:
            return HgKey_q;
        case SDLK_W:
            return HgKey_w;
        case SDLK_E:
            return HgKey_e;
        case SDLK_R:
            return HgKey_r;
        case SDLK_T:
            return HgKey_t;
        case SDLK_Y:
            return HgKey_y;
        case SDLK_U:
            return HgKey_u;
        case SDLK_I:
            return HgKey_i;
        case SDLK_O:
            return HgKey_o;
        case SDLK_P:
            return HgKey_p;
        case SDLK_A:
            return HgKey_a;
        case SDLK_S:
            return HgKey_s;
        case SDLK_D:
            return HgKey_d;
        case SDLK_F:
            return HgKey_f;
        case SDLK_G:
            return HgKey_g;
        case SDLK_H:
            return HgKey_h;
        case SDLK_J:
            return HgKey_j;
        case SDLK_K:
            return HgKey_k;
        case SDLK_L:
            return HgKey_l;
        case SDLK_Z:
            return HgKey_z;
        case SDLK_X:
            return HgKey_x;
        case SDLK_C:
            return HgKey_c;
        case SDLK_V:
            return HgKey_v;
        case SDLK_B:
            return HgKey_b;
        case SDLK_N:
            return HgKey_n;
        case SDLK_M:
            return HgKey_m;

        case SDLK_SEMICOLON:
            return HgKey_semicolon;
        case SDLK_COLON:
            return HgKey_colon;
        case SDLK_APOSTROPHE:
            return HgKey_apostrophe;
        case SDLK_DBLAPOSTROPHE:
            return HgKey_quotation;
        case SDLK_COMMA:
            return HgKey_comma;
        case SDLK_PERIOD:
            return HgKey_period;
        case SDLK_QUESTION:
            return HgKey_question;
        case SDLK_GRAVE:
            return HgKey_grave;
        case SDLK_TILDE:
            return HgKey_tilde;
        case SDLK_EXCLAIM:
            return HgKey_exclamation;
        case SDLK_AT:
            return HgKey_at;
        case SDLK_HASH:
            return HgKey_hash;
        case SDLK_DOLLAR:
            return HgKey_dollar;
        case SDLK_PERCENT:
            return HgKey_percent;
        case SDLK_CARET:
            return HgKey_carot;
        case SDLK_AMPERSAND:
            return HgKey_ampersand;
        case SDLK_ASTERISK:
            return HgKey_asterisk;

        case SDLK_LEFTPAREN:
            return HgKey_lparen;
        case SDLK_RIGHTPAREN:
            return HgKey_rparen;
        case SDLK_LEFTBRACKET:
            return HgKey_lbracket;
        case SDLK_RIGHTBRACKET:
            return HgKey_rbracket;
        case SDLK_LEFTBRACE:
            return HgKey_lbrace;
        case SDLK_RIGHTBRACE:
            return HgKey_rbrace;

        case SDLK_EQUALS:
            return HgKey_equal;
        case SDLK_LESS:
            return HgKey_less;
        case SDLK_GREATER:
            return HgKey_greater;
        case SDLK_PLUS:
            return HgKey_plus;
        case SDLK_MINUS:
            return HgKey_minus;
        case SDLK_SLASH:
            return HgKey_slash;
        case SDLK_BACKSLASH:
            return HgKey_backslash;
        case SDLK_UNDERSCORE:
            return HgKey_underscore;
        case SDLK_PIPE:
            return HgKey_bar;

        case SDLK_UP:
            return HgKey_up;
        case SDLK_DOWN:
            return HgKey_down;
        case SDLK_LEFT:
            return HgKey_left;
        case SDLK_RIGHT:
            return HgKey_right;

        case SDLK_ESCAPE:
            return HgKey_escape;
        case SDLK_SPACE:
            return HgKey_space;
        case SDLK_RETURN:
            return HgKey_enter;
        case SDLK_BACKSPACE:
            return HgKey_backspace;
        case SDLK_DELETE:
            return HgKey_kdelete;
        case SDLK_INSERT:
            return HgKey_insert;
        case SDLK_TAB:
            return HgKey_tab;
        case SDLK_HOME:
            return HgKey_home;
        case SDLK_END:
            return HgKey_end;

        case SDLK_F1:
            return HgKey_f1;
        case SDLK_F2:
            return HgKey_f2;
        case SDLK_F3:
            return HgKey_f3;
        case SDLK_F4:
            return HgKey_f4;
        case SDLK_F5:
            return HgKey_f5;
        case SDLK_F6:
            return HgKey_f6;
        case SDLK_F7:
            return HgKey_f7;
        case SDLK_F8:
            return HgKey_f8;
        case SDLK_F9:
            return HgKey_f9;
        case SDLK_F10:
            return HgKey_f10;
        case SDLK_F11:
            return HgKey_f11;
        case SDLK_F12:
            return HgKey_f12;

        case SDLK_LSHIFT:
            return HgKey_lshift;
        case SDLK_RSHIFT:
            return HgKey_rshift;
        case SDLK_LCTRL:
            return HgKey_lctrl;
        case SDLK_RCTRL:
            return HgKey_rctrl;
        case SDLK_LALT:
            return HgKey_lalt;
        case SDLK_RALT:
            return HgKey_ralt;
        case SDLK_LGUI:
            return HgKey_lsuper;
        case SDLK_RGUI:
            return HgKey_rsuper;
        case SDLK_CAPSLOCK:
            return HgKey_capslock;
    }
    return HgKey_none;
}

static HgKey sdlButtonToHgKey(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return HgKey_mouse1;
        case SDL_BUTTON_RIGHT:
            return HgKey_mouse2;
        case SDL_BUTTON_MIDDLE:
            return HgKey_mouse3;
        case SDL_BUTTON_X1:
            return HgKey_mouse4;
        case SDL_BUTTON_X2:
            return HgKey_mouse5;
    }
    return HgKey_none;
}

HgWindow* hgCreateWindow(const HgCreateWindow* config)
{
    u32 windowIdx = platform.windowNext;
    platform.windowNext = platform.windowFreeList[windowIdx];
    platform.windowFreeList[windowIdx] = windowIdx;

    HgWindow* window = &platform.windowPool[windowIdx];
    *window = {};

    const char* title = config->title != nullptr ? config->title : "Hurdy Gurdy";

    if (config->fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);

        window->internals = SDL_CreateWindow(
            title,
            modes[0]->w,
            modes[0]->h,
            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);

        SDL_free(modes);
    } else {
        window->internals = SDL_CreateWindow(
            title,
            config->width,
            config->height,
            SDL_WINDOW_VULKAN | (config->fixedSize ? 0 : SDL_WINDOW_RESIZABLE));
    }

    bool success = SDL_Vulkan_CreateSurface((SDL_Window*)window->internals, hgVkInstance, nullptr, &window->surface);
    if (!success || window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", SDL_GetError());

    SDL_WindowID windowID = SDL_GetWindowID((SDL_Window*)window->internals);
    platform.windows.add(windowID, window);

    SDL_GetWindowSize((SDL_Window*)window->internals, (int*)&window->width, (int*)&window->height);
    hgInternalCreateWindowSwapchain(window, config);

    return window;
}

void hgDestroyWindow(HgWindow* window)
{
    u32 windowIdx = (u32)(window - platform.windowPool);
    hgAssert(windowIdx < platform.windowPoolCount);
    hgAssert(platform.windowFreeList[windowIdx] == windowIdx);

    hgInternalDestroyWindowSwapchain(window);

    vkDestroySurfaceKHR(hgVkInstance, window->surface, nullptr);
    SDL_DestroyWindow((SDL_Window*)window->internals);

    platform.windowFreeList[windowIdx] = platform.windowNext;
    platform.windowNext = windowIdx;
}

void hgProcessEvents()
{
    f32 oldMouseX, oldMouseY;
    SDL_GetMouseState(&oldMouseX, &oldMouseY);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (platform.imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            {
                platform.wasQuit = true;
            } break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                HgWindow* window = *platform.windows.get(event.window.windowID);

                SDL_GetWindowSize((SDL_Window*)window->internals, (int*)&window->width, (int*)&window->height);
                hgInternalResizeWindowSwapchain(window);
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);

                HgWindow* window = *platform.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;

                platform.isKeyDown[windowEvent.key] = true;
            } break;
            case SDL_EVENT_KEY_UP:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);

                HgWindow* window = *platform.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;

                platform.isKeyDown[windowEvent.key] = false;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlButtonToHgKey(event.button.button);

                HgWindow* window = *platform.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;

                platform.isKeyDown[windowEvent.key] = true;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlButtonToHgKey(event.button.button);

                HgWindow* window = *platform.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;

                platform.isKeyDown[windowEvent.key] = false;
            } break;
        }
    }

    f32 newMouseX, newMouseY;
    SDL_GetMouseState(&newMouseX, &newMouseY);
    platform.mouseDeltaX = newMouseX - oldMouseX;
    platform.mouseDeltaY = newMouseY - oldMouseY;
}

bool hgWasQuit()
{
    return platform.wasQuit;
}

HgKeyEvent* hgGetKeyEvents(HgWindow* window, u32* count)
{
    hgAssert(window != nullptr);
    hgAssert(count != nullptr);

    *count = window->eventCount;
    return window->events;
}

bool hgIsKeyDown(HgKey key)
{
    return platform.isKeyDown[key];
}

void hgGetMousePos(f32* x, f32* y)
{
    SDL_GetMouseState(x, y);
}

void hgGetMouseDelta(f32* x, f32* y)
{
    *x = platform.mouseDeltaX;
    *y = platform.mouseDeltaY;
}

bool hgIsMouseFocused(HgWindow* window)
{
    return SDL_GetMouseFocus() == (SDL_Window*)window->internals;
}

void hgGetWindowSize(HgWindow* window, u32* x, u32* y)
{
    *x = window->width;
    *y = window->height;
}

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat,
    VkFormat stencilFormat)
{
    ImGui_ImplSDL3_InitForVulkan((SDL_Window*)window->internals);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = hgVkInstance;
    imguiInfo.PhysicalDevice = hgVkPhysicalDevice;
    imguiInfo.Device = hgVkDevice;
    imguiInfo.QueueFamily = hgVkQueueFamily;
    imguiInfo.Queue = hgVkQueue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = window->imageCount;
    imguiInfo.ImageCount = window->imageCount;
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilFormat;
#ifdef HG_VK_DEBUG_MESSENGER
    imguiInfo.CheckVkResultFn = [](VkResult err)
    {
        if (err != VK_SUCCESS)
            hgWarn("Vulkan error from ImGui: %s\n", hgVkResultToStr(err));
    };
#endif

    ImGui_ImplVulkan_Init(&imguiInfo);

    platform.imguiInitialized = true;
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    platform.imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

HgResourceManager HgResourceManager::create(HgArena* arena, u32 width, u32 align, u32 capacity)
{
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

bool HgImageData::getInfo(VkFormat* format, u32* width, u32* height, u32* depth)
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
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
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

        VkFormat format;
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

bool HgModelData::getInfo(u32* vertexCount, u32* vertexWidth, u32* indexCount, VkPrimitiveTopology* topology)
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
    hgError("hgImportGltf : TODO\n");
}

void hgExportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgExportGltf : TODO\n");
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
        hgDestroyImageView(tex.view);
        hgDestroyImage(tex.image);
    });
}

void hgLoadEmptyTexture(HgResource id)
{
    gpuTextures.load(id);
}

void hgLoadTexture(HgResource id, VkSampler sampler)
{
    if (gpuTextures.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgImageData data = *hgGetResource(id);

        VkFormat format;
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

        HgCreateImageEx imageInfo{};
        imageInfo.format = format;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.depth = depth;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        tex.image = hgCreateImageEx(&imageInfo);

        hgWriteImage(
            tex.image,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            data.getPixels(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tex.view = hgCreateImageView(tex.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        hgAssert(sampler != nullptr);
        tex.sampler = sampler;

        tex.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

        VkDescriptorImageInfo descInfo = {tex.sampler, tex.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        hgUpdateDescriptor(tex.descriptor, nullptr, &descInfo);
    }
}

void hgUnloadTexture(HgResource id)
{
    HgTextureResource tex;
    if (gpuTextures.unload(id, &tex))
    {
        hgDestroyDescriptor(tex.descriptor);
        hgDestroyImageView(tex.view);
        hgDestroyImage(tex.image);
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
        hgDestroyBuffer(model.vertexBuffer);
        hgDestroyBuffer(model.indexBuffer);
    });
}

void hgLoadModel(HgResource id)
{
    if (gpuModels.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgModelData data = *hgGetResource(id);

        VkPrimitiveTopology topology;
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

        model.vertexBuffer = hgCreateBuffer(
            model.vertexCount * model.vertexWidth,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        model.indexBuffer = hgCreateBuffer(
            model.indexCount * sizeof(u32),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hgWriteBuffer(
            model.vertexBuffer,
            0,
            data.getVertexData(),
            model.vertexCount * model.vertexWidth);

        hgWriteBuffer(
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
        hgDestroyBuffer(model.vertexBuffer);
        hgDestroyBuffer(model.indexBuffer);
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
    bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs)
)
{
    HgECS::System* system = systems.get(componentID);
    hgAssert(system != nullptr);

    QuicksortData q{this, system, componentID, data, compare};
    q.quicksort(0, system->count);
}

void hgAddChildEntity(HgECS* ecs, HgEntity parent, HgEntity child)
{
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
    VkPipelineLayout layout;
    VkPipeline pipeline;
    HgBuffer* vpBuffer;
    HgDescriptor vpDesc;
    HgTextureResource defaultTex;
};

static Pipeline2DState pipeline2D{};

void hgInitPipeline2D(
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);

    HgFence fence;
    const char* spriteVertSpvName = "build/sprite.vert.spv";
    const char* spriteFragSpvName = "build/sprite.frag.spv";
    HgResource spriteVertSpvID = hgResourceID(spriteVertSpvName);
    HgResource spriteFragSpvID = hgResourceID(spriteFragSpvName);
    hgLoadResource(&fence, 1, spriteVertSpvID, spriteVertSpvName);
    hgLoadResource(&fence, 1, spriteFragSpvID, spriteFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, spriteVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, spriteFragSpvID));

    HgBinary* spriteVertSpv = hgGetResource(spriteVertSpvID);
    HgBinary* spriteFragSpv = hgGetResource(spriteFragSpvID);

    VkPushConstantRange push{VK_SHADER_STAGE_ALL, 0, sizeof(Pipeline2DPush)};
    pipeline2D.layout = hgCreatePipelineLayout(&push);

    HgCreateGraphicsPipeline pipelineConfig{};
    pipelineConfig.layout = pipeline2D.layout;
    pipelineConfig.vertexShader = (u8*)spriteVertSpv->data;
    pipelineConfig.vertexShaderSize = spriteVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)spriteFragSpv->data;
    pipelineConfig.fragmentShaderSize = spriteFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.enableDepthRead = depthFormat != VK_FORMAT_UNDEFINED;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    pipeline2D.pipeline = hgCreateGraphicsPipeline(&pipelineConfig);

    pipeline2D.vpBuffer = hgCreateBuffer(
        sizeof(Pipeline2DVPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    Pipeline2DVPUniform vpData{};
    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    hgWriteBuffer(pipeline2D.vpBuffer, 0, &vpData, sizeof(vpData));

    pipeline2D.vpDesc = hgCreateDescriptor(HgDescriptorType_uniformBuffer);

    VkDescriptorBufferInfo bufferInfo{pipeline2D.vpBuffer->buffer, 0, sizeof(Pipeline2DVPUniform)};
    hgUpdateDescriptor(pipeline2D.vpDesc, &bufferInfo, nullptr);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    pipeline2D.defaultTex.image = hgCreateImage(2, 2, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    pipeline2D.defaultTex.view = hgCreateImageView(pipeline2D.defaultTex.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    hgWriteImage(
        pipeline2D.defaultTex.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultColors,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    pipeline2D.defaultTex.sampler = hgCreateSampler(VK_FILTER_NEAREST);

    pipeline2D.defaultTex.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = pipeline2D.defaultTex.sampler;
    imageInfo.imageView = pipeline2D.defaultTex.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateDescriptor(pipeline2D.defaultTex.descriptor, nullptr, &imageInfo);
}

void hgDeinitPipeline2D()
{
    vkDestroyPipeline(hgVkDevice, pipeline2D.pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipeline2D.layout, nullptr);

    hgDestroyDescriptor(pipeline2D.defaultTex.descriptor);
    vkDestroySampler(hgVkDevice, pipeline2D.defaultTex.sampler, nullptr);
    hgDestroyImageView(pipeline2D.defaultTex.view);
    hgDestroyImage(pipeline2D.defaultTex.image);

    hgDestroyDescriptor(pipeline2D.vpDesc);
    hgDestroyBuffer(pipeline2D.vpBuffer);
}

void hgUpdateProjection2D(const HgMat4* projection)
{
    hgWriteBuffer(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, proj), projection, sizeof(*projection));
}

void hgUpdateView2D(const HgMat4* view)
{
    hgWriteBuffer(pipeline2D.vpBuffer, offsetof(Pipeline2DVPUniform, view), view, sizeof(*view));
}

void hgDraw2D(HgECS* ecs, VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);

    ecs->sort<HgSprite2D>(nullptr, [](void*, HgECS* ecs, HgEntity lhs, HgEntity rhs) -> bool {
        hgAssert(ecs->has<HgTransform>(lhs));
        hgAssert(ecs->has<HgTransform>(rhs));
        return ecs->get<HgTransform>(lhs).position.z > ecs->get<HgTransform>(rhs).position.z;
    });

    hgBindGraphicsPipeline(cmd, pipeline2D.pipeline, pipeline2D.layout);

    ecs->forEach<HgSprite2D, HgTransform>([&](HgEntity, HgSprite2D* sprite, HgTransform* transform) 
    {
        HgTextureResource* texture = hgGetTexture(sprite->texture);
        if (texture == nullptr)
            texture = &pipeline2D.defaultTex;

        Pipeline2DPush push{};
        push.model = hgModelMatrix3D(transform->position, transform->scale, transform->rotation);
        push.uvPos = sprite->uvPos;
        push.uvSize = sprite->uvSize;
        push.vpIdx = hgDescriptorIdx(pipeline2D.vpDesc);
        push.texIdx = hgDescriptorIdx(texture->descriptor);

        vkCmdPushConstants(cmd, pipeline2D.layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

        vkCmdDraw(cmd, 6, 1, 0, 0);
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
    HgBuffer* vpBuffer;
    Pipeline3DVPUniform vpData;
    HgDescriptor vpDesc;

    HgBuffer* dirLightBuffer;
    u32 dirLightCapacity;
    HgDescriptor dirLightDesc;

    HgBuffer* pointLightBuffer;
    u32 pointLightCapacity;
    HgDescriptor pointLightDesc;

    HgModelResource defaultModel;
    VkSampler defaultMapSampler;
    HgTextureResource defaultColorMap;
    HgTextureResource defaultNormalMap;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

static Pipeline3DState pipeline3D{};

void hgInitPipeline3D(
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);
    hgAssert(depthFormat != VK_FORMAT_UNDEFINED);

    HgFence fence;
    const char* modelVertSpvName = "build/model.vert.spv";
    const char* modelFragSpvName = "build/model.frag.spv";
    HgResource modelVertSpvID = hgResourceID(modelVertSpvName);
    HgResource modelFragSpvID = hgResourceID(modelFragSpvName);
    hgLoadResource(&fence, 1, modelVertSpvID, modelVertSpvName);
    hgLoadResource(&fence, 1, modelFragSpvID, modelFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, modelVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, modelFragSpvID));

    HgBinary* modelVertSpv = hgGetResource(modelVertSpvID);
    HgBinary* modelFragSpv = hgGetResource(modelFragSpvID);

    pipeline3D.vpData.proj = HgMat4{1.0f};
    pipeline3D.vpData.view = HgMat4{1.0f};

    pipeline3D.vpBuffer = hgCreateBuffer(
        sizeof(Pipeline3DVPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    hgWriteBuffer(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    pipeline3D.vpDesc = hgCreateDescriptor(HgDescriptorType_uniformBuffer);

    VkDescriptorBufferInfo bufferInfo{pipeline3D.vpBuffer->buffer, 0, sizeof(Pipeline3DVPUniform)};
    hgUpdateDescriptor(pipeline3D.vpDesc, &bufferInfo, nullptr);

    pipeline3D.dirLightCapacity = 4;
    pipeline3D.dirLightBuffer = hgCreateBuffer(
        sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    pipeline3D.pointLightCapacity = 4;
    pipeline3D.pointLightBuffer = hgCreateBuffer(
        sizeof(Pipeline3DPointLightData) * pipeline3D.dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    pipeline3D.dirLightDesc = hgCreateDescriptor(HgDescriptorType_storageBuffer);
    pipeline3D.pointLightDesc = hgCreateDescriptor(HgDescriptorType_storageBuffer);

    VkDescriptorBufferInfo dirLightBufferInfo{pipeline3D.dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateDescriptor(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);

    VkDescriptorBufferInfo pointLightBufferInfo{pipeline3D.pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateDescriptor(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);

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

    pipeline3D.defaultModel.vertexBuffer = hgCreateBuffer(
        sizeof(cubeVertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    pipeline3D.defaultModel.indexBuffer = hgCreateBuffer(
        sizeof(cubeIndices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    hgWriteBuffer(pipeline3D.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgWriteBuffer(pipeline3D.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    pipeline3D.defaultModel.vertexCount = sizeof(cubeVertices) / sizeof(*cubeVertices);
    pipeline3D.defaultModel.vertexWidth = sizeof(HgModelVertex);
    pipeline3D.defaultModel.indexCount = sizeof(cubeIndices) / sizeof(*cubeIndices);

    pipeline3D.defaultMapSampler = hgCreateSampler(VK_FILTER_NEAREST);

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

    pipeline3D.defaultColorMap.image = hgCreateImage(3, 3, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    pipeline3D.defaultNormalMap.image = hgCreateImage(2, 2, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    pipeline3D.defaultColorMap.view = hgCreateImageView(
        pipeline3D.defaultColorMap.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    pipeline3D.defaultNormalMap.view = hgCreateImageView(
        pipeline3D.defaultNormalMap.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    hgWriteImage(
        pipeline3D.defaultColorMap.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultColors,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    hgWriteImage(
        pipeline3D.defaultNormalMap.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultNormals,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    pipeline3D.defaultColorMap.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);
    pipeline3D.defaultNormalMap.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo colorInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultColorMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    hgUpdateDescriptor(pipeline3D.defaultColorMap.descriptor, nullptr, &colorInfo);

    VkDescriptorImageInfo normalInfo =
        {pipeline3D.defaultMapSampler, pipeline3D.defaultNormalMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    hgUpdateDescriptor(pipeline3D.defaultNormalMap.descriptor, nullptr, &normalInfo);

    VkPushConstantRange push{VK_SHADER_STAGE_ALL, 0, sizeof(Pipeline3DPush)};
    pipeline3D.pipelineLayout = hgCreatePipelineLayout(&push);

    VkVertexInputBindingDescription vertexBindings[]{
        {0, sizeof(HgModelVertex), VK_VERTEX_INPUT_RATE_VERTEX},
    };
    VkVertexInputAttributeDescription vertexAttributes[]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, norm)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(HgModelVertex, tan)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(HgModelVertex, uv)},
    };
    HgCreateGraphicsPipeline pipelineConfig{};
    pipelineConfig.layout = pipeline3D.pipelineLayout;
    pipelineConfig.vertexShader = (u8*)modelVertSpv->data;
    pipelineConfig.vertexShaderSize = modelVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)modelFragSpv->data;
    pipelineConfig.fragmentShaderSize = modelFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.vertexBindings = vertexBindings;
    pipelineConfig.vertexBindingCount = sizeof(vertexBindings) / sizeof(*vertexBindings);
    pipelineConfig.vertexAttributes = vertexAttributes;
    pipelineConfig.vertexAttributeCount = sizeof(vertexAttributes) / sizeof(*vertexAttributes);
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;

    hgWaitForFenceIndefinite(&fence);
    pipeline3D.pipeline = hgCreateGraphicsPipeline(&pipelineConfig);
}

void hgDeinitPipeline3D()
{
    vkDestroyPipeline(hgVkDevice, pipeline3D.pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipeline3D.pipelineLayout, nullptr);

    hgDestroyDescriptor(pipeline3D.defaultNormalMap.descriptor);
    hgDestroyImageView(pipeline3D.defaultNormalMap.view);
    hgDestroyImage(pipeline3D.defaultNormalMap.image);

    hgDestroyDescriptor(pipeline3D.defaultColorMap.descriptor);
    hgDestroyImageView(pipeline3D.defaultColorMap.view);
    hgDestroyImage(pipeline3D.defaultColorMap.image);

    vkDestroySampler(hgVkDevice, pipeline3D.defaultMapSampler, nullptr);

    hgDestroyBuffer(pipeline3D.defaultModel.indexBuffer);
    hgDestroyBuffer(pipeline3D.defaultModel.vertexBuffer);

    hgDestroyDescriptor(pipeline3D.pointLightDesc);
    hgDestroyDescriptor(pipeline3D.dirLightDesc);
    hgDestroyBuffer(pipeline3D.pointLightBuffer);
    hgDestroyBuffer(pipeline3D.dirLightBuffer);

    hgDestroyDescriptor(pipeline3D.vpDesc);
    hgDestroyBuffer(pipeline3D.vpBuffer);
}

void hgUpdateProjection3D(const HgMat4* projection)
{
    pipeline3D.vpData.proj = *projection;
}

void hgUpdateView3D(const HgMat4* view)
{
    pipeline3D.vpData.view = *view;
}

void hgDraw3D(HgECS* ecs, VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    hgWriteBuffer(pipeline3D.vpBuffer, 0, &pipeline3D.vpData, sizeof(pipeline3D.vpData));

    u32 dirLightCount = ecs->count<HgDirLight3D>();
    u32 pointLightCount = ecs->count<HgPointLight3D>();

    if (dirLightCount > pipeline3D.dirLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        pipeline3D.dirLightCapacity = pipeline3D.dirLightCapacity == 0 ? 1 : pipeline3D.dirLightCapacity * 2;
        while (pipeline3D.dirLightCapacity < dirLightCount)
        {
            pipeline3D.dirLightCapacity *= 2;
        }

        hgDestroyBuffer(pipeline3D.dirLightBuffer);
        pipeline3D.dirLightBuffer = hgCreateBuffer(
            sizeof(Pipeline3DDirLightData) * pipeline3D.dirLightCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);

        VkDescriptorBufferInfo dirLightBufferInfo{pipeline3D.dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateDescriptor(pipeline3D.dirLightDesc, &dirLightBufferInfo, nullptr);
    }

    if (pointLightCount > pipeline3D.pointLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        pipeline3D.pointLightCapacity = pipeline3D.pointLightCapacity == 0 ? 1 : pipeline3D.pointLightCapacity * 2;
        while (pipeline3D.pointLightCapacity < pointLightCount)
        {
            pipeline3D.pointLightCapacity *= 2;
        }

        hgDestroyBuffer(pipeline3D.pointLightBuffer);
        pipeline3D.pointLightBuffer = hgCreateBuffer(
            sizeof(Pipeline3DPointLightData) * pipeline3D.pointLightCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);

        VkDescriptorBufferInfo pointLightBufferInfo{pipeline3D.pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateDescriptor(pipeline3D.pointLightDesc, &pointLightBufferInfo, nullptr);
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
        hgWriteBuffer(pipeline3D.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        hgWriteBuffer(pipeline3D.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    hgBindGraphicsPipeline(cmd, pipeline3D.pipeline, pipeline3D.pipelineLayout);

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
        push.dirLightIdx = hgDescriptorIdx(pipeline3D.dirLightDesc);
        push.dirLightCount = dirLightCount;
        push.pointLightIdx = hgDescriptorIdx(pipeline3D.pointLightDesc);
        push.pointLightCount = pointLightCount;
        push.vpIdx = hgDescriptorIdx(pipeline3D.vpDesc);
        push.colorMapIdx = hgDescriptorIdx(colorMap->descriptor);
        push.normalMapIdx = hgDescriptorIdx(normalMap->descriptor);

        HgModelResource* gpuModel = hgGetModel(model->modelResource);
        if (gpuModel == nullptr)
            gpuModel = &pipeline3D.defaultModel;

        vkCmdBindIndexBuffer(cmd, gpuModel->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

        VkBuffer buffers[]{gpuModel->vertexBuffer->buffer};
        VkDeviceSize offsets[]{0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd, pipeline3D.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

        vkCmdDrawIndexed(cmd, gpuModel->indexCount, 1, 0, 0, 0);
    });
}

