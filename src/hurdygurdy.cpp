#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <emmintrin.h>

#if defined(HG_PLATFORM_LINUX)
#include <alloca.h>
#elif defined(HG_PLATFORM_WINDOWS)
#include <malloc.h>
#endif

void hg_init(void) {
    for (usize i = 0; i < hg_arena_count; ++i) {
        if (hg_arenas[i].memory == nullptr) {
            usize arena_size = (u32)-1;
            hg_arenas[i] = {malloc(arena_size), arena_size};
        }
    }

    HgArena& arena = hg_get_scratch();

    // init arenas for each thread : TODO?

    if (hg_threads == nullptr) {
        u32 thread_count = std::thread::hardware_concurrency()
            - 2; // main thread, io thread
        hg_threads = HgThreadPool::create(arena, thread_count, 4096);
        hg_assert(hg_threads != nullptr);
    }

    if (hg_io == nullptr) {
        hg_io = HgIOThread::create(arena, 4096);
        hg_assert(hg_io != nullptr);
    }

    if (hg_resources == nullptr) {
        hg_resources = arena.alloc<HgResourceManager>(1);
        *hg_resources = hg_resources->create(arena, 4096);
    }

    if (hg_ecs == nullptr) {
        hg_ecs = arena.alloc<HgECS>(1);
        *hg_ecs = hg_ecs->create(arena, 4096);
    }

    hg_graphics_init();

    hg_platform_init();
}

void hg_exit(void) {
    hg_platform_deinit();

    hg_graphics_deinit();

    if (hg_ecs != nullptr) {
        hg_ecs = nullptr;
    }

    if (hg_resources != nullptr) {
        hg_resources->destroy(nullptr, 0);
        hg_resources = nullptr;
    }

    if (hg_io != nullptr) {
        hg_io->destroy();
        hg_io = nullptr;
    }

    if (hg_threads != nullptr) {
        hg_threads->destroy();
        hg_threads = nullptr;
    }

    for (usize i = 0; i < hg_arena_count; ++i) {
        std::free(hg_arenas[i].memory);
    }
}

f64 HgClock::tick() {
    auto prev = time;
    time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<f64>{time - prev}.count();
}

struct HgTestArray {
    HgTest* items;
    usize capacity;
    usize count;

    static HgTestArray create(usize init_count) {
        HgTestArray arr;
        arr.items = (HgTest*)std::malloc(init_count);
        arr.capacity = init_count;
        arr.count = 0;
        return arr;
    }

    void destroy() const {
        std::free(items);
    }

    HgTest& push() {
        if (capacity == count) {
            usize new_capacity = capacity == 0 ? 1 : capacity * 2;
            items = (HgTest*)std::realloc(items, new_capacity);
            capacity = new_capacity;
        }
        return items[count++];
    }
};

static HgTestArray& hg_internal_get_tests() {
    static HgTestArray tests = tests.create(1024);
    return tests;
}

HgTest::HgTest(const char* test_name, bool (*test_function)()) : name(test_name), function(test_function) {
    hg_internal_get_tests().push() = *this;
}

bool hg_run_tests() {
    std::printf("HurdyGurdy: Tests Begun\n");

    HgTestArray& tests = hg_internal_get_tests();
    bool all_succeeded = true;

    HgClock timer{};
    for (usize i = 0; i < tests.count; ++i) {
        std::printf("%s...\n", tests.items[i].name);
        if (tests.items[i].function()) {
            std::printf("\x1b[32mSuccess\n\x1b[0m");
        } else {
            all_succeeded = false;
            std::printf("\x1b[31mFailure\n\x1b[0m");
        }
    }
    f64 ms = timer.tick() * 1000.0f;

    if (all_succeeded) {
        std::printf("HurdyGurdy: Tests Complete in %fms \x1b[32m[Success]\x1b[0m\n", ms);
    } else {
        std::printf("HurdyGurdy: Tests Complete in %fms \x1b[31m[Failure]\x1b[0m\n", ms);
    }

    return all_succeeded;
}

const HgVec2& HgVec2::operator+=(const HgVec2& other) {
    x += other.x;
    y += other.y;
    return* this;
}

const HgVec2& HgVec2::operator-=(const HgVec2& other) {
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgVec2& HgVec2::operator*=(const HgVec2& other) {
    x *= other.x;
    y *= other.y;
    return* this;
}

const HgVec2& HgVec2::operator/=(const HgVec2& other) {
    x /= other.x;
    y /= other.y;
    return* this;
}

const HgVec3& HgVec3::operator+=(const HgVec3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgVec3& HgVec3::operator-=(const HgVec3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgVec3& HgVec3::operator*=(const HgVec3& other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

const HgVec3& HgVec3::operator/=(const HgVec3& other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

const HgVec4& HgVec4::operator+=(const HgVec4& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgVec4& HgVec4::operator-=(const HgVec4& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

const HgVec4& HgVec4::operator*=(const HgVec4& other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

const HgVec4& HgVec4::operator/=(const HgVec4& other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

const HgMat2& HgMat2::operator+=(const HgMat2& other) {
    x += other.x;
    y += other.y;
    return* this;
}

const HgMat2& HgMat2::operator-=(const HgMat2& other) {
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgMat3& HgMat3::operator+=(const HgMat3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgMat3& HgMat3::operator-=(const HgMat3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgMat4& HgMat4::operator+=(const HgMat4& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgMat4& HgMat4::operator-=(const HgMat4& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

const HgComplex& HgComplex::operator+=(const HgComplex& other) {
    r += other.r;
    i += other.i;
    return* this;
}

const HgComplex& HgComplex::operator-=(const HgComplex& other) {
    r -= other.r;
    i -= other.i;
    return* this;
}

const HgQuat& HgQuat::operator+=(const HgQuat& other) {
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

const HgQuat& HgQuat::operator-=(const HgQuat& other) {
    r -= other.r;
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return* this;
}

void hg_vec_add(u32 size, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] + rhs[i];
    }
}

void hg_vec_sub(u32 size, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] - rhs[i];
    }
}

void hg_vec_mul_pairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] * rhs[i];
    }
}

void hg_vec_scalar_mul(u32 size, f32* dst, f32 scalar, const f32* vec) {
    hg_assert(dst != nullptr);
    hg_assert(vec != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar * vec[i];
    }
}

void hg_vec_div(u32 size, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        hg_assert(rhs[i] != 0);
        dst[i] = lhs[i] / rhs[i];
    }
}

void hg_vec_scalar_div(u32 size, f32* dst, const f32* vec, f32 scalar) {
    hg_assert(dst != nullptr);
    hg_assert(vec != nullptr);
    hg_assert(scalar != 0);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / scalar;
    }
}

void hg_dot(u32 size, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    *dst = 0;
    for (u32 i = 0; i < size; ++i) {
        *dst += lhs[i] * rhs[i];
    }
}

void hg_len(u32 size, f32* dst, const f32* vec) {
    hg_assert(dst != nullptr);
    hg_assert(vec != nullptr);
    hg_dot(size, dst, vec, vec);
    *dst = std::sqrt(*dst);
}

f32 hg_len(const HgVec2& vec) {
    return std::sqrt(hg_dot(vec, vec));
}

f32 hg_len(const HgVec3& vec) {
    return std::sqrt(hg_dot(vec, vec));
}

f32 hg_len(const HgVec4& vec) {
    return std::sqrt(hg_dot(vec, vec));
}

void hg_norm(u32 size, f32* dst, const f32* vec) {
    hg_assert(dst != nullptr);
    hg_assert(vec != nullptr);
    f32 len;
    hg_len(size, &len, vec);
    hg_assert(len != 0);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hg_norm(const HgVec2& vec) {
    f32 len = hg_len(vec);
    hg_assert(len != 0);
    return {vec.x / len, vec.y / len};
}

HgVec3 hg_norm(const HgVec3& vec) {
    f32 len = hg_len(vec);
    hg_assert(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hg_norm(const HgVec4& vec) {
    f32 len = hg_len(vec);
    hg_assert(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hg_cross(f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

HgVec3 hg_cross(const HgVec3& lhs, const HgVec3& rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

void hg_mat_add(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_mat_add(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_mat_add(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_mat_add(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hg_mat_sub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs) {
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_mat_sub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_mat_sub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_mat_sub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hg_mat_mul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs) {
    hg_assert(hr == wl);
    hg_assert(dst != nullptr);
    hg_assert(lhs != nullptr);
    hg_assert(rhs != nullptr);
    (void)hr;
    for (u32 i = 0; i < wl; ++i) {
        for (u32 j = 0; j < wr; ++j) {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k) {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_mat_mul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_mat_mul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_mat_mul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

void hg_mat_vec_mul(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec) {
    hg_assert(dst != nullptr);
    hg_assert(mat != nullptr);
    hg_assert(vec != nullptr);
    for (u32 i = 0; i < height; ++i) {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j) {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

HgVec2 operator*(const HgMat2& lhs, const HgVec2& rhs) {
    HgVec2 result{};
    hg_mat_vec_mul(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec3 operator*(const HgMat3& lhs, const HgVec3& rhs) {
    HgVec3 result{};
    hg_mat_vec_mul(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec4 operator*(const HgMat4& lhs, const HgVec4& rhs) {
    HgVec4 result{};
    hg_mat_vec_mul(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgQuat operator*(const HgQuat& lhs, const HgQuat& rhs) {
    return {
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

HgQuat hg_axis_angle(const HgVec3& axis, f32 angle) {
    f32 half_angle = angle * (f32)0.5;
    f32 sin_half_angle = std::sin(half_angle);
    return {
        std::cos(half_angle),
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
    };
}

HgVec3 hg_rotate(const HgQuat& lhs, const HgVec3& rhs) {
    HgQuat q = lhs * HgQuat{0, rhs.x, rhs.y, rhs.z} * hg_conj(lhs);
    return {q.i, q.j, q.k};
}

HgMat3 hg_rotate(const HgQuat& lhs, const HgMat3& rhs) {
    return {
        hg_rotate(lhs, rhs.x),
        hg_rotate(lhs, rhs.y),
        hg_rotate(lhs, rhs.z),
    };
}

HgMat4 hg_model_matrix_2d(const HgVec3& position, const HgVec2& scale, f32 rotation) {
    HgMat2 m2{{scale.x, 0.0f}, {0.0f, scale.y}};
    f32 rot_sin = std::sin(rotation);
    f32 rot_cos = std::cos(rotation);
    HgMat2 rot{{rot_cos, rot_sin}, {-rot_sin, rot_cos}};
    HgMat4 m4 = rot * m2;
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_model_matrix_3d(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation) {
    HgMat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hg_rotate(rotation, m3);
    HgMat4 m4 = m3;
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_view_matrix(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation) {
    HgMat4 rot{hg_rotate(hg_conj(rotation), HgMat3{1.0f})};
    HgMat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

HgMat4 hg_projection_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return {
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hg_projection_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    hg_assert(near > 0.0f);
    hg_assert(far > near);
    f32 scale = 1.0f / std::tan(fov * 0.5f);
    return {
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

u32 hg_max_mipmaps(u32 width, u32 height, u32 depth) {
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return (u32)std::log2((f32)max) + 1;
}

void* HgArena::alloc(usize size, usize alignment) {
    head = hg_align((usize)head, alignment) + size;
    hg_assert(head <= capacity);
    return (void*)((uptr)memory + head - size);
}

void* HgArena::realloc(void* allocation, usize old_size, usize new_size, usize alignment) {
    if (allocation >= memory && (uptr)allocation + old_size <= (uptr)memory + capacity) {
        if ((uptr)allocation + old_size - (uptr)memory == (uptr)head) {
            head = (uptr)allocation + new_size - (uptr)memory;
            hg_assert(head <= capacity);
            return allocation;
        }

        if (new_size < old_size)
            return allocation;
    }

    void* new_allocation = alloc(new_size, alignment);
    if (allocation != nullptr)
        std::memcpy(new_allocation, allocation, std::min(old_size, new_size));
    return new_allocation;
}

HgArena& hg_get_scratch() {
    return hg_arenas[0];
}

HgArena& hg_get_scratch(const HgArena& conflict) {
    for (HgArena& arena : hg_arenas) {
        if (&arena != &conflict)
            return arena;
    }
    hg_error("No scratch arena available\n");
}

HgArena& hg_get_scratch(const HgArena** conflicts, usize count) {
    for (HgArena& arena : hg_arenas) {
        for (usize i = 0; i < count; ++i) {
            if (&arena == conflicts[i])
                goto next;
        }
        return arena;
next:
        continue;
    }
    hg_error("No scratch arena available\n");
}

HgString HgString::create(HgArena& arena, usize capacity) {
    HgString str;
    str.chars = arena.alloc<char>(capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString HgString::create(HgArena& arena, HgStringView init) {
    HgString str;
    str.chars = arena.alloc<char>(init.length);
    str.capacity = init.length;
    str.length = init.length;
    std::memcpy(str.chars, init.chars, init.length);
    return str;
}

void HgString::reserve(HgArena& arena, usize new_capacity) {
    chars = arena.realloc(chars, capacity, new_capacity);
    capacity = new_capacity;
}

void HgString::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

HgString& HgString::insert(HgArena& arena, usize index, char c) {
    hg_assert(index <= length);

    usize new_length = length + 1;
    while (capacity < new_length) {
        grow(arena);
    }

    if (index != length)
        std::memmove(&chars[index + 1], &chars[index], length - index);
    chars[index] = c;
    length = new_length;

    return *this;
}

HgString& HgString::insert(HgArena& arena, usize index, HgStringView str) {
    hg_assert(index <= length);

    usize new_length = length + str.length;
    while (capacity < new_length) {
        grow(arena);
    }

    if (index != length)
        std::memmove(&chars[index + str.length], &chars[index], length - index);
    std::memcpy(&chars[index], str.chars, str.length);
    length = new_length;

    return *this;
}

bool hg_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

bool hg_is_numeral_base10(char c) {
    return c >= '0' && c <= '9';
}

bool hg_is_integer_base10(HgStringView str) {
    if (str.length == 0)
        return false;

    usize head = 0;
    if (!hg_is_numeral_base10(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    ++head;
    while (head < str.length) {
        if (!hg_is_numeral_base10(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool hg_is_float_base10(HgStringView str) {
    if (str.length == 0)
        return false;

    bool has_decimal = false;
    bool has_exponent = false;

    usize head = 0;

    if (!hg_is_numeral_base10(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '.')
        has_decimal = true;

    ++head;
    while (head < str.length) {
        if (hg_is_numeral_base10(str[head])) {
            ++head;
            continue;
        }

        if (str[head] == '.' && !has_decimal) {
            has_decimal = true;
            ++head;
            continue;
        }

        if (str[head] == 'e' && !has_exponent) {
            has_exponent = true;
            ++head;
            if (hg_is_numeral_base10(str[head]) || str[head] == '+' || str[head] == '-') {
                ++head;
                continue;
            }
            return false;
        }

        if (str[head] == 'f' && head == str.length - 1)
            break;

        return false;
    }

    return has_decimal || has_exponent;
}

i64 hg_str_to_int_base10(HgStringView str) {
    hg_assert(hg_is_integer_base10(str));

    i64 power = 1;
    i64 ret = 0;

    usize head = str.length - 1;
    while (head > 0) {
        ret += (i64)(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+') {
        if (str[head] == '-')
            ret *= -1;
        else
            ret += (i64)(str[head] - '0') * power;
    }

    return ret;
}

f64 hg_str_to_float_base10(HgStringView str) {
    hg_assert(hg_is_float_base10(str));

    f64 ret = 0.0;
    usize head = 0;

    bool is_negative = str[head] == '-';
    if (is_negative || str[head] == '+')
        ++head;

    if (hg_is_numeral_base10(str[head])) {
        usize int_part_begin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e') {
            ++head;
        }
        ret += (f64)hg_str_to_int_base10({&str[int_part_begin], &str[head]});
    }

    if (head < str.length && str[head] == '.') {
        ++head;

        f64 power = 0.1;
        while (head < str.length && hg_is_numeral_base10(str[head])) {
            ret += (f64)(str[head] - '0') * power;
            power *= 0.1;
            ++head;
        }
    }

    if (head < str.length && str[head] == 'e') {
        ++head;

        bool exp_is_negative = str[head] == '-';
        if (exp_is_negative || str[head] == '+')
            ++head;

        usize exp_begin = head;
        while (head < str.length && hg_is_numeral_base10(str[head])) {
            ++head;
        }

        i64 exp = hg_str_to_int_base10({&str[exp_begin], str.chars + head});
        if (exp != 0) {
            if (exp_is_negative) {
                for (i64 i = 0; i < exp; ++i) {
                    ret *= 0.1;
                }
            } else {
                for (i64 i = 0; i < exp; ++i) {
                    ret *= 10.0;
                }
            }
        } else {
            ret = 1.0;
        }
    }

    if (is_negative)
        ret *= -1.0;

    return ret;
}

HgString hg_int_to_str_base10(HgArena& arena, i64 num) {
    hg_arena_scope(scratch, hg_get_scratch(arena));

    if (num == 0)
        return HgString::create(arena, "0");

    bool is_negative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgString reverse = reverse.create(scratch, 16);
    while (unum != 0) {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        reverse.append(scratch, '0' + (char)digit);
    }

    HgString ret = ret.create(arena, reverse.length + (is_negative ? 1 : 0));
    if (is_negative)
        ret.append(arena, '-');
    for (usize i = reverse.length - 1; i < reverse.length; --i) {
        ret.append(arena, reverse[i]);
    }
    return ret;
}

HgString hg_float_to_str_base10(HgArena& arena, f64 num, u64 decimal_count) {
    hg_arena_scope(scratch, hg_get_scratch(arena));

    if (num == 0.0)
        return HgString::create(arena, "0.0");

    HgString int_str = hg_int_to_str_base10(scratch, (i64)std::abs(num));

    HgString dec_str = HgString::create(scratch, decimal_count + 1);
    dec_str.append(scratch, '.');

    f64 dec_part = std::abs(num);
    for (usize i = 0; i < decimal_count; ++i) {
        dec_part *= 10.0;
        dec_str.append(scratch, '0' + (char)((u64)dec_part % 10));
    }

    HgString ret{};
    if (num < 0.0)
        ret.append(arena, '-');
    ret.append(arena, int_str);
    ret.append(arena, dec_str);
    return ret;
}

struct HgJsonParser {
    HgArena& arena;
    HgStringView text;
    usize head;
    usize line;

    HgJsonParser(HgArena& arena_val) : arena{arena_val} {}

    HgJson parse_next();
    HgJson parse_struct();
    HgJson parse_array();
    HgJson parse_string();
    HgJson parse_number();
    HgJson parse_boolean();
};

HgJson HgJsonParser::parse_next() {
    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head >= text.length)
        return {};

    switch (text[head]) {
        case '{':
            ++head;
            return parse_struct();
        case '[':
            ++head;
            return parse_array();
        case '\'': [[fallthrough]];
        case '"':
            ++head;
            return parse_string();
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return parse_number();
        case 't': [[fallthrough]];
        case 'f':
            return parse_boolean();
        case '}': {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", found unexpected token \"]\"\n");
            return {nullptr, error};
        }
    }
    if (hg_is_numeral_base10(text[head])) {
        return parse_number();
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
    error->next = nullptr;

    usize begin = head;
    while (head < text.length && !hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", found unexpected token \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

    return {nullptr, error};
}

HgJson HgJsonParser::parse_struct() {
    HgJson json{};
    json.file = arena.alloc<HgJson::Node>(1);
    json.file->type = HgJson::jstruct;
    json.file->jstruct.fields = nullptr;

    HgJson::Field* last_field = nullptr;
    HgJson::Error* last_error = nullptr;

    for (;;) {
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length) {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            break;
        }
        if (text[head] == ']') {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", struct ends with \"]\" instead of \"}\"\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == '}') {
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJson value = parse_next();

        if (value.file != nullptr) {
            if (value.file->type != HgJson::field) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", struct has a literal instead of a field\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else if (value.file->field.value == nullptr) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", struct has a field named \"")
                    .append(arena, value.file->field.name)
                    .append(arena, "\" which has no value\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else {
                if (last_field == nullptr)
                    json.file->jstruct.fields = &value.file->field;
                else
                    last_field->next = &value.file->field;
                last_field = &value.file->field;
            }
        }
        if (value.errors != nullptr) {
            if (last_error == nullptr)
                json.errors = last_error = value.errors;
            else
                last_error->next = value.errors;
            last_error = value.errors;
        }
    }

    return json;
}

HgJson HgJsonParser::parse_array() {
    HgJson json{};
    json.file = arena.alloc<HgJson::Node>(1);
    json.file->type = HgJson::array;

    HgJson::Type type = HgJson::none;
    HgJson::Elem* last_elem = nullptr;
    HgJson::Error* last_error = nullptr;

    for (;;) {
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length) {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            break;
        }
        if (text[head] == '}') {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", array ends with \"}\" instead of \"]\"\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == ']') {
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJson::Elem* elem = arena.alloc<HgJson::Elem>(1);
        elem->next = nullptr;

        HgJson value = parse_next();
        elem->value = value.file;

        if (value.file != nullptr) {
            if (type == HgJson::none) {
                if (value.file->type != HgJson::field) {
                    type = value.file->type;
                } else {
                    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                    error->next = nullptr;
                    error->msg = HgString{}
                        .append(arena, "on line ")
                        .append(arena, hg_int_to_str_base10(arena, (i64)line))
                        .append(arena, ", array has a field as an element\n");
                    if (last_error == nullptr)
                        json.errors = last_error = error;
                    else
                        last_error->next = error;
                    last_error = error;
                }
            }
            if (value.file->type != type) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", array has element which is not the same type as the first valid element\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else {
                if (last_elem == nullptr)
                    json.file->array.elems = elem;
                else
                    last_elem->next = elem;
                last_elem = elem;
            }
        }
        if (value.errors != nullptr) {
            if (last_error == nullptr)
                json.errors = last_error = value.errors;
            else
                last_error->next = value.errors;
            last_error = value.errors;
        }
    }

    return json;
}

HgJson HgJsonParser::parse_string() {
    usize begin = head;
    while (head < text.length && text[head] != '"') {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    usize end = head;
    if (head < text.length) {
        ++head;
        HgString str = str.create(arena, end - begin);
        for (usize i = begin; i < end; ++i) {
            char c = text[i];
            if (c == '\\') {
                // escape sequences : TODO
            }
            str.append(arena, c);
        }

        HgJson json{};
        json.file = arena.alloc<HgJson::Node>(1);

        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ':') {
            ++head;
            json.file->type = HgJson::field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            HgJson next = parse_next();
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = HgJson::string;
            json.file->string = str;
        }
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;
        return json;
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected string to terminate\n");
    return {nullptr, error};
}

HgJson HgJsonParser::parse_number() {
    bool is_float = false;
    usize begin = head;
    while (head < text.length && (
        hg_is_numeral_base10(text[head]) ||
        text[head] == '-' ||
        text[head] == '+' ||
        text[head] == '.' ||
        text[head] == 'e'
    )) {
        if (text[head] == '.' || text[head] == 'e')
            is_float = true;
        ++head;
    }
    HgStringView num{&text[begin], &text[head]};
    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head < text.length && text[head] == ',')
        ++head;

    if (is_float) {
        if (hg_is_float_base10(num)) {
            HgJson::Node* node = arena.alloc<HgJson::Node>(1);
            node->type = HgJson::floating;
            node->floating = hg_str_to_float_base10(num);
            return {node, nullptr};
        }
    } else {
        if (hg_is_integer_base10(num)) {
            HgJson::Node* node = arena.alloc<HgJson::Node>(1);
            node->type = HgJson::integer;
            node->integer = hg_str_to_int_base10(num);
            return {node, nullptr};
        }
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);

    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected numeral value, found \"")
        .append(arena, num)
        .append(arena, "\"\n");

    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']') {
        return {nullptr, error};
    } else {
        HgJson next = parse_next();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson HgJsonParser::parse_boolean() {
    if (head + 4 < text.length && HgStringView{&text[head], 4} == "true") {
        head += 4;
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJson::Node* node = arena.alloc<HgJson::Node>(1);
        node->type = HgJson::boolean;
        node->boolean = true;
        return {node, nullptr};
    }
    if (head + 5 < text.length && HgStringView{&text[head], 5} == "false") {
        head += 5;
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJson::Node* node = arena.alloc<HgJson::Node>(1);
        node->type = HgJson::boolean;
        node->boolean = false;
        return {node, nullptr};
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);

    usize begin = head;
    while (head < text.length && !hg_is_whitespace(text[head])
        && text[head] != ','
        && text[head] != '}'
        && text[head] != ']'
    ) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected boolean value, found \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

    if (text[head] == ',')
        ++head;

    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']') {
        return {nullptr, error};
    } else {
        HgJson next = parse_next();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson HgJson::parse(HgArena& arena, HgStringView text) {
    HgJsonParser parser{arena};
    parser.text = text;
    parser.head = 0;
    parser.line = 1;
    return parser.parse_next();
}

HgAnyArray HgAnyArray::create(
    HgArena& arena,
    u32 width,
    u32 alignment,
    usize count,
    usize capacity
) {
    hg_assert(count <= capacity);

    HgAnyArray arr;
    arr.items = arena.alloc(capacity * width, alignment);
    arr.width = width;
    arr.alignment = alignment;
    arr.capacity = capacity;
    arr.count = count;
    return arr;
}

void HgAnyArray::reserve(HgArena& arena, usize new_capacity) {
    items = arena.realloc(items, capacity * width, new_capacity * width, alignment);
    capacity = new_capacity;
}

void HgAnyArray::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

void* HgAnyArray::insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);

    std::memmove(get(index + 1), get(index), (count++ - index) * width);
    return get(index);
}

void HgAnyArray::remove(usize index) {
    hg_assert(index < count);

    std::memmove(get(index), get(index + 1), (count - index - 1) * width);
    --count;
}

void* HgAnyArray::swap_insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);
    if (index == count)
        return push();

    std::memcpy(get(count++), get(index), width);
    return get(index);
}

void HgAnyArray::swap_remove(usize index) {
    hg_assert(index < count);
    if (index == count - 1) {
        pop();
        return;
    }

    std::memcpy(get(index), get(count - 1), width);
    --count;
}

void HgFence::add() {
    ++counter;
}

void HgFence::add(usize count) {
    counter.fetch_add(count);
}

void HgFence::signal() {
    --counter;
}

void HgFence::signal(usize count) {
    counter.fetch_sub(count);
}

bool HgFence::is_complete() {
    return counter.load() == 0;
}

bool HgFence::wait(f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!is_complete() && std::chrono::steady_clock::now() < end) {
        _mm_pause();
    }
    return is_complete();
}

HgThreadPool* HgThreadPool::create(HgArena& arena, usize thread_count, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    static constexpr usize spin_count = 128;

    auto thread_fn = [](HgThreadPool* pool) {
        for (;;) {
            std::unique_lock<std::mutex> lock{pool->mtx};
            while (pool->count.load() == 0 && !pool->should_close.load()) {
                pool->cv.wait(lock);
            }
            lock.unlock();
            if (pool->should_close.load())
                return;

            for (usize i = 0; i < spin_count; ++i) {
                if (!pool->pop())
                    _mm_pause();
            }
        }
    };

    HgThreadPool *pool = new (arena.alloc<HgThreadPool>(1)) HgThreadPool{};

    pool->should_close.store(false);
    pool->thread_count = std::min((usize)1, thread_count - 1);
    pool->threads = arena.alloc<std::thread>(pool->thread_count);

    pool->has_item = arena.alloc<std::atomic_bool>(queue_size);
    pool->items = arena.alloc<Work>(queue_size);
    pool->capacity = queue_size;

    pool->reset();

    for (usize i = 0; i < pool->thread_count; ++i) {
        new (pool->threads + i) std::thread(thread_fn, pool);
    }

    return pool;
}

void HgThreadPool::destroy() {
    mtx.lock();
    should_close = true;
    mtx.unlock();
    cv.notify_all();

    for (usize i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    for (usize i = 0; i < thread_count; ++i) {
        threads[i].~thread();
    }
    cv.~condition_variable();
    mtx.~mutex();
}

void HgThreadPool::reset() {
    count.store(0);
    tail.store(0);
    working_tail.store(0);
    head.store(0);
    working_head.store(0);
    for (usize i = 0; i < capacity; ++i) {
        has_item[i].store(false);
    }
}

void HgThreadPool::push(HgFence* fences, usize fence_count, void* data, void (*fn)(void*)) {
    hg_assert(fn != nullptr);
    for (usize i = 0; i < fence_count; ++i) {
        fences[i].add();
    }

    usize idx = working_head.fetch_add(1) & (capacity - 1);

    items[idx].fences = fences;
    items[idx].fence_count = fence_count;
    items[idx].data = data;
    items[idx].fn = fn;
    has_item[idx].store(true);

    usize h = head.load();
    while (has_item[h].load()) {
        usize next = (h + 1) & (capacity - 1);
        head.compare_exchange_strong(h, next);
        h = next;
    }

    ++count;
    mtx.lock();
    mtx.unlock();
    cv.notify_one();
}

bool HgThreadPool::pop() {
    usize idx = working_tail.load();
    do {
        if (idx == head.load())
            return false;
    } while (!working_tail.compare_exchange_weak(idx, (idx + 1) & (capacity - 1)));

    Work work = items[idx];
    has_item[idx].store(false);

    usize t = tail.load();
    while (t != head.load() && !has_item[t].load()) {
        usize next = (t + 1) & (capacity - 1);
        tail.compare_exchange_strong(t, next);
        t = next;
    }

    --count;

    hg_assert(work.fn != nullptr);
    work.fn(work.data);

    for (usize i = 0; i < work.fence_count; ++i) {
        work.fences[i].signal();
    }
    return true;
}

bool HgThreadPool::help(HgFence& fence, f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!fence.is_complete() && std::chrono::steady_clock::now() < end) {
        if (!pop())
            _mm_pause();
    }
    return fence.is_complete();
}

HgIOThread* HgIOThread::create(HgArena& arena, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    auto thread_fn = [](HgIOThread* io) {
        for (;;) {
            if (io->should_close.load())
                return;

            Request request;
            if (!io->pop(request))
                hg_threads->pop();
        }
    };

    HgIOThread* io = arena.alloc<HgIOThread>(1);

    io->requests = arena.alloc<Request>(queue_size);
    io->capacity = queue_size;

    io->has_item = arena.alloc<std::atomic_bool>(queue_size);

    io->reset();

    io->should_close.store(false);
    new (&io->thread) std::thread(thread_fn, io);

    return io;
}

void HgIOThread::destroy() {
    should_close = true;
    thread.join();
    thread.~thread();
}

void HgIOThread::reset() {
    tail.store(0);
    head.store(0);
    working_head.store(0);
    for (usize i = 0; i < capacity; ++i) {
        has_item[i].store(false);
    }
}

void HgIOThread::push(const Request& request) {
    hg_assert(request.fn != nullptr);
    for (usize i = 0; i < request.fence_count; ++i) {
        request.fences[i].add();
    }

    usize idx = working_head.fetch_add(1) & (capacity - 1);

    requests[idx] = request;
    has_item[idx].store(true);

    usize h = head.load();
    while (has_item[h].load()) {
        usize next = (h + 1) & (capacity - 1);
        head.compare_exchange_strong(h, next);
        h = next;
    }
}

bool HgIOThread::pop(Request& request) {
    usize idx = tail.load() & (capacity - 1);
    if (idx == head.load())
        return false;

    request = requests[idx];
    has_item[idx].store(false);

    tail.fetch_add(1);

    hg_assert(request.fn != nullptr);
    request.fn(request.data, request.resource, request.path);

    for (usize i = 0; i < request.fence_count; ++i) {
        request.fences[i].signal();
    }
    return true;
}

void HgBinary::load(HgFence* fences, usize fence_count, HgStringView path) {
    auto fn = [](void*, void* pbin, HgStringView fpath) {
        HgBinary& bin = *(HgBinary*)pbin;

        char* cpath = (char*)alloca(fpath.length + 1);
        std::memcpy(cpath, fpath.begin(), fpath.size());
        cpath[fpath.length] = '\0';

        FILE* file_handle = std::fopen(cpath, "rb");
        if (file_handle == nullptr) {
            hg_warn("Could not find file to read binary: %s\n", cpath);
            bin = {};
            return;
        }
        hg_defer(std::fclose(file_handle));

        if (std::fseek(file_handle, 0, SEEK_END) != 0) {
            hg_warn("Failed to read binary from file: %s\n", cpath);
            bin = {};
            return;
        }

        bin.size = (usize)std::ftell(file_handle);
        bin.file = std::malloc(bin.size);
        std::rewind(file_handle);

        if (std::fread(bin.file, 1, bin.size, file_handle) != bin.size) {
            hg_warn("Failed to read binary from file: %s\n", cpath);
            bin = {};
            return;
        }
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    request.path = path;
    hg_io->push(request);
}

void HgBinary::unload(HgFence* fences, usize fence_count) {
    auto fn = [](void*, void* pbin, HgStringView) {
        HgBinary& bin = *(HgBinary*)pbin;
        std::free(bin.file);
        bin = {};
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    hg_io->push(request);
}

void HgBinary::store(HgFence* fences, usize fence_count, HgStringView path) {
    auto fn = [](void*, void* pbin, HgStringView fpath) {
        HgBinary& bin = *(HgBinary*)pbin;

        char* cpath = (char*)alloca(fpath.length + 1);
        std::memcpy(cpath, fpath.chars, fpath.length);
        cpath[fpath.length] = '\0';

        FILE* file_handle = std::fopen(cpath, "wb");
        if (file_handle == nullptr) {
            hg_warn("Failed to create file to write binary: %s\n", cpath);
            return;
        }
        hg_defer(std::fclose(file_handle));

        if (std::fwrite(bin.file, 1, bin.size, file_handle) != bin.size) {
            hg_warn("Failed to write binary data to file: %s\n", cpath);
            return;
        }
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    request.path = path;
    hg_io->push(request);
}

void HgTexture::load_png(HgFence* fences, usize fence_count, HgStringView path) {
    auto fn = [](void*, void* ptexture, HgStringView fpath) {
        HgTexture& texture = *(HgTexture*)ptexture;

        char* cpath = (char*)alloca(fpath.length + 1);
        std::memcpy(cpath, fpath.chars, fpath.length);
        cpath[fpath.length] = '\0';

        int channels;
        texture.pixels = stbi_load(cpath, (int*)&texture.width, (int*)&texture.height, &channels, 4);
        if (texture.pixels == nullptr) {
            hg_warn("Failed to load image file: %s\n", cpath);
            texture = {};
            return;
        }

        texture.format = VK_FORMAT_R8G8B8A8_SRGB;
        texture.location |= (u32)Location::cpu;
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    request.path = path;
    hg_io->push(request);
}

void HgTexture::unload(HgFence* fences, usize fence_count) {
    auto fn = [](void*, void* ptexture, HgStringView) {
        HgTexture& texture = *(HgTexture*)ptexture;
        free(texture.pixels);
        texture.pixels = nullptr;
        texture.location &= ~(u32)Location::cpu;
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    hg_io->push(request);
}

void HgTexture::store_png(HgFence* fences, usize fence_count, HgStringView path) {
    auto fn = [](void*, void* ptexture, HgStringView fpath) {
        HgTexture& texture = *(HgTexture*)ptexture;
        hg_assert(texture.location & (u32)Location::cpu);

        char* cpath = (char*)alloca(fpath.length + 1);
        std::memcpy(cpath, fpath.chars, fpath.length);
        cpath[fpath.length] = '\0';

        stbi_write_png(
            cpath,
            (int)texture.width,
            (int)texture.height,
            4,
            texture.pixels,
            (int)(texture.width * 4));
    };

    HgIOThread::Request request;
    request.fences = fences;
    request.fence_count = fence_count;
    request.fn = fn;
    request.resource = this;
    request.path = path;
    hg_io->push(request);
}

void HgTexture::transfer_to_gpu(VkCommandPool cmd_pool, VkFilter filter) {
    hg_assert(location == (u32)Location::cpu);
    hg_assert(pixels != nullptr);
    hg_assert(format != 0);
    hg_assert(width != 0);
    hg_assert(height != 0);
    hg_assert(depth != 0);

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = {width, height, depth};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(hg_vk_vma, &image_info, &alloc_info, &image, &allocation, nullptr);
    hg_assert(allocation != nullptr);
    hg_assert(image != nullptr);

    HgVkImageStagingWriteConfig staging_config{};
    staging_config.dst_image = image;
    staging_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    staging_config.subresource.layerCount = 1;
    staging_config.src_data = pixels;
    staging_config.width = width;
    staging_config.height = height;
    staging_config.depth = depth;
    staging_config.format = format;
    staging_config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(hg_vk_queue, cmd_pool, staging_config);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(hg_vk_device, &view_info, nullptr, &view);
    hg_assert(view != nullptr);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    vkCreateSampler(hg_vk_device, &sampler_info, nullptr, &sampler);
    hg_assert(sampler != nullptr);

    location |= (u32)Location::gpu;
}

void HgTexture::free_from_gpu() {
    hg_assert(location & (u32)Location::gpu);
    vkDestroySampler(hg_vk_device, sampler, nullptr);
    vkDestroyImageView(hg_vk_device, view, nullptr);
    vmaDestroyImage(hg_vk_vma, image, allocation);

    location &= ~(u32)Location::gpu;
}

void HgResourceManager::Resource::load(HgFence* fences, usize fence_count, HgStringView path) {
    hg_assert(data != nullptr);

    if (ref_count++ != 0)
        return;

    switch (type) {
        case HgResource::binary:
            data->binary.load(fences, fence_count, path);
            break;
        case HgResource::texture:
            data->texture.load_png(fences, fence_count, path);
            break;
        default: {
            hg_arena_scope(scratch, hg_get_scratch());
            hg_warn("resource \"%s\" loaded with invalid type\n",
                HgString::create(scratch, path).append(scratch, 0).chars);
        } break;
    }
}

void HgResourceManager::Resource::unload(HgFence* fences, usize fence_count) {
    hg_assert(data != nullptr);
    if (ref_count == 0)
        return;

    if (--ref_count != 0)
        return;

    switch (type) {
        case HgResource::binary:
            data->binary.unload(fences, fence_count);
            break;
        case HgResource::texture:
            data->texture.unload(fences, fence_count);
            break;
        default:
            hg_warn("resource with invalid type unloaded\n");
            break;
    }

    type = HgResource::none;
}

HgResourceManager HgResourceManager::create(HgArena& arena, usize capacity) {
    HgResourceManager rm;

    rm.resources = arena.alloc<Resource>(capacity);
    rm.capacity = capacity;
    rm.count = 0;

    Resource::Data* data = arena.alloc<Resource::Data>(capacity);
    for (usize i = 0; i < rm.capacity; ++i) {
        rm.resources[i].type = HgResource::none;
        rm.resources[i].ref_count = 0;
        rm.resources[i].data = data + i;
    }

    return rm;
}

void HgResourceManager::destroy(HgFence* fences, usize fence_count) {
    for (usize i = 0; i < capacity; ++i) {
        if (resources[i].ref_count > 0) {
            resources[i].ref_count = 1;
            resources[i].unload(fences, fence_count);
        }
    }
}

void HgResourceManager::reset(HgFence* fences, usize fence_count) {
    for (usize i = 0; i < capacity; ++i) {
        if (resources[i].ref_count > 0) {
            resources[i].ref_count = 1;
            resources[i].unload(fences, fence_count);
        }
        resources[i].type = HgResource::none;
        resources[i].ref_count = 0;
    }
}

void HgResourceManager::register_resource(HgResource type, HgResourceID id) {
    hg_assert(count + 1 < capacity);

    if (type == HgResource::none) {
        hg_warn("Attempted to register resource of type none\n");
        type = HgResource::count;
    }

    usize idx = get_idx(id);
    if (idx != (usize)-1) {
        if (resources[idx].type != type)
            hg_warn("Attempted to reregister resource as a different type\n");
        return;
    }

    idx = id % capacity;

    Resource r;
    r.id = id;
    r.type = type;
    r.ref_count = 0;
    r.data = nullptr;
    Resource* fill_data = nullptr;

    usize dist = 0;
    while (resources[idx].type != HgResource::none) {
        usize other_dist = resources[idx].id % capacity;
        if (other_dist < idx)
            other_dist += capacity;
        other_dist -= idx;

        if (other_dist < dist) {
            if (r.data == nullptr)
                fill_data = &resources[idx];
            Resource tmp = resources[idx];
            resources[idx] = r;
            r = tmp;

            dist = other_dist;
        }

        idx = (idx + 1) % capacity;
        ++dist;
    }

    if (fill_data != nullptr) {
        fill_data->data = resources[idx].data;
        resources[idx].data = r.data;
    }
    resources[idx].id = r.id;
    resources[idx].type = r.type;

    ++count;
}

void HgResourceManager::unregister_resource(HgResourceID id) {
    usize idx = get_idx(id);
    if (idx == (usize)-1)
        return;

    if (resources[idx].ref_count != 0) {
        resources[idx].ref_count = 1;
        resources[idx].unload(nullptr, 0);
    }

    Resource::Data* data = resources[idx].data;

    usize next = (idx + 1) % capacity;
    while (resources[next].type != HgResource::none) {
        if (resources[next].id % capacity != idx) {
            resources[idx] = resources[next];
            idx = next;
        }
        next = (next + 1) % capacity;
    }

    resources[idx].type = HgResource::none;
    resources[idx].ref_count = 0;
    resources[idx].data = data;

    --count;
}

bool HgResourceManager::is_registered(HgResourceID id) {
    usize idx = id % capacity;
    while (resources[idx].type != HgResource::none && resources[idx].id != id) {
        idx = (idx + 1) % capacity;
    }
    return resources[idx].type != HgResource::none;
}

usize HgResourceManager::get_idx(HgResourceID id) {
    usize idx = id % capacity;
    while (resources[idx].type != HgResource::none && resources[idx].id != id) {
        idx = (idx + 1) % capacity;
    }
    return resources[idx].type != HgResource::none ? idx : (usize)-1;
}

static u32& hg_internal_current_component_id() {
    static u32 id = 0;
    return id;
}

u32 hg_create_component_id() {
    u32 id = hg_internal_current_component_id();
    ++hg_internal_current_component_id();
    return id;
}

HgECS HgECS::create(HgArena& arena, u32 max_entities) {
    HgECS ecs;

    ecs.entity_pool = arena.alloc<HgEntity>(max_entities);
    ecs.entity_capacity = max_entities;

    ecs.systems = arena.alloc<Component>(hg_internal_current_component_id());
    ecs.system_count = hg_internal_current_component_id();

    for (u32 i = 0; i < ecs.entity_capacity; ++i) {
        ecs.entity_pool[i] = {i + 1};
    }
    ecs.next_entity = {0};

    for (u32 i = 0; i < ecs.system_count; ++i) {
        ecs.systems[i] = {};
    }

    return ecs;
}

void HgECS::reset() {
    for (u32 i = 0; i < system_count; ++i) {
        if (is_registered(i)) {
            std::memset(systems[i].sparse, -1, entity_capacity * sizeof(*systems[i].sparse));
            systems[i].components.reset();
        }
    }
    for (u32 i = 0; i < entity_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    next_entity = {0};
}

void HgECS::resize_entities(HgArena& arena, u32 new_capacity) {
    entity_pool = arena.realloc(entity_pool, entity_capacity, new_capacity);
    for (u32 i = (u32)entity_capacity; i < (u32)new_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    for (usize i = 0; i < system_count; ++i) {
        systems[i].sparse = arena.realloc(systems[i].sparse, entity_capacity, new_capacity);
    }
    entity_capacity = new_capacity;
}

HgEntity HgECS::spawn() {
    hg_assert(next_entity < entity_capacity);

    HgEntity entity = next_entity;
    next_entity = entity_pool[entity];
    entity_pool[entity] = entity;
    return entity;
}

void HgECS::despawn(HgEntity entity) {
    hg_assert(is_alive(entity));

    for (u32 i = 0; i < system_count; ++i) {
        if (is_registered(i) && has(entity, i))
            remove(entity, i);
    }
    entity_pool[entity] = next_entity;
    next_entity = entity;
}

void HgECS::register_component_untyped(
    HgArena& arena,
    u32 max_components,
    u32 component_size,
    u32 component_alignment,
    u32 component_id
) {
    hg_assert(!is_registered(component_id));
    if (component_id >= system_count) {
        systems = arena.realloc(systems, system_count, component_id + 1);
        system_count = component_id + 1;
    }

    Component& system = systems[component_id];
    system.sparse = arena.alloc<u32>(entity_capacity);
    system.dense = arena.alloc<HgEntity>(max_components);
    system.components = system.components.create(arena, component_size, component_alignment, 0, max_components);
    std::memset(system.sparse, -1, entity_capacity * sizeof(*system.sparse));
}

void HgECS::unregister_component_untyped(u32 component_id) {
    if (!is_registered(component_id))
        return;

    systems[component_id] = {};
}

u32 HgECS::smallest_system_untyped(u32* ids, usize id_count) {
    u32 smallest = ids[0];
    hg_assert(is_registered(ids[0]));
    for (usize i = 1; i < id_count; ++i) {
        hg_assert(is_registered(ids[i]));
        if (systems[ids[i]].components.count < systems[smallest].components.count)
            smallest = ids[i];
    }
    return smallest;
}

void HgECS::swap_idx(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(lhs < system.components.count);
    hg_assert(rhs < system.components.count);

    usize width = system.components.width;
    void* temp = alloca(width);
    std::memcpy(temp, system.components.get(lhs), width);
    std::memcpy(system.components.get(lhs), system.components.get(rhs), width);
    std::memcpy(system.components.get(rhs), temp, width);
}

void HgECS::swap_location_idx(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(lhs < system.components.count);
    hg_assert(rhs < system.components.count);

    HgEntity lhs_entity = system.dense[lhs];
    HgEntity rhs_entity = system.dense[rhs];

    hg_assert(is_alive(lhs_entity));
    hg_assert(is_alive(rhs_entity));
    hg_assert(has(lhs_entity, component_id));
    hg_assert(has(rhs_entity, component_id));

    system.dense[lhs] = rhs_entity;
    system.dense[rhs] = lhs_entity;
    system.sparse[lhs_entity] = rhs;
    system.sparse[rhs_entity] = lhs;

    swap_idx(lhs, rhs, component_id);
}

void HgECS::swap_location(HgEntity lhs, HgEntity rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(is_alive(lhs));
    hg_assert(is_alive(rhs));
    hg_assert(has(lhs, component_id));
    hg_assert(has(rhs, component_id));

    u32 lhs_index = system.sparse[lhs];
    u32 rhs_index = system.sparse[rhs];

    system.dense[lhs_index] = rhs;
    system.dense[rhs_index] = lhs;
    system.sparse[lhs] = rhs_index;
    system.sparse[rhs] = lhs_index;

    swap_idx(lhs_index, rhs_index, component_id);
}

void HgECS::sort_untyped(
    u32 begin,
    u32 end,
    u32 component_id,
    void* data,
    bool (*compare)(void*, HgEntity lhs, HgEntity rhs)
) {
    hg_assert(is_registered(component_id));

    auto inter = [this, component_id, data, compare](u32 pivot, u32 inc, u32 dec) -> u32 {
        while (inc != dec) {
            while (!compare(data, systems[component_id].dense[dec], systems[component_id].dense[pivot])) {
                --dec;
                if (dec == inc)
                    goto finish;
            }
            while (!compare(data, systems[component_id].dense[pivot], systems[component_id].dense[inc])) {
                ++inc;
                if (inc == dec)
                    goto finish;
            }
            swap_location_idx(inc, dec, component_id);
        }

finish:
        if (compare(data, systems[component_id].dense[inc], systems[component_id].dense[pivot]))
            swap_location_idx(pivot, inc, component_id);

        return inc;
    };

    auto quicksort = [this, component_id, inter](const auto& recurse, u32 qbegin, u32 qend) {
        hg_assert(qbegin <= qend && qend <= component_count(component_id));

        if (qbegin + 1 >= qend)
            return;

        u32 middle = inter(qbegin, qbegin + 1, qend - 1);
        recurse(recurse, qbegin, middle);
        recurse(recurse, middle, qend);
    };

    quicksort(quicksort, begin, end);
}

void HgTransform::create_child(HgEntity child) {
    HgTransform& old_first = hg_ecs->get<HgTransform>(first_child);
    HgTransform& new_first = hg_ecs->get<HgTransform>(child);
    old_first.prev_sibling = child;
    new_first.next_sibling = first_child;
    first_child = child;
}

void HgTransform::detach() {
    if (parent != HgEntity{}) {
        if (first_child == HgEntity{}) {
            if (prev_sibling == HgEntity{}) {
                hg_ecs->get<HgTransform>(parent).first_child = next_sibling;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = HgEntity{};
            } else {
                hg_ecs->get<HgTransform>(prev_sibling).next_sibling = next_sibling;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = prev_sibling;
            }
        } else {
            HgEntity last_child = first_child;
            for (;;) {
                HgTransform& tf = hg_ecs->get<HgTransform>(last_child);
                tf.parent = parent;
                if (tf.next_sibling == HgEntity{})
                    break;
                last_child = tf.next_sibling;
            }
            if (prev_sibling == HgEntity{}) {
                hg_ecs->get<HgTransform>(parent).first_child = first_child;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = last_child;
            } else {
                hg_ecs->get<HgTransform>(prev_sibling).next_sibling = first_child;
                hg_ecs->get<HgTransform>(next_sibling).prev_sibling = last_child;
            }
        }
    } else {
        HgEntity child = first_child;
        while (child != HgEntity{}) {
            HgTransform& tf = hg_ecs->get<HgTransform>(child);
            child = tf.next_sibling;
            tf.parent = HgEntity{};
            tf.next_sibling = HgEntity{};
            tf.prev_sibling = HgEntity{};
        }
    }
}

void HgTransform::destroy() {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = hg_ecs->get<HgTransform>(child);
        tf.destroy();
        child = tf.next_sibling;
    }
    if (parent != HgEntity{}) {
        if (prev_sibling == HgEntity{}) {
            hg_ecs->get<HgTransform>(parent).first_child = next_sibling;
            hg_ecs->get<HgTransform>(next_sibling).prev_sibling = HgEntity{};
        } else {
            hg_ecs->get<HgTransform>(prev_sibling).next_sibling = next_sibling;
            hg_ecs->get<HgTransform>(next_sibling).prev_sibling = prev_sibling;
        }
    }
    hg_ecs->despawn(hg_ecs->get_entity(*this));
}

void HgTransform::move(const HgVec3& dp, const HgVec3& ds, const HgQuat& dr) {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = hg_ecs->get<HgTransform>(child);
        // tf.move(
        //     hg_rotate(dr, dp + (tf.position - position) * scale * ds / tf.scale) - tf.position,
        //     ds,
        //     dr);
        child = tf.next_sibling;
    }
    position += dp;
    scale *= ds;
    rotation = dr * rotation;
}

void HgTransform::set(const HgVec3& p, const HgVec3& s, const HgQuat& r) {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = hg_ecs->get<HgTransform>(child);
        // tf.set(
        //     hg_rotate(r, (tf.position - position) * s / tf.scale + p),
        //     s * tf.scale / scale,
        //     r);
        child = tf.next_sibling;
    }
    position = p;
    scale = s;
    rotation = r;
}

#include "sprite.frag.spv.h"
#include "sprite.vert.spv.h"

HgPipeline2D HgPipeline2D::create(
    HgArena& arena,
    usize max_textures,
    VkFormat color_format,
    VkFormat depth_format
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(color_format != VK_FORMAT_UNDEFINED);

    HgPipeline2D pipeline{};
    pipeline.texture_sets = pipeline.texture_sets.create(arena, max_textures);

    VkDescriptorSetLayoutBinding vp_bindings[1]{};
    vp_bindings[0].binding = 0;
    vp_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vp_bindings[0].descriptorCount = 1;
    vp_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo vp_layout_info{};
    vp_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vp_layout_info.bindingCount = hg_countof(vp_bindings);
    vp_layout_info.pBindings = vp_bindings;

    vkCreateDescriptorSetLayout(hg_vk_device, &vp_layout_info, nullptr, &pipeline.vp_layout);
    hg_assert(pipeline.vp_layout != nullptr);

    VkDescriptorSetLayoutBinding texture_bindings[1]{};
    texture_bindings[0].binding = 0;
    texture_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[0].descriptorCount = 1;
    texture_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texture_layout_info{};
    texture_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_layout_info.bindingCount = hg_countof(texture_bindings);
    texture_layout_info.pBindings = texture_bindings;

    vkCreateDescriptorSetLayout(hg_vk_device, &texture_layout_info, nullptr, &pipeline.texture_layout);
    hg_assert(pipeline.texture_layout != nullptr);

    VkDescriptorSetLayout set_layouts[]{pipeline.vp_layout, pipeline.texture_layout};
    VkPushConstantRange push_ranges[1]{};
    push_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_ranges[0].size = sizeof(Push);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = hg_countof(set_layouts);
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = hg_countof(push_ranges);
    layout_info.pPushConstantRanges = push_ranges;

    vkCreatePipelineLayout(hg_vk_device, &layout_info, nullptr, &pipeline.pipeline_layout);
    hg_assert(pipeline.pipeline_layout != nullptr);

    VkShaderModuleCreateInfo vertex_shader_info{};
    vertex_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_info.codeSize = sprite_vert_spv_size;
    vertex_shader_info.pCode = (u32*)sprite_vert_spv;

    VkShaderModule vertex_shader = nullptr;
    vkCreateShaderModule(hg_vk_device, &vertex_shader_info, nullptr, &vertex_shader);
    hg_assert(vertex_shader != nullptr);

    VkShaderModuleCreateInfo fragment_shader_info{};
    fragment_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_info.codeSize = sprite_frag_spv_size;
    fragment_shader_info.pCode = (u32*)sprite_frag_spv;

    VkShaderModule fragment_shader = nullptr;
    vkCreateShaderModule(hg_vk_device, &fragment_shader_info, nullptr, &fragment_shader);
    hg_assert(fragment_shader != nullptr);

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    HgVkPipelineConfig pipeline_config{};
    pipeline_config.color_attachment_formats = &color_format;
    pipeline_config.color_attachment_format_count = 1;
    pipeline_config.depth_attachment_format = depth_format;
    pipeline_config.stencil_attachment_format = VK_FORMAT_UNDEFINED;
    pipeline_config.shader_stages = shader_stages;
    pipeline_config.shader_count = hg_countof(shader_stages);
    pipeline_config.layout = pipeline.pipeline_layout;
    pipeline_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipeline_config.enable_color_blend = true;

    pipeline.pipeline = hg_vk_create_graphics_pipeline(pipeline_config);

    vkDestroyShaderModule(hg_vk_device, fragment_shader, nullptr);
    vkDestroyShaderModule(hg_vk_device, vertex_shader, nullptr);

    VkDescriptorPoolSize desc_pool_sizes[2]{};
    desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_pool_sizes[0].descriptorCount = 1;
    desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_pool_sizes[1].descriptorCount = (u32)max_textures;

    VkDescriptorPoolCreateInfo desc_pool_info{};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc_pool_info.maxSets = 1 + (u32)max_textures;
    desc_pool_info.poolSizeCount = hg_countof(desc_pool_sizes);
    desc_pool_info.pPoolSizes = desc_pool_sizes;

    vkCreateDescriptorPool(hg_vk_device, &desc_pool_info, nullptr, &pipeline.descriptor_pool);
    hg_assert(pipeline.descriptor_pool != nullptr);

    VkDescriptorSetAllocateInfo vp_set_alloc_info{};
    vp_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vp_set_alloc_info.descriptorPool = pipeline.descriptor_pool;
    vp_set_alloc_info.descriptorSetCount = 1;
    vp_set_alloc_info.pSetLayouts = &pipeline.vp_layout;

    vkAllocateDescriptorSets(hg_vk_device, &vp_set_alloc_info, &pipeline.vp_set);
    hg_assert(pipeline.vp_set != nullptr);

    VkBufferCreateInfo vp_buffer_info{};
    vp_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vp_buffer_info.size = sizeof(VPUniform);
    vp_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo vp_alloc_info{};
    vp_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vp_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(
        hg_vk_vma,
        &vp_buffer_info,
        &vp_alloc_info,
        &pipeline.vp_buffer,
        &pipeline.vp_buffer_allocation,
        nullptr);
    hg_assert(pipeline.vp_buffer != nullptr);
    hg_assert(pipeline.vp_buffer_allocation != nullptr);

    VPUniform vp_data{};
    vp_data.proj = 1.0f;
    vp_data.view = 1.0f;

    vmaCopyMemoryToAllocation(hg_vk_vma, &vp_data, pipeline.vp_buffer_allocation, 0, sizeof(vp_data));

    VkDescriptorBufferInfo desc_info{};
    desc_info.buffer = pipeline.vp_buffer;
    desc_info.offset = 0;
    desc_info.range = sizeof(VPUniform);

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = pipeline.vp_set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_write.pBufferInfo = &desc_info;

    vkUpdateDescriptorSets(hg_vk_device, 1, &desc_write, 0, nullptr);

    return pipeline;
}

void HgPipeline2D::destroy() {
    vmaDestroyBuffer(hg_vk_vma, vp_buffer, vp_buffer_allocation);
    vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, &vp_set);
    vkDestroyDescriptorPool(hg_vk_device, descriptor_pool, nullptr);
    vkDestroyPipeline(hg_vk_device, pipeline, nullptr);
    vkDestroyPipelineLayout(hg_vk_device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, texture_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, vp_layout, nullptr);
}

void HgPipeline2D::add_texture(HgResourceID texture_id) {
    hg_assert(hg_resources->is_registered(texture_id));

    HgTexture& texture = hg_resources->get<HgTexture>(texture_id);
    hg_assert(texture.location & (u32)HgTexture::Location::gpu);

    if (texture_sets.has(texture_id))
        return;

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &texture_layout;

    VkDescriptorSet set = nullptr;
    vkAllocateDescriptorSets(hg_vk_device, &set_info, &set);
    hg_assert(set != nullptr);

    VkDescriptorImageInfo desc_info{};
    desc_info.sampler = texture.sampler;
    desc_info.imageView = texture.view;
    desc_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_write.pImageInfo = &desc_info;

    vkUpdateDescriptorSets(hg_vk_device, 1, &desc_write, 0, nullptr);

    texture_sets.insert(texture_id, set);
}

void HgPipeline2D::remove_texture(HgResourceID texture_id) {
    hg_assert(hg_resources->is_registered(texture_id));

    VkDescriptorSet* set = texture_sets.get(texture_id);
    if (set != nullptr) {
        texture_sets.remove(texture_id);
        vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, set);
    }
}

void HgPipeline2D::update_projection(const HgMat4& projection) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &projection,
        vp_buffer_allocation,
        offsetof(VPUniform, proj),
        sizeof(projection));
}

void HgPipeline2D::update_view(const HgMat4& view) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &view,
        vp_buffer_allocation,
        offsetof(VPUniform, view),
        sizeof(view));
}

void HgPipeline2D::draw(VkCommandBuffer cmd) {
    hg_assert(cmd != nullptr);
    hg_assert(hg_ecs->is_registered<HgSprite>());

    hg_ecs->sort<HgSprite>(nullptr, [](void*, HgEntity lhs, HgEntity rhs) -> bool {
        hg_assert(hg_ecs->has<HgTransform>(lhs));
        hg_assert(hg_ecs->has<HgTransform>(rhs));
        return hg_ecs->get<HgTransform>(lhs).position.z > hg_ecs->get<HgTransform>(rhs).position.z;
    });

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        1,
        &vp_set,
        0,
        nullptr);

    hg_ecs->for_each<HgSprite, HgTransform>([&](HgEntity, HgSprite& sprite, HgTransform& transform) {
        hg_assert(texture_sets.has(sprite.texture));
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            1,
            1,
            texture_sets.get(sprite.texture),
            0,
            nullptr);

        Push push;
        push.model = hg_model_matrix_3d(transform.position, transform.scale, transform.rotation);
        push.uv_pos = sprite.uv_pos;
        push.uv_size = sprite.uv_size;

        vkCmdPushConstants(
            cmd,
            pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(push),
            &push);

        vkCmdDraw(cmd, 4, 1, 0, 0);
    });
}

// struct HgSceneDescInfo {
//     u32 version_major;
//     u32 version_minor;
//     u32 version_patch;
//     u32 entity_count;
//     u32 component_count;
//     u32 components_idx;
//     u32 resource_count;
//     u32 resources_idx;
// };
//
// struct HgSceneDescComponent {
//     HgComponent type;
//     u32 count;
//     u32 entities_begin_idx;
//     u32 components_begin_idx;
// };
//
// struct HgSceneDescResource {
//     HgResource type;
//     u32 length;
//     u32 path_idx;
// };
//
// void HgScene::register_resources() {
//     hg_assert(hg_resources != nullptr);
//
//     HgSceneDescInfo info = desc.read<HgSceneDescInfo>(0);
//
//     usize resource_idx = info.resources_idx;
//     usize resource_end = resource_idx + info.resource_count * sizeof(HgSceneDescResource);
//     for (; resource_idx < resource_end; resource_idx += sizeof(HgSceneDescResource)) {
//         HgSceneDescResource resource_desc = desc.read<HgSceneDescResource>(resource_idx);
//
//         HgStringView path = {(char*)desc.file + resource_desc.path_idx, resource_desc.length};
//         hg_resources->register_resource(resource_desc.type, hg_resource_id(path));
//
//         if (!(resource_desc.type > HgResource::none && resource_desc.type < HgResource::count)) {
//             hg_arena_scope(scratch, hg_get_scratch());
//             hg_warn("Invalid resource type found with file: %s\n",
//                 HgString::create(scratch, path).append(scratch, 0).chars);
//         }
//     }
// }
//
// void HgScene::load(HgFence* fences, usize fence_count) {
//     if (loaded)
//         return;
//
//     HgSceneDescInfo info = desc.read<HgSceneDescInfo>(0);
//
//     usize resource_idx = info.resources_idx;
//     usize resource_end = resource_idx + info.resource_count * sizeof(HgSceneDescResource);
//     for (; resource_idx < resource_end; resource_idx += sizeof(HgSceneDescResource)) {
//         HgSceneDescResource resource_desc = desc.read<HgSceneDescResource>(resource_idx);
//
//         HgStringView path = {(char*)desc.file + resource_desc.path_idx, resource_desc.length};
//         hg_resources->load(fences, fence_count, path);
//     }
//
//     loaded = true;
// }
//
// void HgScene::unload(HgFence* fences, usize fence_count) {
//     if (!loaded)
//         return;
//
//     HgSceneDescInfo info = desc.read<HgSceneDescInfo>(0);
//
//     usize resource_idx = info.resources_idx;
//     usize resource_end = resource_idx + info.resource_count * sizeof(HgSceneDescResource);
//     for (; resource_idx < resource_end; resource_idx += sizeof(HgSceneDescResource)) {
//         HgSceneDescResource resource_desc = desc.read<HgSceneDescResource>(resource_idx);
//
//         HgStringView path = {(char*)desc.file + resource_desc.path_idx, resource_desc.length};
//         hg_resources->unload(fences, fence_count, hg_resource_id(path));
//     }
//
//     loaded = false;
// }
//
// void HgScene::instantiate(HgArena& arena) {
//     hg_assert(!instantiated);
//
//     hg_arena_scope(scratch, hg_get_scratch());
//
//     HgSceneDescInfo info = desc.read<HgSceneDescInfo>(0);
//
//     while (entity_capacity < info.entity_count) {
//         usize new_capacity = entity_capacity == 0 ? 1 : entity_capacity * 2;
//         entities = arena.realloc(entities, entity_capacity, new_capacity);
//         entity_capacity = new_capacity;
//     }
//     for (usize i = 0; i < info.entity_count; ++i) {
//         entities[i] = hg_ecs->spawn();
//     }
//
//     usize component_idx = info.components_idx;
//     usize component_end = component_idx + info.component_count * sizeof(HgSceneDescComponent);
//     for (; component_idx < component_end; component_idx += sizeof(HgSceneDescComponent)) {
//         HgSceneDescComponent component_desc = desc.read<HgSceneDescComponent>(component_idx);
//
//         usize entities_idx = component_desc.entities_begin_idx;
//         usize components_idx = component_desc.components_begin_idx;
//         switch (component_desc.type) {
//         case HgComponent::transform:
//             for (usize i = 0; i < component_desc.count; ++i) {
//                 HgEntity entity = entities[desc.read<HgEntity>(entities_idx + i * sizeof(HgEntity))];
//                 hg_ecs->add(entity, desc.read<HgTransform>(components_idx + i * sizeof(HgTransform)));
//             }
//             break;
//         case HgComponent::sprite:
//             for (usize i = 0; i < component_desc.count; ++i) {
//                 HgEntity entity = entities[desc.read<HgEntity>(entities_idx + i * sizeof(HgEntity))];
//                 hg_ecs->add(entity, desc.read<HgSprite>(components_idx + i * sizeof(HgSprite)));
//             }
//             break;
//         default:
//             hg_warn("Invalid component type found\n");
//             break;
//         }
//     }
//
//     instantiated = true;
// }
//
// void HgScene::deinstantiate() {
//     hg_assert(instantiated);
//     for (usize i = 0; i < entity_capacity; ++i) {
//         hg_ecs->despawn(entities[i]);
//     }
//     instantiated = false;
// }
//
// static HgComponent hg_internal_component_str_to_enum(HgStringView str) {
//     if (str.length == 0)
//         return HgComponent::none;
//
//     switch (str[0]) {
//         case 't':
//             if (str == "transform")
//                 return HgComponent::transform;
//             break;
//         case 's':
//             if (str == "sprite")
//                 return HgComponent::sprite;
//             break;
//     }
//
//     return HgComponent::none;
// }
//
// struct HgSceneJsonParser {
//     struct Component {
//         HgAnyArray data;
//         u32* indices;
//
//         static Component create(HgArena& arena, usize max, u32 width, u32 alignment) {
//             Component c;
//             c.data = c.data.create(arena, width, alignment, 0, max);
//             c.indices = arena.alloc<u32>(max);
//             return c;
//         }
//     };
//
//     struct Resource {
//         HgResource type;
//         HgStringView path;
//     };
//
//     static constexpr auto comp_hash = [](HgComponent c) constexpr { return (usize)c; };
//     static constexpr auto res_hash = [](HgResourceID r) constexpr { return (usize)r; };
//
//     HgHashMap<HgStringView, usize> entities;
//     HgHashMap<HgComponent, Component, comp_hash> components;
//     HgHashMap<HgResourceID, Resource, res_hash> resources;
//
//     HgBinary parse(HgArena& arena, const HgJson& json);
//
//     HgTransform parse_transform(HgStringView entity, const HgJson::Field* transform);
//     HgSprite parse_sprite(HgStringView entity, const HgJson::Field* sprite);
// };
//
// HgBinary HgSceneJsonParser::parse(HgArena& arena, const HgJson& json) {
//     hg_arena_scope(scratch, hg_get_scratch(arena));
//
//     entities = entities.create(scratch, 512);
//     components = components.create(scratch, (usize)HgComponent::count);
//     resources = resources.create(scratch, 128);
//
//     for (HgJson::Error* e = json.errors; e != nullptr; e = e->next) {
//         hg_warn("Scene description json file has error: %s", HgString::create(scratch, e->msg).append(scratch, 0).chars);
//     }
//
//     if (json.file == nullptr)
//         return {};
//
//     if (json.file->type != HgJson::jstruct) {
//         hg_warn("Scene description json file does not begin with a struct\n");
//         return {};
//     }
//
//     for (const HgJson::Field* entity = json.file->jstruct.fields; entity != nullptr; entity = entity->next) {
//         if (entities.has(entity->name)) {
//             hg_warn("Json scene file has duplicate entity name \"%s\", may result in unexpected behavior\n",
//                 HgString::create(arena, entity->name).append(scratch, 0).chars);
//             continue;
//         }
//         if (entity->value->type != HgJson::jstruct) {
//             hg_warn("Json scene file has an entity \"%s\" which is not a struct of components\n",
//                 HgString::create(scratch, entity->name).append(scratch, 0).chars);
//             continue;
//         }
//         if (entities.is_half_full())
//             entities.grow(arena);
//         entities.insert(entity->name, entities.load);
//     }
//
// #define hg_macro_parse_json_component(type_enum, type_symbol) \
//     case HgComponent::type_enum: { \
//         if (!components.has(type)) \
//             components.insert(type, Component::create( \
//                 arena, entities.load, sizeof(type_symbol), alignof(type_symbol))); \
//         Component& c = *components.get(type); \
//         c.indices[c.data.count] = (u32)*entities.get(entity->name); \
//         *(type_symbol*)c.data.push() = parse_##type_enum(entity->name, comp); \
//     } break
//
//     for (const HgJson::Field* entity = json.file->jstruct.fields; entity != nullptr; entity = entity->next) {
//         if (entity->value->type != HgJson::jstruct)
//             continue;
//
//         for (HgJson::Field* comp = entity->value->jstruct.fields; comp != nullptr; comp = comp->next) {
//             HgComponent type = hg_internal_component_str_to_enum(comp->name);
//
//             switch(type) {
//             case HgComponent ::transform: {
//               if (!components.has(type))
//                 components.insert(type,
//                                   Component ::create(arena, entities.load,
//                                                      sizeof(HgTransform),
//                                                      alignof(HgTransform)));
//               Component &c = *components.get(type);
//               c.indices[c.data.count] = (u32)*entities.get(entity->name);
//               *(HgTransform *)c.data.push() =
//                   parse_transform(entity->name, comp);
//             } break;
//             case HgComponent ::sprite: {
//               if (!components.has(type))
//                 components.insert(type, Component ::create(arena, entities.load,
//                                                            sizeof(HgSprite),
//                                                            alignof(HgSprite)));
//               Component &c = *components.get(type);
//               c.indices[c.data.count] = (u32)*entities.get(entity->name);
//               *(HgSprite *)c.data.push() = parse_sprite(entity->name, comp);
//             } break;
//
//             default: {
//               hg_arena_scope(s, hg_get_scratch());
//               hg_warn(
//                   "Json scene file has entity \"%s\" which has an invalid "
//                   "component \"%s\"",
//                   HgString::create(s, entity->name).append(scratch, 0).chars,
//                   HgString::create(s, comp->name).append(scratch, 0).chars);
//             }
//             }
//         }
//     }
//
// #undef hg_macro_parse_json_component
//
//     HgBinary bin{};
//
//     HgSceneDescInfo info;
//     info.version_major = HgScene::desc_version_major;
//     info.version_minor = HgScene::desc_version_minor;
//     info.version_patch = HgScene::desc_version_patch;
//     info.entity_count = (u32)entities.load;
//
//     usize info_idx = bin.size;
//     bin.grow(arena, sizeof(HgSceneDescInfo));
//
//     info.component_count = (u32)components.load;
//     info.components_idx = (u32)bin.size;
//     bin.grow(arena, components.load * sizeof(HgSceneDescComponent));
//
//     info.resource_count = (u32)resources.load;
//     info.resources_idx = (u32)bin.size;
//     bin.grow(arena, resources.load * sizeof(HgSceneDescResource));
//
//     bin.overwrite(info_idx, info);
//
//     usize comp_idx = info.components_idx;
//     for (auto [type, comp] : components) {
//         HgSceneDescComponent desc;
//         desc.type = type;
//         desc.count = (u32)comp.data.count;
//
//         desc.entities_begin_idx = (u32)bin.size;
//         bin.grow(arena, desc.count * sizeof(*comp.indices));
//         bin.overwrite(desc.entities_begin_idx, comp.indices, desc.count * sizeof(*comp.indices));
//
//         desc.components_begin_idx = (u32)bin.size;
//         bin.grow(arena, desc.count * comp.data.width);
//         bin.overwrite(desc.components_begin_idx, comp.data.items, desc.count * comp.data.width);
//
//         bin.overwrite(comp_idx, desc);
//         comp_idx += sizeof(desc);
//     }
//
//     usize resource_idx = info.resources_idx;
//     for (auto [id, res] : resources) {
//         HgSceneDescResource desc;
//         desc.type = res.type;
//         desc.length = (u32)res.path.length;
//
//         desc.path_idx = (u32)bin.size;
//         bin.grow(arena, desc.length);
//         bin.overwrite(desc.path_idx, res.path.chars, res.path.length);
//
//         bin.overwrite(resource_idx, desc);
//         resource_idx += sizeof(desc);
//     }
//
//     return bin;
// }
//
// HgTransform HgSceneJsonParser::parse_transform(HgStringView name, const HgJson::Field* transform) {
//     hg_arena_scope(scratch, hg_get_scratch());
//
//     if (transform->value->type != HgJson::jstruct) {
//         hg_warn("In json scene file, entity \"%s\" transform does not contain a struct\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//         return {};
//     }
//
//     HgTransform t{};
//     bool has_pos = false;
//     bool has_scale = false;
//     bool has_rot = false;
//
//     for (HgJson::Field* field = transform->value->jstruct.fields; field != nullptr; field = field->next) {
//
//         if (field->name.length == 0) {
//             hg_warn("In json scene file, entity \"%s\" transform has field with no name\n",
//                 HgString::create(scratch, name).append(scratch, 0).chars);
//             continue;
//         }
//         switch (field->name[0]) {
//             case 'p':
//                 if (field->name != "position")
//                     hg_warn("In json scene file, entity \"%s\" transform has field \"%s\", did you mean position?\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars,
//                         HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 if (field->value->type != HgJson::array) {
//                     hg_warn("In json scene file, entity \"%s\" transform position is not an array\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars);
//                 } else {
//                     if (has_pos) {
//                         hg_warn("In json scene file, entity \"%s\" transform has multiple positions\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                         break;
//                     }
//                     usize count = 0;
//                     for (HgJson::Elem* elem = field->value->array.elems; elem != nullptr; elem = elem->next) {
//                         if (count >= 3) {
//                             hg_warn("In json scene file, entity \"%s\" transform position has too many elements\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                         if (elem->value->type == HgJson::floating) {
//                             t.position[count++] = (f32)elem->value->floating;
//                         } else if (elem->value->type == HgJson::integer) {
//                             t.position[count++] = (f32)elem->value->integer;
//                         } else {
//                             hg_warn("In json scene file, entity \"%s\" transform position does not contain numbers\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                     }
//                     if (count < 3)
//                         hg_warn("In json scene file, entity \"%s\" transform position has too few elements\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     has_pos = true;
//                 }
//                 break;
//             case 's':
//                 if (field->name != "scale")
//                     hg_warn("In json scene file, entity \"%s\" transform has field \"%s\", did you mean scale?\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars,
//                         HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 if (field->value->type != HgJson::array) {
//                     hg_warn("In json scene file, entity \"%s\" transform scale is not an array\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars);
//                 } else {
//                     if (has_scale) {
//                         hg_warn("In json scene file, entity \"%s\" transform has multiple scales\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                         break;
//                     }
//                     usize count = 0;
//                     for (HgJson::Elem* elem = field->value->array.elems; elem != nullptr; elem = elem->next) {
//                         if (count >= 3) {
//                             hg_warn("In json scene file, entity \"%s\" transform scale has too many elements\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                         if (elem->value->type == HgJson::floating) {
//                             t.scale[count++] = (f32)elem->value->floating;
//                         } else if (elem->value->type == HgJson::integer) {
//                             t.scale[count++] = (f32)elem->value->integer;
//                         } else {
//                             hg_warn("In json scene file, entity \"%s\" transform scale does not contain numbers\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                     }
//                     if (count < 3)
//                         hg_warn("In json scene file, entity \"%s\" transform scale has too few elements\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     has_scale = true;
//                 }
//                 break;
//             case 'r':
//                 if (field->name != "rotation")
//                     hg_warn("In json scene file, entity \"%s\" transform has field \"%s\", did you mean rotation?\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars,
//                         HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 if (field->value->type != HgJson::array) {
//                     hg_warn("In json scene file, entity \"%s\" transform rotation is not an array\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars);
//                 } else {
//                     if (has_rot) {
//                         hg_warn("In json scene file, entity \"%s\" transform has multiple rotations\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                         break;
//                     }
//                     usize count = 0;
//                     for (HgJson::Elem* elem = field->value->array.elems; elem != nullptr; elem = elem->next) {
//                         if (count >= 4) {
//                             hg_warn("In json scene file, entity \"%s\" transform rotations has too many elements\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                         if (elem->value->type == HgJson::floating) {
//                             t.rotation[count++] = (f32)elem->value->floating;
//                         } else if (elem->value->type == HgJson::integer) {
//                             t.rotation[count++] = (f32)elem->value->integer;
//                         } else {
//                             hg_warn("In json scene file, entity \"%s\" transform rotation does not contain numbers\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                     }
//                     if (count < 4)
//                         hg_warn("In json scene file, entity \"%s\" transform rotation has too few elements\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     has_rot = true;
//                 }
//                 break;
//             default:
//                 hg_warn("In json scene file, entity \"%s\" transform has unknown field \"%s\"\n",
//                     HgString::create(scratch, name).append(scratch, 0).chars,
//                     HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 break;
//         }
//     }
//
//     if (!has_pos)
//         hg_warn("In json scene file, entity \"%s\" transform has no position\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//     if (!has_scale)
//         hg_warn("In json scene file, entity \"%s\" transform has no scale\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//     if (!has_rot)
//         hg_warn("In json scene file, entity \"%s\" transform has no rotation\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//
//     return t;
// }
//
// HgSprite HgSceneJsonParser::parse_sprite(HgStringView name, const HgJson::Field* sprite) {
//     hg_arena_scope(scratch, hg_get_scratch());
//
//     if (sprite->value->type != HgJson::jstruct) {
//         hg_warn("In json scene file, entity \"%s\" sprite does not contain a struct\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//         return {};
//     }
//
//     HgSprite s{};
//     bool has_tex = false;
//     bool has_pos = false;
//     bool has_size = false;
//
//     for (HgJson::Field* field = sprite->value->jstruct.fields; field != nullptr; field = field->next) {
//
//         if (field->name.length == 0) {
//             hg_warn("In json scene file, entity \"%s\" sprite has field with no name\n",
//                 HgString::create(scratch, name).append(scratch, 0).chars);
//             continue;
//         }
//         switch (field->name[0]) {
//             case 't':
//                 if (field->name != "texture")
//                     hg_warn("In json scene file, entity \"%s\" sprite has field \"%s\", did you mean texture?\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars,
//                         HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 if (field->value->type != HgJson::string) {
//                     hg_warn("In json scene file, entity \"%s\" sprite texture is not a string\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars);
//                 } else {
//                     if (has_pos) {
//                         hg_warn("In json scene file, entity \"%s\" sprite has multiple textures\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     }
//                     s.texture = hg_hash(field->value->string);
//                     if (!resources.has(s.texture)) {
//                         resources.insert(s.texture, {HgResource::texture, field->value->string});
//                     } else {
//                         Resource& tex = *resources.get(s.texture);
//                         if (tex.type != HgResource::texture) {
//                             hg_warn("In json scene file, resource \"%s\" is used as more than one type\n",
//                                 HgString::create(scratch, field->value->string).append(scratch, 0).chars);
//                         }
//                         if (tex.path != field->value->string) {
//                             hg_warn("In json scene file, resource \"%s\" collides with \"%s\"\n",
//                                 HgString::create(scratch, tex.path).append(scratch, 0).chars,
//                                 HgString::create(scratch, field->value->string).append(scratch, 0).chars);
//                         }
//                     }
//                     has_tex = true;
//                 }
//                 break;
//             case 'u':
//                 if (field->name == "uv_pos") {
//                     if (field->value->type != HgJson::array) {
//                         hg_warn("In json scene file, entity \"%s\" sprite uv_pos is not an array\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     } else {
//                         if (has_pos) {
//                             hg_warn("In json scene file, entity \"%s\" sprite has multiple uv_pos\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                         usize count = 0;
//                         for (HgJson::Elem* elem = field->value->array.elems; elem != nullptr; elem = elem->next) {
//                             if (count >= 2) {
//                                 hg_warn("In json scene file, entity \"%s\" sprite uv_pos has too many elements\n",
//                                     HgString::create(scratch, name).append(scratch, 0).chars);
//                                 break;
//                             }
//                             if (elem->value->type == HgJson::floating) {
//                                 s.uv_pos[count++] = (f32)elem->value->floating;
//                             } else if (elem->value->type == HgJson::integer) {
//                                 s.uv_pos[count++] = (f32)elem->value->integer;
//                             } else {
//                                 hg_warn("In json scene file, entity \"%s\" sprite uv_pos does not contain numbers\n",
//                                     HgString::create(scratch, name).append(scratch, 0).chars);
//                                 break;
//                             }
//                         }
//                         if (count < 2)
//                             hg_warn("In json scene file, entity \"%s\" sprite uv_pos has too few elements\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                         has_pos = true;
//                     }
//                 } else if (field->name == "uv_size") {
//                     if (field->value->type != HgJson::array) {
//                         hg_warn("In json scene file, entity \"%s\" sprite uv_size is not an array\n",
//                             HgString::create(scratch, name).append(scratch, 0).chars);
//                     } else {
//                         if (has_size) {
//                             hg_warn("In json scene file, entity \"%s\" sprite has multiple uv_size\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                             break;
//                         }
//                         usize count = 0;
//                         for (HgJson::Elem* elem = field->value->array.elems; elem != nullptr; elem = elem->next) {
//                             if (count >= 2) {
//                                 hg_warn("In json scene file, entity \"%s\" sprite uv_size has too many elements\n",
//                                     HgString::create(scratch, name).append(scratch, 0).chars);
//                                 break;
//                             }
//                             if (elem->value->type == HgJson::floating) {
//                                 s.uv_size[count++] = (f32)elem->value->floating;
//                             } else if (elem->value->type == HgJson::integer) {
//                                 s.uv_size[count++] = (f32)elem->value->integer;
//                             } else {
//                                 hg_warn("In json scene file, entity \"%s\" sprite uv_size does not contain numbers\n",
//                                     HgString::create(scratch, name).append(scratch, 0).chars);
//                                 break;
//                             }
//                         }
//                         if (count < 2)
//                             hg_warn("In json scene file, entity \"%s\" sprite uv_size has too few elements\n",
//                                 HgString::create(scratch, name).append(scratch, 0).chars);
//                         has_size = true;
//                     }
//                 } else {
//                     hg_warn("In json scene file, entity \"%s\" sprite has unknown field \"%s\"\n",
//                         HgString::create(scratch, name).append(scratch, 0).chars,
//                         HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 }
//
//                 break;
//             default:
//                 hg_warn("In json scene file, entity \"%s\" sprite has unknown field \"%s\"\n",
//                     HgString::create(scratch, name).append(scratch, 0).chars,
//                     HgString::create(scratch, field->name).append(scratch, 0).chars);
//                 break;
//         }
//     }
//
//     if (!has_tex)
//         hg_warn("In json scene file, entity \"%s\" sprite has no texture\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//     if (!has_pos)
//         hg_warn("In json scene file, entity \"%s\" sprite has no uv_pos\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//     if (!has_size)
//         hg_warn("In json scene file, entity \"%s\" sprite has no uv_size\n",
//             HgString::create(scratch, name).append(scratch, 0).chars);
//
//     return s;
// }
//
// HgBinary hg_create_scene(HgArena& arena, const HgJson& json) {
//     HgSceneJsonParser parser{};
//     return parser.parse(arena, json);
// }

void hg_vulkan_init();

#ifdef HG_VK_DEBUG_MESSENGER
VkDebugUtilsMessengerEXT hg_internal_vk_debug_messenger = nullptr;
#endif

void hg_graphics_init() {
    hg_vulkan_init();

    if (hg_vk_instance == nullptr) {
        hg_vk_instance = hg_vk_create_instance();
        hg_vk_load_instance(hg_vk_instance);
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (hg_internal_vk_debug_messenger == nullptr)
        hg_internal_vk_debug_messenger = hg_vk_create_debug_messenger();
#endif

    if (hg_vk_physical_device == nullptr) {
        hg_vk_physical_device = hg_vk_find_single_queue_physical_device();
        hg_vk_find_queue_family(hg_vk_physical_device, hg_vk_queue_family,
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    if (hg_vk_device == nullptr) {
        hg_vk_device = hg_vk_create_single_queue_device();
        hg_vk_load_device(hg_vk_device);
        vkGetDeviceQueue(hg_vk_device, hg_vk_queue_family, 0, &hg_vk_queue);
    }

    if (hg_vk_vma == nullptr) {
        hg_vk_vma = hg_vk_create_vma_allocator();
    }
}

void hg_vulkan_deinit();

void hg_graphics_deinit() {
    if (hg_vk_vma != nullptr) {
        vmaDestroyAllocator(hg_vk_vma);
        hg_vk_vma = nullptr;
    }

    if (hg_vk_device != nullptr) {
        vkDestroyDevice(hg_vk_device, nullptr);
        hg_vk_device = nullptr;
    }

    if (hg_vk_physical_device != nullptr) {
        hg_vk_physical_device = nullptr;
        hg_vk_queue_family = (u32)-1;
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (hg_internal_vk_debug_messenger != nullptr) {
        vkDestroyDebugUtilsMessengerEXT(hg_vk_instance, hg_internal_vk_debug_messenger, nullptr);
        hg_internal_vk_debug_messenger = nullptr;
    }
#endif

    if (hg_vk_instance != nullptr) {
        vkDestroyInstance(hg_vk_instance, nullptr);
        hg_vk_instance = nullptr;
    }

    hg_vulkan_deinit();
}

const char* hg_vk_result_string(VkResult result) {
    switch (result) {
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

u32 hg_vk_format_to_size(VkFormat format) {
    switch (format) {
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
            hg_warn("Unrecognized Vulkan format value\n");
            return 0;
    }
}

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name

struct HgVulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);

    HG_MAKE_VULKAN_FUNC(vkCreateInstance);
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance);

    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);

    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);

    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateDevice);

    HG_MAKE_VULKAN_FUNC(vkDestroyDevice);
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);
    HG_MAKE_VULKAN_FUNC(vkCreateFence);
    HG_MAKE_VULKAN_FUNC(vkDestroyFence);
    HG_MAKE_VULKAN_FUNC(vkResetFences);
    HG_MAKE_VULKAN_FUNC(vkWaitForFences);

    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkFreeDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);

    HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateImage);
    HG_MAKE_VULKAN_FUNC(vkDestroyImage);
    HG_MAKE_VULKAN_FUNC(vkCreateImageView);
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
    HG_MAKE_VULKAN_FUNC(vkCreateSampler);
    HG_MAKE_VULKAN_FUNC(vkDestroySampler);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);

    HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
    HG_MAKE_VULKAN_FUNC(vkFreeMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
    HG_MAKE_VULKAN_FUNC(vkMapMemory);
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);

    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);

    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);

    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport);
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor);
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline);
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants);
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers);
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdDraw);
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed);
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch);
};

static HgVulkanFuncs hg_internal_vulkan_funcs{};

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    return hg_internal_vulkan_funcs.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyInstance(instance, pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) {
    return hg_internal_vulkan_funcs.vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pCount, VkPhysicalDevice* pDevices) {
    return hg_internal_vulkan_funcs.vkEnumeratePhysicalDevices(instance, pCount, pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* pLayerName, uint32_t* pCount, VkExtensionProperties* pProps) {
    return hg_internal_vulkan_funcs.vkEnumerateDeviceExtensionProperties(device, pLayerName, pCount, pProps);
}

void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, uint32_t* pCount, VkQueueFamilyProperties* pProps) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceQueueFamilyProperties(device, pCount, pProps);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult vkCreateDevice(VkPhysicalDevice device, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    return hg_internal_vulkan_funcs.vkCreateDevice(device, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDevice(device, pAllocator);
}

VkResult vkDeviceWaitIdle(VkDevice device) {
    return hg_internal_vulkan_funcs.vkDeviceWaitIdle(device);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkSurfaceFormatKHR* pFormats) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, pCount, pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkPresentModeKHR* pModes) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, pCount, pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pCaps) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, pCaps);
}

VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    return hg_internal_vulkan_funcs.vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pImages) {
    return hg_internal_vulkan_funcs.vkGetSwapchainImagesKHR(device, swapchain, pCount, pImages);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t* pIndex) {
    return hg_internal_vulkan_funcs.vkAcquireNextImageKHR(device, swapchain, timeout, sem, fence, pIndex);
}

VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) {
    return hg_internal_vulkan_funcs.vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySemaphore(device, sem, pAllocator);
}

VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) {
    return hg_internal_vulkan_funcs.vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(VkDevice device, uint32_t count, const VkFence* pFences) {
    return hg_internal_vulkan_funcs.vkResetFences(device, count, pFences);
}

VkResult vkWaitForFences(VkDevice device, uint32_t count, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) {
    return hg_internal_vulkan_funcs.vkWaitForFences(device, count, pFences, waitAll, timeout);
}

void vkGetDeviceQueue(VkDevice device, uint32_t family, uint32_t index, VkQueue* pQueue) {
    hg_internal_vulkan_funcs.vkGetDeviceQueue(device, family, index, pQueue);
}

VkResult vkQueueWaitIdle(VkQueue queue) {
    return hg_internal_vulkan_funcs.vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmits, VkFence fence) {
    return hg_internal_vulkan_funcs.vkQueueSubmit(queue, count, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pInfo) {
    return hg_internal_vulkan_funcs.vkQueuePresentKHR(queue, pInfo);
}

VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pPool) {
    return hg_internal_vulkan_funcs.vkCreateCommandPool(device, pCreateInfo, pAllocator, pPool);
}

void vkDestroyCommandPool(VkDevice device, VkCommandPool pool, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyCommandPool(device, pool, pAllocator);
}

VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pInfo, VkCommandBuffer* pBufs) {
    return hg_internal_vulkan_funcs.vkAllocateCommandBuffers(device, pInfo, pBufs);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer* pBufs) {
    hg_internal_vulkan_funcs.vkFreeCommandBuffers(device, pool, count, pBufs);
}

VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pPool) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorPool(device, pInfo, pAllocator, pPool);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorPool(device, pool, pAllocator);
}

VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool pool, uint32_t flags) {
    return hg_internal_vulkan_funcs.vkResetDescriptorPool(device, pool, flags);
}

VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pInfo, VkDescriptorSet* pSets) {
    return hg_internal_vulkan_funcs.vkAllocateDescriptorSets(device, pInfo, pSets);
}

VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    return hg_internal_vulkan_funcs.vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pWrites, uint32_t copyCount, const VkCopyDescriptorSet* pCopies) {
    hg_internal_vulkan_funcs.vkUpdateDescriptorSets(device, writeCount, pWrites, copyCount, pCopies);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pLayout) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorSetLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorSetLayout(device, layout, pAllocator);
}

VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pLayout) {
    return hg_internal_vulkan_funcs.vkCreatePipelineLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipelineLayout(device, layout, pAllocator);
}

VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pModule) {
    return hg_internal_vulkan_funcs.vkCreateShaderModule(device, pInfo, pAllocator, pModule);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyShaderModule(device, module, pAllocator);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateGraphicsPipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateComputePipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuf) {
    return hg_internal_vulkan_funcs.vkCreateBuffer(device, pInfo, pAllocator, pBuf);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buf, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyBuffer(device, buf, pAllocator);
}

VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
    return hg_internal_vulkan_funcs.vkCreateImage(device, pInfo, pAllocator, pImage);
}

void vkDestroyImage(VkDevice device, VkImage img, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImage(device, img, pAllocator);
}

VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
    return hg_internal_vulkan_funcs.vkCreateImageView(device, pInfo, pAllocator, pView);
}

void vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImageView(device, view, pAllocator);
}

VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) {
    return hg_internal_vulkan_funcs.vkCreateSampler(device, pInfo, pAllocator, pSampler);
}

void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySampler(device, sampler, pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2*pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) {
    return hg_internal_vulkan_funcs.vkAllocateMemory(device, pInfo, pAllocator, pMemory);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks* pAllocator) {
    hg_internal_vulkan_funcs.vkFreeMemory(device, mem, pAllocator);
}

VkResult vkBindBufferMemory(VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory(device, buf, mem, offset);
}

VkResult vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkBindImageMemory(VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindImageMemory(device, img, mem, offset);
}

VkResult vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
    return hg_internal_vulkan_funcs.vkMapMemory(device, mem, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory mem) {
    hg_internal_vulkan_funcs.vkUnmapMemory(device, mem);
}

VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges) {
    return hg_internal_vulkan_funcs.vkFlushMappedMemoryRanges(device, count, pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges) {
    return hg_internal_vulkan_funcs.vkInvalidateMappedMemoryRanges(device, count, pRanges);
}

VkResult vkBeginCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferBeginInfo* pInfo) {
    return hg_internal_vulkan_funcs.vkBeginCommandBuffer(cmd, pInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer cmd) {
    return hg_internal_vulkan_funcs.vkEndCommandBuffer(cmd);
}

VkResult vkResetCommandBuffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags) {
    return hg_internal_vulkan_funcs.vkResetCommandBuffer(cmd, flags);
}

void vkCmdCopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy* pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBuffer(cmd, src, dst, count, pRegions);
}

void vkCmdCopyImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy* pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions);
}

void vkCmdBlitImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit* pRegions, VkFilter filter) {
    hg_internal_vulkan_funcs.vkCmdBlitImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter);
}

void vkCmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy* pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBufferToImage(cmd, src, dst, dstLayout, count, pRegions);
}

void vkCmdCopyImageToBuffer(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy* pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImageToBuffer(cmd, src, srcLayout, dst, count, pRegions);
}

void vkCmdPipelineBarrier2(VkCommandBuffer cmd, const VkDependencyInfo* pInfo) {
    hg_internal_vulkan_funcs.vkCmdPipelineBarrier2(cmd, pInfo);
}

void vkCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo* pInfo) {
    hg_internal_vulkan_funcs.vkCmdBeginRendering(cmd, pInfo);
}

void vkCmdEndRendering(VkCommandBuffer cmd) {
    hg_internal_vulkan_funcs.vkCmdEndRendering(cmd);
}

void vkCmdSetViewport(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport* pViewports) {
    hg_internal_vulkan_funcs.vkCmdSetViewport(cmd, first, count, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D* pScissors) {
    hg_internal_vulkan_funcs.vkCmdSetScissor(cmd, first, count, pScissors);
}

void vkCmdBindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    hg_internal_vulkan_funcs.vkCmdBindPipeline(cmd, bindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet* pSets, uint32_t dynCount, const uint32_t* pDyn) {
    hg_internal_vulkan_funcs.vkCmdBindDescriptorSets(cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn);
}

void vkCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void* pData) {
    hg_internal_vulkan_funcs.vkCmdPushConstants(cmd, layout, stages, offset, size, pData);
}

void vkCmdBindVertexBuffers(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer* pBufs, const VkDeviceSize* pOffsets) {
    hg_internal_vulkan_funcs.vkCmdBindVertexBuffers(cmd, first, count, pBufs, pOffsets);
}

void vkCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type) {
    hg_internal_vulkan_funcs.vkCmdBindIndexBuffer(cmd, buf, offset, type);
}

void vkCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z) {
    hg_internal_vulkan_funcs.vkCmdDispatch(cmd, x, y, z);
}

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_instance(VkInstance instance) {
    hg_assert(instance != nullptr);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyInstance);
#ifdef HG_VK_DEBUG_MESSENGER
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDebugUtilsMessengerEXT);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyDebugUtilsMessengerEXT);
#endif
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroySurfaceKHR)
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDevice)
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_device(VkDevice device) {
    hg_assert(device != nullptr);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDevice)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDeviceWaitIdle)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetSwapchainImagesKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAcquireNextImageKHR)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkWaitForFences);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceQueue);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueWaitIdle);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueSubmit);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueuePresentKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeCommandBuffers);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUpdateDescriptorSets);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreatePipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateGraphicsPipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateComputePipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipeline);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSampler);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySampler);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkMapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUnmapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFlushMappedMemoryRanges);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkInvalidateMappedMemoryRanges);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBeginCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkEndCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetCommandBuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBlitImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBufferToImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImageToBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPipelineBarrier2);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBeginRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdEndRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetViewport);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetScissor);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindPipeline);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPushConstants);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindVertexBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindIndexBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDraw);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDrawIndexed);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDispatch);
}

#undef HG_LOAD_VULKAN_DEVICE_FUNC

static VkBool32 hg_internal_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Error: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Verbose: %s\n", callback_data->pMessage);
    } else {
        (void)std::fprintf(stderr, "Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT hg_internal_debug_utils_messenger_info{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr, 0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    hg_internal_debug_callback, nullptr,
};

VkInstance hg_vk_create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hurdy Gurdy Application",
    app_info.applicationVersion = 0;
    app_info.pEngineName = "Hurdy Gurdy Engine";
    app_info.engineVersion = 0;
    app_info.apiVersion = VK_API_VERSION_1_3;

#ifdef HG_VK_DEBUG_MESSENGER
    const char* layers[]{
        "VK_LAYER_KHRONOS_validation",
    };
#endif

    const char* exts[]{
#ifdef HG_VK_DEBUG_MESSENGER
        "VK_EXT_debug_utils",
#endif
        "VK_KHR_surface",
#if defined(HG_PLATFORM_LINUX)
        "VK_KHR_xlib_surface",
#elif defined(HG_PLATFORM_WINDOWS)
        "VK_KHR_win32_surface",
#endif
    };

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef HG_VK_DEBUG_MESSENGER
    instance_info.pNext = &hg_internal_debug_utils_messenger_info;
#endif
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
#ifdef HG_VK_DEBUG_MESSENGER
    instance_info.enabledLayerCount = hg_countof(layers);
    instance_info.ppEnabledLayerNames = layers;
#endif
    instance_info.enabledExtensionCount = hg_countof(exts);
    instance_info.ppEnabledExtensionNames = exts;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
    if (instance == nullptr)
        hg_error("Failed to create Vulkan instance: %s\n", hg_vk_result_string(result));

    return instance;
}

VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger() {
    hg_assert(hg_vk_instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        hg_vk_instance, &hg_internal_debug_utils_messenger_info, nullptr, &messenger);
    if (messenger == nullptr)
        hg_error("Failed to create Vulkan debug messenger: %s\n", hg_vk_result_string(result));

    return messenger;
}

bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32& queue_family, VkQueueFlags queue_flags) {
    hg_assert(gpu != nullptr);

    hg_arena_scope(scratch, hg_get_scratch());

    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
    VkQueueFamilyProperties* families = scratch.alloc<VkQueueFamilyProperties>(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families);

    for (u32 i = 0; i < family_count; ++i) {
        if (families[i].queueFlags & queue_flags) {
            queue_family = i;
            return true;
        }
    }
    return false;
}

static const char* const hg_internal_vk_device_extensions[]{
    "VK_KHR_swapchain",
};

VkPhysicalDevice hg_vk_find_single_queue_physical_device() {
    hg_assert(hg_vk_instance != nullptr);

    hg_arena_scope(scratch, hg_get_scratch());

    u32 gpu_count;
    vkEnumeratePhysicalDevices(hg_vk_instance, &gpu_count, nullptr);
    VkPhysicalDevice* gpus = scratch.alloc<VkPhysicalDevice>(gpu_count);
    vkEnumeratePhysicalDevices(hg_vk_instance, &gpu_count, gpus);

    VkExtensionProperties* ext_props = nullptr;
    u32 ext_prop_count = 0;

    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 new_prop_count = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &new_prop_count, nullptr);
        if (new_prop_count > ext_prop_count) {
            ext_props = scratch.realloc(ext_props, ext_prop_count, new_prop_count);
            ext_prop_count = new_prop_count;
        }
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &new_prop_count, ext_props);

        for (usize j = 0; j < hg_countof(hg_internal_vk_device_extensions); j++) {
            for (usize k = 0; k < new_prop_count; k++) {
                if (strcmp(hg_internal_vk_device_extensions[j], ext_props[k].extensionName) == 0)
                    goto next_ext;
            }
            goto next_gpu;
next_ext:
            continue;
        }

        if (!hg_vk_find_queue_family(gpu, family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto next_gpu;

        return gpu;

next_gpu:
        continue;
    }

    hg_warn("Could not find a suitable gpu\n");
    return nullptr;
}

VkDevice hg_vk_create_single_queue_device() {
    hg_assert(hg_vk_physical_device != nullptr);
    hg_assert(hg_vk_queue_family != (u32)-1);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{};
    dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_feature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2_feature{};
    synchronization2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2_feature.pNext = &dynamic_rendering_feature;
    synchronization2_feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = hg_vk_queue_family;
    queue_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &synchronization2_feature;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = hg_countof(hg_internal_vk_device_extensions);
    device_info.ppEnabledExtensionNames = hg_internal_vk_device_extensions;
    device_info.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(hg_vk_physical_device, &device_info, nullptr, &device);

    if (device == nullptr)
        hg_error("Could not create Vulkan device: %s\n", hg_vk_result_string(result));
    return device;
}

VmaAllocator hg_vk_create_vma_allocator() {
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = hg_vk_physical_device;
    allocator_info.device = hg_vk_device;
    allocator_info.instance = hg_vk_instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma = nullptr;
    VkResult result = vmaCreateAllocator(&allocator_info, &vma);

    if (vma == nullptr)
        hg_error("Could note create Vulkan memory allocator: %s\n", hg_vk_result_string(result));
    return vma;
}

VkPipeline hg_vk_create_graphics_pipeline(const HgVkPipelineConfig& config) {
    if (config.color_attachment_format_count > 0)
        hg_assert(config.color_attachment_formats != nullptr);
    hg_assert(config.shader_stages != nullptr);
    hg_assert(config.layout != nullptr);
    if (config.vertex_binding_count > 0)
        hg_assert(config.vertex_bindings != nullptr);

    VkPipelineVertexInputStateCreateInfo vertex_input_state{};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount = (u32)config.vertex_binding_count;
    vertex_input_state.pVertexBindingDescriptions = config.vertex_bindings;
    vertex_input_state.vertexAttributeDescriptionCount = (u32)config.vertex_attribute_count;
    vertex_input_state.pVertexAttributeDescriptions = config.vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = config.topology;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellation_state{};
    tessellation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_state.patchControlPoints = config.tesselation_patch_control_points;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state{};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = config.polygon_mode;
    rasterization_state.cullMode = config.cull_mode;
    rasterization_state.frontFace = config.front_face;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state{};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = config.multisample_count != 0
        ? config.multisample_count
        : VK_SAMPLE_COUNT_1_BIT,
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = nullptr;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthWriteEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthCompareOp = config.enable_color_blend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.stencilTestEnable = config.stencil_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
    // depth_stencil_state.front = {}; : TODO
    // depth_stencil_state.back = {}; : TODO
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = config.enable_color_blend ? VK_TRUE : VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment;
    color_blend_state.blendConstants[0] = {1.0f};
    color_blend_state.blendConstants[1] = {1.0f};
    color_blend_state.blendConstants[2] = {1.0f};
    color_blend_state.blendConstants[3] = {1.0f};

    VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = hg_countof(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = (u32)config.color_attachment_format_count;
    rendering_info.pColorAttachmentFormats = config.color_attachment_formats;
    rendering_info.depthAttachmentFormat = config.depth_attachment_format;
    rendering_info.stencilAttachmentFormat = config.stencil_attachment_format;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = &rendering_info;
    pipeline_info.stageCount = (u32)config.shader_count;
    pipeline_info.pStages = config.shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.pTessellationState = &tessellation_state;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = config.layout;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateGraphicsPipelines(hg_vk_device, nullptr, 1, &pipeline_info, nullptr, &pipeline);
    if (pipeline == nullptr)
        hg_error("Failed to create Vulkan graphics pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

VkPipeline hg_vk_create_compute_pipeline(const HgVkPipelineConfig& config) {
    hg_assert(config.color_attachment_formats == nullptr);
    hg_assert(config.depth_attachment_format == VK_FORMAT_UNDEFINED);
    hg_assert(config.stencil_attachment_format == VK_FORMAT_UNDEFINED);
    hg_assert(config.shader_stages != nullptr);
    hg_assert(config.shader_count == 1);
    hg_assert(config.shader_stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    hg_assert(config.layout != nullptr);
    hg_assert(config.vertex_bindings == nullptr);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = config.shader_stages[0];
    pipeline_info.layout = config.layout;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateComputePipelines(hg_vk_device, nullptr, 1, &pipeline_info, nullptr, &pipeline);
    if (pipeline == nullptr)
        hg_error("Failed to create Vulkan compute pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

static VkFormat hg_internal_vk_find_swapchain_format(VkSurfaceKHR surface) {
    hg_assert(hg_vk_physical_device != nullptr);
    hg_assert(surface != nullptr);

    hg_arena_scope(scratch, hg_get_scratch());

    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(hg_vk_physical_device, surface, &format_count, nullptr);
    VkSurfaceFormatKHR* formats = scratch.alloc<VkSurfaceFormatKHR>(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(hg_vk_physical_device, surface, &format_count, formats);

    for (usize i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hg_error("No supported swapchain formats\n");
}

static VkPresentModeKHR hg_internal_vk_find_swapchain_present_mode(
    VkSurfaceKHR surface,
    VkPresentModeKHR desired_mode
) {
    hg_assert(hg_vk_physical_device != nullptr);
    hg_assert(surface != nullptr);

    if (desired_mode == VK_PRESENT_MODE_FIFO_KHR)
        return desired_mode;

    u32 mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(hg_vk_physical_device, surface, &mode_count, nullptr);
    VkPresentModeKHR* present_modes = (VkPresentModeKHR*)alloca(mode_count * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(hg_vk_physical_device, surface, &mode_count, present_modes);

    for (usize i = 0; i < mode_count; ++i) {
        if (present_modes[i] == desired_mode)
            return desired_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

HgSwapchainData hg_vk_create_swapchain(
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(hg_vk_physical_device != nullptr);
    hg_assert(surface != nullptr);
    hg_assert(image_usage != 0);

    HgSwapchainData swapchain{};

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hg_vk_physical_device, surface, &surface_capabilities);

    if (surface_capabilities.currentExtent.width == 0 ||
        surface_capabilities.currentExtent.height == 0 ||
        surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width ||
        surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height ||
        surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width ||
        surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height
    ) {
        hg_warn("Could not create swapchain of the surface's size\n");
        return swapchain;
    }

    swapchain.width = surface_capabilities.currentExtent.width;
    swapchain.height = surface_capabilities.currentExtent.height;
    swapchain.format = hg_internal_vk_find_swapchain_format(surface);

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_info.imageFormat = swapchain.format;
    swapchain_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = image_usage;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = hg_internal_vk_find_swapchain_present_mode(surface, desired_mode);
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;

    VkResult result = vkCreateSwapchainKHR(hg_vk_device, &swapchain_info, nullptr, &swapchain.handle);
    if (swapchain.handle == nullptr)
        hg_error("Failed to create swapchain: %s\n", hg_vk_result_string(result));

    return swapchain;
}

HgSwapchainCommands HgSwapchainCommands::create(HgArena& arena, VkSwapchainKHR swapchain, VkCommandPool cmd_pool) {
    HgSwapchainCommands sync{};
    sync.recreate(arena, swapchain, cmd_pool);
    return sync;
}

void HgSwapchainCommands::recreate(HgArena& arena, VkSwapchainKHR swapchain_val, VkCommandPool cmd_pool_val) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(cmd_pool_val != nullptr);
    hg_assert(swapchain_val != nullptr);

    cmd_pool = cmd_pool_val;
    swapchain = swapchain_val;

    vkGetSwapchainImagesKHR(hg_vk_device, swapchain, &frame_count, nullptr);

    cmds = arena.alloc<VkCommandBuffer>(frame_count);

    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = nullptr;
    cmd_alloc_info.commandPool = cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = frame_count;

    vkAllocateCommandBuffers(hg_vk_device, &cmd_alloc_info, cmds);

    frame_finished = arena.alloc<VkFence>(frame_count);
    for (usize i = 0; i < frame_count; ++i) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hg_vk_device, &info, nullptr, &frame_finished[i]);
    }

    image_available = arena.alloc<VkSemaphore>(frame_count);
    for (usize i = 0; i < frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hg_vk_device, &info, nullptr, &image_available[i]);
    }

    ready_to_present = arena.alloc<VkSemaphore>(frame_count);
    for (usize i = 0; i < frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hg_vk_device, &info, nullptr, &ready_to_present[i]);
    }
}

void HgSwapchainCommands::destroy() {
    hg_assert(hg_vk_device != nullptr);

    vkFreeCommandBuffers(hg_vk_device, cmd_pool, frame_count, cmds);
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroyFence(hg_vk_device, frame_finished[i], nullptr);
    }
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(hg_vk_device, image_available[i], nullptr);
    }
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(hg_vk_device, ready_to_present[i], nullptr);
    }

    swapchain = nullptr;
    cmd_pool = nullptr;
}

VkCommandBuffer HgSwapchainCommands::acquire_and_record() {
    hg_assert(hg_vk_device != nullptr);
    if (swapchain == nullptr)
        return nullptr;

    current_frame = (current_frame + 1) % frame_count;

    vkWaitForFences(hg_vk_device, 1, &frame_finished[current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(hg_vk_device, 1, &frame_finished[current_frame]);

    VkResult result = vkAcquireNextImageKHR(
        hg_vk_device, swapchain, UINT64_MAX, image_available[current_frame], nullptr, &current_image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return nullptr;

    VkCommandBuffer cmd = cmds[current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

void HgSwapchainCommands::end_and_present(VkQueue queue) {
    hg_assert(queue != nullptr);

    VkCommandBuffer cmd = cmds[current_frame];
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &image_available[current_frame];
    VkPipelineStageFlags stage_flags{VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
    submit.pWaitDstStageMask = &stage_flags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &ready_to_present[current_image];

    vkQueueSubmit(queue, 1, &submit, frame_finished[current_frame]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &ready_to_present[current_image];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;

    vkQueuePresentKHR(queue, &present_info);
}

u32 hg_vk_find_memory_type_index(
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags
) {
    hg_assert(hg_vk_physical_device != nullptr);
    hg_assert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(hg_vk_physical_device, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & undesired_flags) != 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type without undesired flags\n");
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type with desired flags\n");
        return i;
    }
    hg_error("Could not find Vulkan memory type\n");
}

void hg_vk_buffer_staging_write(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkBuffer dst,
    usize offset,
    const void* src,
    usize size
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(hg_vk_vma != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(dst != nullptr);
    hg_assert(src != nullptr);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(hg_vk_vma, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    vmaCopyMemoryToAllocation(hg_vk_vma, src, stage_alloc, offset, size);
    hg_defer(vmaDestroyBuffer(hg_vk_vma, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hg_vk_device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(hg_vk_device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, stage, dst, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(hg_vk_device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(hg_vk_device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(hg_vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void hg_vk_buffer_staging_read(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    void* dst,
    VkBuffer src,
    usize offset,
    usize size
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(hg_vk_vma != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(dst != nullptr);
    hg_assert(src != nullptr);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(hg_vk_vma, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    hg_defer(vmaDestroyBuffer(hg_vk_vma, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hg_vk_device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(hg_vk_device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, src, stage, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(hg_vk_device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(hg_vk_device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(hg_vk_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(hg_vk_vma, stage_alloc, offset, dst, size);
}

void hg_vk_image_staging_write(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingWriteConfig& config
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(hg_vk_vma != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(config.dst_image != nullptr);
    hg_assert(config.src_data != nullptr);
    hg_assert(config.width > 0);
    hg_assert(config.height > 0);
    hg_assert(config.depth > 0);
    hg_assert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hg_vk_format_to_size(config.format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(hg_vk_vma, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    vmaCopyMemoryToAllocation(hg_vk_vma, config.src_data, stage_alloc, 0, size);
    hg_defer(vmaDestroyBuffer(hg_vk_vma, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hg_vk_device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(hg_vk_device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transfer_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transfer_barrier.image = config.dst_image;
    transfer_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyBufferToImage(cmd, stage, config.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (config.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        VkImageMemoryBarrier2 end_barrier{};
        end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        end_barrier.newLayout = config.layout;
        end_barrier.image = config.dst_image;
        end_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
        end_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
        end_barrier.subresourceRange.levelCount = 1;
        end_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
        end_barrier.subresourceRange.layerCount = config.subresource.layerCount;

        VkDependencyInfo end_dep{};
        end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        end_dep.imageMemoryBarrierCount = 1;
        end_dep.pImageMemoryBarriers = &end_barrier;

        vkCmdPipelineBarrier2(cmd, &end_dep);
    }

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = nullptr;
    vkCreateFence(hg_vk_device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(hg_vk_device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(hg_vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void hg_vk_image_staging_read(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingReadConfig& config
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(hg_vk_vma != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(config.src_image != nullptr);
    hg_assert(config.layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(config.dst != nullptr);
    hg_assert(config.width > 0);
    hg_assert(config.height > 0);
    hg_assert(config.depth > 0);
    hg_assert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hg_vk_format_to_size(config.format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(hg_vk_vma, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    hg_defer(vmaDestroyBuffer(hg_vk_vma, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hg_vk_device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(hg_vk_device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transfer_barrier.oldLayout = config.layout;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transfer_barrier.image = config.src_image;
    transfer_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyImageToBuffer(cmd, config.src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage, 1, &region);

    VkImageMemoryBarrier2 end_barrier{};
    end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    end_barrier.newLayout = config.layout;
    end_barrier.image = config.src_image;
    end_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    end_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    end_barrier.subresourceRange.levelCount = 1;
    end_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    end_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo end_dep{};
    end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    end_dep.imageMemoryBarrierCount = 1;
    end_dep.pImageMemoryBarriers = &end_barrier;

    vkCmdPipelineBarrier2(cmd, &end_dep);

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(hg_vk_device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(hg_vk_device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(hg_vk_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(hg_vk_vma, stage_alloc, 0, config.dst, size);
}

void hg_vk_image_generate_mipmaps(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkImage image,
    VkImageAspectFlags aspect_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    u32 width,
    u32 height,
    u32 depth,
    u32 mip_count
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(image != nullptr);
    hg_assert(old_layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(new_layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(width > 0);
    hg_assert(height > 0);
    hg_assert(depth > 0);
    hg_assert(mip_count > 0);
    if (mip_count == 1)
        return;

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hg_vk_device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(hg_vk_device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkOffset3D mip_offset{};
    mip_offset.x = (i32)width;
    mip_offset.y = (i32)height;
    mip_offset.z = (i32)depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = old_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    for (u32 level = 0; level < mip_count - 1; ++level) {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspect_mask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = aspect_mask;
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mip_offset;
        if (mip_offset.x > 1)
            mip_offset.x /= 2;
        if (mip_offset.y > 1)
            mip_offset.y /= 2;
        if (mip_offset.z > 1)
            mip_offset.z /= 2;
        blit.dstSubresource.aspectMask = aspect_mask;
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mip_offset;

        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspect_mask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);
    }

    barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.dstAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = new_layout;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_count;

    vkCmdPipelineBarrier2(cmd, &dep);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit_info, nullptr);
}

struct HgWindowInput {
    u32 width;
    u32 height;
    f64 mouse_pos_x;
    f64 mouse_pos_y;
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    bool was_resized;
    bool was_closed;
    bool keys_down[(u32)HgKey::count];
    bool keys_pressed[(u32)HgKey::count];
    bool keys_released[(u32)HgKey::count];
};

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

struct HgX11Funcs {
    Display* (*XOpenDisplay)(_Xconst char*);
    int (*XCloseDisplay)(Display*);
    Window (*XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int,
        unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    int (*XDestroyWindow)(Display*, Window);
    int (*XStoreName)(Display*, Window, _Xconst char*);
    Atom (*XInternAtom)(Display*, _Xconst char*, Bool);
    Status (*XSetWMProtocols)(Display*, Window, Atom*, int);
    int (*XMapWindow)(Display*, Window);
    Status (*XSendEvent)(Display*, Window, Bool, long, XEvent*);
    int (*XFlush)(Display*);
    int (*XNextEvent)(Display*, XEvent*);
    int (*XPending)(Display*);
    KeySym (*XLookupKeysym)(XKeyEvent*, int);
};

static void* hg_internal_libx11 = nullptr;
static HgX11Funcs hg_internal_x11_funcs{};

Display* XOpenDisplay(_Xconst char* name) {
    return hg_internal_x11_funcs.XOpenDisplay(name);
}

int XCloseDisplay(Display* dpy) {
    return hg_internal_x11_funcs.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display* dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int xclass,
    Visual* visual,
    unsigned long valuemask,
    XSetWindowAttributes* attributes
) {
    return hg_internal_x11_funcs.XCreateWindow(
        dpy, parent, x, y, width, height, border_width,
        depth, xclass, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display* dpy, Window w) {
    return hg_internal_x11_funcs.XDestroyWindow(dpy, w);
}

int XStoreName(Display* dpy, Window w, _Xconst char* name) {
    return hg_internal_x11_funcs.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display* dpy, _Xconst char* name, Bool only_if_exists) {
    return hg_internal_x11_funcs.XInternAtom(dpy, name, only_if_exists);
}

Status XSetWMProtocols(Display* dpy, Window w, Atom* protocols, int count) {
    return hg_internal_x11_funcs.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display* dpy, Window w) {
    return hg_internal_x11_funcs.XMapWindow(dpy, w);
}

Status XSendEvent(Display* dpy, Window w, Bool propagate, long event_mask, XEvent* event) {
    return hg_internal_x11_funcs.XSendEvent(dpy, w, propagate, event_mask, event);
}

int XFlush(Display* dpy) {
    return hg_internal_x11_funcs.XFlush(dpy);
}

int XNextEvent(Display* dpy, XEvent* event) {
    return hg_internal_x11_funcs.XNextEvent(dpy, event);
}

int XPending(Display* dpy) {
    return hg_internal_x11_funcs.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent* key_event, int index) {
    return hg_internal_x11_funcs.XLookupKeysym(key_event, index);
}

#define HG_LOAD_X11_FUNC(name)* (void**)&hg_internal_x11_funcs. name \
    = dlsym(hg_internal_libx11, #name); \
    if (hg_internal_x11_funcs. name == nullptr) { hg_error("Could not load Xlib function: \n" #name); }

Display* hg_internal_x11_display = nullptr;

void hg_platform_init() {
    if (hg_internal_libx11 == nullptr)
        hg_internal_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (hg_internal_libx11 == nullptr)
        hg_error("Could not open Xlib\n");

    HG_LOAD_X11_FUNC(XOpenDisplay);
    HG_LOAD_X11_FUNC(XCloseDisplay);
    HG_LOAD_X11_FUNC(XCreateWindow);
    HG_LOAD_X11_FUNC(XDestroyWindow);
    HG_LOAD_X11_FUNC(XStoreName);
    HG_LOAD_X11_FUNC(XInternAtom);
    HG_LOAD_X11_FUNC(XSetWMProtocols);
    HG_LOAD_X11_FUNC(XMapWindow);
    HG_LOAD_X11_FUNC(XSendEvent);
    HG_LOAD_X11_FUNC(XFlush);
    HG_LOAD_X11_FUNC(XPending);
    HG_LOAD_X11_FUNC(XNextEvent);
    HG_LOAD_X11_FUNC(XLookupKeysym);

    if (hg_internal_x11_display == nullptr)
        hg_internal_x11_display = XOpenDisplay(nullptr);
    if (hg_internal_x11_display == nullptr)
        hg_error("Could not open X display\n");
}

void hg_platform_deinit() {
    if (hg_internal_x11_display != nullptr) {
        XCloseDisplay(hg_internal_x11_display);
        hg_internal_x11_display = nullptr;
    }
    if (hg_internal_libx11 != nullptr) {
        dlclose(hg_internal_libx11);
        hg_internal_libx11 = nullptr;
    }
}

static Window hg_internal_create_x11_window(
    Display* display,
    u32 width,
    u32 height,
    const char* title
) {
    XSetWindowAttributes window_attributes{};
    window_attributes.event_mask
        = KeyPressMask | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | StructureNotifyMask;
    Window window = XCreateWindow(
        display, RootWindow(display, DefaultScreen(display)),
        0, 0,
        width,
        height,
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &window_attributes
    );
    if (window == ~0U)
        hg_error("X11 could not create window\n");

    if (title != nullptr) {
        int name_result = XStoreName(display, window, title);
        if (name_result == 0)
            hg_error("X11 could not set window title\n");
    }

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        hg_error("X11 could not map window\n");

    return window;
}

static Atom hg_internal_set_delete_behavior(
    Display* display,
    Window window
) {
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (delete_atom == None)
        hg_error("X11 could not get WM_DELETE_WINDOW atom\n");

    int set_protocols_result = XSetWMProtocols(
        display,
        window,
        &delete_atom,
        1
    );
    if (set_protocols_result == 0)
        hg_error("X11 could not set WM_DELETE_WINDOW protocol\n");

    return delete_atom;
}

static void hg_internal_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        hg_error("X11 failed to get state atom\n");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        hg_error("X11 failed to get fullscreen atom\n");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = state_atom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = (long)fullscreen_atom;

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event
    );
    if (fullscreen_result == 0)
        hg_error("X11 could not send fullscreen message\n");
}

struct HgWindow::Internals {
    HgWindowInput input;
    Window x11_window;
    Atom delete_atom;
};

HgWindow HgWindow::create(HgArena& arena, const HgWindow::Config& config) {
    u32 width = config.windowed ? config.width
        : (u32)DisplayWidth(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));
    u32 height = config.windowed ? config.height
        : (u32)DisplayHeight(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));

    HgWindow window;
    window.internals = arena.alloc<Internals>(1);
    *window.internals = {};

    window.internals->input.width = width;
    window.internals->input.height = height;

    window.internals->x11_window = hg_internal_create_x11_window(
        hg_internal_x11_display, width, height, config.title);
    window.internals->delete_atom = hg_internal_set_delete_behavior(
        hg_internal_x11_display, window.internals->x11_window);

    if (!config.windowed)
        hg_internal_set_fullscreen(hg_internal_x11_display, window.internals->x11_window);

    int flush_result = XFlush(hg_internal_x11_display);
    if (flush_result == 0)
        hg_error("X11 could not flush window\n");

    return window;
}

void HgWindow::destroy() {
    XDestroyWindow(hg_internal_x11_display, internals->x11_window);
    XFlush(hg_internal_x11_display);
}

void HgWindow::set_icon(u32* icon_data, u32 width, u32 height) {
    hg_error("window set_icon : TODO\n");
    (void)icon_data;
    (void)width;
    (void)height;
}

bool HgWindow::is_fullscreen() {
    hg_error("window is_fullscreen : TODO\n");
}

void HgWindow::set_fullscreen(bool fullscreen) {
    hg_error("window set_fullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::set_cursor(HgWindow::Cursor cursor) {
    hg_error("window set_cursor : TODO\n");
    (void)cursor;
}

void HgWindow::set_cursor_image(u32* data, u32 width, u32 height) {
    hg_error("window set_cursor_image : TODO\n");
    (void)data;
    (void)width;
    (void)height;
}

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window) {
    hg_assert(instance != nullptr);
    hg_assert(window.internals != nullptr);

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == nullptr)
        hg_error("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = hg_internal_x11_display;
    info.window = window.internals->x11_window;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    return surface;
}

void hg_process_window_events(const HgWindow* windows, usize window_count) {
    hg_assert(windows != nullptr);

    if (window_count != 1)
        hg_error("Multiple windows unsupported\n"); // : TODO
    HgWindow window = windows[0];

    std::memset(window.internals->input.keys_pressed, 0, sizeof(window.internals->input.keys_pressed));
    std::memset(window.internals->input.keys_released, 0, sizeof(window.internals->input.keys_released));
    window.internals->input.was_resized = false;

    u32 old_window_width = window.internals->input.width;
    u32 old_window_height = window.internals->input.height;
    f64 old_mouse_pos_x = window.internals->input.mouse_pos_x;
    f64 old_mouse_pos_y = window.internals->input.mouse_pos_y;

    while (XPending(hg_internal_x11_display)) {
        XEvent event;
        int event_result = XNextEvent(hg_internal_x11_display, &event);
        if (event_result != 0)
            hg_error("X11 could not get next event\n");

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window.internals->delete_atom)
                    window.internals->input.was_closed = true;
                break;
            case ConfigureNotify:
                window.internals->input.width = (u32)event.xconfigure.width;
                window.internals->input.height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HgKey::none;
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_0:
                        key = HgKey::k0;
                        break;
                    case XK_1:
                        key = HgKey::k1;
                        break;
                    case XK_2:
                        key = HgKey::k2;
                        break;
                    case XK_3:
                        key = HgKey::k3;
                        break;
                    case XK_4:
                        key = HgKey::k4;
                        break;
                    case XK_5:
                        key = HgKey::k5;
                        break;
                    case XK_6:
                        key = HgKey::k6;
                        break;
                    case XK_7:
                        key = HgKey::k7;
                        break;
                    case XK_8:
                        key = HgKey::k8;
                        break;
                    case XK_9:
                        key = HgKey::k9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HgKey::q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HgKey::w;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HgKey::e;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HgKey::r;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HgKey::t;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HgKey::y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HgKey::u;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HgKey::i;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HgKey::o;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HgKey::p;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HgKey::a;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HgKey::s;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HgKey::d;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HgKey::f;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HgKey::g;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HgKey::h;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HgKey::j;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HgKey::k;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HgKey::l;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HgKey::z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HgKey::x;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HgKey::c;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HgKey::v;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HgKey::b;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HgKey::n;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HgKey::m;
                        break;

                    case XK_semicolon:
                        key = HgKey::semicolon;
                        break;
                    case XK_colon:
                        key = HgKey::colon;
                        break;
                    case XK_apostrophe:
                        key = HgKey::apostrophe;
                        break;
                    case XK_quotedbl:
                        key = HgKey::quotation;
                        break;
                    case XK_comma:
                        key = HgKey::comma;
                        break;
                    case XK_period:
                        key = HgKey::period;
                        break;
                    case XK_question:
                        key = HgKey::question;
                        break;
                    case XK_grave:
                        key = HgKey::grave;
                        break;
                    case XK_asciitilde:
                        key = HgKey::tilde;
                        break;
                    case XK_exclam:
                        key = HgKey::exclamation;
                        break;
                    case XK_at:
                        key = HgKey::at;
                        break;
                    case XK_numbersign:
                        key = HgKey::hash;
                        break;
                    case XK_dollar:
                        key = HgKey::dollar;
                        break;
                    case XK_percent:
                        key = HgKey::percent;
                        break;
                    case XK_asciicircum:
                        key = HgKey::carot;
                        break;
                    case XK_ampersand:
                        key = HgKey::ampersand;
                        break;
                    case XK_asterisk:
                        key = HgKey::asterisk;
                        break;
                    case XK_parenleft:
                        key = HgKey::lparen;
                        break;
                    case XK_parenright:
                        key = HgKey::rparen;
                        break;
                    case XK_bracketleft:
                        key = HgKey::lbracket;
                        break;
                    case XK_bracketright:
                        key = HgKey::rbracket;
                        break;
                    case XK_braceleft:
                        key = HgKey::lbrace;
                        break;
                    case XK_braceright:
                        key = HgKey::rbrace;
                        break;
                    case XK_equal:
                        key = HgKey::equal;
                        break;
                    case XK_less:
                        key = HgKey::less;
                        break;
                    case XK_greater:
                        key = HgKey::greater;
                        break;
                    case XK_plus:
                        key = HgKey::plus;
                        break;
                    case XK_minus:
                        key = HgKey::minus;
                        break;
                    case XK_slash:
                        key = HgKey::slash;
                        break;
                    case XK_backslash:
                        key = HgKey::backslash;
                        break;
                    case XK_underscore:
                        key = HgKey::underscore;
                        break;
                    case XK_bar:
                        key = HgKey::bar;
                        break;

                    case XK_Up:
                        key = HgKey::up;
                        break;
                    case XK_Down:
                        key = HgKey::down;
                        break;
                    case XK_Left:
                        key = HgKey::left;
                        break;
                    case XK_Right:
                        key = HgKey::right;
                        break;
                    case XK_Escape:
                        key = HgKey::escape;
                        break;
                    case XK_space:
                        key = HgKey::space;
                        break;
                    case XK_Return:
                        key = HgKey::enter;
                        break;
                    case XK_BackSpace:
                        key = HgKey::backspace;
                        break;
                    case XK_Delete:
                        key = HgKey::kdelete;
                        break;
                    case XK_Insert:
                        key = HgKey::insert;
                        break;
                    case XK_Tab:
                        key = HgKey::tab;
                        break;
                    case XK_Home:
                        key = HgKey::home;
                        break;
                    case XK_End:
                        key = HgKey::end;
                        break;

                    case XK_F1:
                        key = HgKey::f1;
                        break;
                    case XK_F2:
                        key = HgKey::f2;
                        break;
                    case XK_F3:
                        key = HgKey::f3;
                        break;
                    case XK_F4:
                        key = HgKey::f4;
                        break;
                    case XK_F5:
                        key = HgKey::f5;
                        break;
                    case XK_F6:
                        key = HgKey::f6;
                        break;
                    case XK_F7:
                        key = HgKey::f7;
                        break;
                    case XK_F8:
                        key = HgKey::f8;
                        break;
                    case XK_F9:
                        key = HgKey::f9;
                        break;
                    case XK_F10:
                        key = HgKey::f10;
                        break;
                    case XK_F11:
                        key = HgKey::f11;
                        break;
                    case XK_F12:
                        key = HgKey::f12;
                        break;

                    case XK_Shift_L:
                        key = HgKey::lshift;
                        break;
                    case XK_Shift_R:
                        key = HgKey::rshift;
                        break;
                    case XK_Control_L:
                        key = HgKey::lctrl;
                        break;
                    case XK_Control_R:
                        key = HgKey::rctrl;
                        break;
                    case XK_Meta_L:
                        key = HgKey::lmeta;
                        break;
                    case XK_Meta_R:
                        key = HgKey::rmeta;
                        break;
                    case XK_Alt_L:
                        key = HgKey::lalt;
                        break;
                    case XK_Alt_R:
                        key = HgKey::ralt;
                        break;
                    case XK_Super_L:
                        key = HgKey::lsuper;
                        break;
                    case XK_Super_R:
                        key = HgKey::rsuper;
                        break;
                    case XK_Caps_Lock:
                        key = HgKey::capslock;
                        break;
                }
                if (event.type == KeyPress) {
                    window.internals->input.keys_pressed[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = true;
                } else if (event.type == KeyRelease) {
                    window.internals->input.keys_released[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HgKey::none;
                switch (event.xbutton.button) {
                    case Button1:
                        key = HgKey::mouse1;
                        break;
                    case Button2:
                        key = HgKey::mouse2;
                        break;
                    case Button3:
                        key = HgKey::mouse3;
                        break;
                    case Button4:
                        key = HgKey::mouse4;
                        break;
                    case Button5:
                        key = HgKey::mouse5;
                        break;
                }
                if (event.type == ButtonPress) {
                    window.internals->input.keys_pressed[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = true;
                } else if (event.type == ButtonRelease) {
                    window.internals->input.keys_released[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = false;
                }
            } break;
            case MotionNotify:
                window.internals->input.mouse_pos_x = (f64)event.xmotion.x / (f64)window.internals->input.height;
                window.internals->input.mouse_pos_y = (f64)event.xmotion.y / (f64)window.internals->input.height;
                break;
            default:
                break;
        }
    }

    if (window.internals->input.width != old_window_width || window.internals->input.height != old_window_height) {
        window.internals->input.was_resized = true;
    }

    window.internals->input.mouse_delta_x = window.internals->input.mouse_pos_x - old_mouse_pos_x;
    window.internals->input.mouse_delta_y = window.internals->input.mouse_pos_y - old_mouse_pos_y;
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

HINSTANCE hg_internal_win32_instance = nullptr;

void hg_platform_init() {
    hg_internal_win32_instance = GetModuleHandle(nullptr);
}

void hg_platform_deinit() {
    hg_internal_win32_instance = nullptr;
}

struct HgWindow::Internals {
    HgWindowInput input;
    HWND hwnd;
};

static LRESULT CALLBACK hg_internal_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HgWindow::Internals* window = (HgWindow::Internals*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA*)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->input.was_closed = true;
            break;
        case WM_SIZE:
            window->input.width = LOWORD(lparam);
            window->input.height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            std::memset(window->input.keys_down, 0, sizeof(window->input.keys_down));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HgKey::none;
            HgKey shift_key = HgKey::none;

            switch (wparam) {
                case '0':
                    key = HgKey::k0;
                    shift_key = HgKey::rparen;
                    break;
                case '1':
                    key = HgKey::k1;
                    shift_key = HgKey::exclamation;
                    break;
                case '2':
                    key = HgKey::k2;
                    shift_key = HgKey::at;
                    break;
                case '3':
                    key = HgKey::k3;
                    shift_key = HgKey::hash;
                    break;
                case '4':
                    key = HgKey::k4;
                    shift_key = HgKey::dollar;
                    break;
                case '5':
                    key = HgKey::k5;
                    shift_key = HgKey::percent;
                    break;
                case '6':
                    key = HgKey::k6;
                    shift_key = HgKey::carot;
                    break;
                case '7':
                    key = HgKey::k7;
                    shift_key = HgKey::ampersand;
                    break;
                case '8':
                    key = HgKey::k8;
                    shift_key = HgKey::asterisk;
                    break;
                case '9':
                    key = HgKey::k9;
                    shift_key = HgKey::lparen;
                    break;

                case 'A':
                    key = HgKey::a;
                    break;
                case 'B':
                    key = HgKey::b;
                    break;
                case 'C':
                    key = HgKey::c;
                    break;
                case 'D':
                    key = HgKey::d;
                    break;
                case 'E':
                    key = HgKey::e;
                    break;
                case 'F':
                    key = HgKey::f;
                    break;
                case 'G':
                    key = HgKey::g;
                    break;
                case 'H':
                    key = HgKey::h;
                    break;
                case 'I':
                    key = HgKey::i;
                    break;
                case 'J':
                    key = HgKey::j;
                    break;
                case 'K':
                    key = HgKey::k;
                    break;
                case 'L':
                    key = HgKey::l;
                    break;
                case 'M':
                    key = HgKey::m;
                    break;
                case 'N':
                    key = HgKey::n;
                    break;
                case 'O':
                    key = HgKey::o;
                    break;
                case 'P':
                    key = HgKey::p;
                    break;
                case 'Q':
                    key = HgKey::q;
                    break;
                case 'R':
                    key = HgKey::r;
                    break;
                case 'S':
                    key = HgKey::s;
                    break;
                case 'T':
                    key = HgKey::t;
                    break;
                case 'U':
                    key = HgKey::u;
                    break;
                case 'V':
                    key = HgKey::v;
                    break;
                case 'W':
                    key = HgKey::w;
                    break;
                case 'X':
                    key = HgKey::x;
                    break;
                case 'Y':
                    key = HgKey::y;
                    break;
                case 'Z':
                    key = HgKey::z;
                    break;

                case VK_OEM_1:
                    key = HgKey::semicolon;
                    shift_key = HgKey::colon;
                    break;
                case VK_OEM_7:
                    key = HgKey::apostrophe;
                    shift_key = HgKey::quotation;
                    break;
                case VK_OEM_COMMA:
                    key = HgKey::comma;
                    shift_key = HgKey::less;
                    break;
                case VK_OEM_PERIOD:
                    key = HgKey::period;
                    shift_key = HgKey::greater;
                    break;
                case VK_OEM_2:
                    key = HgKey::slash;
                    shift_key = HgKey::question;
                    break;
                case VK_OEM_3:
                    key = HgKey::grave;
                    shift_key = HgKey::tilde;
                    break;
                case VK_OEM_4:
                    key = HgKey::lbracket;
                    shift_key = HgKey::lbrace;
                    break;
                case VK_OEM_6:
                    key = HgKey::rbracket;
                    shift_key = HgKey::rbrace;
                    break;
                case VK_OEM_5:
                    key = HgKey::backslash;
                    shift_key = HgKey::bar;
                    break;
                case VK_OEM_PLUS:
                    key = HgKey::equal;
                    shift_key = HgKey::plus;
                    break;
                case VK_OEM_MINUS:
                    key = HgKey::minus;
                    shift_key = HgKey::underscore;
                    break;

                case VK_UP:
                    key = HgKey::up;
                    break;
                case VK_DOWN:
                    key = HgKey::down;
                    break;
                case VK_LEFT:
                    key = HgKey::left;
                    break;
                case VK_RIGHT:
                    key = HgKey::right;
                    break;
                case VK_ESCAPE:
                    key = HgKey::escape;
                    break;
                case VK_SPACE:
                    key = HgKey::space;
                    break;
                case VK_RETURN:
                    key = HgKey::enter;
                    break;
                case VK_BACK:
                    key = HgKey::backspace;
                    break;
                case VK_DELETE:
                    key = HgKey::kdelete;
                    break;
                case VK_INSERT:
                    key = HgKey::insert;
                    break;
                case VK_TAB:
                    key = HgKey::tab;
                    break;
                case VK_HOME:
                    key = HgKey::home;
                    break;
                case VK_END:
                    key = HgKey::end;
                    break;

                case VK_F1:
                    key = HgKey::f1;
                    break;
                case VK_F2:
                    key = HgKey::f2;
                    break;
                case VK_F3:
                    key = HgKey::f3;
                    break;
                case VK_F4:
                    key = HgKey::f4;
                    break;
                case VK_F5:
                    key = HgKey::f5;
                    break;
                case VK_F6:
                    key = HgKey::f6;
                    break;
                case VK_F7:
                    key = HgKey::f7;
                    break;
                case VK_F8:
                    key = HgKey::f8;
                    break;
                case VK_F9:
                    key = HgKey::f9;
                    break;
                case VK_F10:
                    key = HgKey::f10;
                    break;
                case VK_F11:
                    key = HgKey::f11;
                    break;
                case VK_F12:
                    key = HgKey::f12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HgKey::rshift;
                    else if (scancode == 0x2A)
                        key = HgKey::lshift;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HgKey::ralt;
                    else
                        key = HgKey::lalt;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HgKey::rctrl;
                    else
                        key = HgKey::lctrl;
                    break;
                case VK_LWIN:
                    key = HgKey::lsuper;
                    break;
                case VK_RWIN:
                    key = HgKey::rsuper;
                    break;
                case VK_CAPITAL:
                    key = HgKey::capslock;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
                if (shift_key != HgKey::none &&
                   (window->input.keys_down[(u32)HgKey::lshift] ||
                    window->input.keys_down[(u32)HgKey::rshift])
                ) {
                    window->input.keys_pressed[(u32)shift_key] = true;
                    window->input.keys_down[(u32)shift_key] = true;
                } else {
                    window->input.keys_pressed[(u32)key] = true;
                    window->input.keys_down[(u32)key] = true;
                }
            } else {
                window->input.keys_released[(u32)shift_key] = window->input.keys_down[(u32)shift_key];
                window->input.keys_down[(u32)shift_key] = false;
                window->input.keys_released[(u32)key] = window->input.keys_down[(u32)key];
                window->input.keys_down[(u32)key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::lmouse] = true;
            window->input.keys_down[(u32)HgKey::lmouse] = true;
            break;
        case WM_RBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::rmouse] = true;
            window->input.keys_down[(u32)HgKey::rmouse] = true;
            break;
        case WM_MBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::mmouse] = true;
            window->input.keys_down[(u32)HgKey::mmouse] = true;
            break;
        case WM_LBUTTONUP:
            window->input.keys_released[(u32)HgKey::lmouse] = true;
            window->input.keys_down[(u32)HgKey::lmouse] = false;
            break;
        case WM_RBUTTONUP:
            window->input.keys_released[(u32)HgKey::rmouse] = true;
            window->input.keys_down[(u32)HgKey::rmouse] = false;
            break;
        case WM_MBUTTONUP:
            window->input.keys_released[(u32)HgKey::mmouse] = true;
            window->input.keys_down[(u32)HgKey::mmouse] = false;
            break;
        case WM_MOUSEMOVE:
            window->input.mouse_pos_x = (f64)LOWORD(lparam) / (f64)window->input.height;
            window->input.mouse_pos_y = (f64)HIWORD(lparam) / (f64)window->input.height;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow HgWindow::create(HgArena& arena, const HgWindow::Config& config) {
    const char* title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    HgWindow window;
    window.internals = arena.alloc<Internals>(1);
    *window.internals = {};

    WNDCLASSA window_class{};
    window_class.hInstance = hg_internal_win32_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = title;
    window_class.lpfnWndProc = hg_internal_window_callback;
    if (!RegisterClassA(&window_class))
        hg_error("Win32 failed to register window class for window: %s\n", config.title);

    if (config.windowed) {
        window.internals->input.width = config.width;
        window.internals->input.height = config.height;
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    } else {
        window.internals->input.width = GetSystemMetrics(SM_CXSCREEN);
        window.internals->input.height = GetSystemMetrics(SM_CYSCREEN);
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    }
    if (window.internals->hwnd == nullptr)
        hg_error("Win32 window creation failed\n");

    ShowWindow(window.internals->hwnd, SW_SHOW);
    return window;
}

void HgWindow::destroy() {
    DestroyWindow(internals->hwnd);
}

void HgWindow::set_icon(u32* icon_data, u32 width, u32 height) {
    hg_error("window set_icon : TODO\n");
    (void)icon_data;
    (void)width;
    (void)height;
}

bool HgWindow::is_fullscreen() {
    hg_error("window is_fullscreen : TODO\n");
}

void HgWindow::set_fullscreen(bool fullscreen) {
    hg_error("window set_fullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::set_cursor(HgWindow::Cursor cursor) {
    hg_error("window set_cursor : TODO\n");
    (void)cursor;
}

void HgWindow::set_cursor_image(u32* data, u32 width, u32 height) {
    hg_error("window set_cursor_image : TODO\n");
    (void)data;
    (void)width;
    (void)height;
}

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window) {
    hg_assert(instance != nullptr);
    hg_assert(window.internals != nullptr);

    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (pfn_vkCreateWin32SurfaceKHR == nullptr)
        hg_error("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = hg_internal_win32_instance;
    info.hwnd = window.internals->hwnd;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    hg_assert(surface != nullptr);
    return surface;
}

void hg_process_window_events(const HgWindow* windows, usize window_count) {
    hg_assert(windows != nullptr);

    for (usize i = 0; i < window_count; ++i) {
        HgWindow::Internals* window = windows[i].internals;

        std::memset(window->input.keys_pressed, 0, sizeof(window->input.keys_pressed));
        std::memset(window->input.keys_released, 0, sizeof(window->input.keys_released));
        window->input.was_resized = false;

        u32 old_window_width = window->input.width;
        u32 old_window_height = window->input.height;
        f64 old_mouse_pos_x = window->input.mouse_pos_x;
        f64 old_mouse_pos_y = window->input.mouse_pos_y;

        MSG msg;
        while (PeekMessageA(&msg, window->hwnd, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (window->input.width != old_window_width || window->input.height != old_window_height) {
            window->input.was_resized = true;
        }

        window->input.mouse_delta_x = window->input.mouse_pos_x - old_mouse_pos_x;
        window->input.mouse_delta_y = window->input.mouse_pos_y - old_mouse_pos_y;

        if (window->input.keys_down[(u32)HgKey::lshift] && window->input.keys_down[(u32)HgKey::rshift]) {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift) {
                window->input.keys_released[(u32)HgKey::lshift] = true;
                window->input.keys_down[(u32)HgKey::lshift] = false;
            }
            if (!rshift) {
                window->input.keys_released[(u32)HgKey::rshift] = true;
                window->input.keys_down[(u32)HgKey::rshift] = false;
            }
        }
    }
}

#else

#error "unsupported platform"

#endif

bool HgWindow::was_closed() {
    return internals->input.was_closed;
}

bool HgWindow::was_resized() {
    return internals->input.was_resized;
}

void HgWindow::get_size(u32* width, u32* height) {
    *width = internals->input.width;
    *height = internals->input.height;
}

void HgWindow::get_mouse_pos(f64& x, f64& y) {
    x = internals->input.mouse_pos_x;
    y = internals->input.mouse_pos_y;
}

void HgWindow::get_mouse_delta(f64& x, f64& y) {
    x = internals->input.mouse_delta_x;
    y = internals->input.mouse_delta_y;
}

bool HgWindow::is_key_down(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_down[(u32)key];
}

bool HgWindow::was_key_pressed(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_pressed[(u32)key];
}

bool HgWindow::was_key_released(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_released[(u32)key];
}

#undef HG_MAKE_VULKAN_FUNC

static void* hg_internal_libvulkan = nullptr;

#define HG_LOAD_VULKAN_FUNC(name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(nullptr, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { \
        hg_error("Could not load " #name "\n"); \
    }

void hg_vulkan_init() {

#if defined(HG_PLATFORM_LINUX)

    if (hg_internal_libvulkan == nullptr)
        hg_internal_libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (hg_internal_libvulkan == nullptr)
        hg_error("Could not load vulkan dynamic lib: %s\n", dlerror());

    *(void**)&hg_internal_vulkan_funcs.vkGetInstanceProcAddr = dlsym(hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == nullptr)
        hg_error("Could not load vkGetInstanceProcAddr: %s\n", dlerror());

#elif defined(HG_PLATFORM_WINDOWS)

    if (hg_internal_libvulkan == nullptr)
        hg_internal_libvulkan = (void*)LoadLibraryA("vulkan-1.dll");
    if (hg_internal_libvulkan == nullptr)
        hg_error("Could not load vulkan dynamic lib\n");

    hg_internal_vulkan_funcs.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress((HMODULE)hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == nullptr)
        hg_error("Could not load vkGetInstanceProcAddr\n");

#endif

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

void hg_vulkan_deinit() {
    if (hg_internal_libvulkan != nullptr) {

#if defined(HG_PLATFORM_LINUX)
        dlclose(hg_internal_libvulkan);
#elif defined(HG_PLATFORM_WINDOWS)
        FreeLibrary((HMODULE)hg_internal_libvulkan);
#endif

        hg_internal_libvulkan = nullptr;
    }
}

#undef HG_LOAD_VULKAN_FUNC

