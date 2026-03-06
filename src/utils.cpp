#include "hurdygurdy.hpp"

void hg_init(void) {
    hg_scratch_memory_init();
    HgArena& arena = hg_get_scratch();

    hg_thread_pool_init(arena, 4096, hg_hardware_concurrency()
        - 2); // main thread, io thread
    hg_io_thread_init(arena, 4096);
    hg_resources_init(arena, 4096);
    hg_gpu_resources_init(arena, 4096);

    hg_platform_init();
    hg_graphics_init();
}

void hg_exit(void) {
    hg_graphics_deinit();
    hg_platform_deinit();

    hg_gpu_resources_reset();
    hg_resources_reset();
    hg_io_thread_deinit();
    hg_thread_pool_deinit();

    hg_scratch_memory_deinit();
}

namespace {
    struct Test {
        const char* name;
        bool (*function)();
    };

    struct TestArr {
        Test* items;
        usize capacity;
        usize count;

        static TestArr create(usize init_count) {
            TestArr arr;
            arr.items = (Test*)std::malloc(init_count * sizeof(Test));
            arr.capacity = init_count;
            arr.count = 0;
            return arr;
        }

        void destroy() const {
            std::free(items);
        }

        Test& push() {
            if (capacity == count) {
                usize new_capacity = capacity == 0 ? 1 : capacity * 2;
                items = (Test*)std::realloc(items, new_capacity);
                capacity = new_capacity;
            }
            return items[count++];
        }
    };
}

static TestArr& hg_internal_get_tests() {
    static TestArr tests = tests.create(1024);
    return tests;
}

void hg_tests_register(const char* name, bool (*function)()) {
    hg_internal_get_tests().push() = {name, function};
}

bool hg_tests_run() {
    std::printf("HurdyGurdy: Tests Begun\n");

    TestArr& tests = hg_internal_get_tests();
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

static constexpr usize arena_count = 2;
static thread_local HgArena arenas[arena_count]{};

void hg_scratch_memory_init() {
    for (usize i = 0; i < arena_count; ++i) {
        if (arenas[i].memory == nullptr) {
            usize arena_size = (u32)-1;
            arenas[i] = {malloc(arena_size), arena_size};
        }
    }
}

void hg_scratch_memory_deinit() {
    for (usize i = 0; i < arena_count; ++i) {
        if (arenas[i].memory != nullptr) {
            std::free(arenas[i].memory);
            arenas[i] = {};
        }
    }
}

HgArena& hg_get_scratch() {
    return arenas[0];
}

HgArena& hg_get_scratch(const HgArena& conflict) {
    for (HgArena& arena : arenas) {
        if (&arena != &conflict)
            return arena;
    }
    hg_error("No scratch arena available\n");
}

HgArena& hg_get_scratch(const HgArena** conflicts, usize count) {
    for (HgArena& arena : arenas) {
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

HgDynamicArray HgDynamicArray::create(
    HgArena& arena,
    u32 width,
    u32 alignment,
    usize count,
    usize capacity
) {
    hg_assert(count <= capacity);

    HgDynamicArray arr;
    arr.items = arena.alloc(capacity * width, alignment);
    arr.width = width;
    arr.alignment = alignment;
    arr.capacity = capacity;
    arr.count = count;
    return arr;
}

void HgDynamicArray::reserve(HgArena& arena, usize new_capacity) {
    items = arena.realloc(items, capacity * width, new_capacity * width, alignment);
    capacity = new_capacity;
}

void HgDynamicArray::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

void* HgDynamicArray::insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);

    std::memmove(get(index + 1), get(index), (count++ - index) * width);
    return get(index);
}

void HgDynamicArray::remove(usize index) {
    hg_assert(index < count);

    std::memmove(get(index), get(index + 1), (count - index - 1) * width);
    --count;
}

void* HgDynamicArray::swap_insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);
    if (index == count)
        return push();

    std::memcpy(get(count++), get(index), width);
    return get(index);
}

void HgDynamicArray::swap_remove(usize index) {
    hg_assert(index < count);
    if (index == count - 1) {
        pop();
        return;
    }

    std::memcpy(get(index), get(count - 1), width);
    --count;
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

namespace {
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
}

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

f64 HgClock::tick() {
    auto prev = time;
    time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<f64>{time - prev}.count();
}

namespace {
    struct ComponentArr {
        u32* widths;
        u32 capacity;
        u32 count;

        static ComponentArr create(u32 init_count) {
            ComponentArr arr;
            arr.widths = (u32*)std::malloc(init_count * sizeof(u32));
            arr.capacity = init_count;
            arr.count = 0;
            return arr;
        }

        void destroy() const {
            std::free(widths);
        }

        void push(u32 width) {
            if (capacity == count) {
                u32 new_capacity = capacity == 0 ? 1 : capacity * 2;
                widths = (u32*)std::realloc(widths, new_capacity);
                capacity = new_capacity;
            }
            widths[count++] = width;
        }
    };
}

ComponentArr& component_arr() {
    static ComponentArr components = components.create(1024);
    return components;
}

static u32 component_width(u32 id) {
    return component_arr().widths[id];
}

u32 hg_create_component_id(u32 width) {
    u32 id = component_arr().count;
    component_arr().push(width);
    return id;
}

HgECS HgECS::create(u32 max_entities) {
    HgECS ecs{};

    ecs.pool_size = max_entities;
    ecs.pool = (HgEntity*)std::malloc(sizeof(HgEntity) * ecs.pool_size);

    ecs.system_count = component_arr().count;
    ecs.systems = (System*)std::malloc(sizeof(System) * ecs.system_count);

    for (u32 i = 0; i < ecs.system_count; ++i) {
        ecs.systems[i] = {};
        ecs.systems[i].indices = (u32*)std::malloc(sizeof(u32) * ecs.pool_size);
    }

    ecs.reset();

    return ecs;
}

void HgECS::destroy() {
    for (u32 i = 0; i < system_count; ++i) {
        std::free(systems[i].indices);
        std::free(systems[i].entities);
        std::free(systems[i].components);
    }
    std::free(systems);
    std::free(pool);
}

void HgECS::reset() {
    for (u32 i = 0; i < pool_size; ++i) {
        pool[i] = {i + 1};
    }
    next = {0};

    for (u32 i = 0; i < system_count; ++i) {
        std::memset(systems[i].indices, -1, pool_size * sizeof(*systems[i].indices));
        systems[i].count = 0;
    }
}

HgEntity HgECS::spawn() {
    hg_assert(next.idx() < pool_size);

    HgEntity entity = next;
    next= pool[entity.idx()];
    pool[entity.idx()] = entity;
    return entity;
}

void HgECS::despawn(HgEntity e) {
    hg_assert(alive(e));

    for (u32 i = 0; i < system_count; ++i) {
        if (has(e, i))
            remove(e, i);
    }
    pool[e.idx()] = next;
    next = e;
    next.increment_generation();
}

bool HgECS::alive(HgEntity e) {
    return e.idx() < pool_size && pool[e.idx()] == e;
}

void* HgECS::add(HgEntity e, u32 component_id) {
    hg_assert(alive(e));
    hg_assert(!has(e, component_id));

    if (systems[component_id].count == systems[component_id].capacity) {
        u32 new_capacity = systems[component_id].capacity == 0 ? 1 : systems[component_id].capacity * 2;
        systems[component_id].entities = (HgEntity*)std::realloc(
            systems[component_id].entities, sizeof(HgEntity) * new_capacity);
        systems[component_id].components = (HgEntity*)std::realloc(
            systems[component_id].components, component_width(component_id) * new_capacity);
        systems[component_id].capacity = new_capacity;
    }

    systems[component_id].indices[e.idx()] = systems[component_id].count;
    systems[component_id].entities[systems[component_id].count] = e;
    void* c = (u8*)systems[component_id].components + component_width(component_id) * systems[component_id].count;
    ++systems[component_id].count;
    return c;
}

void HgECS::remove(HgEntity e, u32 component_id) {
    hg_assert(alive(e));
    hg_assert(has(e, component_id));

    HgEntity last = systems[component_id].entities[systems[component_id].count - 1];
    systems[component_id].entities[systems[component_id].count - 1] = HgEntity{};
    if (e != last) {
        u32 idx = systems[component_id].indices[e.idx()];
        systems[component_id].entities[idx] = last;
        systems[component_id].indices[last.idx()] = idx;
        std::memcpy(
            (u8*)systems[component_id].components + component_width(component_id) * idx,
            (u8*)systems[component_id].components + component_width(component_id) * (systems[component_id].count - 1),
            component_width(component_id));
    }
    systems[component_id].indices[e.idx()] = (u32)-1;
    --systems[component_id].count;
}

bool HgECS::has(HgEntity e, u32 component_id) {
    hg_assert(alive(e));
    return systems[component_id].indices[e.idx()] < systems[component_id].count;
}

void* HgECS::get(HgEntity e, u32 component_id) {
    hg_assert(alive(e));
    hg_assert(has(e, component_id));
    return (u8*)systems[component_id].components + component_width(component_id) * systems[component_id].indices[e.idx()];
}

HgEntity HgECS::get(const void* component, u32 component_id) {
    hg_assert(component != nullptr);

    usize idx = ((uptr)component - (uptr)systems[component_id].components) / component_width(component_id);
    return systems[component_id].entities[idx];
}

u32 HgECS::find_smallest(u32* ids, usize id_count) {
    u32 smallest = ids[0];
    for (usize i = 1; i < id_count; ++i) {
        if (systems[ids[i]].count < systems[smallest].count)
            smallest = ids[i];
    }
    return smallest;
}

static void swap_idx_location(HgECS& ecs, u32 lhs, u32 rhs, u32 component_id) {
    HgECS::System& system = ecs.systems[component_id];
    hg_assert(lhs < system.count);
    hg_assert(rhs < system.count);

    HgEntity lhs_entity = system.entities[lhs];
    HgEntity rhs_entity = system.entities[rhs];

    hg_assert(ecs.alive(lhs_entity));
    hg_assert(ecs.alive(rhs_entity));
    hg_assert(ecs.has(lhs_entity, component_id));
    hg_assert(ecs.has(rhs_entity, component_id));

    system.entities[lhs] = rhs_entity;
    system.entities[rhs] = lhs_entity;
    system.indices[lhs_entity.id] = rhs;
    system.indices[rhs_entity.id] = lhs;

    u32 width = component_width(component_id);
    void* temp = alloca(component_width(component_id));
    std::memcpy(temp, (u8*)system.components + width * lhs, width);
    std::memcpy((u8*)system.components + width * lhs, (u8*)system.components + width * rhs, width);
    std::memcpy((u8*)system.components + width * rhs, temp, width);
}

namespace {
    struct QuicksortData {
        HgECS* ecs;
        u32 comp;
        void* data;
        bool (*compare)(void*, HgECS& ecs, HgEntity lhs, HgEntity rhs);

        u32 quicksort_inter(u32 pivot, u32 inc, u32 dec) {
            while (inc != dec) {
                while (!compare(data, *ecs, ecs->systems[comp].entities[dec], ecs->systems[comp].entities[pivot])) {
                    --dec;
                    if (dec == inc)
                        goto finish;
                }
                while (!compare(data, *ecs, ecs->systems[comp].entities[pivot], ecs->systems[comp].entities[inc])) {
                    ++inc;
                    if (inc == dec)
                        goto finish;
                }
                swap_idx_location(*ecs, inc, dec, comp);
            }

        finish:
            if (compare(data, *ecs, ecs->systems[comp].entities[inc], ecs->systems[comp].entities[pivot]))
                swap_idx_location(*ecs, pivot, inc, comp);

            return inc;
        }

        void quicksort(u32 begin, u32 end) {
            hg_assert(begin <= end && end <= ecs->systems[comp].count);

            if (begin + 1 >= end)
                return;

            u32 middle = quicksort_inter(begin, begin + 1, end - 1);
            quicksort(begin, middle);
            quicksort(middle, end);
        }
    };
}

void HgECS::sort_untyped(
    u32 begin,
    u32 end,
    u32 component_id,
    void* data,
    bool (*compare)(void*, HgECS& ecs, HgEntity lhs, HgEntity rhs)
) {
    QuicksortData q{this, component_id, data, compare};
    q.quicksort(begin, end);
}

void hg_add_child_entity(HgECS& ecs, HgEntity parent, HgEntity child) {
    HgHierarchy& node = ecs.get<HgHierarchy>(parent);
    HgHierarchy& old_first = ecs.get<HgHierarchy>(node.first_child);
    HgHierarchy& new_first = ecs.get<HgHierarchy>(child);

    hg_assert(new_first.parent == HgEntity{});
    hg_assert(new_first.prev_sibling == HgEntity{});
    hg_assert(new_first.next_sibling == HgEntity{});

    new_first.parent = parent;
    new_first.next_sibling = node.first_child;

    old_first.prev_sibling = child;
    node.first_child = child;
}

void hg_detach_entity(HgECS& ecs, HgEntity e) {
    HgHierarchy& node = ecs.get<HgHierarchy>(e);
    if (node.parent != HgEntity{}) {
        if (node.prev_sibling == HgEntity{})
            ecs.get<HgHierarchy>(node.parent).first_child = node.next_sibling;
        else
            ecs.get<HgHierarchy>(node.prev_sibling).next_sibling = node.next_sibling;
        ecs.get<HgHierarchy>(node.next_sibling).prev_sibling = node.prev_sibling;

        HgEntity child = node.first_child;
        while (child != HgEntity{}) {
            HgHierarchy& tf = ecs.get<HgHierarchy>(child);
            HgEntity next = tf.next_sibling;
            tf.parent = HgEntity{};
            tf.next_sibling = HgEntity{};
            tf.prev_sibling = HgEntity{};
            hg_add_child_entity(ecs, node.parent, child);
            child = next;
        }
    } else {
        hg_assert(node.prev_sibling == HgEntity{});
        hg_assert(node.next_sibling == HgEntity{});
        HgEntity child = node.first_child;
        while (child != HgEntity{}) {
            HgHierarchy& tf = ecs.get<HgHierarchy>(child);
            child = tf.next_sibling;
            tf.parent = HgEntity{};
            tf.next_sibling = HgEntity{};
            tf.prev_sibling = HgEntity{};
        }
    }
    node = {};
}

void hg_destroy_entity(HgECS& ecs, HgEntity e) {
    HgHierarchy& node = ecs.get<HgHierarchy>(e);
    HgEntity child = node.first_child;
    while (child != HgEntity{}) {
        HgHierarchy& tf = ecs.get<HgHierarchy>(child);
        HgEntity next = tf.next_sibling;
        tf.parent = HgEntity{};
        tf.prev_sibling = HgEntity{};
        tf.next_sibling = HgEntity{};
        hg_destroy_entity(ecs, child);
        child = next;
    }
    if (node.parent != HgEntity{}) {
        if (node.prev_sibling != HgEntity{})
            ecs.get<HgHierarchy>(node.prev_sibling).next_sibling = node.next_sibling;
        else
            ecs.get<HgHierarchy>(node.parent).first_child = node.next_sibling;
        if (node.next_sibling != HgEntity{})
            ecs.get<HgHierarchy>(node.next_sibling).prev_sibling = HgEntity{};
    }
    ecs.despawn(e);
}

void hg_set_entity(HgECS& ecs, HgEntity e, const HgVec3& p, const HgVec3& s, const HgQuat& r) {
    HgTransform& tf = ecs.get<HgTransform>(e);
    if (ecs.has<HgHierarchy>(e)) {
        HgHierarchy& node = ecs.get<HgHierarchy>(e);
        HgEntity child = node.first_child;
        while (child != HgEntity{}) {
            HgHierarchy& c_node = ecs.get<HgHierarchy>(child);
            HgTransform& c_tf = ecs.get<HgTransform>(child);
            HgTransform rel;
            rel.position = c_tf.position - tf.position;
            rel.scale = c_tf.scale / tf.scale;
            rel.rotation = hg_conj(tf.rotation) * c_tf.rotation;
            // hg_set_entity(ecs, child,
            //     hg_rotate(r, (c_tf.position - tf.position) * s / c_tf.scale + p),
            //     s * c_tf.scale / tf.scale,
            //     r);
            child = c_node.next_sibling;
        }
    }
    tf.position = p;
    tf.scale = s;
    tf.rotation = r;
}

void hg_move_entity(HgECS& ecs, HgEntity e, const HgVec3& dp, const HgVec3& ds, const HgQuat& dr) {
    HgTransform& tf = ecs.get<HgTransform>(e);
    hg_set_entity(ecs, e, tf.position + dp, tf.scale * ds, dr * tf.rotation);
}

