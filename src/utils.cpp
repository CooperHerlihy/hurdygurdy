#include "hurdygurdy.hpp"

void hgInit(void)
{
    hgInitScratchMemory();
    HgArena* arena = hgGetScratch();

    hgInitThreadPool(arena, 4096, hgHardwareThreadCount() - 2); // main thread, io thread
    hgInitIOThread(arena, 4096);
    hgInitResources();
    hgInitGpuResources();

    hgInitPlatform();
    hgInitGraphics();
}

void hgExit(void)
{
    hgDeinitGraphics();
    hgDeinitPlatform();

    hgDeinitGpuResources();
    hgDeinitResource();
    hgDeinitIOThread();
    hgDeinitThreadPool();

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

HgMat4 hgOrthographicProjection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return HgMat4{
        HgVec4{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        HgVec4{-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hgPerspectiveProjection(f32 fov, f32 aspect, f32 near, f32 far)
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

void hgInitScratchMemory()
{
    for (u32 i = 0; i < arenaCount; ++i)
    {
        if (arenas[i].memory == nullptr)
        {
            u64 arenaSize = (u32)-1;
            arenas[i] = {malloc(arenaSize), arenaSize};
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

HgString HgString::create(HgArena* arena, u64 capacity)
{
    HgString str;
    str.chars = hgAlloc<char>(arena, capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString HgString::copy(HgArena* arena, HgStringView str)
{
    HgString copy;
    copy.chars = hgAlloc<char>(arena, str.length);
    copy.capacity = str.length;
    copy.length = str.length;
    memcpy(copy.chars, str.chars, str.length);
    return copy;
}

void HgString::reserve(HgArena* arena, u64 newCapacity)
{
    chars = hgRealloc(arena, chars, capacity, newCapacity);
    capacity = newCapacity;
}

void HgString::grow(HgArena* arena, f32 factor)
{
    hgAssert(factor > 1.0f);
    hgAssert(capacity <= (u64)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (u64)((f32)capacity * factor));
}

HgString& HgString::insert(HgArena* arena, u64 index, char c)
{
    hgAssert(index <= length);

    u64 newLength = length + 1;
    while (capacity < newLength)
    {
        grow(arena);
    }

    if (index != length)
        memmove(&chars[index + 1], &chars[index], length - index);
    chars[index] = c;
    length = newLength;

    return *this;
}

HgString& HgString::insert(HgArena* arena, u64 index, HgStringView str)
{
    hgAssert(index <= length);

    u64 newLength = length + str.length;
    while (capacity < newLength)
    {
        grow(arena);
    }

    if (index != length)
        memmove(&chars[index + str.length], &chars[index], length - index);
    memcpy(&chars[index], str.chars, str.length);
    length = newLength;

    return *this;
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
        return HgString::copy(arena, "0");

    bool isNegative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgString reverse = reverse.create(scratch, 16);
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        reverse.append(scratch, '0' + (char)digit);
    }

    HgString ret = ret.create(arena, reverse.length + (isNegative ? 1 : 0));
    if (isNegative)
        ret.append(arena, '-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        ret.append(arena, reverse[i]);
    }
    return ret;
}

HgString hgFloatToStr(HgArena* arena, f64 num, u32 decimalCount)
{
    HgArena* scratch = hgGetScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    if (num == 0.0)
        return HgString::copy(arena, "0.0");

    HgString intStr = hgIntToStr(scratch, (i64)fabs(num));

    HgString decStr = HgString::create(scratch, decimalCount + 1);
    decStr.append(scratch, '.');

    f64 decPart = fabs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        decStr.append(scratch, '0' + (char)((u64)decPart % 10));
    }

    HgString ret{};
    if (num < 0.0)
        ret.append(arena, '-');
    ret.append(arena, intStr);
    ret.append(arena, decStr);
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
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", found unexpected token \"]\"\n");
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
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hgIntToStr(arena, (i64)line))
        .append(arena, ", found unexpected token \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

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
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
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
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", struct ends with \"]\" instead of \"}\"\n");
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
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hgIntToStr(arena, (i64)line))
                    .append(arena, ", struct has a literal instead of a field\n");
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else if (value.file->field.value == nullptr)
            {
                HgJsonError* error = hgAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hgIntToStr(arena, (i64)line))
                    .append(arena, ", struct has a field named \"")
                    .append(arena, value.file->field.name)
                    .append(arena, "\" which has no value\n");
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
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
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
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hgIntToStr(arena, (i64)line))
                .append(arena, ", array ends with \"}\" instead of \"]\"\n");
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
                    error->msg = HgString{}
                        .append(arena, "on line ")
                        .append(arena, hgIntToStr(arena, (i64)line))
                        .append(arena, ", array has a field as an element\n");
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
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hgIntToStr(arena, (i64)line))
                    .append(arena, ", array has element which is not the same type as the first valid element\n");
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
        HgString str = str.create(arena, end - begin);
        for (u64 i = begin; i < end; ++i)
        {
            char c = text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            str.append(arena, c);
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
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hgIntToStr(arena, (i64)line))
        .append(arena, ", expected string to terminate\n");
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

    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hgIntToStr(arena, (i64)line))
        .append(arena, ", expected numeral value, found \"")
        .append(arena, num)
        .append(arena, "\"\n");

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
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hgIntToStr(arena, (i64)line))
        .append(arena, ", expected boolean value, found \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

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

namespace {
    struct ComponentArr {
        u32* widths;
        u32 capacity;
        u32 count;

        static ComponentArr create(u32 initCount)
        {
            ComponentArr arr;
            arr.widths = (u32*)malloc(initCount * sizeof(u32));
            arr.capacity = initCount;
            arr.count = 0;
            return arr;
        }

        void destroy() const {
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
    static ComponentArr components = components.create(1024);
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

HgECS HgECS::create(u32 maxEntities)
{
    HgECS ecs{};

    ecs.poolSize = maxEntities;
    ecs.pool = (HgEntity*)malloc(sizeof(HgEntity) * ecs.poolSize);

    ecs.systemCount = componentArr().count;
    ecs.systems = (System*)malloc(sizeof(System) * ecs.systemCount);

    for (u32 i = 0; i < ecs.systemCount; ++i)
    {
        ecs.systems[i] = {};
        ecs.systems[i].indices = (u32*)malloc(sizeof(u32) * ecs.poolSize);
    }

    ecs.reset();

    return ecs;
}

void HgECS::destroy()
{
    for (u32 i = 0; i < systemCount; ++i)
    {
        free(systems[i].indices);
        free(systems[i].entities);
        free(systems[i].components);
    }
    free(systems);
    free(pool);
}

void HgECS::reset()
{
    for (u32 i = 0; i < poolSize; ++i)
    {
        pool[i] = {i + 1};
    }
    next = {0};

    for (u32 i = 0; i < systemCount; ++i)
    {
        memset(systems[i].indices, -1, poolSize * sizeof(*systems[i].indices));
        systems[i].count = 0;
    }
}

HgEntity HgECS::spawn()
{
    hgAssert(next.idx() < poolSize);

    HgEntity entity = next;
    next= pool[entity.idx()];
    pool[entity.idx()] = entity;
    return entity;
}

void HgECS::despawn(HgEntity e)
{
    hgAssert(alive(e));

    for (u32 i = 0; i < systemCount; ++i)
    {
        if (has(e, i))
            remove(e, i);
    }
    pool[e.idx()] = next;
    next = e;
    next.incrementGeneration();
}

bool HgECS::alive(HgEntity e)
{
    return e.idx() < poolSize && pool[e.idx()] == e;
}

void* HgECS::add(HgEntity e, u32 componentId)
{
    hgAssert(alive(e));
    hgAssert(!has(e, componentId));

    if (systems[componentId].count == systems[componentId].capacity)
    {
        u32 newCapacity = systems[componentId].capacity == 0 ? 1 : systems[componentId].capacity * 2;
        systems[componentId].entities = (HgEntity*)realloc(
            systems[componentId].entities, sizeof(HgEntity) * newCapacity);
        systems[componentId].components = (HgEntity*)realloc(
            systems[componentId].components, componentWidth(componentId) * newCapacity);
        systems[componentId].capacity = newCapacity;
    }

    systems[componentId].indices[e.idx()] = systems[componentId].count;
    systems[componentId].entities[systems[componentId].count] = e;
    void* c = (u8*)systems[componentId].components + componentWidth(componentId) * systems[componentId].count;
    ++systems[componentId].count;
    return c;
}

void HgECS::remove(HgEntity e, u32 componentId)
{
    hgAssert(alive(e));
    hgAssert(has(e, componentId));

    HgEntity last = systems[componentId].entities[systems[componentId].count - 1];
    systems[componentId].entities[systems[componentId].count - 1] = HgEntity{};
    if (e != last)
    {
        u32 idx = systems[componentId].indices[e.idx()];
        systems[componentId].entities[idx] = last;
        systems[componentId].indices[last.idx()] = idx;
        memcpy(
            (u8*)systems[componentId].components + componentWidth(componentId) * idx,
            (u8*)systems[componentId].components + componentWidth(componentId) * (systems[componentId].count - 1),
            componentWidth(componentId));
    }
    systems[componentId].indices[e.idx()] = (u32)-1;
    --systems[componentId].count;
}

bool HgECS::has(HgEntity e, u32 componentId)
{
    hgAssert(alive(e));
    return systems[componentId].indices[e.idx()] < systems[componentId].count;
}

void* HgECS::get(HgEntity e, u32 componentId)
{
    hgAssert(alive(e));
    hgAssert(has(e, componentId));
    return (u8*)systems[componentId].components + componentWidth(componentId) * systems[componentId].indices[e.idx()];
}

HgEntity HgECS::get(const void* component, u32 componentId)
{
    hgAssert(component != nullptr);

    u32 idx = (u32)((uptr)component - (uptr)systems[componentId].components) / componentWidth(componentId);
    return systems[componentId].entities[idx];
}

u32 HgECS::findSmallest(u32* ids, u32 idCount)
{
    u32 smallest = ids[0];
    for (u32 i = 1; i < idCount; ++i)
    {
        if (systems[ids[i]].count < systems[smallest].count)
            smallest = ids[i];
    }
    return smallest;
}

static void swapIdxLocation(HgECS* ecs, u32 lhs, u32 rhs, u32 componentId)
{
    HgECS::System& system = ecs->systems[componentId];
    hgAssert(lhs < system.count);
    hgAssert(rhs < system.count);

    HgEntity lhsEntity = system.entities[lhs];
    HgEntity rhsEntity = system.entities[rhs];

    hgAssert(ecs->alive(lhsEntity));
    hgAssert(ecs->alive(rhsEntity));
    hgAssert(ecs->has(lhsEntity, componentId));
    hgAssert(ecs->has(rhsEntity, componentId));

    system.entities[lhs] = rhsEntity;
    system.entities[rhs] = lhsEntity;
    system.indices[lhsEntity.id] = rhs;
    system.indices[rhsEntity.id] = lhs;

    u32 width = componentWidth(componentId);
    void* temp = alloca(componentWidth(componentId));
    memcpy(temp, (u8*)system.components + width * lhs, width);
    memcpy((u8*)system.components + width * lhs, (u8*)system.components + width * rhs, width);
    memcpy((u8*)system.components + width * rhs, temp, width);
}

namespace {
    struct QuicksortData {
        HgECS* ecs;
        u32 comp;
        void* data;
        bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs);

        u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
        {
            while (inc != dec)
            {
                while (!compare(data, ecs, ecs->systems[comp].entities[dec], ecs->systems[comp].entities[pivot]))
                {
                    --dec;
                    if (dec == inc)
                        goto finish;
                }
                while (!compare(data, ecs, ecs->systems[comp].entities[pivot], ecs->systems[comp].entities[inc]))
                {
                    ++inc;
                    if (inc == dec)
                        goto finish;
                }
                swapIdxLocation(ecs, inc, dec, comp);
            }

        finish:
            if (compare(data, ecs, ecs->systems[comp].entities[inc], ecs->systems[comp].entities[pivot]))
                swapIdxLocation(ecs, pivot, inc, comp);

            return inc;
        }

        void quicksort(u32 begin, u32 end)
        {
            hgAssert(begin <= end && end <= ecs->systems[comp].count);

            if (begin + 1 >= end)
                return;

            u32 middle = quicksortInter(begin, begin + 1, end - 1);
            quicksort(begin, middle);
            quicksort(middle, end);
        }
    };
}

void HgECS::sort(
    u32 componentId,
    void* data,
    bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs)
)
{
    QuicksortData q{this, componentId, data, compare};
    q.quicksort(0, systems[componentId].count);
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

