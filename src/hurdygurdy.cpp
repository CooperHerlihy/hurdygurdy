#include "hurdygurdy.hpp"
#include "internal.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include <random>

#include <emmintrin.h>

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

thread_local static char errorData[4096];
thread_local static u64 errorLength = 0;

StringView getError()
{
    return {errorData, errorLength};
}

void setError(StringView error)
{
    u64 newLength = std::min(error.length, sizeof(errorData));
    memcpy(errorData, error.chars, newLength);
    errorLength = newLength;
}

void logError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", (int)errorLength, errorData);
}

static u32 initialized = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized > 0)
        return some<HurdyGurdy>();

    internal::initConcurrency();

    if (!internal::initPlatform())
        goto platformFailed;

    if (!internal::initGpu())
        goto gpuFailed;

    if (!internal::initAudio())
        goto audioFailed;

    return some<HurdyGurdy>();

audioFailed:
    internal::deinitGpu();
gpuFailed:
    internal::deinitPlatform();
platformFailed:
    internal::deinitConcurrency();

    return none<HurdyGurdy>();
}

HurdyGurdy::HurdyGurdy() noexcept
{
    ++initialized;
}

HurdyGurdy::HurdyGurdy(const HurdyGurdy&)
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(const HurdyGurdy&)
{
    ++initialized;
    return *this;
}

HurdyGurdy::HurdyGurdy(HurdyGurdy&&) noexcept
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(HurdyGurdy&&) noexcept
{
    ++initialized;
    return *this;
}

HurdyGurdy::~HurdyGurdy()
{
    if (--initialized == 0)
    {
        internal::deinitAudio();
        internal::deinitGpu();
        internal::deinitPlatform();
        internal::deinitConcurrency();
    }
}

void BinaryView::read(u64 idx, void* dst, u64 len) const
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}

void* heapAlloc(u64 size, u64 alignment)
{
    static_cast<void>(alignment);
    void* alloc = malloc(size);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

void heapFree(void* allocation, u64 size)
{
    static_cast<void>(size);
    free(allocation);
}

Arena::Arena(u64 capacityVal)
    : memory{heapAlloc(capacityVal, alignof(std::max_align_t))}
    , capacity{capacityVal}
    , head{0}
{}

Arena::~Arena() noexcept
{
    if (memory != nullptr)
        heapFree(memory, capacity);
}

void* Arena::alloc(u64 size, u64 alignment)
{
    u64 newHead = align(static_cast<u64>(head), alignment) + size;
    if (newHead > capacity)
    {
        setError("Arena out of memory");
        return nullptr;
    }

    head = newHead;
    return reinterpret_cast<void*>(reinterpret_cast<uptr>(memory) + head - size);
}

bool Arena::extend(void* allocation, u64 oldSize, u64 newSize)
{
    if (reinterpret_cast<uptr>(allocation) + oldSize - reinterpret_cast<uptr>(memory) != static_cast<uptr>(head))
        return false;

    u64 newHead = reinterpret_cast<uptr>(allocation) + newSize - reinterpret_cast<uptr>(memory);
    if (newHead > capacity)
    {
        return false;
    }

    head = newHead;
    return true;
}

static thread_local Array<Arena> scratchArenas{};

ArenaScope getScratch(Arena const* const* conflicts, u32 count)
{
    if (count > 0)
        HG_ASSERT(conflicts != nullptr);

    for (u32 i = 0; i < scratchArenas.count; ++i)
    {
        for (u32 j = 0; j < count; ++j)
        {
            if (&scratchArenas[i] == conflicts[j])
                goto next;
        }
        return &scratchArenas[i];
next:
        continue;
    }

    return scratchArenas.push(Arena{((u64)1 << 28) - 1});
}

struct ThreadWork {
    Fence* fence = nullptr;
    void* data = nullptr;
    void (*fn)(void*) = nullptr;
};

struct ThreadPoolState {
    Array<ThreadWork> work{};
    Array<std::atomic_bool> hasWork{};

    std::atomic<u32> workCount = 0;
    std::atomic<u32> tail = 0;
    std::atomic<u32> workingTail = 0;
    std::atomic<u32> head = 0;
    std::atomic<u32> workingHead = 0;

    std::mutex mtx{};
    std::condition_variable_any cv{};
    Array<std::jthread> threads{};
};

static ThreadPoolState threadPool{};

static bool threadPoolExecute()
{
    u32 idx = threadPool.workingTail.load();
    do {
        if (idx == threadPool.head.load())
            return false;
    } while (!threadPool.workingTail.compare_exchange_weak(idx, (idx + 1) & (threadPool.work.count - 1)));

    ThreadWork work = threadPool.work[idx];
    threadPool.hasWork[idx].store(false);

    u32 t = threadPool.tail.load();
    while (t != threadPool.head.load() && !threadPool.hasWork[t].load())
    {
        u32 next = (t + 1) & (threadPool.work.count - 1);
        threadPool.tail.compare_exchange_strong(t, next);
        t = next;
    }

    --threadPool.workCount;

    HG_ASSERT(work.fn != nullptr);
    work.fn(work.data);

    if (work.fence != nullptr)
        work.fence->signal();
    return true;
}

void internal::initConcurrency()
{
    u32 workCapacity = 4096;
    threadPool.work = {workCapacity, workCapacity};
    threadPool.hasWork = {workCapacity, workCapacity};
    for (std::atomic_bool& hasWork : threadPool.hasWork)
    {
        hasWork.store(false);
    }

    threadPool.workCount.store(0);
    threadPool.tail.store(0);
    threadPool.workingTail.store(0);
    threadPool.head.store(0);
    threadPool.workingHead.store(0);

    auto threadFn = [](std::stop_token st) {
        while (!st.stop_requested())
        {
            static constexpr u32 spinCount = 128;
            for (u32 j = 0; j < spinCount; ++j)
            {
                if (!threadPoolExecute())
                    _mm_pause();
            }

            std::unique_lock lock{threadPool.mtx};
            threadPool.cv.wait(lock, st, [&] {
                return threadPool.workCount.load() > 0 || st.stop_requested();
            });
        }
    };

    u32 threadCount = std::max((u32)1, std::thread::hardware_concurrency() - 1);
    threadPool.threads.reserve(threadCount);
    for (u32 i = 0; i < threadCount; ++i)
    {
        threadPool.threads.push(std::jthread{threadFn});
    }
}

void internal::deinitConcurrency()
{
    threadPool.~ThreadPoolState();
    new (&threadPool) ThreadPoolState{};
}

void SpinLock::acquire()
{
    bool acquiredLocal = false;
    while (!acquired.compare_exchange_weak(acquiredLocal, true))
    {
        _mm_pause();
        acquiredLocal = false;
    }
}

bool SpinLock::tryAcquire()
{
    bool acquiredLocal = false;
    return acquired.compare_exchange_weak(acquiredLocal, true);
}

void SpinLock::release()
{
    acquired.store(false);
}

void Fence::add(u32 count)
{
    counter.fetch_add(count);
}

void Fence::signal(u32 count)
{
    u32 prev = counter.fetch_sub(count);
    HG_ASSERT(prev >= count);
}

bool Fence::isComplete()
{
    return counter.load() == 0;
}

bool Fence::wait(f64 timeout)
{
    Clock c{};
    while (!isComplete() && (timeout -= c.tick()) > 0)
    {
        _mm_pause();
    }
    return isComplete();
}

void Fence::waitIndefinite()
{
    while (!isComplete())
    {
        _mm_pause();
    }
}

bool helpThreads(Fence* fence, f64 timeout)
{
    Clock c{};
    while (!fence->isComplete() && (timeout -= c.tick()) > 0)
    {
        if (!threadPoolExecute())
            _mm_pause();
    }
    return fence->isComplete();
}

void callPar(Fence* fence, void* data, void (*fn)(void* data))
{
    HG_ASSERT(fn != nullptr);
    if (fence != nullptr)
        fence->add();

    u32 idx = threadPool.workingHead.fetch_add(1) & (threadPool.work.count - 1);

    threadPool.work[idx].fence = fence;
    threadPool.work[idx].data = data;
    threadPool.work[idx].fn = fn;
    threadPool.hasWork[idx].store(true);

    u32 h = threadPool.head.load();
    while (threadPool.hasWork[h].load())
    {
        u32 next = (h + 1) & (threadPool.work.count - 1);
        threadPool.head.compare_exchange_strong(h, next);
        h = next;
    }

    ++threadPool.workCount;
    threadPool.mtx.lock();
    threadPool.mtx.unlock();
    threadPool.cv.notify_one();
}

void forPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    HG_ASSERT(begin <= end);
    HG_ASSERT(fn != nullptr);

    ArenaScope scratch = getScratch();

    u64 chunkSize = static_cast<u64>(std::ceil(static_cast<f64>(end - begin) / (8.0 * static_cast<f64>(threadPool.threads.count))));

    Fence fence{};
    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data = nullptr;
            void (*fn)(void* data, u64 idx) = nullptr;
            u64 begin = 0;
            u64 end = 0;
        };

        Capture* capture = scratch.alloc<Capture>(1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = std::min(i + chunkSize, end);

        callPar(&fence, capture, [](void* pcapture)
        {
            Capture* capture = static_cast<Capture*>(pcapture);
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    helpThreads(&fence, INFINITY);
}

Vec2& Vec2::operator+=(Vec2 other)
{
    x += other.x;
    y += other.y;
    return* this;
}

Vec2& Vec2::operator-=(Vec2 other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

Vec2& Vec2::operator*=(Vec2 other)
{
    x *= other.x;
    y *= other.y;
    return* this;
}

Vec2& Vec2::operator/=(Vec2 other)
{
    x /= other.x;
    y /= other.y;
    return* this;
}

Vec3& Vec3::operator+=(Vec3 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

Vec3& Vec3::operator-=(Vec3 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

Vec3& Vec3::operator*=(Vec3 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

Vec3& Vec3::operator/=(Vec3 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

Vec4& Vec4::operator+=(Vec4 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

Vec4& Vec4::operator-=(Vec4 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

Vec4& Vec4::operator*=(Vec4 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

Vec4& Vec4::operator/=(Vec4 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

Mat2& Mat2::operator+=(const Mat2& other)
{
    x += other.x;
    y += other.y;
    return* this;
}

Mat2& Mat2::operator-=(const Mat2& other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

Mat3& Mat3::operator+=(const Mat3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

Mat3& Mat3::operator-=(const Mat3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

Mat4& Mat4::operator+=(const Mat4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

Mat4& Mat4::operator-=(const Mat4& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

void matTranspose(u32 width, u32 height, f32* dst, const f32* mat)
{
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[j * width + i] = mat[i * height + j];
        }
    }
}

Mat2 matTranspose2(const Mat2& mat)
{
    Mat2 ret;
    matTranspose(2, 2, &ret.x.x, &mat.x.x);
    return ret;
}

Mat3 matTranspose3(const Mat3& mat)
{
    Mat3 ret;
    matTranspose(3, 3, &ret.x.x, &mat.x.x);
    return ret;
}

Mat4 matTranspose4(const Mat4& mat)
{
    Mat4 ret;
    matTranspose(4, 4, &ret.x.x, &mat.x.x);
    return ret;
}

Complex& Complex::operator+=(Complex other)
{
    r += other.r;
    i += other.i;
    return* this;
}

Complex& Complex::operator-=(Complex other)
{
    r -= other.r;
    i -= other.i;
    return* this;
}

Quat& Quat::operator+=(Quat other)
{
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

Quat& Quat::operator-=(Quat other)
{
    r -= other.r;
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return* this;
}

f32 vecLen2(Vec2 vec)
{
    return std::sqrt(vecLenSqr2(vec));
}

f32 vecLen3(Vec3 vec)
{
    return std::sqrt(vecLenSqr3(vec));
}

f32 vecLen4(Vec4 vec)
{
    return std::sqrt(vecLenSqr4(vec));
}

Vec2 vecNorm2(Vec2 vec)
{
    f32 len = vecLen2(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len};
}

Vec3 vecNorm3(Vec3 vec)
{
    f32 len = vecLen3(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len};
}

Vec4 vecNorm4(Vec4 vec)
{
    f32 len = vecLen4(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void vecCross3(f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

f32 vecCross2(Vec2 lhs, Vec2 rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

Vec3 vecCross3(Vec3 lhs, Vec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

static void matAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

Mat2 operator+(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matAdd(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat3 operator+(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matAdd(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat4 operator+(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matAdd(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

static void matSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

Mat2 operator-(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matSub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat3 operator-(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matSub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat4 operator-(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matSub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

static void matMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
{
    HG_ASSERT(hr == wl);
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    static_cast<void>(hr);
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

Mat2 operator*(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matMul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

Mat3 operator*(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matMul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

Mat4 operator*(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matMul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

static void matMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(mat != nullptr);
    HG_ASSERT(vec != nullptr);
    for (u32 i = 0; i < height; ++i)
    {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j)
        {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

Vec2 operator*(const Mat2& lhs, Vec2 rhs)
{
    Vec2 result{};
    matMulVec(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

Vec3 operator*(const Mat3& lhs, Vec3 rhs)
{
    Vec3 result{};
    matMulVec(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

Vec4 operator*(const Mat4& lhs, Vec4 rhs)
{
    Vec4 result{};
    matMulVec(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

f32 complexAbsSqr(Complex comp)
{
    return comp.r * comp.r + comp.i * comp.i;
}

f32 complexAbs(Complex comp)
{
    return sqrtf(complexAbsSqr(comp));
}

Complex complexNorm(Complex comp)
{
    f32 len = complexAbs(comp);
    HG_ASSERT(len != 0);
    return Complex{comp.r / len, comp.i / len};
}

Vec2 vecRot2(Complex lhs, Vec2 rhs)
{
    Complex c = lhs * Complex{rhs.x, rhs.y};
    return {c.r, c.i};
}

Quat operator*(Quat lhs, Quat rhs)
{
    return Quat{
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

f32 quatAbsSqr(Quat quat)
{
    return quat.r * quat.r + quat.i * quat.i + quat.j * quat.j + quat.k * quat.k;
}

f32 quatAbs(Quat quat)
{
    return sqrtf(quatAbsSqr(quat));
}

Quat quatNorm(Quat quat)
{
    f32 len = quatAbs(quat);
    return Quat{quat.r / len, quat.i / len, quat.j / len, quat.k / len};
}

Quat quatAxisAngle(Vec3 axis, f32 angle)
{
    f32 halfAngle = angle * (f32)0.5;
    f32 sinHalfAngle = sinf(halfAngle);
    return Quat{
        cosf(halfAngle),
        axis.x * sinHalfAngle,
        axis.y * sinHalfAngle,
        axis.z * sinHalfAngle,
    };
}

Quat quatBetween(Vec3 from, Vec3 to)
{
    HG_ASSERT(from != Vec3{0});
    HG_ASSERT(to != Vec3{0});

    from = vecNorm3(from);
    to = vecNorm3(to);

    f32 dot = vecDot3(from, to);

    if (dot > 1 - FLT_EPSILON)
    {
        return Quat{1};
    }

    if (dot < -1 + FLT_EPSILON)
    {
        Vec3 axis = std::abs(from.x) >= 1 - FLT_EPSILON
            ? vecCross3(from, {0, 1, 0})
            : vecCross3(from, {1, 0, 0});
        return Quat(0, axis.x, axis.y, axis.z);
    }

    Vec3 axis = vecCross3(from, to);
    return quatNorm(Quat{dot + 1, axis.x, axis.y, axis.z});
}

Vec3 vecRot3(Quat lhs, Vec3 rhs)
{
    Quat q = lhs * Quat{0, rhs.x, rhs.y, rhs.z} * quatConj(lhs);
    return {q.i, q.j, q.k};
}

Mat3 matRot3(Quat lhs, Mat3 rhs)
{
    return Mat3{
        vecRot3(lhs, rhs.x),
        vecRot3(lhs, rhs.y),
        vecRot3(lhs, rhs.z),
    };
}

Mat4 matModel2D(Vec3 position, Vec2 scale, f32 rotation)
{
    Mat2 m2{{scale.x, 0.0f}, {0.0f, scale.y}};
    f32 rotSin = sinf(rotation);
    f32 rotCos = cosf(rotation);
    Mat2 rot{{rotCos, rotSin}, {-rotSin, rotCos}};
    Mat4 m4 = Mat4{rot * m2};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

Mat4 matModel3D(const Vec3& position, const Vec3& scale, const Quat& rotation)
{
    Mat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = matRot3(rotation, m3);
    Mat4 m4 = Mat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

Mat4 matView(const Vec3& position, const Vec3& zoom, const Quat& rotation)
{
    Mat4 rot{matRot3(quatConj(rotation), Mat3{1.0f})};
    Mat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

Mat4 matModelToView(const Mat4& model)
{
    if (Vec3{model.x} == Vec3{0} || Vec3{model.y} == Vec3{0} || Vec3{model.z} == Vec3{0})
        return Mat4{Mat3{0}};

    Mat3 inv3 = matTranspose3(Mat3{
        vecNorm3(Vec3{model.x}),
        vecNorm3(Vec3{model.y}),
        vecNorm3(Vec3{model.z}),
    });
    Mat4 inv4{inv3};
    inv4.w = Vec4{inv3 * Vec3{model.w} * -1, 1};
    return inv4;
}

Mat4 matOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return Mat4{
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

Mat4 matPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    HG_ASSERT(near > 0.0f);
    HG_ASSERT(far > near);
    f32 scale = 1.0f / static_cast<f32>(tan(fov * 0.5f));
    return Mat4{
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

bool containsPointCircle(Vec2 point, Circle circle)
{
    return distPointCircle(point, circle) <= FLT_EPSILON;
}

f32 distPointCircle(Vec2 point, Circle circle)
{
    return vecLen2(point - circle.pos) - circle.radius;
}

Vec2 closestPointCircle(Vec2 pos, Circle circle)
{
    return circle.pos + circle.radius * vecNorm2(pos - circle.pos);
}

bool intersectCircles(Circle a, Circle b)
{
    return distCircles(a, b) <= FLT_EPSILON;
}

f32 distCircles(Circle a, Circle b)
{
    return vecLen2(a.pos - b.pos) - a.radius - b.radius;
}

Rect rectEmpty()
{
    return {
        Vec2{INFINITY},
        Vec2{-INFINITY},
    };
}

Rect rectAddPoint(Rect rect, Vec2 point)
{
    Rect newRect;
    newRect.begin.x = std::min(rect.begin.x, point.x - FLT_EPSILON);
    newRect.begin.y = std::min(rect.begin.y, point.y - FLT_EPSILON);
    newRect.end.x = std::max(rect.end.x, point.x + FLT_EPSILON);
    newRect.end.y = std::max(rect.end.y, point.y + FLT_EPSILON);
    return newRect;
}

bool containsPointRect(Vec2 point, Rect rect)
{
    return point.x >= rect.begin.x - FLT_EPSILON && point.x <= rect.end.x + FLT_EPSILON
        && point.y >= rect.begin.y - FLT_EPSILON && point.y <= rect.end.y + FLT_EPSILON;
}

Vec2 closestPointRect(Vec2 pos, Rect rect)
{
    return {
        std::clamp(pos.x, rect.begin.x, rect.end.x),
        std::clamp(pos.y, rect.begin.y, rect.end.y),
    };
}

bool intersectRects(Rect a, Rect b)
{
    return a.end.x >= b.begin.x && a.begin.x <= b.end.x
        && a.end.y >= b.begin.y && a.begin.y <= b.end.y;
}

bool intersectRectCircle(Rect rect, Circle circle)
{
    return containsPointCircle(closestPointRect(circle.pos, rect), circle);
}

Maybe<Hit2D> intersectRays2D(Ray2D ray, Ray2D other)
{
    HG_ASSERT(ray.dir != Vec2{0});
    HG_ASSERT(other.dir != Vec2{0});

    f32 denom = vecCross2(ray.dir, other.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit2D>();

    Vec2 diff = other.pos - ray.pos;

    f32 t = vecCross2(diff, other.dir) / denom;
    if (t < -FLT_EPSILON)
        return none<Hit2D>();

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({other.dir.y, -other.dir.x})
        : vecNorm2({-other.dir.y, other.dir.x}));
}

Maybe<Hit2D> intersectRayLine2D(Ray2D ray, Line2D line)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(line.begin, line.end))
        return none<Hit2D>();

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(ray.dir, lineDir);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit2D>();

    Vec2 diff = line.begin - ray.pos;

    f32 t = vecCross2(diff, lineDir) / denom;
    if (t < -FLT_EPSILON)
        return none<Hit2D>();

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({lineDir.y, -lineDir.x})
        : vecNorm2({-lineDir.y, lineDir.x}));
}

Maybe<Hit2D> intersectRayCircle(Ray2D ray, Circle circle)
{
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 rel = ray.pos - circle.pos;
    f32 a = vecDot2(ray.dir, ray.dir);
    f32 b = 2 * vecDot2(ray.dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return none<Hit2D>();
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, (ray.pos + t * ray.dir - circle.pos) / circle.radius);
}

Maybe<Hit2D> intersectRayRect(Ray2D ray, Rect rect)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(rect.begin, rect.end))
        return none<Hit2D>();

    if (containsPointRect(ray.pos, rect))
    {
        return some<Hit2D>(0, -ray.dir);
    }

    f32 hits[4] = {
        (rect.begin.x - ray.pos.x) / ray.dir.x,
        (rect.begin.y - ray.pos.y) / ray.dir.y,
        (rect.end.x - ray.pos.x) / ray.dir.x,
        (rect.end.y - ray.pos.y) / ray.dir.y,
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!containsPointRect(ray.pos + hits[i] * ray.dir, rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return none<Hit2D>();

    return some<Hit2D>(t, norm);
}

Maybe<Hit2D> intersectLines2D(Line2D line, Line2D other)
{
    if (vecEq2(line.begin, line.end) || vecEq2(other.begin, other.end))
        return none<Hit2D>();

    Vec2 lineDir = line.end - line.begin;
    Vec2 otherDir = other.end - other.begin;

    f32 denom = vecCross2(lineDir, otherDir);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit2D>();

    Vec2 diff = other.begin - line.begin;

    f32 t = vecCross2(diff, otherDir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit2D>();

    f32 tOther = vecCross2(diff, lineDir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({otherDir.y, -otherDir.x})
        : vecNorm2({-otherDir.y, otherDir.x}));
}

Maybe<Hit2D> intersectLineRay2D(Line2D line, Ray2D ray)
{
    if (vecEq2(line.begin, line.end))
        return none<Hit2D>();
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(lineDir, ray.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit2D>();

    Vec2 diff = ray.pos - line.begin;

    f32 t = vecCross2(diff, ray.dir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit2D>();

    f32 tRay = vecCross2(diff, lineDir) / denom;
    if (tRay < -FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({ray.dir.y, -ray.dir.x})
        : vecNorm2({-ray.dir.y, ray.dir.x}));
}

Maybe<Hit2D> intersectLineCircle(Line2D line, Circle circle)
{
    Vec2 dir = line.end - line.begin;

    Vec2 rel = line.begin - circle.pos;
    f32 a = vecDot2(dir, dir);
    f32 b = 2 * vecDot2(dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return none<Hit2D>();
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return none<Hit2D>();
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit2D>();

    return some<Hit2D>(t, (line.begin + t * dir - circle.pos) / circle.radius);
}

Maybe<Hit2D> intersectLineRect(Line2D line, Rect rect)
{
    if (vecEq2(line.begin, line.end) || vecEq2(rect.begin, rect.end))
        return none<Hit2D>();

    f32 hits[4] = {
        (rect.begin.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.begin.y - line.begin.y) / (line.end.y - line.begin.y),
        (rect.end.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.end.y - line.begin.y) / (line.end.y - line.begin.y),
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!containsPointRect(line.begin + hits[i] * (line.end - line.begin), rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return none<Hit2D>();

    return some<Hit2D>(t, norm);
}

bool containsPointSphere(Vec3 point, Sphere sphere)
{
    return distPointSphere(point, sphere) <= 0;
}

f32 distPointSphere(Vec3 point, Sphere sphere)
{
    return vecLen3(point - sphere.pos) - sphere.radius;
}

Vec3 closestPointSphere(Vec3 pos, Sphere sphere)
{
    Vec3 rel = pos - sphere.pos;
    if (vecLenSqr3(rel) <= sphere.radius + FLT_EPSILON)
        return pos;
    return sphere.pos + sphere.radius * vecNorm3(rel);
}

bool intersectSpheres(Sphere a, Sphere b)
{
    return distSpheres(a, b) <= 0;
}

f32 distSpheres(Sphere a, Sphere b)
{
    return vecLen3(a.pos - b.pos) - a.radius - b.radius;
}

Box boxEmpty()
{
    return {
        Vec3{INFINITY},
        Vec3{-INFINITY},
    };
}

Box boxAddPoint(Box box, Vec3 point)
{
    Box newBox;
    newBox.begin.x = std::min(box.begin.x, point.x - FLT_EPSILON);
    newBox.begin.y = std::min(box.begin.y, point.y - FLT_EPSILON);
    newBox.begin.z = std::min(box.begin.z, point.z - FLT_EPSILON);
    newBox.end.x = std::max(box.end.x, point.x + FLT_EPSILON);
    newBox.end.y = std::max(box.end.y, point.y + FLT_EPSILON);
    newBox.end.z = std::max(box.end.z, point.z + FLT_EPSILON);
    return newBox;
}

bool containsPointBox(Vec3 point, Box box)
{
    return point.x >= box.begin.x && point.x <= box.end.x
        && point.y >= box.begin.y && point.y <= box.end.y
        && point.z >= box.begin.z && point.z <= box.end.z;
}

Vec3 closestPointBox(Vec3 pos, Box box)
{
    return {
        std::clamp(pos.x, box.begin.x, box.end.x),
        std::clamp(pos.y, box.begin.y, box.end.y),
        std::clamp(pos.z, box.begin.z, box.end.z),
    };
}

bool intersectBox(Box a, Box b)
{
    return a.end.x >= b.begin.x && a.begin.x <= b.end.x
        && a.end.y >= b.begin.y && a.begin.y <= b.end.y
        && a.end.z >= b.begin.z && a.begin.z <= b.end.z;
}

bool intersectBoxSphere(Box box, Sphere sphere)
{
    return containsPointSphere(closestPointBox(sphere.pos, box), sphere);
}

Plane planeFromPoint(Vec3 point, Vec3 normal)
{
    Plane plane;
    plane.normal = vecNorm3(normal);
    plane.dist = vecDot3(plane.normal, point);
    return plane;
}

Plane planeFromTri(Tri tri)
{
    Plane plane;
    plane.normal = vecNorm3(vecCross3(tri.b - tri.a, tri.c - tri.a));
    plane.dist = vecDot3(plane.normal, tri.a);
    return plane;
}

Maybe<Hit3D> intersectRaySphere(Ray3D ray, Sphere sphere)
{
    HG_ASSERT(ray.dir != Vec3{0});

    Vec3 rel = ray.pos - sphere.pos;
    f32 a = vecDot3(ray.dir, ray.dir);
    f32 b = 2 * vecDot3(ray.dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return none<Hit3D>();
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, (ray.pos + t * ray.dir - sphere.pos) / sphere.radius);
}

Maybe<Hit3D> intersectRayBox(Ray3D ray, Box box)
{
    HG_ASSERT(ray.dir != Vec3{0});
    if (vecEq3(box.begin, box.end))
        return none<Hit3D>();

    if (containsPointBox(ray.pos, box))
    {
        return some<Hit3D>(0, -ray.dir);
    }

    f32 hits[6] = {
        (box.begin.x - ray.pos.x) / ray.dir.x,
        (box.begin.y - ray.pos.y) / ray.dir.y,
        (box.begin.z - ray.pos.z) / ray.dir.z,
        (box.end.x - ray.pos.x) / ray.dir.x,
        (box.end.y - ray.pos.y) / ray.dir.y,
        (box.end.z - ray.pos.z) / ray.dir.z,
    };

    constexpr Vec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    Vec3 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!containsPointBox(ray.pos + hits[i] * ray.dir, box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return none<Hit3D>();

    return some<Hit3D>(t, norm);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
Maybe<Hit3D> intersectRayTri(Ray3D ray, Tri tri)
{
    HG_ASSERT(ray.dir != Vec3{0});

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return none<Hit3D>();

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(ray.dir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return none<Hit3D>();

    Vec3 s = ray.pos - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return none<Hit3D>();

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(ray.dir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return none<Hit3D>();

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, a < 0
        ? vecNorm3(vecCross3(e2, e1))
        : vecNorm3(vecCross3(e1, e2)));
}

Maybe<Hit3D> intersectRayPlane(Ray3D ray, Plane plane)
{
    HG_ASSERT(ray.dir != Vec3{0});
    HG_ASSERT(plane.normal != Vec3{0});

    f32 denom = vecDot3(ray.dir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit3D>();

    f32 t = (plane.dist - vecDot3(ray.pos, plane.normal)) / denom;
    if (t < -FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

Maybe<Hit3D> intersectLineSphere(Line3D line, Sphere sphere)
{
    Vec3 dir = line.end - line.begin;

    Vec3 rel = line.begin - sphere.pos;
    f32 a = vecDot3(dir, dir);
    f32 b = 2 * vecDot3(dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return none<Hit3D>();
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return none<Hit3D>();
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, (line.begin + t * dir - sphere.pos) / sphere.radius);
}

Maybe<Hit3D> intersectLineBox(Line3D line, Box box)
{
    if (vecEq3(line.begin, line.end) || vecEq3(box.begin, box.end))
        return none<Hit3D>();

    f32 hits[6] = {
        (box.begin.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.begin.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.begin.z - line.begin.z) / (line.end.z - line.begin.z),
        (box.end.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.end.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.end.z - line.begin.z) / (line.end.z - line.begin.z),
    };

    constexpr Vec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    Vec3 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!containsPointBox(line.begin + hits[i] * (line.end - line.begin), box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return none<Hit3D>();

    return some<Hit3D>(t, norm);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
Maybe<Hit3D> intersectLineTri(Line3D line, Tri tri)
{
    if (vecEq3(line.begin, line.end))
        return none<Hit3D>();

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return none<Hit3D>();

    Vec3 lineDir = line.end - line.begin;

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(lineDir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return none<Hit3D>();

    Vec3 s = line.begin - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return none<Hit3D>();

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(lineDir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return none<Hit3D>();

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, a < 0
        ? vecNorm3(vecCross3(e2, e1))
        : vecNorm3(vecCross3(e1, e2)));
}

Maybe<Hit3D> intersectLinePlane(Line3D line, Plane plane)
{
    if (line.begin == line.end)
        return none<Hit3D>();

    HG_ASSERT(plane.normal != Vec3{0});

    Vec3 lineDir = line.end - line.begin;

    f32 denom = vecDot3(lineDir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return none<Hit3D>();

    f32 t = (plane.dist - vecDot3(line.begin, plane.normal)) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return none<Hit3D>();

    return some<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

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
    f32 rot = 2.0f * static_cast<f32>(HG_PI) * noiseNorm2D(seed, pos);
    return Vec2(cosf(rot), sinf(rot));
}

u32 trueRandom()
{
    static std::random_device trueRandom{};
    return trueRandom();
}

void rngSeed(Rng* rng, u32 seed)
{
    rng->seed = seed;
}

u32 rngNext(Rng* rng)
{
    return rng->pos = noise(rng->seed, rng->pos);
}

u64 rngNext64(Rng* rng)
{
    return (static_cast<u64>(rngNext(rng)) << 32) | static_cast<u64>(rngNext(rng));
}

const char* serialTypeToString(SerialType s)
{
    switch (s)
    {
        case SerialType_object:
            return "SerialType_object";
        case SerialType_string:
            return "SerialType_string";
        case SerialType_integer:
            return "SerialType_integer";
        case SerialType_floating:
            return "SerialType_floating";
        case SerialType_boolean:
            return "SerialType_boolean";
        default:
            return "invalid SerialType";
    }
}

Serializer serialWriter(Arena* arena)
{
    Serializer s{};
    s.arena = arena;
    s.root = arena->alloc<SerialNode>(1);
    s.root->parent = nullptr;
    s.root->next = nullptr;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = true;
    return s;
}

Serializer serialReader(Arena* arena, SerialNode* begin)
{
    Serializer s{};
    s.arena = arena;
    s.root = begin;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = false;
    return s;
}

void serializeNodeStart(Serializer* s)
{
    if (s->writing)
    {
        if (s->current != nullptr)
        {
            s->current->next = s->arena->alloc<SerialNode>(1);
            s->current = s->current->next;
            s->current->parent = s->parent;
            s->current->next = nullptr;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = s->arena->alloc<SerialNode>(1);
                s->parent->children = s->current;
                s->current->parent = s->parent;
                s->current->next = nullptr;
            }
            else
            {
                HG_ASSERT(s->root != nullptr);
                s->current = s->root;
            }
        }

        if (s->parent != nullptr)
            ++s->parent->count;
    }
    else
    {
        if (s->current != nullptr)
        {
            s->current = s->current->next;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = s->parent->children;
            }
            else
            {
                HG_ASSERT(s->root != nullptr);
                s->current = s->root;
            }
        }
    }
}

void serializeBegin(Serializer* s, u32* size)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_object;
        s->current->count = 0;
        s->current->children = nullptr;
    }
    else
    {
        if (size != nullptr)
            *size = s->current->count;
    }

    s->parent = s->current;
    s->current = nullptr;
}

void serializeEnd(Serializer* s)
{
    HG_ASSERT(s->parent != nullptr);

    s->current = s->parent;
    s->parent = s->parent->parent;
}

void serializeVoid(Serializer* s, void* val, u32 size)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = StringBuilder{s->arena, {static_cast<char*>(val), size}};
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        HG_ASSERT(s->current->string.length == size);
        memcpy(val, s->current->string.chars, size);
    }
}

template<typename T>
static void serializeInt(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_integer;
        s->current->integer = static_cast<i64>(*val);
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_integer);
        *val = static_cast<T>(s->current->integer);
    }
}

template<>
void serialize(Serializer* s, u8* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u16* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u32* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u64* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i8* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i16* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i32* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i64* val)
{
    serializeInt(s, val);
}

template<typename T>
static void serializeFloat(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_floating;
        s->current->floating = static_cast<f64>(*val);
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_floating);
        *val = static_cast<T>(s->current->floating);
    }
}

template<>
void serialize(Serializer* s, f32* val)
{
    serializeFloat(s, val);
}

template<>
void serialize(Serializer* s, f64* val)
{
    serializeFloat(s, val);
}

template<>
void serialize(Serializer* s, bool* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_boolean;
        s->current->boolean = *val;
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_boolean);
        *val = s->current->boolean;
    }
}

template<>
void serialize(Serializer* s, Vec2* val)
{
    serializeObject(s,
        &val->x,
        &val->y);
}

template<>
void serialize(Serializer* s, Vec3* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void serialize(Serializer* s, Vec4* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void serialize(Serializer* s, Mat2* val)
{
    serializeObject(s,
        &val->x,
        &val->y);
}

template<>
void serialize(Serializer* s, Mat3* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void serialize(Serializer* s, Mat4* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void serialize(Serializer* s, Complex* val)
{
    serializeObject(s,
        &val->r,
        &val->i);
}

template<>
void serialize(Serializer* s, Quat* val)
{
    serializeObject(s,
        &val->r,
        &val->i,
        &val->j,
        &val->k);
}

static constexpr char serialBinTag[] = "Data";
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)] = {};
    u32 versionMajor = 0;
    u32 versionMinor = 0;
    u32 versionPatch = 0;

    u32 nodeBegin = 0;
};

struct SerialBinObject {
    u32 fieldCount;
    u32 fieldsBegin;
};

struct SerialBinString {
    u32 begin;
    u32 length;
};

struct SerialBinNode {
    SerialType type = {};
    union {
        SerialBinObject object;
        SerialBinString string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

static void serialBinWriteNode(BinaryBuilder* bin, u32 idx, SerialNode* node);

static void serialBinWriteString(BinaryBuilder* bin, u32 idx, StringView string)
{
    SerialBinNode node{};
    node.type = SerialType_string;
    node.string.length = static_cast<u32>(string.length);
    node.string.begin = static_cast<u32>(bin->size);

    bin->overwrite(idx, node);
    bin->append(string.chars, string.length);
}

static void serialBinWriteInteger(BinaryBuilder* bin, u32 idx, i64 integer)
{
    SerialBinNode node{};
    node.type = SerialType_integer;
    node.integer = integer;
    bin->overwrite(idx, node);
}

static void serialBinWriteFloating(BinaryBuilder* bin, u32 idx, f64 floating)
{
    SerialBinNode node{};
    node.type = SerialType_floating;
    node.floating = floating;
    bin->overwrite(idx, node);
}

static void serialBinWriteBoolean(BinaryBuilder* bin, u32 idx, bool boolean)
{
    SerialBinNode node{};
    node.type = SerialType_boolean;
    node.boolean = boolean;
    bin->overwrite(idx, node);
}

static void serialBinWriteObject(BinaryBuilder* bin, u32 idx, SerialNode* object)
{
    SerialBinNode node{};
    node.type = SerialType_object;
    node.object.fieldCount = object->count;

    node.object.fieldsBegin = static_cast<u32>(bin->size);
    bin->resize(bin->size + object->count * sizeof(SerialBinNode));

    bin->overwrite(idx, node);

    SerialNode* data = object->children;
    for (u32 i = 0; i < object->count; ++i)
    {
        serialBinWriteNode(bin, node.object.fieldsBegin + i * static_cast<u32>(sizeof(SerialBinNode)), data);
        data = data->next;
    }
}

static void serialBinWriteNode(BinaryBuilder* bin, u32 idx, SerialNode* node)
{
    switch (node->type)
    {
        case SerialType_object:
            serialBinWriteObject(bin, idx, node);
            return;
        case SerialType_string:
            serialBinWriteString(bin, idx, node->string);
            return;
        case SerialType_integer:
            serialBinWriteInteger(bin, idx, node->integer);
            return;
        case SerialType_floating:
            serialBinWriteFloating(bin, idx, node->floating);
            return;
        case SerialType_boolean:
            serialBinWriteBoolean(bin, idx, node->boolean);
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node->type));
    }
}

BinaryView binaryWriteSerial(Arena* arena, Serializer* serial)
{
    BinaryBuilder bin{arena, sizeof(SerialBinHeader)};

    SerialBinHeader header{};
    memcpy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;
    header.nodeBegin = static_cast<u32>(bin.size);
    bin.overwrite(0, header);

    bin.resize(bin.size + sizeof(SerialBinNode));
    serialBinWriteNode(&bin, header.nodeBegin, serial->current);
    return bin;
}

static void serialBinReadNode(BinaryView bin, u32 idx, Serializer* s);

static void serialBinReadObject(BinaryView bin, SerialBinObject object, Serializer* s)
{
    serializeBegin(s);
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        serialBinReadNode(bin, object.fieldsBegin + i * static_cast<u32>(sizeof(SerialBinNode)), s);
    }
    serializeEnd(s);
}

static void serialBinReadString(BinaryView bin, SerialBinString string, Serializer* s)
{
    StringView val = {static_cast<const char*>(bin.data) + string.begin, string.length};
    serialize(s, &val);
}

static void serialBinReadNode(BinaryView bin, u32 idx, Serializer* s)
{
    SerialBinNode node = bin.read<SerialBinNode>(idx);
    switch (node.type)
    {
        case SerialType_object:
            serialBinReadObject(bin, node.object, s);
            return;
        case SerialType_string:
            serialBinReadString(bin, node.string, s);
            return;
        case SerialType_integer:
            serialize(s, &node.integer);
            return;
        case SerialType_floating:
            serialize(s, &node.floating);
            return;
        case SerialType_boolean:
            serialize(s, &node.boolean);
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node.type));
    }
}

Serializer binaryReadSerial(Arena* arena, BinaryView bin)
{
    SerialBinHeader header = bin.read<SerialBinHeader>(0);

    if (memcmp(header.tag, serialBinTag, sizeof(serialBinTag)) != 0)
    {
        HG_WARN("Serial binary could not be read, does not have a header\n");
        return {};
    }
    else if (header.versionMajor != serialBinVersionMajor)
    {
        HG_WARN("Serial binary has wrong major version: %d instead of %d", header.versionMajor, serialBinVersionMajor);
    }
    else if (header.versionMinor != serialBinVersionMinor)
    {
        HG_WARN("Serial binary has wrong minor version: %d instead of %d", header.versionMinor, serialBinVersionMinor);
    }
    else if (header.versionPatch != serialBinVersionPatch)
    {
        HG_WARN("Serial binary has wrong patch version: %d instead of %d", header.versionPatch, serialBinVersionPatch);
    }

    Serializer s = serialWriter(arena);
    serialBinReadNode(bin, header.nodeBegin, &s);
    return serialReader(arena, s.current);
}

static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node);

static void serialJsonWriteString(StringBuilder* str, StringView string)
{
    str->append('"');
    for (u32 i = 0; i < string.length; ++i)
    {
        switch (string[i])
        {
        case '\\':
            str->append("\\\\");
            break;
        case '\"':
            str->append("\\\"");
            break;
        case '\n':
            str->append("\\n");
            break;
        case '\r':
            str->append("\\r");
            break;
        case '\f':
            str->append("\\f");
            break;
        case '\b':
            str->append("\\b");
            break;
        default:
            str->append(string[i]);
            break;
        }
    }
    str->append('"');
}

static void serialJsonWriteArray(StringBuilder* str, u32 indentation, SerialNode* node)
{
    if (node->count > 0)
    {
        str->append("[\n");

        SerialNode* elem = node->children;

        for (u32 i = 0; i < indentation + 1; ++i)
        {
            str->append("    ");
        }
        serialJsonWriteNode(str, indentation + 1, elem);

        elem = elem->next;

        for (u32 i = 1; i < node->count; ++i)
        {
            str->append(",\n");
            for (u32 i = 0; i < indentation + 1; ++i)
            {
                str->append("    ");
            }
            serialJsonWriteNode(str, indentation + 1, elem);

            elem = elem->next;
        }

        str->append( '\n');
        for (u32 i = 0; i < indentation; ++i)
        {
            str->append("    ");
        }
        str->append(']');
    }
    else
    {
        str->append("[]");
    }
}

// static void serialJsonWriteObject(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node)
// {
//     if (node->count > 0)
//     {
//         stringAppend(arena, str, "{\n");
//
//         SerialNode* field = node->children;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         serialJsonWriteString(arena, str, field->name);
//         stringAppend(arena, str, " : ");
//         serialJsonWriteNode(arena, str, indentation + 1, field);
//
//         field = field->next;
//
//         for (u32 i = 1; i < node->count; ++i)
//         {
//             stringAppend(arena, str, ",\n");
//             for (u32 i = 0; i < indentation + 1; ++i)
//             {
//                 stringAppend(arena, str, "    ");
//             }
//             serialJsonWriteString(arena, str, field->name);
//             stringAppend(arena, str, " : ");
//             serialJsonWriteNode(arena, str, indentation + 1, field);
//
//             field = field->next;
//         }
//
//         stringAppendC(arena, str, '\n');
//         for (u32 i = 0; i < indentation; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         stringAppendC(arena, str, '}');
//     }
//     else
//     {
//         stringAppend(arena, str, "{}");
//     }
// }

static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node)
{
    switch (node->type)
    {
        case SerialType_object:
            serialJsonWriteArray(str, indentation, node);
            return;
        case SerialType_string:
            serialJsonWriteString(str, node->string);
            return;
        case SerialType_integer:
            str->append(integerToString(getScratch(), node->integer));
            return;
        case SerialType_floating:
            str->append(floatToString(getScratch(), node->floating, 6));
            return;
        case SerialType_boolean:
            if (node->boolean)
                str->append("true");
            else
                str->append("false");
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node->type));
    }
}

StringView jsonWriteSerial(Arena* arena, Serializer* serial)
{
    StringBuilder str{arena};
    serialJsonWriteNode(&str, 0, serial->current);
    str.append('\n');
    return str;
}

// Serializer jsonReadSerial(Arena* arena, StringView json)
// {
// }

struct JsonParseState {
    StringView text = {};
    u64 head = 0;
    u64 line = 0;
};

static Json jsonParseNext(Arena* arena, JsonParseState* state);
static Json jsonParseStruct(Arena* arena, JsonParseState* state);
static Json jsonParseArray(Arena* arena, JsonParseState* state);
static Json jsonParseString(Arena* arena, JsonParseState* state);
static Json jsonParseNumber(Arena* arena, JsonParseState* state);
static Json jsonParseBoolean(Arena* arena, JsonParseState* state);

static Json jsonParseNext(Arena* arena, JsonParseState* state)
{
    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
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
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(arena, static_cast<i64>(state->line)));
            msg.append(", found unexpected token \"}\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
        case ']': {
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(arena, static_cast<i64>(state->line)));
            msg.append(", found unexpected token \"]\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
    }
    if (isNumeral(state->text[state->head]))
    {
        return jsonParseNumber(arena, state);
    }

    JsonError* error = arena->alloc<JsonError>(1);
    error->next = nullptr;

    u64 begin = state->head;
    while (state->head < state->text.length && !isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    StringBuilder msg{arena};
    msg.append("on line ");
    msg.append(integerToString(arena, static_cast<i64>(state->line)));
    msg.append(", found unexpected token \"");
    msg.append({&state->text[begin], &state->text[state->head]});
    msg.append("\"\n");
    error->msg = msg;

    return {nullptr, error};
}

static Json jsonParseStruct(Arena* arena, JsonParseState* state)
{
    Json json{};
    json.file = arena->alloc<JsonNode>(1);
    json.file->type = JsonType::JsonType_struct;
    json.file->jstruct.fields = nullptr;

    JsonField* lastField = nullptr;
    JsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
            msg.append(", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == ']')
        {
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
            msg.append(", struct ends with \"]\" instead of \"}\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
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
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        Json value = jsonParseNext(arena, state);

        if (value.file != nullptr)
        {
            if (value.file->type != JsonType::JsonType_field)
            {
                JsonError* error = arena->alloc<JsonError>(1);
                error->next = nullptr;
                StringBuilder msg{arena};
                msg.append("on line ");
                msg.append(integerToString(arena, static_cast<i64>(state->line)));
                msg.append(", struct has a literal instead of a field\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else if (value.file->field.value == nullptr)
            {
                JsonError* error = arena->alloc<JsonError>(1);
                error->next = nullptr;
                StringBuilder msg{arena};
                msg.append("on line ");
                msg.append(integerToString(arena, static_cast<i64>(state->line)));
                msg.append(", struct has a field named \"");
                msg.append(value.file->field.name);
                msg.append("\" which has no value\n");
                error->msg = msg;
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

static Json jsonParseArray(Arena* arena, JsonParseState* state)
{
    Json json{};
    json.file = arena->alloc<JsonNode>(1);
    json.file->type = JsonType::JsonType_array;

    JsonType type = JsonType::JsonType_none;
    JsonElem* lastElem = nullptr;
    JsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
            msg.append(", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == '}')
        {
            JsonError* error = arena->alloc<JsonError>(1);
            error->next = nullptr;
            StringBuilder msg{arena};
            msg.append("on line ");
            msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
            msg.append(", array ends with \"}\" instead of \"]\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
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
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        JsonElem* elem = arena->alloc<JsonElem>(1);
        elem->next = nullptr;

        Json value = jsonParseNext(arena, state);
        elem->value = value.file;

        if (value.file != nullptr)
        {
            if (type == JsonType::JsonType_none)
            {
                if (value.file->type != JsonType::JsonType_field)
                {
                    type = value.file->type;
                } else {
                    JsonError* error = arena->alloc<JsonError>(1);
                    error->next = nullptr;
                    StringBuilder msg{arena};
                    msg.append("on line ");
                    msg.append(integerToString(arena, static_cast<i64>(state->line)));
                    msg.append(", array has a field as an element\n");
                    error->msg = msg;
                    if (lastError == nullptr)
                        json.errors = lastError = error;
                    else
                        lastError->next = error;
                    lastError = error;
                }
            }
            if (value.file->type != type)
            {
                JsonError* error = arena->alloc<JsonError>(1);
                error->next = nullptr;
                StringBuilder msg{arena};
                msg.append("on line ");
                msg.append(integerToString(arena, static_cast<i64>(state->line)));
                msg.append(", array has element which is not the same type as the first valid element\n");
                error->msg = msg;
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

static Json jsonParseString(Arena* arena, JsonParseState* state)
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
        StringBuilder str{};
        for (u64 i = begin; i < end; ++i)
        {
            char c = state->text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            str.append(c);
        }

        Json json{};
        json.file = arena->alloc<JsonNode>(1);

        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ':')
        {
            ++state->head;
            json.file->type = JsonType::JsonType_field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            Json next = jsonParseNext(arena, state);
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = JsonType::JsonType_string;
            json.file->string = str;
        }
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;
        return json;
    }

    JsonError* error = arena->alloc<JsonError>(1);
    StringBuilder msg{arena};
    msg.append("on line ");
    msg.append(integerToString(arena, static_cast<i64>(state->line)));
    msg.append(", expected string to terminate\n");
    error->msg = msg;
    return {nullptr, error};
}

static Json jsonParseNumber(Arena* arena, JsonParseState* state)
{
    bool isNumFloat = false;
    u64 begin = state->head;
    while (state->head < state->text.length && (
        isNumeral(state->text[state->head]) ||
        state->text[state->head] == '-' ||
        state->text[state->head] == '+' ||
        state->text[state->head] == '.' ||
        state->text[state->head] == 'e'
    ))
    {
        if (state->text[state->head] == '.' || state->text[state->head] == 'e')
            isNumFloat = true;
        ++state->head;
    }
    StringView num{&state->text[begin], &state->text[state->head]};
    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head < state->text.length && state->text[state->head] == ',')
        ++state->head;

    if (isNumFloat)
    {
        if (isFloat(num))
        {
            JsonNode* node = arena->alloc<JsonNode>(1);
            node->type = JsonType::JsonType_float;
            node->floating = stringToFloat(num);
            return {node, nullptr};
        }
    } else {
        if (isInteger(num))
        {
            JsonNode* node = arena->alloc<JsonNode>(1);
            node->type = JsonType::JsonType_integer;
            node->integer = stringToInteger(num);
            return {node, nullptr};
        }
    }

    JsonError* error = arena->alloc<JsonError>(1);

    StringBuilder msg{arena};
    msg.append("on line ");
    msg.append(integerToString(arena, static_cast<i64>(state->line)));
    msg.append(", expected numeral value, found \"");
    msg.append(num);
    msg.append("\"\n");
    error->msg = msg;

    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        Json next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

static Json jsonParseBoolean(Arena* arena, JsonParseState* state)
{
    if (state->head + 4 <= state->text.length && StringView{&state->text[state->head], 4} == "true")
    {
        state->head += 4;
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        JsonNode* node = arena->alloc<JsonNode>(1);
        node->type = JsonType::JsonType_bool;
        node->boolean = true;
        return {node, nullptr};
    }
    if (state->head + 5 <= state->text.length && StringView{&state->text[state->head], 5} == "false")
    {
        state->head += 5;
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        JsonNode* node = arena->alloc<JsonNode>(1);
        node->type = JsonType::JsonType_bool;
        node->boolean = false;
        return {node, nullptr};
    }

    JsonError* error = arena->alloc<JsonError>(1);

    u64 begin = state->head;
    while (state->head < state->text.length && !isWhitespace(state->text[state->head])
        && state->text[state->head] != ','
        && state->text[state->head] != '}'
        && state->text[state->head] != ']'
    )
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    StringBuilder msg{arena};
    msg.append("on line ");
    msg.append(integerToString(arena, static_cast<i64>(state->line)));
    msg.append(", expected boolean value, found \"");
    msg.append({&state->text[begin], &state->text[state->head]});
    msg.append("\"\n");
    error->msg = msg;

    if (state->text[state->head] == ',')
        ++state->head;

    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        Json next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

Json parseJson(Arena* arena, StringView text)
{
    HG_ASSERT(arena != nullptr);
    if (text.length > 0)
        HG_ASSERT(text.chars != nullptr);

    JsonParseState parseState{};
    parseState.text = text;
    parseState.head = 0;
    parseState.line = 1;
    return jsonParseNext(arena, &parseState);
}

char* cString(Arena* arena, StringView str)
{
    HG_ASSERT(arena != nullptr);
    if (str.length > 0)
        HG_ASSERT(str.chars != nullptr);

    char* cStr = arena->alloc<char>(str.length + 1);
    if (str.length > 0)
        memcpy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

void StringBuilder::insert(u64 idx, StringView src)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(idx <= length);
    if (src.length > 0)
        HG_ASSERT(src.chars != nullptr);

    u64 newLength = length + src.length;

    if (!arena->extend(chars, length, newLength))
    {
        char* newChars = arena->alloc<char>(newLength);
        if (length > 0)
            memcpy(newChars, chars, length);
        chars = newChars;
    }

    if (idx != length)
        memmove(&chars[idx + src.length], &chars[idx], length - idx);
    memcpy(&chars[idx], src.chars, src.length);

    length = newLength;
}

String String::create(StringView data)
{
    String str;
    str.chars = heapAlloc<char>(data.length);
    str.length = data.length;
    memcpy(str.chars, data.chars, data.length);
    return str;
}

String::~String() noexcept
{
    if (chars != nullptr)
        heapFree(chars, length);
}

template<>
void serialize(Serializer* s, String* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = StringBuilder{s->arena, *val};
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        *val = String::create(s->current->string);
    }
}

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool isInteger(StringView str)
{
    if (str.length == 0)
        return false;

    u64 head = 0;
    if (!isNumeral(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '+' || str[head] == '-')
    {
        ++head;
        if (head >= str.length)
            return false;
    }

    while (head < str.length)
    {
        if (!isNumeral(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool isFloat(StringView str)
{
    if (str.length == 0)
        return false;

    bool hasDecimal = false;
    bool hasExponent = false;
    bool hasDigit = false;

    u64 head = 0;

    if (!isNumeral(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (isNumeral(str[head]))
        hasDigit = true;

    if (str[head] == '.')
        hasDecimal = true;

    ++head;
    while (head < str.length)
    {
        if (isNumeral(str[head]))
        {
            hasDigit = true;
            ++head;
            continue;
        }

        if (str[head] == '.' && !hasDecimal && !hasExponent)
        {
            hasDecimal = true;
            ++head;
            continue;
        }

        if (str[head] == 'e' && !hasExponent)
        {
            hasExponent = true;
            ++head;
            if (isNumeral(str[head]) || str[head] == '+' || str[head] == '-')
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

    return hasDigit && (hasDecimal || hasExponent);
}

i64 stringToInteger(StringView str)
{
    HG_ASSERT(isInteger(str));

    i64 power = 1;
    i64 ret = 0;

    u64 head = str.length - 1;
    while (head > 0)
    {
            ret += static_cast<i64>(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+')
    {
        if (str[head] == '-')
            ret *= -1;
        else
        ret += static_cast<i64>(str[head] - '0') * power;
    }

    return ret;
}

f64 stringToFloat(StringView str)
{
    HG_ASSERT(isFloat(str));

    f64 ret = 0.0;
    u64 head = 0;

    bool isNegative = str[head] == '-';
    if (isNegative || str[head] == '+')
        ++head;

    if (isNumeral(str[head]))
    {
        u64 intPartBegin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e')
        {
            ++head;
        }
        ret += static_cast<f64>(stringToInteger({&str[intPartBegin], &str[head]}));
    }

    if (head < str.length && str[head] == '.')
    {
        ++head;

        f64 power = 0.1;
        while (head < str.length && isNumeral(str[head]))
        {
            ret += static_cast<f64>(str[head] - '0') * power;
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
        while (head < str.length && isNumeral(str[head]))
        {
            ++head;
        }

        i64 exp = stringToInteger({&str[expBegin], str.chars + head});
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

StringBuilder integerToString(Arena* arena, i64 num)
{
    HG_ASSERT(arena != nullptr);

    ArenaScope scratch = getScratch(&arena, 1);

    if (num == 0)
        return {arena, "0"};

    bool isNegative = num < 0;
    u64 unum = static_cast<u64>(std::abs(num));

    StringBuilder reverse{scratch};
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = static_cast<u64>(static_cast<f64>(unum) / 10.0);
        reverse.append('0' + static_cast<char>(digit));
    }

    StringBuilder ret{arena};
    if (isNegative)
        ret.append('-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        ret.append(reverse[i]);
    }
    return ret;
}

StringBuilder floatToString(Arena* arena, f64 num, u32 decimalCount)
{
    HG_ASSERT(arena != nullptr);

    ArenaScope scratch = getScratch(&arena, 1);

    if (num == 0.0)
        return {arena, "0.0"};

    StringBuilder intStr = integerToString(scratch, static_cast<i64>(std::abs(num)));

    StringBuilder decStr{scratch};
    decStr.append('.');

    f64 decPart = std::abs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        decStr.append('0' + static_cast<char>(static_cast<u64>(decPart) % 10));
    }

    StringBuilder ret{arena};
    if (num < 0.0)
        ret.append('-');
    ret.append(intStr);
    ret.append(decStr);
    return ret;
}

void BinaryBuilder::read(u64 idx, void* dst, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}

void BinaryBuilder::resize(u64 newSize)
{
    HG_ASSERT(arena != nullptr);
    if (!arena->extend(data, size, newSize))
    {
        char* newData = arena->alloc<char>(newSize);
        memcpy(newData, data, size);
        data = newData;
    }
    size = newSize;
}

void BinaryBuilder::overwrite(u64 idx, const void* src, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(static_cast<u8*>(data) + idx, src, len);
}

void BinaryBuilder::append(const void* src, u64 len)
{
    HG_ASSERT(arena != nullptr);
    if (!arena->extend(data, size, size + len))
    {
        char* newData = arena->alloc<char>(size + len);
        memcpy(newData, data, size);
        data = newData;
    }
    memcpy(static_cast<u8*>(data) + size, src, len);
    size += len;
}

void Binary::read(u64 idx, void* dst, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}

Binary Binary::create(BinaryView data)
{
    Binary bin;
    bin.size = data.size;
    bin.data = heapAlloc(bin.size, 1);
    if (bin.size > 0)
        memcpy(bin.data, data.data, bin.size);
    return bin;
}

Binary::~Binary() noexcept
{
    if (data != nullptr)
        heapFree(data, size);
}

template<>
void serialize(Serializer* s, Binary* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = StringBuilder{s->arena, {static_cast<char*>(val->data), val->size}};
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        *val = Binary::create({s->current->string.chars, s->current->string.length});
    }
}

HandlePool handlePoolCreate()
{
    HandlePool handles{};
    handles.handles = Array<Handle>{0, 1024};
    handles.freed = Array<Handle>{0, 1024};

    handlePoolAlloc(&handles);

    return handles;
}

void handlePoolDestroy(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    pool->handles = {};
    pool->freed = {};
}

void handlePoolReset(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    pool->handles.count = 0;
    pool->freed.count = 0;

    handlePoolAlloc(pool);
}

Handle handlePoolAlloc(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    if (pool->freed.count > 0)
    {
        Handle handle = pool->freed.pop();
        pool->handles[handleIdx(handle)] = handle;
        return handle;
    }
    else
    {
        Handle handle = {pool->handles.count};
        pool->handles.push(handle);
        return handle;
    }
}

bool handlePoolAlive(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);

    u32 idx = handleIdx(handle);
    return handle != handleNull && idx < pool->handles.count && pool->handles[idx] == handle;
}

void handlePoolFree(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);
    HG_ASSERT(handlePoolAlive(pool, handle));
    pool->handles[handleIdx(handle)] = handleNull;
    pool->freed.push(handleNextGeneration(handle));
}

template<>
void assetLoadImpl(AssetData<Binary>* data)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        setError("Could not find file to read binary: %s", cpath);
        return;
    }
    HG_DEFER(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        setError("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = static_cast<u64>(ftell(fileHandle));
    data->asset.data = heapAlloc(data->asset.size, 1);

    rewind(fileHandle);
    if (fread(const_cast<void*>(data->asset.data), 1, data->asset.size, fileHandle) != data->asset.size)
    {
        heapFree(const_cast<void*>(data->asset.data), data->asset.size);
        setError("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

template<>
void assetUnloadImpl(AssetData<Binary>* data)
{
    data->asset = {};
}

bool binaryStore(BinaryView bin, StringView path)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        setError("Failed to create file to write binary: %s", cpath);
        return false;
    }
    HG_DEFER(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        setError("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

f64 Clock::tick()
{
    f64 prev = time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    time = static_cast<f64>(ts.tv_sec) + static_cast<f64>(ts.tv_nsec) * 1e-9;
    return time - prev;
}

void sleep(f64 time)
{
    std::this_thread::sleep_for(std::chrono::duration<f64>(time));
}

Perf perfCreate(Arena* arena, u32 count)
{
    HG_ASSERT(arena != nullptr);

    Perf perf;
    perf.times = arena->alloc<f64>(count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void perfBegin(Perf* perf)
{
    HG_ASSERT(perf != nullptr);
    perf->clock.tick();
}

f64 perfEnd(Perf* perf)
{
    HG_ASSERT(perf != nullptr);
    HG_ASSERT(perf->current < perf->count);

    f64 time = perf->clock.tick();
    perf->times[perf->current++] = time;

    return time;
}

PerfStats perfAnalyze(const Perf* perf)
{
    HG_ASSERT(perf != nullptr);

    PerfStats stats;
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
    stats.avg /= static_cast<f64>(perf->current);

    return stats;
}

void perfLog(StringView title, const PerfStats* stats, PerfScale scale)
{
    HG_ASSERT(stats != nullptr);
    if (title.length == 0 || title.chars == nullptr)
        title = "Title Missing";

    switch (scale)
    {
        case PerfScale_seconds:
            printf("HG Performance - %.*s: avg: %.4fs, best: %.4fs, worst: %.4fs\n",
                static_cast<int>(title.length), title.chars, stats->avg, stats->best, stats->worst);
            break;
        case PerfScale_milli:
            printf("HG Performance - %.*s: avg: %.4fms, best: %.4fms, worst: %.4fms\n",
                static_cast<int>(title.length), title.chars, stats->avg * 1.e3, stats->best * 1.e3, stats->worst * 1.e3);
            break;
        case PerfScale_micro:
            printf("HG Performance - %.*s: avg: %.4fmcs, best: %.4fmcs, worst: %.4fmcs\n",
                static_cast<int>(title.length), title.chars, stats->avg * 1.e6, stats->best * 1.e6, stats->worst * 1.e6);
            break;
        case PerfScale_nano:
            printf("HG Performance - %.*s: avg: %.4fns, best: %.4fns, worst: %.4fns\n",
                static_cast<int>(title.length), title.chars, stats->avg * 1.e9, stats->best * 1.e9, stats->worst * 1.e9);
            break;
    }
}

template<>
void assetLoadImpl(AssetData<Sound>* data)
{
    static_cast<void>(data);
    HG_PANIC("Load audio file impl : TODO\n");
}

AudioPlayer audioPlayerCreate()
{
    AudioPlayer player{};
    player.music = Array<AudioPlayerMusic>{0, 1024};
    player.sounds = Array<AudioStream*>{0, 1024};
    return player;
}

void audioPlayerDestroy(AudioPlayer* player)
{
    HG_ASSERT(player != nullptr);

    for (u32 i = 0; i < player->sounds.count; ++i)
    {
        audioStreamDestroy(player->sounds[i]);
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        audioStreamDestroy(player->music[i].stream);
    }

    player->sounds = {};
    player->music = {};
}

void audioPlayerUpdate(AudioPlayer* player)
{
    for (u32 i = player->sounds.count - 1; i < player->sounds.count; --i)
    {
        if (audioStreamQueuedSize(player->sounds[i]) == 0)
        {
            AudioStream* stream = player->sounds.removeShift(i);
            audioStreamDestroy(stream);
        }
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        AudioPlayerMusic* music = &player->music[i];
        if (!music->playing)
            continue;

        ArenaScope scratch = getScratch();

        u32 width = sizeof(f32);

        u32 total = music->sound->frequency * width / 16;
        u32 queued = audioStreamQueuedSize(music->stream);
        if (queued >= total)
            continue;
        u32 sizeToPush = total - queued;

        f32* queue = static_cast<f32*>(scratch.alloc(sizeToPush, width));
        u32 queueSize = 0;

        while (queueSize < sizeToPush)
        {
            if (music->pos == music->sound->size)
                music->pos = 0;

            u32 sizeToQueue = std::min(sizeToPush - queueSize, static_cast<u32>(music->sound->size - music->pos));
            memcpy(reinterpret_cast<u8*>(queue) + queueSize, reinterpret_cast<u8*>(music->sound->data) + music->pos, sizeToQueue);
            queueSize += sizeToQueue;
            music->pos += sizeToQueue;
        }

        HG_ASSERT(queueSize <= sizeToPush);
        audioStreamPush(music->stream, queue, queueSize);
    }
}

void audioPlayerMusic(AudioPlayer* player, Asset<Sound>* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == *music)
        {
            player->music[i].playing = true;
            return;
        }
    }

    AudioPlayerMusic* track = player->music.push();
    track->stream = audioStreamCreate((*music)->frequency, (*music)->channels);
    track->sound = music->clone();
    track->pos = 0;
    track->playing = true;
}

void audioPlayerMusicKill(AudioPlayer* player, Asset<Sound>* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == *music)
        {
            AudioPlayerMusic track = player->music.removeShift(i);
            audioStreamDestroy(track.stream);
            return;
        }
    }
}

void audioPlayerMusicPause(AudioPlayer* player, Asset<Sound>* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == *music)
        {
            player->music[i].playing = false;
            return;
        }
    }
}

void audioPlayerSetMusicGain(AudioPlayer* player, Asset<Sound>* music, f32 gain)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == *music)
        {
            audioStreamSetGain(player->music[i].stream, gain);
            return;
        }
    }
}

void audioPlayerSound(AudioPlayer* player, Asset<Sound>* sound, f32 gain)
{
    AudioStream** stream = player->sounds.push();
    *stream = audioStreamCreate((*sound)->frequency, (*sound)->channels);
    audioStreamSetGain(*stream, gain);
    audioStreamPush(*stream, (*sound)->data, (*sound)->size);
}

// AudioSource* audioSourceAdd(Ecs* ecs, Entity e, Asset<Sound>* audio, bool repeat)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     AudioSource* src = ecsAdd<AudioSource>(ecs, e);
//     *src = {};
//     src->player = audioStreamCreate(8000, 1);
//     src->audio = audio->clone();
//     src->position = 0;
//     src->repeat = repeat;
//
//     return src;
// }

template<>
void assetLoadImpl(AssetData<TextureData>* data)
{
    ArenaScope scratch = getScratch();
    char* cpath = cString(scratch, data->path);

    int x, y, channels;
    data->asset.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->asset.pixels == nullptr)
    {
        setError("Could not load image: %s", cpath);
        return;
    }
    data->asset.width = static_cast<u32>(x);
    data->asset.height = static_cast<u32>(y);
    data->asset.depth = 1;
    data->asset.format = Format_r8g8b8a8_srgb;
}

template<>
void assetUnloadImpl(AssetData<TextureData>* data)
{
    free(data->asset.pixels);
}

bool textureStorePng(TextureData* texture, StringView path)
{
    ArenaScope scratch = getScratch();

    const char* cpath = cString(scratch, path);

    if (!stbi_write_png(
         cpath,
         static_cast<int>(texture->width),
         static_cast<int>(texture->height),
         4,
         texture->pixels,
         static_cast<int>(texture->width * sizeof(u32))))
    {
        setError("Could not store image: %s", cpath);
        return false;
    }
    return true;
}

template<>
void assetLoadImpl(AssetData<Texture>* data)
{
    Asset<TextureData> tex = load<TextureData>(data->path);
    if (tex->pixels == nullptr)
        return;

    GpuImageCreateInfo imageInfo{};
    imageInfo.format = tex->format;
    imageInfo.width = tex->width;
    imageInfo.height = tex->height;
    imageInfo.depth = tex->depth;
    imageInfo.usage = GpuImageUsage_transferDst | GpuImageUsage_sampled;

    data->asset.image = {imageInfo};
    data->asset.view = {data->asset.image, GpuAspect_color, GpuFilter_nearest};

    data->asset.view.write(tex->pixels);
}

template<>
void assetUnloadImpl(AssetData<Texture>*) {}

template<>
void assetLoadImpl(AssetData<MeshData>* data)
{
    static_cast<void>(data);
    HG_PANIC("load gltf file : TODO\n");
}

template<>
void assetUnloadImpl(AssetData<MeshData>* data)
{
    heapFree(data->asset.indices, data->asset.indexCount);
    heapFree(data->asset.vertices, data->asset.vertexCount);
}

void meshStoreGltf(MeshData* data, StringView path, Fence* fence)
{
    static_cast<void>(data);
    static_cast<void>(path);
    static_cast<void>(fence);
    HG_PANIC("store gltf file : TODO\n");
}

template<>
void assetLoadImpl(AssetData<Mesh>* data)
{
    Asset<MeshData> mesh = load<MeshData>(data->path);
    if (mesh->vertices == nullptr || mesh->indices == nullptr)
        return;

    data->asset.vertexCount = mesh->vertexCount;
    data->asset.vertexWidth = mesh->vertexWidth;
    data->asset.indexCount = mesh->indexCount;

    data->asset.vertexBuffer = GpuBuffer{
        data->asset.vertexCount * data->asset.vertexWidth, GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst};

    data->asset.indexBuffer = GpuBuffer{
        data->asset.indexCount * sizeof(u32), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst};

    data->asset.vertexBuffer.write(mesh->vertices, 0, data->asset.vertexCount * data->asset.vertexWidth);
    data->asset.indexBuffer.write(mesh->indices, 0, data->asset.indexCount * sizeof(u32));
}

template<>
void serialize(Serializer* s, Camera* camera)
{
    serializeBegin(s);
    serialize(s, &camera->rotation);
    serialize(s, &camera->position);
    serialize(s, &camera->type);
    if (camera->type == CameraType_perspective)
    {
        serializeObject(s,
            &camera->perspective.aspect,
            &camera->perspective.fov,
            &camera->perspective.near,
            &camera->perspective.far);
    }
    else
    {
        serializeObject(s,
            &camera->orthographic.left,
            &camera->orthographic.right,
            &camera->orthographic.top,
            &camera->orthographic.bottom,
            &camera->orthographic.near,
            &camera->orthographic.far);
    }
    serializeEnd(s);
}

struct VPUniform {
    Mat4 proj = {};
    Mat4 view = {};
};

Camera cameraCreate()
{
    Camera camera{};

    camera.vpBuffer = GpuBuffer{
        sizeof(VPUniform),
        GpuBufferUsage_uniformBuffer,
        GpuMemoryUsage_frequentUpdate};

    camera.rotation = Quat{1.0f};
    camera.position = Vec3{0.0f};

    return camera;
}

void cameraSetPerspective(Camera* camera, f32 aspect, f32 fov, f32 near, f32 far)
{
    camera->type = CameraType_perspective;
    camera->perspective.aspect = aspect;
    camera->perspective.fov = fov;
    camera->perspective.near = near;
    camera->perspective.far = far;
}

void cameraSetOrthographic(Camera* camera, f32 width, f32 height, f32 actualAspect)
{
    camera->type = CameraType_orthographic;
    camera->orthographic.left = 0;
    camera->orthographic.right = width;
    camera->orthographic.top = 0;
    camera->orthographic.bottom = height;
    camera->orthographic.near = 0;
    camera->orthographic.far = 1;

    if (actualAspect != 0.0)
    {
        if (actualAspect > static_cast<f32>(width) / static_cast<f32>(height))
        {
            f32 margin = actualAspect - static_cast<f32>(width) / static_cast<f32>(height);
            camera->orthographic.left -= margin * width / 2.0f;
            camera->orthographic.right += margin * width / 2.0f;
        }
        else
        {
            f32 margin = 1.0f / actualAspect - static_cast<f32>(height) / static_cast<f32>(width);
            camera->orthographic.top -= margin * height / 2.0f;
            camera->orthographic.bottom += margin * height / 2.0f;
        }
    }
}

void cameraUpdate(Camera* camera)
{
    VPUniform vp{};
    vp.view = matView(camera->position, Vec3{1.0f}, camera->rotation);
    if (camera->type == CameraType_perspective)
    {
        vp.proj = matPerspective(
            camera->perspective.fov,
            camera->perspective.aspect,
            camera->perspective.near,
            camera->perspective.far);
    }
    else
    {
        vp.proj = matOrthographic(
            camera->orthographic.left,
            camera->orthographic.right,
            camera->orthographic.top,
            camera->orthographic.bottom,
            camera->orthographic.near,
            camera->orthographic.far);
    }

    camera->vpBuffer.write(&vp, 0, sizeof(vp));
}

// Camera* cameraAdd(Ecs* ecs, Entity e)
// {
//     Camera* camera = ecsAdd<Camera>(ecs, e);
//     *camera = cameraCreate();
//     return camera;
// }
//
// template<>
// void ecsDtor(Camera* camera)
// {
//     cameraDestroy(camera);
// }
//
// void cameraUpdateEcs(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsHas<Camera>(ecs, e));
//     HG_ASSERT(ecsHas<Transform>(ecs, e));
//
//     Camera* camera = ecsGet<Camera>(ecs, e);
//     Transform* tf = ecsGet<Transform>(ecs, e);
//     HG_ASSERT(camera->type == CameraType_perspective || camera->type == CameraType_orthographic);
//
//     VPUniform vp{};
//     vp.view = matModelToView(tf->mat);
//     if (camera->type == CameraType_perspective)
//     {
//         vp.proj = matPerspective(
//             camera->perspective.fov,
//             camera->perspective.aspect,
//             camera->perspective.near,
//             camera->perspective.far);
//     }
//     else
//     {
//         vp.proj = matOrthographic(
//             camera->orthographic.left,
//             camera->orthographic.right,
//             camera->orthographic.top,
//             camera->orthographic.bottom,
//             camera->orthographic.near,
//             camera->orthographic.far);
//     }
//
//     gpuBufferWrite(camera->vpBuffer, 0, &vp, sizeof(vp));
// }

struct RenderState2D {
    GpuPipeline pipeline{};
    GpuPipeline debugPipeline{};
    Texture defaultTex{};
};

static RenderState2D render2D;

struct RenderPush2D {
    Mat4 model = {};
    u32 vpIdx = 0;
    u32 instIdx = 0;
};

#include "render2d.vert.spv.h"
#include "render2d.frag.spv.h"
#include "debug2d.frag.spv.h"

void rendererInit2D(Format colorFormat)
{
    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = render2d_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(render2d_vert_spv);
    pipelineConfig.fragmentShader = render2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(render2d_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    render2D.pipeline = GpuPipeline{pipelineConfig};

    pipelineConfig.fragmentShader = debug2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(debug2d_frag_spv);
    pipelineConfig.topology = GpuTopology_lineStrip;

    render2D.debugPipeline = GpuPipeline{pipelineConfig};

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    render2D.defaultTex.image = GpuImage{2, 2, Format_r8g8b8a8_srgb, GpuImageUsage_sampled | GpuImageUsage_transferDst};
    render2D.defaultTex.view = GpuView{render2D.defaultTex.image, GpuAspect_color, GpuFilter_nearest};
    render2D.defaultTex.view.write(defaultColors);
}

void rendererDeinit2D()
{
    render2D = {};
}

Layer2D layerCreate2D()
{
    Layer2D layer{};

    layer.instances = Array<Render2DInstance>{0, 1024};
    layer.instanceBuffer = GpuBuffer{layer.instances.capacity * sizeof(Render2DInstance),
        GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate};
    layer.instanceCapacity = layer.instances.capacity;
    layer.transform = Mat4{1.0f};
    layer.changed = true;

    return layer;
}

void layerDestroy2D(Layer2D* layer)
{
    HG_ASSERT(layer != nullptr);

    *layer = {};
}

void layerClear2D(Layer2D* layer)
{
    HG_ASSERT(layer != nullptr);

    layer->instances.count = 0;
    layer->changed = true;
}

static void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer, const GpuPipeline& pipeline)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(camera != nullptr);
    HG_ASSERT(layer != nullptr);

    if (layer->changed)
    {
        if (layer->instances.capacity > layer->instanceCapacity)
        {
            gpuWaitIdle();

            layer->instanceBuffer = GpuBuffer{layer->instances.capacity * sizeof(Render2DInstance),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate};
            layer->instanceCapacity = layer->instances.capacity;
        }

        layer->instanceBuffer.write(layer->instances.vals, 0, layer->instances.count * sizeof(Render2DInstance));

        layer->changed = false;
    }

    gpuBindPipeline(cmd, pipeline);

    RenderPush2D push{};
    push.model = layer->transform;
    push.vpIdx = camera->vpBuffer.uniformDescriptor();
    push.instIdx = layer->instanceBuffer.storageDescriptor();

    gpuPushConstants(cmd, pipeline, &push, sizeof(push));

    gpuDraw(cmd, 0, 6, 0, layer->instances.count);
}

void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.pipeline);
}

void renderDebug2D(GpuCmd* cmd, Camera* camera, Layer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.debugPipeline);
}

void drawRect2D(Layer2D* layer, Vec4 color, Rect dst)
{
    HG_ASSERT(layer != nullptr);

    Render2DInstance instance{};
    instance.rect.pos = dst.begin;
    instance.rect.size = dst.end - dst.begin;
    instance.rect.type = Render2DInstanceType_color;
    instance.rect.color = color;

    layer->instances.push(instance);

    layer->changed = true;
}

void drawSprite2D(Layer2D* layer, Sprite2D* sprite, Rect dst)
{
    HG_ASSERT(layer != nullptr);
    HG_ASSERT(sprite != nullptr);

    Texture* texture = sprite->texture == nullptr
        ? &render2D.defaultTex
        : &*sprite->texture;

    Render2DInstance instance{};
    instance.sprite.pos = dst.begin;
    instance.sprite.size = dst.end - dst.begin;
    instance.sprite.type = Render2DInstanceType_sprite;
    instance.sprite.tex = texture->view.samplerDescriptor();
    instance.sprite.uvPos = sprite->uv.begin;
    instance.sprite.uvSize = sprite->uv.end - sprite->uv.begin;

    layer->instances.push(instance);

    layer->changed = true;
}

Atlas2D atlasCreate2D(Asset<Texture>* texture)
{
    HG_ASSERT(texture != nullptr);

    Atlas2D atlas{};
    atlas.texture = texture->clone();
    atlas.sprites = Array<Rect>{0, 1024};
    return atlas;
}

void atlasDestroy2D(Atlas2D* atlas)
{
    HG_ASSERT(atlas != nullptr);
    atlas->sprites = {};
}

u32 atlasAdd2D(Atlas2D* atlas, Rect sprite)
{
    HG_ASSERT(atlas != nullptr);

    u32 idx = atlas->sprites.count;
    atlas->sprites.push(sprite);
    return idx;
}

u32 atlasAddGrid2D(Atlas2D* atlas, Rect grid, u32 width, u32 height)
{
    HG_ASSERT(atlas != nullptr);

    u32 idx = atlas->sprites.count;

    Vec2 spriteSize = (grid.end - grid.begin) / Vec2{static_cast<f32>(width), static_cast<f32>(height)};
    Vec2 pos = grid.begin;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x = grid.begin.x;
        for (u32 x = 0; x < width; ++x)
        {
            atlas->sprites.push({pos, pos + spriteSize});
            pos.x += spriteSize.x;
        }
        pos.y += spriteSize.y;
    }

    return idx;
}

Sprite2D atlasGet2D(Atlas2D* atlas, u32 idx)
{
    HG_ASSERT(atlas != nullptr);

    return {atlas->texture.clone(), atlas->sprites[idx]};
}

Tilemap2D tilemapCreate2D(u32 width, u32 height)
{
    Tilemap2D tilemap{};
    tilemap.tiles = heapAlloc<u32>(width * height);
    tilemap.width = width;
    tilemap.height = height;
    for (u32 i = 0; i < width * height; ++i)
    {
        tilemap.tiles[i] = static_cast<u32>(-1);
    }

    return tilemap;
}

void tilemapDestroy2D(Tilemap2D* tilemap)
{
    HG_ASSERT(tilemap != nullptr);
    heapFree(tilemap->tiles, tilemap->width * tilemap->height);
}

u32 tilemapGet2D(Tilemap2D* tilemap, u32 x, u32 y)
{
    HG_ASSERT(tilemap != nullptr);
    return tilemap->tiles[y * tilemap->width + x];
}

void tilemapSet2D(Tilemap2D* tilemap, u32 x, u32 y, u32 tile)
{
    HG_ASSERT(tilemap != nullptr);
    tilemap->tiles[y * tilemap->width + x] = tile;
}

void drawTilemap2D(Layer2D* layer, Atlas2D* atlas, Tilemap2D* tilemap, Rect dst)
{
    HG_ASSERT(layer != nullptr);
    HG_ASSERT(tilemap != nullptr);

    Vec2 pos = dst.begin;
    Vec2 size = (dst.end - dst.begin) / Vec2{static_cast<f32>(tilemap->width), static_cast<f32>(tilemap->height)};
    for (u32 y = 0; y < tilemap->width; ++y)
    {
        pos.x = dst.begin.x;
        for (u32 x = 0; x < tilemap->height; ++x)
        {
            u32 tile = tilemapGet2D(tilemap, x, y);
            Sprite2D sprite = atlasGet2D(atlas, tile);
            drawSprite2D(layer, &sprite, {pos, pos + size});
            pos.x += size.x;
        }
        pos.y += size.y;
    }
}

// Ecs ecsCreate()
// {
//     Ecs ecs{};
//     ecs.entities = handlePoolCreate();
//     ecs.components = Map<u64, Component>(128);
//     ecsReset(&ecs);
//     return ecs;
// }
//
// void ecsDestroy(Ecs* ecs)
// {
//     ecsReset(ecs);
//
//     ecs->components.forEach([&](u64*, Component* system)
//     {
//         arrayAnyDestroy(&system->components);
//         system->entities = {};
//         system->indices = {};
//         system->name = {};
//     });
//
//     ecs->components = {};
//     handlePoolDestroy(&ecs->entities);
// }
//
// void ecsReset(Ecs* ecs)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     ecs->components.forEach([&](u64*, Component* system)
//     {
//         for (u32 c = 1; c < system->components.count; ++c)
//         {
//             system->dtor(system->components[c]);
//         }
//         system->entities.count = 1;
//         system->components.count = 1;
//         memset(system->indices.vals, 0, system->indices.count * sizeof(*system->indices.vals));
//     });
//     handlePoolReset(&ecs->entities);
// }
//
// void ecsRegisterComponent(Ecs* ecs, EcsRegisterComponent* config)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     u64 id = hash(config->name);
//     HG_ASSERT(ecs->components.get(id) == nullptr);
//
//     if (ecs->components.count * 2 > ecs->components.capacity)
//         ecs->components.resize(ecs->components.capacity * 2);
//     Component* system = ecs->components.add(id, {});
//
//     system->name = String::create(config->name);
//     system->indices = Array<u32>{0, 1024};
//     system->entities = Array<Entity>{0, 1024};
//     system->components = arrayAnyCreate(config->width, config->align);
//     system->dtor = config->dtor;
//     system->serialize = config->serialize;
//
//     system->entities.push();
//     arrayAnyPush(&system->components);
// }
//
// void ecsUnregisterComponent(Ecs* ecs, u64 componentId)
// {
//     Component* system = ecs->components.get(componentId);
//
//     for (u32 c = 1; c < system->components.count; ++c)
//     {
//         system->dtor(system->components[c]);
//     }
//     arrayAnyDestroy(&system->components);
//     system->entities = {};
//     system->indices = {};
//     system->name = {};
//
//     ecs->components.remove(componentId);
// }
//
// StringView ecsComponentName(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//     return system->name;
// }
//
// Entity ecsSpawn(Ecs* ecs)
// {
//     HG_ASSERT(ecs != nullptr);
//     return {handlePoolAlloc(&ecs->entities)};
// }
//
// void ecsDespawn(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     ecs->components.forEach([&](u64* id, Component*)
//     {
//         if (ecsHas(ecs, e, *id))
//             ecsRemove(ecs, e, *id);
//     });
//     handlePoolFree(&ecs->entities, e.handle);
// }
//
// bool ecsAlive(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     return handlePoolAlive(&ecs->entities, e.handle);
// }
//
// void* ecsAdd(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(!ecsHas(ecs, e, componentId));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     u32 idx = handleIdx(e.handle);
//     if (idx >= system->indices.count)
//     {
//         u32 oldCount = system->indices.count;
//         u32 newCount = idx * 2;
//         system->indices.resize(newCount);
//         for (u32 i = oldCount; i < newCount; ++i)
//             system->indices[i] = 0;
//     }
//     system->indices[idx] = system->entities.count;
//     system->entities.push(e);
//     return arrayAnyPush(&system->components);
// }
//
// void ecsRemove(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(ecsHas(ecs, e, componentId));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     u32 idx = system->indices[handleIdx(e.handle)];
//     system->dtor(system->components[idx]);
//
//     Entity last = system->entities.pop();
//     if (e != last)
//     {
//         system->entities[idx] = last;
//         system->indices[handleIdx(last.handle)] = idx;
//         memcpy(
//             system->components[idx],
//             system->components[system->components.count - 1],
//             system->components.width);
//     }
//     system->indices[handleIdx(e.handle)] = 0;
//     --system->components.count;
// }
//
// bool ecsHas(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Component* system = ecs->components.get(componentId);
//     if (system == nullptr)
//         return false;
//
//     u32 idx = handleIdx(e.handle);
//     return idx < system->indices.count && system->indices[idx] != 0;
// }
//
// void* ecsGet(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//     HG_ASSERT(system->indices[handleIdx(e.handle)] != 0);
//     HG_ASSERT(system->indices[handleIdx(e.handle)] < system->entities.count);
//
//     return system->components[system->indices[handleIdx(e.handle)]];
// }
//
// Entity ecsGetEntity(Ecs* ecs, const void* component, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(component != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     return system->entities[static_cast<u32>(reinterpret_cast<uptr>(component) - reinterpret_cast<uptr>(system->components.vals)) / system->components.width];
// }
//
// Entity* ecsEntities(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->entities.count != 0);
//     return ecs->components.get(componentId)->entities.vals + 1;
// }
//
// void* ecsComponents(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->components.count != 0);
//     Component* system = ecs->components.get(componentId);
//     return static_cast<u8*>(system->components.vals) + system->components.width;
// }
//
// u32 ecsCount(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->entities.count != 0);
//     return ecs->components.get(componentId)->entities.count - 1;
// }
//
// u64 ecsFindSmallest(Ecs* ecs, u64* ids, u32 idCount)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(idCount > 0);
//
//     u32 smallestCount = static_cast<u32>(-1);
//     u64 smallest = ids[0];
//
//     for (u32 i = 1; i < idCount; ++i)
//     {
//         Component* system = ecs->components.get(ids[i]);
//         HG_ASSERT(system != nullptr);
//
//         if (system->entities.count < smallestCount)
//         {
//             smallestCount = system->entities.count;
//             smallest = ids[i];
//         }
//     }
//     return smallest;
// }
//
// static void swapIdxLocation(Ecs* ecs, u32 lhs, u32 rhs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     HG_ASSERT(lhs != 0 && lhs < system->entities.count);
//     HG_ASSERT(rhs != 0 && rhs < system->entities.count);
//
//     Entity lhsEntity = system->entities[lhs];
//     Entity rhsEntity = system->entities[rhs];
//
//     HG_ASSERT(ecsAlive(ecs, lhsEntity));
//     HG_ASSERT(ecsAlive(ecs, rhsEntity));
//     HG_ASSERT(ecsHas(ecs, lhsEntity, componentId));
//     HG_ASSERT(ecsHas(ecs, rhsEntity, componentId));
//
//     ArenaScope scratch = getScratch();
//
//     system->entities[lhs] = rhsEntity;
//     system->entities[rhs] = lhsEntity;
//     system->indices[handleIdx(lhsEntity.handle)] = rhs;
//     system->indices[handleIdx(rhsEntity.handle)] = lhs;
//
//     void* temp = scratch.alloc(system->components.width, 1);
//     memcpy(temp, system->components[lhs], system->components.width);
//     memcpy(system->components[lhs], system->components[rhs], system->components.width);
//     memcpy(system->components[rhs], temp, system->components.width);
// }
//
// namespace {
//     struct QuicksortData {
//         Ecs* ecs = nullptr;
//         Component* system = nullptr;
//         u64 comp = 0;
//         void* data = nullptr;
//         bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs) = nullptr;
//
//         u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
//         {
//             while (inc != dec)
//             {
//                 while (!compare(data, ecs, system->entities[dec], system->entities[pivot]))
//                 {
//                     --dec;
//                     if (dec == inc)
//                         goto finish;
//                 }
//                 while (!compare(data, ecs, system->entities[pivot], system->entities[inc]))
//                 {
//                     ++inc;
//                     if (inc == dec)
//                         goto finish;
//                 }
//                 swapIdxLocation(ecs, inc, dec, comp);
//             }
//
//         finish:
//             if (compare(data, ecs, system->entities[inc], system->entities[pivot]))
//                 swapIdxLocation(ecs, pivot, inc, comp);
//
//             return inc;
//         }
//
//         void quicksort(u32 begin, u32 end)
//         {
//             if (begin + 1 >= end)
//                 return;
//
//             u32 middle = quicksortInter(begin, begin + 1, end - 1);
//             quicksort(begin, middle);
//             quicksort(middle, end);
//         }
//     };
// }
//
// void ecsSort(
//     Ecs* ecs,
//     u64 componentId,
//     void* data,
//     bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs))
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(compare != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     QuicksortData q{ecs, system, componentId, data, compare};
//     q.quicksort(1, system->entities.count);
// }
//
// template<>
// void serialize(Serializer* s, Ecs* ecs)
// {
//     HG_ASSERT(s != nullptr);
//     HG_ASSERT(ecs != nullptr);
//
//     ArenaScope scratch = getScratch(&s->arena, 1);
//
//     serializeBegin(s);
//     HG_DEFER(serializeEnd(s));
//
//     EntitySerializer ecsSerial{};
//     u32 entityCount = 0;
//     if (s->writing)
//     {
//         ecsSerial.entityToIdx = scratch.alloc<u32>(ecs->entities.handles.count);
//         for (u32 i = 1; i < ecs->entities.handles.count; ++i)
//         {
//             if (ecs->entities.handles[i] != handleNull)
//                 ecsSerial.entityToIdx[handleIdx(ecs->entities.handles[i])] = entityCount++;
//         }
//
//         serialize(s, &entityCount);
//     }
//     else
//     {
//         serialize(s, &entityCount);
//
//         ecsSerial.idxToEntity = scratch.alloc<Entity>(entityCount);
//         for (u32 i = 0; i < entityCount; ++i)
//         {
//             ecsSerial.idxToEntity[i] = ecsSpawn(ecs);
//         }
//     }
//
//     u32 systemCount;
//     if (s->writing)
//         systemCount = ecs->components.count;
//     serializeBegin(s, &systemCount);
//     HG_DEFER(serializeEnd(s));
//
//     u32 systemIdx = static_cast<u32>(-1);
//     for (u32 i = 0; i < systemCount; ++i)
//     {
//         serializeBegin(s);
//         HG_DEFER(serializeEnd(s));
//
//         u64 systemId = static_cast<u64>(-1);
//         Component* system;
//         if (s->writing)
//         {
//             ++systemIdx;
//             while (!ecs->components.hasVal[systemIdx])
//             {
//                 ++systemIdx;
//             }
//             systemId = ecs->components.keys[systemIdx];
//             system = &ecs->components.vals[systemIdx];
//             serialize(s, &system->name);
//         }
//         else
//         {
//             StringView compName;
//             serialize(s, &compName);
//             systemId = hash(compName);
//             system = ecs->components.get(systemId);
//         }
//
//         u32 compCount;
//         if (s->writing)
//             compCount = system->entities.count - 1;
//         serializeBegin(s, &compCount);
//         HG_DEFER(serializeEnd(s));
//
//         for (u32 c = 0; c < compCount; ++c)
//         {
//             serializeBegin(s);
//             HG_DEFER(serializeEnd(s));
//
//             u32 entityIdx;
//             if (s->writing)
//                 entityIdx = ecsSerial.entityToIdx[handleIdx(system->entities[c + 1].handle)];
//             serialize(s, &entityIdx);
//
//             void* compData;
//             if (s->writing)
//                 compData = system->components[c + 1];
//             else
//                 compData = ecsAdd(ecs, ecsSerial.idxToEntity[entityIdx], systemId);
//             system->serialize(s, compData, &ecsSerial);
//         }
//     }
// }
//
// void entitySerialize(Serializer* s, Entity* val, EntitySerializer* ecs)
// {
//     if (s->writing)
//     {
//         u32 idx = *val != entityNull ? ecs->entityToIdx[handleIdx(val->handle)] : static_cast<u32>(-1);
//         serialize(s, reinterpret_cast<i32*>(&idx));
//     }
//     else
//     {
//         u32 idx = static_cast<u32>(-1);
//         serialize(s, reinterpret_cast<i32*>(&idx));
//         *val = idx != static_cast<u32>(-1) ? ecs->idxToEntity[idx] : entityNull;
//     }
// }
//
// template<>
// void ecsSerialize(Serializer* s, Node* node, EntitySerializer* ecs)
// {
//     serializeBegin(s);
//     entitySerialize(s, &node->parent, ecs);
//     entitySerialize(s, &node->nextSibling, ecs);
//     entitySerialize(s, &node->prevSibling, ecs);
//     entitySerialize(s, &node->firstChild, ecs);
//     serializeEnd(s);
// }
//
// Node* nodeAdd(Ecs* ecs, Entity e)
// {
//     Node* node = ecsAdd<Node>(ecs, e);
//     *node = {};
//     return node;
// }
//
// void nodeDestroy(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//     if (node->parent.handle != handleNull)
//     {
//         if (node->prevSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
//         else
//             ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;
//
//         if (node->nextSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
//     }
//
//     Entity child = node->firstChild;
//     while (child.handle != handleNull)
//     {
//         Node* childNode = ecsGet<Node>(ecs, child);
//         Entity next = childNode->nextSibling;
//         nodeDestroy(ecs, child);
//         child = next;
//     }
//
//     ecsDespawn(ecs, e);
// }
//
// void nodeDetach(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//     if (node->parent.handle == handleNull)
//     {
//         HG_ASSERT(node->prevSibling.handle == handleNull);
//         HG_ASSERT(node->nextSibling.handle == handleNull);
//
//         Entity child = node->firstChild;
//         while (child.handle != handleNull)
//         {
//             Node* childNode = ecsGet<Node>(ecs, child);
//             Entity next = childNode->nextSibling;
//             childNode->parent = Entity{};
//             childNode->nextSibling = Entity{};
//             childNode->prevSibling = Entity{};
//             child = next;
//         }
//     } else {
//         if (node->prevSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
//         else
//             ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;
//
//         if (node->nextSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
//
//         Entity child = node->firstChild;
//         while (child.handle != handleNull)
//         {
//             Node* childNode = ecsGet<Node>(ecs, child);
//             Entity next = childNode->nextSibling;
//             childNode->parent = Entity{};
//             childNode->nextSibling = Entity{};
//             childNode->prevSibling = Entity{};
//             nodeAddChild(ecs, node->parent, child);
//             child = next;
//         }
//     }
//     *node = {};
// }
//
// void nodeAddChild(Ecs* ecs, Entity parent, Entity child)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, parent));
//     HG_ASSERT(ecsAlive(ecs, child));
//
//     Node* node = ecsGet<Node>(ecs, parent);
//     Node* childNode = ecsGet<Node>(ecs, child);
//
//     HG_ASSERT(childNode->parent.handle == handleNull);
//     HG_ASSERT(childNode->prevSibling.handle == handleNull);
//     HG_ASSERT(childNode->nextSibling.handle == handleNull);
//
//     if (node->firstChild.handle != handleNull)
//     {
//         ecsGet<Node>(ecs, node->firstChild)->prevSibling = child;
//         childNode->nextSibling = node->firstChild;
//     }
//     node->firstChild = child;
//     childNode->parent = parent;
// }
//
// template<>
// void serialize(Serializer* s, Transform* node)
// {
//     serializeObject(s,
//         &node->position,
//         &node->scale,
//         &node->rotation);
// }
//
// Transform* transformAdd(Ecs* ecs, Entity e, Vec3 position, Vec3 scale, Quat rotation)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Transform* tf = ecsAdd<Transform>(ecs, e);
//     *tf = {};
//     tf->position = position;
//     tf->scale = scale;
//     tf->rotation = rotation;
//     transformUpdate(ecs, e);
//
//     return tf;
// }
//
// static void transformUpdateChild(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecsHas<Transform>(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//
//     Transform* tf = ecsGet<Transform>(ecs, e);
//
//     tf->mat = ecsGet<Transform>(ecs, node->parent)->mat
//             * matModel3D(tf->position, tf->scale, tf->rotation);
//
//     Entity child = node->firstChild;
//     while (child.handle != handleNull)
//     {
//         transformUpdate(ecs, child);
//         child = ecsGet<Node>(ecs, child)->nextSibling;
//     }
// }
//
// void transformUpdate(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(ecsHas<Transform>(ecs, e));
//
//     if (ecsHas<Node>(ecs, e))
//     {
//         Node* node = ecsGet<Node>(ecs, e);
//         if (node->parent.handle != handleNull && ecsHas<Transform>(ecs, node->parent))
//         {
//             transformUpdateChild(ecs, e);
//         }
//         else
//         {
//             Transform* tf = ecsGet<Transform>(ecs, e);
//             tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);
//
//             Entity child = node->firstChild;
//             while (child.handle != handleNull)
//             {
//                 transformUpdateChild(ecs, child);
//                 child = ecsGet<Node>(ecs, child)->nextSibling;
//             }
//         }
//     }
//     else
//     {
//         Transform* tf = ecsGet<Transform>(ecs, e);
//         tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);
//     }
// }
//
// template<>
// void ecsDtor(AudioSource* src)
// {
//     src->audio = {};
//     audioStreamDestroy(src->player);
// }
//
// void audioUpdate(Ecs* ecs, Entity listener)
// {
//     ecsForEach<AudioSource>(ecs, [&](Entity e, AudioSource* src)
//     {
//         Sound* audio = &*src->audio;
//         if (src->position == audio->size && !src->repeat)
//             return;
//
//         ArenaScope scratch = getScratch();
//
//         u32 width = sizeof(f32);
//
//         u32 total = audio->frequency * width / 8;
//         u32 queued = audioStreamQueuedSize(src->player);
//         if (queued >= total)
//             return;
//         u32 sizeToPush = total - queued;
//
//         f32* queue = static_cast<f32*>(scratch.alloc(sizeToPush, width));
//         u32 queueSize = 0;
//
//         if (src->repeat)
//         {
//             while (queueSize < sizeToPush)
//             {
//                 if (src->position == audio->size)
//                     src->position = 0;
//
//                 u32 sizeToQueue = std::min(sizeToPush - queueSize, static_cast<u32>(audio->size - src->position));
//                 memcpy(reinterpret_cast<u8*>(queue) + queueSize, reinterpret_cast<u8*>(audio->data) + src->position, sizeToQueue);
//                 queueSize += sizeToQueue;
//                 src->position += sizeToQueue;
//             }
//         }
//         else
//         {
//             queueSize = std::min(sizeToPush, static_cast<u32>(audio->size - src->position));
//             memcpy(queue, reinterpret_cast<u8*>(audio->data) + src->position, queueSize);
//             src->position += queueSize;
//         }
//
//         if (ecsHas<Transform>(ecs, e))
//         {
//             HG_ASSERT(ecsHas<Transform>(ecs, listener));
//
//             Vec3 relPos = transformWorldPos(*ecsGet<Transform>(ecs, listener))
//                           - transformWorldPos(*ecsGet<Transform>(ecs, e));
//             f32 factor = 1.0f / vecDot3(relPos, relPos);
//
//             for (u64 i = 0; i < sizeToPush / sizeof(f32); ++i)
//             {
//                 queue[i] *= factor;
//             }
//         }
//
//         HG_ASSERT(queueSize <= sizeToPush);
//         audioStreamPush(src->player, queue, queueSize);
//     });
// }
//
// struct SpritePipelinePush {
//     Mat4 model = {};
//     Vec2 uvPos = {};
//     Vec2 uvSize = {};
//     u32 viewProj = 0;
//     u32 texture = 0;
// };
//
// struct SpritePipelineState {
//     GpuPipeline* pipeline = nullptr;
//     Texture defaultTex = {};
// };
//
// static SpritePipelineState spritePipeline{};
//
// #include "sprite.vert.spv.h"
// #include "sprite.frag.spv.h"
//
// void spritesInit(
//     Format colorFormat,
//     Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//     HG_ASSERT(depthFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = sprite_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(sprite_vert_spv);
//     pipelineConfig.fragmentShader = sprite_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(sprite_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(SpritePipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     pipelineConfig.enableDepthRead = true;
//     pipelineConfig.enableDepthWrite = true;
//     bool enableColorBlend = true;
//     pipelineConfig.colorBlendEnables = &enableColorBlend;
//
//     spritePipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color defaultColors[]{
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
//         {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//     };
//
//     spritePipeline.defaultTex.image = gpuImageCreate(2, 2, Format_r8g8b8a8_srgb,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//
//     spritePipeline.defaultTex.view = gpuViewCreate(
//         spritePipeline.defaultTex.image, GpuAspect_color, GpuFilter_nearest);
//
//     gpuImageWrite(spritePipeline.defaultTex.view, defaultColors);
// }
//
// void spritesDeinit()
// {
//     gpuPipelineDestroy(spritePipeline.pipeline);
//
//     gpuViewDestroy(spritePipeline.defaultTex.view);
//     gpuImageDestroy(spritePipeline.defaultTex.image);
// }
//
// template<>
// void serialize(Serializer* s, Sprite* sprite)
// {
//     serializeObject(s,
//         &sprite->texture,
//         &sprite->uvPos,
//         &sprite->uvSize);
// }
//
// Sprite* spriteAdd(Ecs* ecs, Entity e, Asset<Texture>* texture, Vec2 uvPos, Vec2 uvSize)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Sprite* sprite = ecsAdd<Sprite>(ecs, e);
//     *sprite = {};
//     sprite->texture = texture->clone();
//     sprite->uvPos = uvPos;
//     sprite->uvSize = uvSize;
//
//     return sprite;
// }
//
// template<>
// void ecsDtor(Sprite* sprite)
// {
//     *sprite = {};
// }
//
// void spritesDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(cmd != nullptr);
//
//     gpuBindPipeline(cmd, spritePipeline.pipeline);
//
//     ecsForEach<Sprite, Transform>(ecs, [&](Entity, Sprite* sprite, Transform* tf)
//     {
//         Texture* texture = sprite->texture == nullptr
//             ? &spritePipeline.defaultTex
//             : &*sprite->texture;
//
//         SpritePipelinePush push{};
//         push.model = tf->mat;
//         push.uvPos = sprite->uvPos;
//         push.uvSize = sprite->uvSize;
//         push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
//         push.texture = gpuImageSamplerDescriptor(texture->view);
//
//         gpuPushConstants(cmd, spritePipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, 6, 0, 1);
//     });
// }
//
// struct SkyboxPipelinePush {
//     u32 viewProj = 0;
//     u32 texture = 0;
// };
//
// struct SkyboxPipelineState {
//     GpuPipeline* pipeline = nullptr;
//     Texture defaultTex = {};
// };
//
// static SkyboxPipelineState skyboxPipeline{};
//
// #include "skybox.vert.spv.h"
// #include "skybox.frag.spv.h"
//
// void skyboxInit(Format colorFormat, Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = skybox_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(skybox_vert_spv);
//     pipelineConfig.fragmentShader = skybox_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(skybox_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(SkyboxPipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     bool enableColorBlend = true;
//     pipelineConfig.colorBlendEnables = &enableColorBlend;
//
//     skyboxPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color top = {0x00, 0x22, 0x44, 0xff};
//     Color mid = {0x00, 0x11, 0x33, 0xff};
//     Color bot = {0x00, 0x00, 0x00, 0xff};
//     Color nul = {};
//     Color defaultColors[]{
//         nul, top, nul, nul,
//         mid, mid, mid, mid,
//         nul, bot, nul, nul,
//     };
//
//     GpuImageCreateEx imageConfig{};
//     imageConfig.width = 1;
//     imageConfig.height = 1;
//     imageConfig.format = Format_r8g8b8a8_srgb;
//     imageConfig.arrayLayers = 6;
//     imageConfig.usage = GpuImageUsage_sampled | GpuImageUsage_transferDst;
//     imageConfig.flags = GpuImageConfig_cubeCompatible;
//
//     skyboxPipeline.defaultTex.image = gpuImageCreateEx(&imageConfig);
//
//     GpuViewCreateEx viewConfig{};
//     viewConfig.image = skyboxPipeline.defaultTex.image;
//     viewConfig.baseMipLevel = 0;
//     viewConfig.levelCount = 1;
//     viewConfig.baseArrayLayer = 0;
//     viewConfig.layerCount = 6;
//     viewConfig.aspectFlags = GpuAspect_color;
//     viewConfig.type = GpuViewType_cube;
//     viewConfig.filter = GpuFilter_nearest;
//     viewConfig.edgeMode = GpuSamplerEdgeMode_repeat;
//     viewConfig.border = GpuSamplerBorder_floatTransparentBlack;
//
//     skyboxPipeline.defaultTex.view = gpuViewCreateEx(&viewConfig);
//
//     gpuImageWriteCubemap(skyboxPipeline.defaultTex.view, defaultColors);
// }
//
// void skyboxDeinit()
// {
//     gpuPipelineDestroy(skyboxPipeline.pipeline);
//
//     gpuViewDestroy(skyboxPipeline.defaultTex.view);
//     gpuImageDestroy(skyboxPipeline.defaultTex.image);
// }
//
// template<>
// void serialize(Serializer* s, Skybox* skybox)
// {
//     serializeObject(s,
//         &skybox->texture);
// }
//
// Skybox* skyboxAdd(Ecs* ecs, Entity e, Asset<Texture>* texture)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Skybox* skybox = ecsAdd<Skybox>(ecs, e);
//     *skybox = {texture->clone()};
//
//     return skybox;
// }
//
// template<>
// void ecsDtor(Skybox* skybox)
// {
//     *skybox = {};
// }
//
// void skyboxDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     gpuBindPipeline(cmd, skyboxPipeline.pipeline);
//
//     ecsForEach<Skybox>(ecs, [&](Entity, Skybox* skybox)
//     {
//         Texture* texture = skybox->texture == nullptr
//             ? &skyboxPipeline.defaultTex
//             : &*skybox->texture;
//
//         SkyboxPipelinePush push{};
//         push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
//         push.texture = gpuImageSamplerDescriptor(texture->view);
//
//         gpuPushConstants(cmd, skyboxPipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, 36, 0, 1);
//     });
// }
//
// template<>
// void serialize(Serializer* s, DirLight* light)
// {
//     serializeObject(s,
//         &light->dir,
//         &light->color);
// }
//
// DirLight* dirLightAdd(Ecs* ecs, Entity e, Vec3 dir, Vec4 color)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     DirLight* light = ecsAdd<DirLight>(ecs, e);
//     *light = {dir, color};
//
//     return light;
// }
//
// template<>
// void serialize(Serializer* s, PointLight* light)
// {
//     serializeObject(s,
//         &light->color);
// }
//
// PointLight* pointLightAdd(Ecs* ecs, Entity e, Vec4 color)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     PointLight* light = ecsAdd<PointLight>(ecs, e);
//     *light = {color};
//
//     return light;
// }
//
// struct ModelPipelineDirLightData {
//     Vec4 dir = {};
//     Vec4 color = {};
// };
//
// struct ModelPipelinePointLightData {
//     Vec4 pos = {};
//     Vec4 color = {};
// };
//
// struct ModelPipelinePush {
//     Mat4 model = {};
//     u32 indices = 0;
//     u32 vertices = 0;
//     u32 viewProj = 0;
//     u32 normalMap = 0;
//     u32 colorMap = 0;
//     u32 dirLights = 0;
//     u32 dirLightCount = 0;
//     u32 pointLights = 0;
//     u32 pointLightCount = 0;
// };
//
// struct ModelPipelineState {
//     GpuPipeline* pipeline = nullptr;
//
//     GpuBuffer* dirLightBuffer = nullptr;
//     u32 dirLightCapacity = 0;
//
//     GpuBuffer* pointLightBuffer = nullptr;
//     u32 pointLightCapacity = 0;
//
//     Mesh defaultModel = {};
//     Texture defaultColorMap = {};
//     Texture defaultNormalMap = {};
// };
//
// static ModelPipelineState modelPipeline{};
//
// #include "model.vert.spv.h"
// #include "model.frag.spv.h"
//
// void modelsInit(
//     Format colorFormat,
//     Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//     HG_ASSERT(depthFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = model_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(model_vert_spv);
//     pipelineConfig.fragmentShader = model_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(model_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(ModelPipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     pipelineConfig.cullMode = GpuCull_back;
//     pipelineConfig.enableDepthRead = true;
//     pipelineConfig.enableDepthWrite = true;
//
//     modelPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     modelPipeline.dirLightCapacity = 16;
//     modelPipeline.dirLightBuffer = gpuBufferCreate(
//         sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
//         GpuBufferUsage_storageBuffer,
//         GpuMemoryUsage_frequentUpdate);
//
//     modelPipeline.pointLightCapacity = 64;
//     modelPipeline.pointLightBuffer = gpuBufferCreate(
//         sizeof(ModelPipelinePointLightData) * modelPipeline.dirLightCapacity,
//         GpuBufferUsage_storageBuffer,
//         GpuMemoryUsage_frequentUpdate);
//
//     MeshVertex cubeVertices[]{
//         {{ 0.5f,-0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,0}},
//         {{ 0.5f, 0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,1}},
//         {{ 0.5f,-0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,0}},
//         {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,0}},
//         {{-0.5f, 0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,1}},
//         {{-0.5f,-0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,1}},
//
//         {{-0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,0}},
//         {{-0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,1}},
//         {{ 0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,0}},
//         {{-0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,1}},
//         {{ 0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,0}},
//         {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,1}},
//         {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,1}},
//
//         {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,0}},
//         {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,0}},
//         {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,1}},
//         {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,1}},
//     };
//
//     u32 cubeIndices[]{
//          0,  1,  2,  0,  2,  3,
//          4,  5,  6,  4,  6,  7,
//          8,  9, 10,  8, 10, 11,
//         12, 13, 14, 12, 14, 15,
//         16, 17, 18, 16, 18, 19,
//         20, 21, 22, 20, 22, 23
//     };
//
//     modelPipeline.defaultModel.vertexBuffer = gpuBufferCreate(
//         sizeof(cubeVertices),
//         GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
//
//     modelPipeline.defaultModel.indexBuffer = gpuBufferCreate(
//         sizeof(cubeIndices),
//         GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
//
//     gpuBufferWrite(modelPipeline.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
//     gpuBufferWrite(modelPipeline.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));
//
//     modelPipeline.defaultModel.vertexCount = static_cast<u32>(std::size(cubeVertices));
//     modelPipeline.defaultModel.vertexWidth = static_cast<u32>(sizeof(MeshVertex));
//     modelPipeline.defaultModel.indexCount = static_cast<u32>(std::size(cubeIndices));
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color defaultColors[]{
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//         {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//     };
//
//     Vec4 defaultNormal{0, 0, -1, 0};
//
//     modelPipeline.defaultColorMap.image = gpuImageCreate(3, 3, Format_r8g8b8a8_srgb,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//     modelPipeline.defaultNormalMap.image = gpuImageCreate(1, 1, Format_r32g32b32a32_sfloat,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//
//     modelPipeline.defaultColorMap.view = gpuViewCreate(
//         modelPipeline.defaultColorMap.image, GpuAspect_color, GpuFilter_nearest);
//     modelPipeline.defaultNormalMap.view = gpuViewCreate(
//         modelPipeline.defaultNormalMap.image, GpuAspect_color, GpuFilter_nearest);
//
//     gpuImageWrite(modelPipeline.defaultColorMap.view, defaultColors);
//     gpuImageWrite(modelPipeline.defaultNormalMap.view, &defaultNormal);
// }
//
// void modelsDeinit()
// {
//     gpuPipelineDestroy(modelPipeline.pipeline);
//
//     gpuViewDestroy(modelPipeline.defaultNormalMap.view);
//     gpuImageDestroy(modelPipeline.defaultNormalMap.image);
//
//     gpuViewDestroy(modelPipeline.defaultColorMap.view);
//     gpuImageDestroy(modelPipeline.defaultColorMap.image);
//
//     gpuBufferDestroy(modelPipeline.defaultModel.indexBuffer);
//     gpuBufferDestroy(modelPipeline.defaultModel.vertexBuffer);
//
//     gpuBufferDestroy(modelPipeline.pointLightBuffer);
//     gpuBufferDestroy(modelPipeline.dirLightBuffer);
// }
//
// template<>
// void serialize(Serializer* s, Model* model)
// {
//     serializeObject(s,
//         &model->mesh,
//         &model->colorMap,
//         &model->normalMap);
// }
//
// Model* modelAdd(
//     Ecs* ecs,
//     Entity e,
//     Asset<Mesh>* mesh,
//     Asset<Texture>* colorMap,
//     Asset<Texture>* normalMap)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Model* model = ecsAdd<Model>(ecs, e);
//     *model = {};
//     model->mesh = mesh->clone();
//     model->colorMap = colorMap->clone();
//     model->normalMap = normalMap->clone();
//
//     return model;
// }
//
// template<>
// void ecsDtor(Model* model)
// {
//     *model = {};
// }
//
// void modelsDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(cmd != nullptr);
//
//     ArenaScope scratch = getScratch();
//
//     Camera* cameraC = ecsGet<Camera>(ecs, camera);
//     Transform* cameraTf = ecsGet<Transform>(ecs, camera);
//
//     Mat4 view = matModelToView(cameraTf->mat);
//
//     u32 dirLightCount = ecsCount<DirLight>(ecs);
//     u32 pointLightCount = ecsCount<PointLight>(ecs);
//
//     if (dirLightCount > modelPipeline.dirLightCapacity)
//     {
//         if (modelPipeline.dirLightCapacity == 0)
//             modelPipeline.dirLightCapacity = 1;
//         while (modelPipeline.dirLightCapacity < dirLightCount)
//         {
//             modelPipeline.dirLightCapacity *= 2;
//         }
//
//         gpuWaitIdle();
//
//         gpuBufferDestroy(modelPipeline.dirLightBuffer);
//         modelPipeline.dirLightBuffer = gpuBufferCreate(
//             sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
//             GpuBufferUsage_storageBuffer,
//             GpuMemoryUsage_frequentUpdate);
//     }
//
//     if (pointLightCount > modelPipeline.pointLightCapacity)
//     {
//         if (modelPipeline.pointLightCapacity == 0)
//             modelPipeline.pointLightCapacity = 1;
//         while (modelPipeline.pointLightCapacity < pointLightCount)
//         {
//             modelPipeline.pointLightCapacity *= 2;
//         }
//
//         gpuWaitIdle();
//
//         gpuBufferDestroy(modelPipeline.pointLightBuffer);
//         modelPipeline.pointLightBuffer = gpuBufferCreate(
//             sizeof(ModelPipelinePointLightData) * modelPipeline.pointLightCapacity,
//             GpuBufferUsage_storageBuffer,
//             GpuMemoryUsage_frequentUpdate);
//     }
//
//     ModelPipelineDirLightData* dirLights = scratch.alloc<ModelPipelineDirLightData>(dirLightCount);
//     ModelPipelinePointLightData* pointLights = scratch.alloc<ModelPipelinePointLightData>(pointLightCount);
//
//     u32 i = 0;
//     ecsForEach<DirLight>(ecs, [&](Entity, DirLight* light)
//     {
//         dirLights[i].dir = Vec4{Mat3{view} * light->dir, 0.0};
//         dirLights[i].color = light->color;
//         ++i;
//     });
//
//     i = 0;
//     ecsForEach<PointLight, Transform>(ecs, [&](Entity, PointLight* light, Transform* tf)
//     {
//         pointLights[i].pos = view * Vec4{transformWorldPos(*tf), 1.0};
//         pointLights[i].color = light->color;
//         ++i;
//     });
//
//     if (dirLightCount > 0)
//         gpuBufferWrite(modelPipeline.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);
//
//     if (pointLightCount > 0)
//         gpuBufferWrite(modelPipeline.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);
//
//     gpuBindPipeline(cmd, modelPipeline.pipeline);
//
//     ecsForEach<Model, Transform>(ecs, [&](Entity, Model* model, Transform* tf)
//     {
//         Texture* colorMap = model->colorMap == nullptr
//             ? &modelPipeline.defaultColorMap
//             : &*model->colorMap;
//
//         Texture* normalMap = model->normalMap == nullptr
//             ? &modelPipeline.defaultNormalMap
//             : &*model->normalMap;
//
//         Mesh* gpuModel = model->mesh == nullptr
//             ? &modelPipeline.defaultModel
//             : &*model->mesh;
//
//         ModelPipelinePush push{};
//         push.model = tf->mat;
//         push.vertices = gpuBufferStorageDescriptor(gpuModel->vertexBuffer);
//         push.indices = gpuBufferStorageDescriptor(gpuModel->indexBuffer);
//         push.viewProj = gpuBufferUniformDescriptor(cameraC->vpBuffer);
//         push.normalMap = gpuImageSamplerDescriptor(normalMap->view);
//         push.colorMap = gpuImageSamplerDescriptor(colorMap->view);
//         push.dirLights = gpuBufferStorageDescriptor(modelPipeline.dirLightBuffer);
//         push.dirLightCount = dirLightCount;
//         push.pointLights = gpuBufferStorageDescriptor(modelPipeline.pointLightBuffer);
//         push.pointLightCount = pointLightCount;
//
//         gpuPushConstants(cmd, modelPipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, gpuModel->indexCount, 0, 1);
//     });
// }

} // namespace hg
