#include "hurdygurdy.h"

#ifdef _WIN32
#include <malloc.h>
#endif
#ifdef __unix__
#include <alloca.h>
#endif

usize hg_align(usize value, usize alignment) {
    hg_assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (value + alignment - 1) & ~(alignment - 1);
}

HgVec2 hg_svec2(f32 scalar) {
    return (HgVec2){scalar, scalar};
}

HgVec3 hg_svec3(f32 scalar) {
    return (HgVec3){scalar, scalar, scalar};
}

HgVec4 hg_svec4(f32 scalar) {
    return (HgVec4){scalar, scalar, scalar, scalar};
}

HgMat2 hg_smat2(f32 scalar) {
    return (HgMat2){
        {scalar, 0.0f},
        {0.0f, scalar},
    };
}

HgMat3 hg_smat3(f32 scalar) {
    return (HgMat3){
        {scalar, 0.0f, 0.0f},
        {0.0f, scalar, 0.0f},
        {0.0f, 0.0f, scalar},
    };
}

HgMat4 hg_smat4(f32 scalar) {
    return (HgMat4){
        {scalar, 0.0f, 0.0f, 0.0f},
        {0.0f, scalar, 0.0f, 0.0f},
        {0.0f, 0.0f, scalar, 0.0f},
        {0.0f, 0.0f, 0.0f, scalar},
    };
}

HgVec3 hg_vec2to3(HgVec2 lhs) {
    return (HgVec3){lhs.x, lhs.y, 0.0f};
}

HgVec4 hg_vec2to4(HgVec2 lhs) {
    return (HgVec4){lhs.x, lhs.y, 0.0f, 0.0f};
}

HgVec4 hg_vec3to4(HgVec3 lhs) {
    return (HgVec4){lhs.x, lhs.y, lhs.z, 0.0f};
}

HgMat3 hg_mat2to3(HgMat2 lhs) {
    return (HgMat3){
        {lhs.x.x, lhs.x.y, 0.0f},
        {lhs.y.x, lhs.y.y, 0.0f},
        {0.0f,    0.0f,    1.0f},
    };
}

HgMat4 hg_mat2to4(HgMat2 lhs) {
    return (HgMat4){
        {lhs.x.x, lhs.x.y, 0.0f, 0.0f},
        {lhs.y.x, lhs.y.y, 0.0f, 0.0f},
        {0.0f,    0.0f,    1.0f, 0.0f},
        {0.0f,    0.0f,    0.0f, 1.0f},
    };
}

HgMat4 hg_mat3to4(HgMat3 lhs) {
    return (HgMat4){
        {lhs.x.x, lhs.x.y, lhs.x.z, 0.0f},
        {lhs.y.x, lhs.y.y, lhs.y.z, 0.0f},
        {lhs.z.x, lhs.z.y, lhs.z.z, 0.0f},
        {0.0f,    0.0f,    0.0f,    1.0f},
    };
}

void hg_vadd(u32 size, f32* dst, f32* lhs, f32* rhs){
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] + rhs[i];
    }
}

HgVec2 hg_vadd2(HgVec2 lhs, HgVec2 rhs){
    return (HgVec2){lhs.x + rhs.x, lhs.y + rhs.y};
}

HgVec3 hg_vadd3(HgVec3 lhs, HgVec3 rhs){
    return (HgVec3){lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

HgVec4 hg_vadd4(HgVec4 lhs, HgVec4 rhs){
    return (HgVec4){lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

void hg_vsub(u32 size, f32* dst, f32* lhs, f32* rhs){
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] - rhs[i];
    }
}

HgVec2 hg_vsub2(HgVec2 lhs, HgVec2 rhs){
    return (HgVec2){lhs.x - rhs.x, lhs.y - rhs.y};
}

HgVec3 hg_vsub3(HgVec3 lhs, HgVec3 rhs){
    return (HgVec3){lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

HgVec4 hg_vsub4(HgVec4 lhs, HgVec4 rhs){
    return (HgVec4){lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

void hg_vmul(u32 size, f32* dst, f32* lhs, f32* rhs){
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] * rhs[i];
    }
}

HgVec2 hg_vmul2(HgVec2 lhs, HgVec2 rhs){
    return (HgVec2){lhs.x * rhs.x, lhs.y * rhs.y};
}

HgVec3 hg_vmul3(HgVec3 lhs, HgVec3 rhs){
    return (HgVec3){lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

HgVec4 hg_vmul4(HgVec4 lhs, HgVec4 rhs){
    return (HgVec4){lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}

void hg_svmul(u32 size, f32* dst, f32 scalar, f32* vec) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(vec != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar * vec[i];
    }
}

HgVec2 hg_svmul2(f32 scalar, HgVec2 vec) {
    return (HgVec2) {scalar * vec.x, scalar * vec.y};
}

HgVec3 hg_svmul3(f32 scalar, HgVec3 vec) {
    return (HgVec3) {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

HgVec4 hg_svmul4(f32 scalar, HgVec4 vec) {
    return (HgVec4) {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

void hg_vdiv(u32 size, f32* dst, f32* lhs, f32* rhs) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] / rhs[i];
    }
}

HgVec2 hg_vdiv2(HgVec2 lhs, HgVec2 rhs) {
    return (HgVec2) {lhs.x / rhs.x, lhs.y / rhs.y};
}

HgVec3 hg_vdiv3(HgVec3 lhs, HgVec3 rhs) {
    return (HgVec3) {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

HgVec4 hg_vdiv4(HgVec4 lhs, HgVec4 rhs) {
    return (HgVec4) {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

void hg_svdiv(u32 size, f32* dst, f32 scalar, f32* vec) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(vec != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar / vec[i];
    }
}

HgVec2 hg_svdiv2(f32 scalar, HgVec2 vec) {
    return (HgVec2) {scalar / vec.x, scalar / vec.y};
}

HgVec3 hg_svdiv3(f32 scalar, HgVec3 vec) {
    return (HgVec3) {scalar / vec.x, scalar / vec.y, scalar / vec.z};
}

HgVec4 hg_svdiv4(f32 scalar, HgVec4 vec) {
    return (HgVec4) {scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w};
}

void hg_vdot(u32 size, f32* dst, f32* lhs, f32* rhs) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    *dst = 0.0f;
    for (u32 i = 0; i < size; ++i) {
        *dst += lhs[i] * rhs[i];
    }
}

float hg_vdot2(HgVec2 lhs, HgVec2 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

float hg_vdot3(HgVec3 lhs, HgVec3 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

float hg_vdot4(HgVec4 lhs, HgVec4 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

void hg_vlen(u32 size, f32* dst, f32* vec) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(vec != NULL);
    hg_vdot(size, dst, vec, vec);
    *dst = sqrtf(*dst);
}

float hg_vlen2(HgVec2 vec) {
    return sqrtf(hg_vdot2(vec, vec));
}

float hg_vlen3(HgVec3 vec) {
    return sqrtf(hg_vdot3(vec, vec));
}

float hg_vlen4(HgVec4 vec) {
    return sqrtf(hg_vdot4(vec, vec));
}

void hg_vnorm(u32 size, f32* dst, f32* vec) {
    if (size == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(vec != NULL);
    f32 len;
    hg_vlen(size, &len, vec);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hg_vnorm2(HgVec2 vec) {
    f32 len = hg_vlen2(vec);
    return (HgVec2) {vec.x / len, vec.y / len};
}

HgVec3 hg_vnorm3(HgVec3 vec) {
    f32 len = hg_vlen3(vec);
    return (HgVec3) {vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hg_vnorm4(HgVec4 vec) {
    f32 len = hg_vlen4(vec);
    return (HgVec4) {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hg_vcross(f32* dst, f32* lhs, f32* rhs) {
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

HgVec3 hg_vcross3(HgVec3 lhs, HgVec3 rhs) {
    return (HgVec3) {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x};
}

void hg_madd(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs) {
    if (width == 0 || height == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

HgMat2 hg_madd2(HgMat2 lhs, HgMat2 rhs) {
    HgMat2 result;
    hg_madd(2, 2, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat3 hg_madd3(HgMat3 lhs, HgMat3 rhs) {
    HgMat3 result;
    hg_madd(3, 3, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat4 hg_madd4(HgMat4 lhs, HgMat4 rhs) {
    HgMat4 result;
    hg_madd(4, 4, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

void hg_msub(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs) {
    if (width == 0 || height == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

HgMat2 hg_msub2(HgMat2 lhs, HgMat2 rhs) {
    HgMat2 result;
    hg_msub(2, 2, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat3 hg_msub3(HgMat3 lhs, HgMat3 rhs) {
    HgMat3 result;
    hg_msub(3, 3, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat4 hg_msub4(HgMat4 lhs, HgMat4 rhs) {
    HgMat4 result;
    hg_msub(4, 4, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

void hg_mmul(f32* dst, u32 wl, u32 hl, f32* lhs, u32 wr, u32 hr, f32* rhs) {
    if (wl == 0 || hl == 0 || wr == 0 || hr == 0)
        return;
    hg_assert(hr == wl);
    hg_assert(dst != NULL);
    hg_assert(lhs != NULL);
    hg_assert(rhs != NULL);
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

HgMat2 hg_mmul2(HgMat2 lhs, HgMat2 rhs) {
    HgMat2 result;
    hg_mmul((f32*)&result, 2, 2, (f32*)&lhs, 2, 2, (f32*)&rhs);
    return result;
}

HgMat3 hg_mmul3(HgMat3 lhs, HgMat3 rhs) {
    HgMat3 result;
    hg_mmul((f32*)&result, 3, 3, (f32*)&lhs, 3, 3, (f32*)&rhs);
    return result;
}

HgMat4 hg_mmul4(HgMat4 lhs, HgMat4 rhs) {
    HgMat4 result;
    hg_mmul((f32*)&result, 4, 4, (f32*)&lhs, 4, 4, (f32*)&rhs);
    return result;
}

void hg_mvmul(u32 width, u32 height, f32* dst, f32* mat, f32* vec) {
    if (width == 0 || height == 0)
        return;
    hg_assert(dst != NULL);
    hg_assert(mat != NULL);
    hg_assert(vec != NULL);
    for (u32 i = 0; i < height; ++i) {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j) {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

HgVec2 hg_mvmul2(HgMat2 lhs, HgVec2 rhs) {
    HgVec2 result;
    hg_mvmul(2, 2, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgVec3 hg_mvmul3(HgMat3 lhs, HgVec3 rhs) {
    HgVec3 result;
    hg_mvmul(3, 3, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgVec4 hg_mvmul4(HgMat4 lhs, HgVec4 rhs) {
    HgVec4 result;
    hg_mvmul(4, 4, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgComplex hg_cadd(HgComplex lhs, HgComplex rhs) {
    return (HgComplex) {lhs.r + rhs.r, lhs.i + rhs.i};
}

HgComplex hg_csub(HgComplex lhs, HgComplex rhs) {
    return (HgComplex) {lhs.r - rhs.r, lhs.i - rhs.i};
}

HgComplex hg_cmul(HgComplex lhs, HgComplex rhs) {
    return (HgComplex) {lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

HgQuat hg_qadd(HgQuat lhs, HgQuat rhs) {
    return (HgQuat) {lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

HgQuat hg_qsub(HgQuat lhs, HgQuat rhs) {
    return (HgQuat) {lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

HgQuat hg_qmul(HgQuat lhs, HgQuat rhs) {
    return (HgQuat) {
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

HgQuat hg_qconj(HgQuat quat) {
    return (HgQuat) {quat.r, -quat.i, -quat.j, -quat.k};
}

HgQuat hg_axis_angle(HgVec3 axis, f32 angle) {
    f32 half_angle = angle / 2.0f;
    f32 sin_half_angle = sinf(half_angle);
    return (HgQuat) {
        cosf(half_angle),
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
    };
}

HgVec3 hg_rotate_vec3(HgQuat lhs, HgVec3 rhs) {
    HgQuat q = hg_qmul(lhs, hg_qmul((HgQuat) {0.0f, rhs.x, rhs.y, rhs.z}, hg_qconj(lhs)));
    return (HgVec3) {q.i, q.j, q.k};
}

HgMat3 hg_rotate_mat3(HgQuat lhs, HgMat3 rhs) {
    return (HgMat3) {
        hg_rotate_vec3(lhs, rhs.x),
        hg_rotate_vec3(lhs, rhs.y),
        hg_rotate_vec3(lhs, rhs.z),
    };
}

HgMat4 hg_model_matrix_2d(HgVec3 position, HgVec2 scale, f32 rotation) {
    HgMat2 m2 = hg_smat2(1.0f);
    m2.x.x = scale.x;
    m2.y.y = scale.y;
    m2 = hg_mmul2((HgMat2){
        {cosf(rotation), sinf(rotation)},
        {-sinf(rotation), cosf(rotation)},
    }, m2);
    HgMat4 m4 = hg_mat2to4(m2);
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_model_matrix_3d(HgVec3 position, HgVec3 scale, HgQuat rotation) {
    HgMat3 m3 = hg_smat3(1.0f);
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hg_rotate_mat3(rotation, m3);
    HgMat4 m4 = hg_mat3to4(m3);
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_view_matrix(HgVec3 position, f32 zoom, HgQuat rotation) {
    HgMat4 rot = hg_mat3to4(hg_rotate_mat3(hg_qconj(rotation), hg_smat3(1.0f)));
    HgMat4 pos = hg_smat4(1.0f);
    pos.x.x = zoom;
    pos.y.y = zoom;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return hg_mmul4(rot, pos);
}

HgMat4 hg_orthographic_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return (HgMat4){
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hg_perspective_projection(f32 fov, f32 aspect, f32 near, f32 far) {
    hg_assert(near > 0.0f);
    hg_assert(far > near);
    f32 scale = 1 / tanf(fov / 2);
    return (HgMat4){
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

HgArena hg_arena_create(usize capacity) {
    return (HgArena){
        .data = malloc(capacity),
        .capacity = capacity,
    };
}

void hg_arena_destroy(HgArena *arena) {
    free(arena->data);
}

void hg_arena_reset(HgArena *arena) {
    hg_assert(arena != NULL);
    arena->head = 0;
}

void *hg_arena_alloc(HgArena *arena, usize size) {
    hg_assert(arena != NULL);
    if (size == 0)
        return NULL;

    usize new_head = arena->head + hg_align(size, 16);
    if (new_head > arena->capacity)
        return NULL;

    void *allocation = (u8 *)arena->data + arena->head;
    arena->head = new_head;
    return allocation;
}

void *hg_arena_realloc(HgArena *arena, void *allocation, usize old_size, usize new_size) {
    hg_assert(arena != NULL);
    if (new_size == 0) {
        arena->head = (usize)allocation - (usize)arena->data;
        return NULL;
    }

    if ((usize)allocation + hg_align(old_size, 16) == arena->head) {
        usize new_head = (usize)allocation
                       - (usize)arena->data
                       + hg_align(new_size, 16);
        if (new_head > arena->capacity)
            return NULL;
        arena->head = new_head;
        return allocation;
    }

    void *new_allocation = hg_arena_alloc(arena, new_size);
    memcpy(new_allocation, allocation, old_size);
    return new_allocation;
}

void hg_arena_free(HgArena *arena, void *allocation, usize size) {
    hg_assert(arena != NULL);
    if ((usize)allocation + hg_align(size, 16) == arena->head)
        arena->head = (usize)allocation - (usize)arena->data;
}


f64 hg_clock_tick(HgClock *hclock) {
    hg_assert(hclock != NULL);
    f64 prev = (f64)hclock->time.tv_sec + (f64)hclock->time.tv_nsec / 1.0e9;
    if (timespec_get(&hclock->time, TIME_UTC) == 0)
        hg_warn("timespec_get failed\n");
    return ((f64)hclock->time.tv_sec + (f64)hclock->time.tv_nsec / 1.0e9) - prev;
}

bool hg_file_load_binary(u8** data, usize* size, const char *path) {
    hg_assert(data != NULL);
    hg_assert(size != NULL);
    hg_assert(path != NULL);
    *data = NULL;
    *size = 0;

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        hg_warn("Could not find file to read binary: %s\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = (usize)ftell(file);
    rewind(file);

    u8* file_data = malloc(file_size);
    if (fread(file_data, 1, file_size, file) != file_size) {
        fclose(file);
        hg_warn("Failed to read binary from file: %s\n", path);
        return false;
    }

    *data = file_data;
    *size = file_size;
    fclose(file);
    return true;
}

void hg_file_unload_binary(u8* data, usize size) {
    (void)size;
    free(data);
}

bool hg_file_save_binary(const u8* data, usize size, const char *path) {
    if (size == 0)
        hg_assert(data != NULL);
    hg_assert(path != NULL);

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        hg_warn("Failed to create file to write binary: %s\n", path);
        return false;
    }

    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        hg_warn("Failed to write binary data to file: %s\n", path);
        return false;
    }

    fclose(file);
    return true;
}

void hg_vk_check(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            break;

        case VK_NOT_READY:
            hg_warn("Vulkan result not ready\n");
            break;
        case VK_TIMEOUT:
            hg_warn("Vulkan result timeout\n");
            break;
        case VK_EVENT_SET:
            hg_warn("Vulkan result event set\n");
            break;
        case VK_EVENT_RESET:
            hg_warn("Vulkan result event reset\n");
            break;
        case VK_INCOMPLETE:
            hg_warn("Vulkan result incomplete\n");
            break;
        case VK_SUBOPTIMAL_KHR:
            hg_warn("Vulkan result suboptimal\n");
            break;
        case VK_THREAD_IDLE_KHR:
            hg_warn("Vulkan result thread idle\n");
            break;
        case VK_THREAD_DONE_KHR:
            hg_warn("Vulkan result thread done\n");
            break;
        case VK_OPERATION_DEFERRED_KHR:
            hg_warn("Vulkan result operation deferred\n");
            break;
        case VK_OPERATION_NOT_DEFERRED_KHR:
            hg_warn("Vulkan result operation not deferred\n");
            break;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
            hg_error("Vulkan error out of host memory\n");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            hg_error("Vulkan error out of device memory\n");
        case VK_ERROR_INITIALIZATION_FAILED:
            hg_error("Vulkan error initialization failed\n");
        case VK_ERROR_DEVICE_LOST:
            hg_error("Vulkan error device lost\n");
        case VK_ERROR_MEMORY_MAP_FAILED:
            hg_error("Vulkan error memory map failed\n");
        case VK_ERROR_LAYER_NOT_PRESENT:
            hg_error("Vulkan error layer not present\n");
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            hg_error("Vulkan error extension not present\n");
        case VK_ERROR_FEATURE_NOT_PRESENT:
            hg_error("Vulkan error feature not present\n");
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            hg_error("Vulkan error incompatible driver\n");
        case VK_ERROR_TOO_MANY_OBJECTS:
            hg_error("Vulkan error too many objects\n");
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            hg_error("Vulkan error format not supported\n");
        case VK_ERROR_FRAGMENTED_POOL:
            hg_error("Vulkan error fragmented pool\n");
        case VK_ERROR_UNKNOWN:
            hg_error("Vulkan error unknown\n");
        case VK_ERROR_VALIDATION_FAILED:
            hg_error("Vulkan error validation failed\n");
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            hg_error("Vulkan error out of pool memory\n");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            hg_error("Vulkan error invalid external handle\n");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            hg_error("Vulkan error invalid opaque capture address\n");
        case VK_ERROR_FRAGMENTATION:
            hg_error("Vulkan error fragmentation\n");
        case VK_PIPELINE_COMPILE_REQUIRED:
            hg_error("Vulkan error pipeline compile required\n");
        case VK_ERROR_NOT_PERMITTED:
            hg_error("Vulkan error not permitted\n");
        case VK_ERROR_SURFACE_LOST_KHR:
            hg_error("Vulkan error surface lost\n");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            hg_error("Vulkan error native window in use\n");
        case VK_ERROR_OUT_OF_DATE_KHR:
            hg_error("Vulkan error out of date\n");
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            hg_error("Vulkan error incompatible display\n");
        case VK_ERROR_INVALID_SHADER_NV:
            hg_error("Vulkan error invalid shader (NV)\n");
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error image usage not supported\n");
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error video picture layout not supported\n");
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error video profile operation not supported\n");
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error video profile format not supported\n");
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error video profile codec not supported\n");
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            hg_error("Vulkan error video std version not supported\n");
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            hg_error("Vulkan error invalid DRM format modifier plane layout\n");
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            hg_error("Vulkan error full screen exclusive mode lost\n");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            hg_error("Vulkan error invalid video std parameters\n");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            hg_error("Vulkan error compression exhausted\n");
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            hg_error("Vulkan error incompatible shader binary\n");
        case VK_PIPELINE_BINARY_MISSING_KHR:
            hg_error("Vulkan error pipeline binary missing\n");
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            hg_error("Vulkan error not enough space\n");
        default:
            hg_error("Vulkan unrecognized error code\n");
    }
}

static VkBool32 hg_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        (void)fprintf(stderr, "Vulkan Error: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        (void)fprintf(stderr, "Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                        |  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
        (void)fprintf(stderr, "Vulkan Info: %s\n", callback_data->pMessage);
    } else {
        (void)fprintf(stderr, "Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT hg_debug_utils_messenger_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = hg_debug_callback,
};

VkInstance hg_vk_create_instance(const char *app_name) {
    hg_vk_load();

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name != NULL ? app_name : "Hurdy Gurdy Application",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy Engine",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

#ifndef NDEBUG
    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
#endif

    const char *exts[] = {
#ifndef NDEBUG
        "VK_EXT_debug_utils",
#endif
        "VK_KHR_surface",
#if defined(__linux__)
        "VK_KHR_xlib_surface",
#elif defined(_WIN32)
        "VK_KHR_win32_surface",
#else
#error "unsupported platform"
#endif
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
        .pNext = &hg_debug_utils_messenger_create_info,
#endif
        .pApplicationInfo = &app_info,
#ifndef NDEBUG
        .enabledLayerCount = hg_countof(layers),
        .ppEnabledLayerNames = layers,
#endif
        .enabledExtensionCount = hg_countof(exts),
        .ppEnabledExtensionNames = exts,
    };

    VkInstance instance = VK_NULL_HANDLE;
    hg_vk_check(vkCreateInstance(&instance_info, NULL, &instance));

    hg_assert(instance != NULL);
    hg_vk_load_instance(instance);
    return instance;
}

void hg_vk_destroy_instance(VkInstance instance) {
    vkDestroyInstance(instance, NULL);
}

VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance) {
    hg_assert(instance != VK_NULL_HANDLE);

    VkDebugUtilsMessengerEXT messenger = NULL;
    hg_vk_check(vkCreateDebugUtilsMessengerEXT(
        instance, &hg_debug_utils_messenger_create_info, NULL, &messenger));

    hg_assert(messenger != VK_NULL_HANDLE);
    return messenger;
}

void hg_vk_destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
    hg_assert(instance != VK_NULL_HANDLE);
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
}

static const char *const hg_vk_device_extensions[] = {
    "VK_KHR_swapchain",
};

static VkPhysicalDevice hg_internal_find_single_queue_gpu(VkInstance instance, u32 *queue_family) {
    hg_assert(instance != VK_NULL_HANDLE);
    if (queue_family == NULL)
        hg_debug("queue_family is NULL, but the physical device's queue family should be stored somewhere\n");

    u32 gpu_count;
    hg_vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, NULL));
    VkPhysicalDevice *gpus = alloca(gpu_count * sizeof(*gpus));
    hg_vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus));

    u32 ext_prop_count = 0;
    VkExtensionProperties* ext_props = NULL;

    VkPhysicalDevice suitable_gpu = VK_NULL_HANDLE;
    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];

        u32 new_prop_count = 0;
        VkResult ext_count_res = vkEnumerateDeviceExtensionProperties(gpu, NULL, &new_prop_count, NULL);
        hg_vk_check(ext_count_res);

        if (new_prop_count > ext_prop_count) {
            ext_prop_count = new_prop_count;
            ext_props = realloc(ext_props, ext_prop_count * sizeof(VkExtensionProperties));
        }

        hg_vk_check(vkEnumerateDeviceExtensionProperties(gpu, NULL, &new_prop_count, ext_props));

        for (usize j = 0; j < hg_countof(hg_vk_device_extensions); j++) {
            for (usize k = 0; k < new_prop_count; k++) {
                if (strcmp(hg_vk_device_extensions[j], ext_props[k].extensionName) == 0)
                    goto next_ext;
            }
            goto next_gpu;
next_ext:
            continue;
        }

        u32 tmp_queue_family;
        if (!hg_vk_find_queue_family(gpu, &tmp_queue_family, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            goto next_gpu;
        if (queue_family != NULL)
            *queue_family = tmp_queue_family;

        suitable_gpu = gpu;
        break;

next_gpu:
        continue;
    }

    free(ext_props);
    if (suitable_gpu == VK_NULL_HANDLE)
        hg_warn("Could not find a suitable gpu\n");
    return suitable_gpu;
}

static VkDevice hg_internal_create_single_queue_device(VkPhysicalDevice gpu, u32 queue_family) {
    hg_assert(gpu != NULL);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features synchronization2_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering_feature,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceFeatures features = {0};

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = hg_countof(hg_vk_device_extensions),
        .ppEnabledExtensionNames = hg_vk_device_extensions,
        .pEnabledFeatures = &features,
    };

    VkDevice device = VK_NULL_HANDLE;
    hg_vk_check(vkCreateDevice(gpu, &device_info, NULL, &device));

    hg_assert(device != VK_NULL_HANDLE);
    return device;
}

HgSingleQueueDeviceData hg_vk_create_single_queue_device(VkInstance instance) {
    hg_assert(instance != VK_NULL_HANDLE);

    HgSingleQueueDeviceData device = {0};
    device.gpu = hg_internal_find_single_queue_gpu(instance, &device.queue_family);
    device.handle = hg_internal_create_single_queue_device(device.gpu, device.queue_family);
    hg_vk_load_device(device.handle);
    device.queue = hg_vk_get_queue(device.handle, device.queue_family, 0);

    return device;
}

void hg_vk_destroy_device(VkDevice device) {
    vkDestroyDevice(device, NULL);
}

void hg_vk_wait_for_device(VkDevice device) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_vk_check(vkDeviceWaitIdle(device));
}

static VkFormat hg_vk_find_swapchain_format(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    assert(gpu != VK_NULL_HANDLE);
    assert(surface != VK_NULL_HANDLE);

    u32 format_count = 0;
    hg_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, NULL));
    VkSurfaceFormatKHR *formats = alloca(format_count * sizeof(*formats));
    hg_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats));

    for (usize i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hg_error("No supported swapchain formats\n");
}

static VkPresentModeKHR hg_vk_find_swapchain_present_mode(
    VkPhysicalDevice gpu,
    VkSurfaceKHR surface,
    VkPresentModeKHR desired_mode
) {
    assert(gpu != VK_NULL_HANDLE);
    assert(surface != VK_NULL_HANDLE);

    if (desired_mode == VK_PRESENT_MODE_FIFO_KHR)
        return desired_mode;

    u32 mode_count = 0;
    hg_vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, NULL));
    VkPresentModeKHR *present_modes = alloca(mode_count * sizeof(*present_modes));
    hg_vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, present_modes));

    for (usize i = 0; i < mode_count; ++i) {
        if (present_modes[i] == desired_mode)
            return desired_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

HgSwapchainData hg_vk_create_swapchain(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(gpu != VK_NULL_HANDLE);
    hg_assert(surface != VK_NULL_HANDLE);
    hg_assert(image_usage != 0);

    HgSwapchainData swapchain = {0};

    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    hg_vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities));

    if (surface_capabilities.currentExtent.width == 0 ||
        surface_capabilities.currentExtent.height == 0 ||
        surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width ||
        surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height ||
        surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width ||
        surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height
    ) {
        hg_warn("Could not create swapchain from the surface's size\n");
        return swapchain;
    }

    swapchain.width = surface_capabilities.currentExtent.width;
    swapchain.height = surface_capabilities.currentExtent.height;
    swapchain.format = hg_vk_find_swapchain_format(gpu, surface);

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = swapchain.format,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = image_usage,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = hg_vk_find_swapchain_present_mode(gpu, surface, desired_mode),
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };
    hg_vk_check(vkCreateSwapchainKHR(device, &swapchain_info, NULL, &swapchain.handle));
    hg_vk_check(vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchain.image_count, NULL));

    hg_assert(swapchain.handle != NULL);
    return swapchain;
}

void hg_vk_destroy_swapchain(VkDevice device, VkSwapchainKHR swapchain) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

void hg_vk_get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkImage *images, u32 count) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(swapchain != VK_NULL_HANDLE);
    hg_assert(images != NULL);
    hg_vk_check(vkGetSwapchainImagesKHR(device, swapchain, &count, images));
}

bool hg_vk_acquire_next_image(
    VkDevice device,
    VkSwapchainKHR swapchain,
    u32 *image_index,
    VkSemaphore signal_semaphore,
    VkFence signal_fence
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(swapchain != VK_NULL_HANDLE);
    hg_assert(image_index != NULL);

    VkResult result = vkAcquireNextImageKHR(
        device,
        swapchain,
        UINT64_MAX,
        signal_semaphore,
        signal_fence,
        image_index
    );
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        return false;

    hg_vk_check(result);
    return true;
}

void hg_vk_present(
    VkQueue queue,
    VkSwapchainKHR swapchain,
    u32 image_index,
    VkSemaphore *wait_semaphores,
    u32 semaphore_count
) {
    hg_assert(queue != VK_NULL_HANDLE);
    hg_assert(swapchain != VK_NULL_HANDLE);
    if (semaphore_count > 0)
        hg_assert(wait_semaphores != NULL);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = semaphore_count,
        .pWaitSemaphores = wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };
    hg_vk_check(vkQueuePresentKHR(queue, &present_info));
}

VkSemaphore hg_vk_create_semaphore(VkDevice device, VkSemaphoreCreateFlags flags) {
    hg_assert(device != VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = flags,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    hg_vk_check(vkCreateSemaphore(device, &semaphore_info, NULL, &semaphore));

    hg_assert(semaphore != VK_NULL_HANDLE);
    return semaphore;
}

void hg_vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroySemaphore(device, semaphore, NULL);
}

VkFence hg_vk_create_fence(VkDevice device, VkFenceCreateFlags flags) {
    hg_assert(device != VK_NULL_HANDLE);

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    VkFence fence = VK_NULL_HANDLE;
    hg_vk_check(vkCreateFence(device, &fence_info, NULL, &fence));

    hg_assert(fence != VK_NULL_HANDLE);
    return fence;
}

void hg_vk_destroy_fence(VkDevice device, VkFence fence) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyFence(device, fence, NULL);
}

void hg_vk_wait_for_fences(VkDevice device, VkFence *fences, u32 count) {
    hg_assert(device != VK_NULL_HANDLE);
    if (count > 0) {
        hg_assert(fences != NULL);
    } else {
        return;
    }
    hg_vk_check(vkWaitForFences(device, count, fences, VK_TRUE, UINT64_MAX));
}

void hg_vk_reset_fences(VkDevice device, VkFence *fences, u32 count) {
    hg_assert(device != VK_NULL_HANDLE);
    if (count > 0) {
        hg_assert(fences != NULL);
    } else {
        return;
    }
    hg_vk_check(vkResetFences(device, count, fences));
}

bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32 *queue_family, VkQueueFlags queue_flags) {
    if (gpu == VK_NULL_HANDLE)
        return false;
    hg_assert(queue_family != NULL);

    *queue_family = UINT32_MAX;

    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, NULL);
    VkQueueFamilyProperties *families = alloca(family_count * sizeof(*families));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families);

    for (u32 i = 0; i < family_count; ++i) {
        if (families[i].queueFlags & queue_flags) {
            *queue_family = i;
            return true;
        }
    }
    return false;
}

VkQueue hg_vk_get_queue(VkDevice device, u32 queue_family, u32 queue_index) {
    hg_assert(device != VK_NULL_HANDLE);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_family, queue_index, &queue);
    return queue;
}

void hg_vk_queue_wait(VkQueue queue) {
    if (queue != VK_NULL_HANDLE)
        return;
    hg_vk_check(vkQueueWaitIdle(queue));
}

void hg_vk_submit_commands(VkQueue queue, const VkSubmitInfo *submits, u32 submit_count, VkFence fence) {
    hg_assert(queue != VK_NULL_HANDLE);
    hg_vk_check(vkQueueSubmit(queue, submit_count, submits, fence));
}

VkCommandPool hg_vk_create_command_pool(VkDevice device, u32 queue_family, VkCommandPoolCreateFlags flags) {
    hg_assert(device != VK_NULL_HANDLE);

    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queue_family,
    };
    VkCommandPool pool = VK_NULL_HANDLE;
    hg_vk_check(vkCreateCommandPool(device, &info, NULL, &pool));

    hg_assert(pool != VK_NULL_HANDLE);
    return pool;
}

void hg_vk_destroy_command_pool(VkDevice device, VkCommandPool pool) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyCommandPool(device, pool, NULL);
}

void hg_vk_allocate_command_buffers(
    VkDevice device,
    VkCommandPool pool,
    VkCommandBuffer *cmds,
    u32 count,
    VkCommandBufferLevel level
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(pool != VK_NULL_HANDLE);
    if (count > 0) {
        hg_assert(cmds != NULL);
    } else {
        return;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = level,
        .commandBufferCount = count,
    };
    hg_vk_check(vkAllocateCommandBuffers(device, &alloc_info, cmds));
}

void hg_vk_free_command_buffers(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmds, u32 count) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(pool != VK_NULL_HANDLE);
    if (count > 0) {
        hg_assert(cmds != NULL);
    } else {
        return;
    }
    vkFreeCommandBuffers(device, pool, count, cmds);
}

void hg_vk_reset_command_buffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags) {
    hg_assert(cmd != NULL);
    vkResetCommandBuffer(cmd, flags);
}

VkDescriptorPool hg_vk_create_descriptor_pool(
    VkDevice device,
    u32 max_sets,
    VkDescriptorPoolSize *sizes,
    u32 count
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(max_sets > 0);
    hg_assert(sizes != NULL);
    hg_assert(count > 0);

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = max_sets,
        .poolSizeCount = count,
        .pPoolSizes = sizes
    };
    VkDescriptorPool pool = VK_NULL_HANDLE;
    hg_vk_check(vkCreateDescriptorPool(device, &pool_info, NULL, &pool));

    hg_assert(pool != VK_NULL_HANDLE);
    return pool;
}

void hg_vk_destroy_descriptor_pool(VkDevice device, VkDescriptorPool pool) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyDescriptorPool(device, pool, NULL);
}

void hg_vk_reset_descriptor_pool(VkDevice device, VkDescriptorPool pool) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(pool != VK_NULL_HANDLE);
    vkResetDescriptorPool(device, pool, 0);
}

bool hg_vk_allocate_descriptor_sets(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorSetLayout *layouts,
    VkDescriptorSet *sets,
    u32 count
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(pool != VK_NULL_HANDLE);
    hg_assert(layouts != NULL);
    hg_assert(sets != NULL);
    hg_assert(count > 0);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts,
    };
    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, sets);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        return false;
    hg_vk_check(result);
    return true;
}

void hg_vk_update_descriptor_sets(
    VkDevice device,
    const VkWriteDescriptorSet* writes,
    u32 write_count,
    const VkCopyDescriptorSet* copies,
    u32 copy_count
) {
    hg_assert(device != NULL);
    hg_assert(write_count > 0 || copy_count > 0);
    hg_assert(writes != NULL || copies != NULL);
    if (write_count > 0)
        hg_assert(writes != NULL);
    if (copy_count > 0)
        hg_assert(copies != NULL);
    vkUpdateDescriptorSets(device, write_count, writes, copy_count, copies);
}

void hg_vk_write_descriptor_set(
    VkDevice device,
    VkDescriptorSet set,
    u32 binding,
    u32 first_array_elem,
    u32 descriptor_count,
    VkDescriptorType descriptor_type,
    const VkDescriptorImageInfo *image_info,
    const VkDescriptorBufferInfo *buffer_info,
    const VkBufferView *texel_buffer_view
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(descriptor_count > 0);
    hg_assert(image_info != NULL || buffer_info != NULL || texel_buffer_view != NULL);

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = first_array_elem,
        .descriptorCount = descriptor_count,
        .descriptorType = descriptor_type,
        .pImageInfo = image_info,
        .pBufferInfo = buffer_info,
        .pTexelBufferView = texel_buffer_view
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
}

VkDescriptorSetLayout hg_vk_create_descriptor_set_layout(
    VkDevice device,
    const VkDescriptorSetLayoutBinding *bindings,
    u32 count
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(bindings != NULL);
    hg_assert(count > 0);

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = count,
        .pBindings = bindings,
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    hg_vk_check(vkCreateDescriptorSetLayout(device, &layout_info, NULL, &layout));

    return layout;
}

void hg_vk_destroy_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout layout) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(device, layout, NULL);
}

VkPipelineLayout hg_vk_create_pipeline_layout(
    VkDevice device,
    const VkDescriptorSetLayout* layouts,
    u32 layout_count,
    const VkPushConstantRange* push_constants,
    u32 push_constant_count
) {
    hg_assert(device != VK_NULL_HANDLE);
    if (layout_count > 0)
        hg_assert(layouts != NULL);
    if (push_constant_count > 0)
        hg_assert(push_constants != NULL);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layout_count,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = push_constant_count,
        .pPushConstantRanges = push_constants,
    };
    VkPipelineLayout layout = VK_NULL_HANDLE;
    hg_vk_check(vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &layout));

    hg_assert(layout != VK_NULL_HANDLE);
    return layout;
}

void hg_vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout layout) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyPipelineLayout(device, layout, NULL);
}

VkShaderModule hg_vk_create_shader_module(VkDevice device, const u8 *code, usize size) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(code != NULL);
    hg_assert(size > 0);

    VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const u32*)code,
    };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    hg_vk_check(vkCreateShaderModule(device, &shader_module_info, NULL, &shader_module));

    hg_assert(shader_module != VK_NULL_HANDLE);
    return shader_module;
}

void hg_vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyShaderModule(device, shader_module, NULL);
}

VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig *config) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(config != NULL);
    hg_assert(config->layout != NULL);
    hg_assert(config->shader_stages != NULL);
    hg_assert(config->shader_count > 0);
    if (config->vertex_binding_count > 0)
        hg_assert(config->vertex_bindings != NULL);
    if (config->color_attachment_count > 0)
        hg_assert(config->color_attachment_formats != NULL);

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = config->vertex_binding_count,
        .pVertexBindingDescriptions = config->vertex_bindings,
        .vertexAttributeDescriptionCount = config->vertex_attribute_count,
        .pVertexAttributeDescriptions = config->vertex_attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = config->topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineTessellationStateCreateInfo tessellation_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = config->tesselation_patch_control_points,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = config->polygon_mode,
        .cullMode = config->cull_mode,
        .frontFace = config->front_face,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = config->multisample_count != 0
            ? config->multisample_count
            : VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .depthWriteEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .depthCompareOp = config->enable_color_blend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .stencilTestEnable = config->stencil_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .front = {0}, // stencil : TODO
        .back = {0}, // stencil : TODO
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = config->enable_color_blend ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f},
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamic_states,
        .dynamicStateCount = hg_countof(dynamic_states),
    };

    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = config->color_attachment_count,
        .pColorAttachmentFormats = config->color_attachment_formats,
        .depthAttachmentFormat = config->depth_attachment_format,
        .stencilAttachmentFormat = config->stencil_attachment_format,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = config->shader_count,
        .pStages = config->shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = &tessellation_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = config->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    hg_vk_check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));

    hg_assert(pipeline != VK_NULL_HANDLE);
    return pipeline;
}

VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig *config) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(config != NULL);
    hg_assert(config->layout != NULL);
    hg_assert(config->shader_stages != NULL);
    hg_assert(config->shader_stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    hg_assert(config->shader_count == 1);
    if (config->vertex_binding_count > 0)
        hg_assert(config->vertex_bindings != NULL);
    hg_assert(config->color_attachment_count == 0);
    hg_assert(config->color_attachment_formats == NULL);
    hg_assert(config->depth_attachment_format == VK_FORMAT_UNDEFINED);
    hg_assert(config->stencil_attachment_format == VK_FORMAT_UNDEFINED);

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = config->shader_stages[0],
        .layout = config->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    hg_vk_check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));

    hg_assert(pipeline != VK_NULL_HANDLE);
    return pipeline;
}

void hg_vk_destroy_pipeline(VkDevice device, VkPipeline pipeline) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyPipeline(device, pipeline, NULL);
}

VkBuffer hg_vk_create_buffer(VkDevice device, const VkBufferCreateInfo *config) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(config != NULL);

    VkBuffer buffer = VK_NULL_HANDLE;
    hg_vk_check(vkCreateBuffer(device, config, NULL, &buffer));

    hg_assert(buffer != VK_NULL_HANDLE);
    return buffer;
}

void hg_vk_destroy_buffer(VkDevice device, VkBuffer buffer) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyBuffer(device, buffer, NULL);
}

VkImage hg_vk_create_image(VkDevice device, const VkImageCreateInfo *config) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(config != NULL);
    hg_assert(config->format != VK_FORMAT_UNDEFINED);
    hg_assert(config->usage != 0);

    VkImageCreateInfo create_info = *config;
    create_info.extent.width = hg_max(config->extent.width, 1);
    create_info.extent.height = hg_max(config->extent.height, 1);
    create_info.extent.depth = hg_max(config->extent.depth, 1);
    create_info.mipLevels = hg_max(config->mipLevels, 1);
    if (config->mipLevels == UINT32_MAX) {
        u32 max_length = hg_max(hg_max(
            config->extent.width,
            config->extent.height),
            config->extent.depth
        );
        create_info.mipLevels = (u32)log2f((f32)max_length) + 1;
    }
    create_info.arrayLayers = hg_max(config->arrayLayers, 1);
    create_info.samples = hg_max(config->samples, VK_SAMPLE_COUNT_1_BIT);

    VkImage image = VK_NULL_HANDLE;
    hg_vk_check(vkCreateImage(device, config, NULL, &image));

    hg_assert(image != VK_NULL_HANDLE);
    return image;
}

void hg_vk_destroy_image(VkDevice device, VkImage image) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyImage(device, image, NULL);
}

VkImageView hg_vk_create_image_view(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType type,
    VkImageSubresourceRange subresource
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(image != VK_NULL_HANDLE);
    hg_assert(format != VK_FORMAT_UNDEFINED);
    hg_assert(subresource.aspectMask != 0);

    VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = type,
        .format = format,
        .subresourceRange = {
            .aspectMask = subresource.aspectMask,
            .baseMipLevel = subresource.baseMipLevel,
            .levelCount = subresource.levelCount != 0
                ? subresource.levelCount
                : VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = subresource.baseArrayLayer,
            .layerCount = subresource.layerCount != 0
                ? subresource.layerCount
                : VK_REMAINING_ARRAY_LAYERS,
        },
    };
    VkImageView view = VK_NULL_HANDLE;
    hg_vk_check(vkCreateImageView(device, &create_info, NULL, &view));

    hg_assert(view != VK_NULL_HANDLE);
    return view;
}

void hg_vk_destroy_image_view(VkDevice device, VkImageView view) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroyImageView(device, view, NULL);
}

VkSampler hg_vk_create_sampler(VkDevice device, VkFilter filter, VkSamplerAddressMode edge_mode) {
    hg_assert(device != VK_NULL_HANDLE);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = edge_mode,
        .addressModeV = edge_mode,
        .addressModeW = edge_mode,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    VkSampler sampler = VK_NULL_HANDLE;
    hg_vk_check(vkCreateSampler(device, &sampler_info, NULL, &sampler));

    hg_assert(sampler != VK_NULL_HANDLE);
    return sampler;
}

void hg_vk_destroy_sampler(VkDevice device, VkSampler sampler) {
    hg_assert(device != VK_NULL_HANDLE);
    vkDestroySampler(device, sampler, NULL);
}

VkMemoryRequirements hg_vk_get_buffer_mem_reqs(VkDevice device, VkBuffer buffer) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(buffer != VK_NULL_HANDLE);
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device, buffer, &reqs);
    return reqs;
}

VkMemoryRequirements hg_vk_get_image_mem_reqs(VkDevice device, VkImage image) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(image != VK_NULL_HANDLE);
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(device, image, &reqs);
    return reqs;
}

static u32 hg_vk_find_memory_type_index(
    VkPhysicalDevice gpu,
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags
) {
    hg_assert(gpu != VK_NULL_HANDLE);
    hg_assert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i))) {
            if ((mem_props.memoryTypes[i].propertyFlags & undesired_flags) != 0)
                continue;
            if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
                continue;
            return i;
        }
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i))) {
            if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
                continue;
            hg_warn("Could not find Vulkan memory type without undesired flags\n");
            return i;
        }
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i))) {
            hg_warn("Could not find Vulkan memory type with desired flags\n");
            return i;
        }
    }
    hg_error("Could not find Vulkan memory type\n");
}

VkDeviceMemory hg_vk_allocate_memory(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkMemoryRequirements *mem_reqs,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags
) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(mem_reqs != NULL);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs->size,
        .memoryTypeIndex = hg_vk_find_memory_type_index(
            gpu, mem_reqs->memoryTypeBits, desired_flags, undesired_flags),
    };
    VkDeviceMemory memory = VK_NULL_HANDLE;
    hg_vk_check(vkAllocateMemory(device, &alloc_info, NULL, &memory));

    hg_assert(memory != VK_NULL_HANDLE);
    return memory;
}

void hg_vk_free_memory(VkDevice device, VkDeviceMemory memory) {
    hg_assert(device != VK_NULL_HANDLE);
    vkFreeMemory(device, memory, NULL);
}

void hg_vk_bind_buffer_memory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, usize offset) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(buffer != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    hg_vk_check(vkBindBufferMemory(device, buffer, memory, offset));
}

void hg_vk_bind_image_memory(VkDevice device, VkImage image, VkDeviceMemory memory, usize offset) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(image != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    hg_vk_check(vkBindImageMemory(device, image, memory, offset));
}

void *hg_vk_map_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    hg_assert(size > 0);

    void *data;
    hg_vk_check(vkMapMemory(device, memory, offset, size, 0, &data));
    return data;
}

void hg_vk_unmap_memory(VkDevice device, VkDeviceMemory memory) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    vkUnmapMemory(device, memory);
}

void hg_vk_flush_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    hg_assert(size > 0);

    VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = offset,
        .size = size,
    };
    hg_vk_check(vkFlushMappedMemoryRanges(device, 1, &range));
}

void hg_vk_invalidate_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(memory != VK_NULL_HANDLE);
    hg_assert(size > 0);

    VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = offset,
        .size = size,
    };
    hg_vk_check(vkInvalidateMappedMemoryRanges(device, 1, &range));
}

void hg_vk_begin_cmd(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags) {
    hg_assert(cmd != VK_NULL_HANDLE);

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };
    hg_vk_check(vkBeginCommandBuffer(cmd, &begin_info));
}

void hg_vk_end_cmd(VkCommandBuffer cmd) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_vk_check(vkEndCommandBuffer(cmd));
}

void hg_vk_copy_buffer(
    VkCommandBuffer cmd,
    VkBuffer dst,
    VkBuffer src,
    const VkBufferCopy *regions,
    u32 region_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dst != NULL);
    hg_assert(src != NULL);
    hg_assert(regions != NULL);
    hg_assert(region_count > 0);
    vkCmdCopyBuffer(cmd, src, dst, region_count, regions);
}

void hg_vk_copy_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkImage src,
    const VkImageCopy *regions,
    u32 region_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dst != NULL);
    hg_assert(src != NULL);
    hg_assert(regions != NULL);
    hg_assert(region_count > 0);
    vkCmdCopyImage(
        cmd,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        region_count,
        regions);
}

void hg_vk_blit_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkImage src,
    const VkImageBlit *regions,
    u32 region_count,
    VkFilter filter
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dst != NULL);
    hg_assert(src != NULL);
    hg_assert(regions != NULL);
    hg_assert(region_count > 0);
    vkCmdBlitImage(
        cmd,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        region_count,
        regions,
        filter);
}

void hg_vk_copy_buffer_to_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkBuffer src,
    VkBufferImageCopy *regions,
    u32 region_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dst != NULL);
    hg_assert(src != NULL);
    hg_assert(regions != NULL);
    hg_assert(region_count > 0);
    vkCmdCopyBufferToImage(
        cmd,
        src,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        region_count,
        regions);
}

void hg_vk_copy_image_to_buffer(
    VkCommandBuffer cmd,
    VkBuffer dst,
    VkImage src,
    VkBufferImageCopy *regions,
    u32 region_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dst != NULL);
    hg_assert(src != NULL);
    hg_assert(regions != NULL);
    hg_assert(region_count > 0);
    vkCmdCopyImageToBuffer(
        cmd,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst,
        region_count,
        regions);
}

void hg_vk_pipeline_barrier(VkCommandBuffer cmd, const VkDependencyInfo *dependencies) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(dependencies != NULL);
    vkCmdPipelineBarrier2(cmd, dependencies);
}

void hg_vk_begin_rendering(VkCommandBuffer cmd, const VkRenderingInfo *info) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(info != NULL);
    vkCmdBeginRendering(cmd, info);
}

void hg_vk_end_rendering(VkCommandBuffer cmd) {
    hg_assert(cmd != VK_NULL_HANDLE);
    vkCmdEndRendering(cmd);
}

void hg_vk_set_viewport(
    VkCommandBuffer cmd,
    f32 x,
    f32 y,
    f32 width,
    f32 height,
    f32 near,
    f32 far
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    VkViewport viewport = {x, y, width, height, near, far};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
}

void hg_vk_set_scissor(VkCommandBuffer cmd, i32 x, i32 y, u32 width, u32 height) {
    hg_assert(cmd != VK_NULL_HANDLE);
    VkRect2D scissor = {{x, y}, {width, height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void hg_vk_bind_pipeline(
    VkCommandBuffer cmd,
    VkPipeline pipeline,
    VkPipelineBindPoint bind_point
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(pipeline != VK_NULL_HANDLE);
    vkCmdBindPipeline(cmd, bind_point, pipeline);
}

void hg_vk_bind_descriptor_sets(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    u32 begin_index,
    u32 set_count,
    const VkDescriptorSet *descriptor_sets
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(layout != VK_NULL_HANDLE);
    vkCmdBindDescriptorSets(cmd, bind_point, layout, begin_index, set_count, descriptor_sets, 0, NULL);
}

void hg_vk_push_constants(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkShaderStageFlags stages,
    u32 offset,
    u32 size,
    const void *data
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(layout != VK_NULL_HANDLE);
    hg_assert(stages != 0);
    hg_assert(data != NULL);
    hg_assert(size > 0);
    vkCmdPushConstants(cmd, layout, stages, offset, size, data);
}

void hg_vk_bind_vertex_buffers(
    VkCommandBuffer cmd,
    u32 begin_index,
    u32 count,
    VkBuffer *vertex_buffers,
    usize *offsets

) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(count > 0);
    hg_assert(vertex_buffers != NULL);
    hg_assert(offsets != NULL);
    vkCmdBindVertexBuffers(cmd, begin_index, count, vertex_buffers, offsets);
}

void hg_vk_bind_vertex_buffer(VkCommandBuffer cmd, VkBuffer vertex_buffer) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(vertex_buffer != VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer, &(usize){0});
}

void hg_vk_bind_index_buffer(
    VkCommandBuffer cmd,
    VkBuffer index_buffer,
    usize offset,
    VkIndexType type
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(index_buffer != VK_NULL_HANDLE);
    vkCmdBindIndexBuffer(cmd, index_buffer, offset, type);
}

void hg_vk_draw(
    VkCommandBuffer cmd,
    u32 first_vertex,
    u32 vertex_count,
    u32 first_instance,
    u32 instance_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(vertex_count > 0);
    hg_assert(instance_count > 0);
    vkCmdDraw(cmd, vertex_count, instance_count, first_vertex, first_instance);
}

void hg_vk_draw_indexed(
    VkCommandBuffer cmd,
    u32 vertex_offset,
    u32 first_index,
    u32 index_count,
    u32 first_instance,
    u32 instance_count
) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(index_count > 0);
    hg_assert(instance_count > 0);
    vkCmdDrawIndexed(cmd, index_count, instance_count, first_index, (i32)vertex_offset, first_instance);
}

void hg_vk_dispatch(VkCommandBuffer cmd, u32 x, u32 y, u32 z) {
    hg_assert(cmd != VK_NULL_HANDLE);
    hg_assert(x > 0);
    hg_assert(y > 0);
    hg_assert(z > 0);
    vkCmdDispatch(cmd, x, y, z);
}

HgFrameSync hg_frame_sync_create(VkDevice device, u32 queue_family, u32 image_count) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(image_count > 0);

    HgFrameSync sync = {.frame_count = image_count};

    sync.pool = hg_vk_create_command_pool(
        device, queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    HgArena arena = hg_arena_create(
        (sync.frame_count * sizeof(*sync.cmds)) +
        (sync.frame_count * sizeof(*sync.frame_finished)) +
        (sync.frame_count * sizeof(*sync.image_available)) +
        (sync.frame_count * sizeof(*sync.ready_to_present)));

    sync.cmds = hg_arena_alloc(&arena, sync.frame_count * sizeof(*sync.cmds));
    hg_vk_allocate_command_buffers(
        device, sync.pool, sync.cmds, sync.frame_count, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    sync.frame_finished = hg_arena_alloc(&arena, sync.frame_count * sizeof(*sync.frame_finished));
    for (usize i = 0; i < sync.frame_count; ++i) {
        sync.frame_finished[i] = hg_vk_create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT);
    }
    sync.image_available = hg_arena_alloc(&arena, sync.frame_count * sizeof(*sync.image_available));
    for (usize i = 0; i < sync.frame_count; ++i) {
        sync.image_available[i] = hg_vk_create_semaphore(device, 0);
    }
    sync.ready_to_present = hg_arena_alloc(&arena, sync.frame_count * sizeof(*sync.ready_to_present));
    for (usize i = 0; i < sync.frame_count; ++i) {
        sync.ready_to_present[i] = hg_vk_create_semaphore(device, 0);
    }

    return sync;
}

void hg_frame_sync_destroy(VkDevice device, HgFrameSync *sync) {
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(sync != NULL);

    hg_vk_free_command_buffers(device, sync->pool, sync->cmds, sync->frame_count);
    for (usize i = 0; i < sync->frame_count; ++i) {
        hg_vk_destroy_fence(device, sync->frame_finished[i]);
    }
    for (usize i = 0; i < sync->frame_count; ++i) {
        hg_vk_destroy_semaphore(device, sync->image_available[i]);
    }
    for (usize i = 0; i < sync->frame_count; ++i) {
        hg_vk_destroy_semaphore(device, sync->ready_to_present[i]);
    }
    hg_vk_destroy_command_pool(device, sync->pool);
    free(sync->cmds);
}

VkCommandBuffer hg_frame_sync_begin_frame(VkDevice device, HgFrameSync *sync, VkSwapchainKHR swapchain) {
    hg_assert(sync != NULL);
    hg_assert(device != VK_NULL_HANDLE);
    hg_assert(swapchain != VK_NULL_HANDLE);

    sync->current_frame = (sync->current_frame + 1) % sync->frame_count;

    hg_vk_wait_for_fences(device, &sync->frame_finished[sync->current_frame], 1);
    hg_vk_reset_fences(device, &sync->frame_finished[sync->current_frame], 1);

    hg_vk_acquire_next_image(
        device,
        swapchain,
        &sync->current_image,
        sync->image_available[sync->current_frame],
        VK_NULL_HANDLE);

    VkCommandBuffer cmd = sync->cmds[sync->current_frame];
    hg_vk_begin_cmd(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return cmd;
}

void hg_frame_sync_end_frame_and_present(VkQueue queue, HgFrameSync *sync, VkSwapchainKHR swapchain) {
    VkCommandBuffer cmd = sync->cmds[sync->current_frame];
    hg_vk_end_cmd(cmd);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sync->image_available[sync->current_frame],
        .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sync->ready_to_present[sync->current_image],
    };
    hg_vk_submit_commands(queue, &submit, 1, sync->frame_finished[sync->current_frame]);

    hg_vk_present(queue, swapchain, sync->current_image, &sync->ready_to_present[sync->current_image], 1);
}

void hg_vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) {
    hg_assert(instance != VK_NULL_HANDLE);
    vkDestroySurfaceKHR(instance, surface, NULL);
}

typedef struct HgWindowInput {
    u32 width;
    u32 height;
    f64 mouse_pos_x;
    f64 mouse_pos_y;
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    bool was_resized;
    bool was_closed;
    bool keys_down[HG_KEY_COUNT];
    bool keys_pressed[HG_KEY_COUNT];
    bool keys_released[HG_KEY_COUNT];
} HgWindowInput;

#if defined(__linux__)

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

typedef struct HgX11Funcs {
    Display *(*XOpenDisplay)(_Xconst char*);
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
} HgX11Funcs;

static void *hg_libx11 = NULL;
static HgX11Funcs hg_x11_pfn = {0};

Display *XOpenDisplay(_Xconst char *name) {
    return hg_x11_pfn.XOpenDisplay(name);
}

int XCloseDisplay(Display *dpy) {
    return hg_x11_pfn.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display *dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int class,
    Visual *visual,
    unsigned long valuemask,
    XSetWindowAttributes *attributes
) {
    return hg_x11_pfn.XCreateWindow(
        dpy, parent, x, y, width, height, border_width,
        depth, class, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display *dpy, Window w) {
    return hg_x11_pfn.XDestroyWindow(dpy, w);
}

int XStoreName(Display *dpy, Window w, _Xconst char *name) {
    return hg_x11_pfn.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display *dpy, _Xconst char *name, Bool only_if_exists) {
    return hg_x11_pfn.XInternAtom(dpy, name, only_if_exists);
}

Status XSetWMProtocols(Display *dpy, Window w, Atom *protocols, int count) {
    return hg_x11_pfn.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display *dpy, Window w) {
    return hg_x11_pfn.XMapWindow(dpy, w);
}

Status XSendEvent(Display *dpy, Window w, Bool propagate, long event_mask, XEvent *event) {
    return hg_x11_pfn.XSendEvent(dpy, w, propagate, event_mask, event);
}

int XFlush(Display *dpy) {
    return hg_x11_pfn.XFlush(dpy);
}

int XNextEvent(Display *dpy, XEvent *event) {
    return hg_x11_pfn.XNextEvent(dpy, event);
}

int XPending(Display *dpy) {
    return hg_x11_pfn.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent *key_event, int index) {
    return hg_x11_pfn.XLookupKeysym(key_event, index);
}

#define HG_LOAD_X11_FUNC(name) *(void **)&hg_x11_pfn. name \
    = dlsym(hg_libx11, #name); \
    if (hg_x11_pfn. name == NULL) { hg_error("Could not load Xlib function: \n" #name) }

HgPlatform *hg_platform_create(void) {
    HgPlatform *platform = NULL;

    if (hg_libx11 == NULL) {
        hg_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
        if (hg_libx11 == NULL)
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
    }

    platform = (HgPlatform *)XOpenDisplay(NULL);
    if (platform == NULL)
        hg_error("Could not open X display\n");

    return platform;
}

void hg_platform_destroy(HgPlatform *platform) {
    XCloseDisplay((Display *)platform);
}

static Window hg_create_x11_window(
    Display *display,
    u32 width,
    u32 height,
    const char *title
) {
    XSetWindowAttributes window_attributes = {
        .event_mask
            = KeyPressMask
            | KeyReleaseMask
            | ButtonPressMask
            | ButtonReleaseMask
            | PointerMotionMask
            | StructureNotifyMask
    };
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

    if (title != NULL) {
        int name_result = XStoreName(display, window, title);
        if (name_result == 0)
            hg_error("X11 could not set window title\n");
    }

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        hg_error("X11 could not map window\n");

    return window;
}

static Atom hg_set_delete_behavior(
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

static void hg_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        hg_error("X11 failed to get state atom\n");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        hg_error("X11 failed to get fullscreen atom\n");

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &(XEvent){.xclient = {
            .type = ClientMessage,
            .window = window,
            .message_type = state_atom,
            .format = 32,
            .data.l = {
                [0] = 1,
                [1] = (long)fullscreen_atom,
                [2] = 0,
                [3] = 0,
                [4] = 0,
            },
        }}
    );
    if (fullscreen_result == 0)
        hg_error("X11 could not send fullscreen message\n");
}

struct HgWindow {
    HgWindowInput input;
    Window x11_window;
    Atom delete_atom;
};

HgWindow *hg_window_create(const HgPlatform *platform, const HgWindowConfig *config) {
    hg_assert(platform != NULL);
    hg_assert(config != NULL);

    u32 width = config->windowed ? config->width
        : (u32)DisplayWidth((Display *)platform, DefaultScreen((Display *)platform));
    u32 height = config->windowed ? config->height
        : (u32)DisplayHeight((Display *)platform, DefaultScreen((Display *)platform));

    HgWindow *window = malloc(sizeof(*window));
    window->input = (HgWindowInput){
        .width = width,
        .height = height,
    };

    window->x11_window = hg_create_x11_window((Display *)platform, width, height, config->title);

    window->delete_atom = hg_set_delete_behavior((Display *)platform, window->x11_window);

    if (!config->windowed)
        hg_set_fullscreen((Display *)platform, window->x11_window);

    int flush_result = XFlush((Display *)platform);
    if (flush_result == 0)
        hg_error("X11 could not flush window\n");

    return window;
}

void hg_window_destroy(const HgPlatform *platform, HgWindow *window) {
    hg_assert(platform != NULL);
    if (window != NULL)
        XDestroyWindow((Display *)platform, window->x11_window);
    XFlush((Display *)platform);
    free(window);
}

void hg_window_set_icon(const HgPlatform *platform, HgWindow *window, u32 *icon_data, u32 width, u32 height);

bool hg_window_get_fullscreen(const HgPlatform *platform, HgWindow *window);

void hg_window_set_fullscreen(const HgPlatform *platform, HgWindow *window, bool fullscreen);

void hg_window_set_cursor(const HgPlatform *platform, HgWindow *window, HgCursor cursor);

void hg_window_set_cursor_image(const HgPlatform *platform, HgWindow *window, u32 *data, u32 width, u32 height);

VkSurfaceKHR hg_vk_create_surface(
    VkInstance instance,
    const HgPlatform *platform,
    const HgWindow *window
) {
    hg_assert(instance != VK_NULL_HANDLE);
    hg_assert(platform != NULL);
    hg_assert(window != NULL);

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == NULL)
        hg_error("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = (Display *)platform,
        .window = window->x11_window,
    };
    hg_vk_check(pfn_vkCreateXlibSurfaceKHR(instance, &create_info, NULL, &surface));

    hg_assert(surface != NULL);
    return surface;
}

void hg_window_process_events(const HgPlatform *platform, HgWindow **windows, u32 window_count) {
    hg_assert(platform != NULL);
    hg_assert(windows != NULL);
    hg_assert(window_count > 0);

    if (window_count > 1)
        hg_error("Multiple windows unsupported\n"); // : TODO
    HgWindow* window = windows[0];

    memset(window->input.keys_pressed, 0, sizeof(window->input.keys_pressed));
    memset(window->input.keys_released, 0, sizeof(window->input.keys_released));
    window->input.was_resized = false;

    u32 old_window_width = window->input.width;
    u32 old_window_height = window->input.height;
    f64 old_mouse_pos_x = window->input.mouse_pos_x;
    f64 old_mouse_pos_y = window->input.mouse_pos_y;

    while (XPending((Display *)platform)) {
        XEvent event;
        int event_result = XNextEvent((Display *)platform, &event);
        if (event_result != 0)
            hg_error("X11 could not get next event\n");

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window->delete_atom)
                    window->input.was_closed = true;
                break;
            case ConfigureNotify:
                window->input.width = (u32)event.xconfigure.width;
                window->input.height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HG_KEY_NONE;
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_0:
                        key = HG_KEY_0;
                        break;
                    case XK_1:
                        key = HG_KEY_1;
                        break;
                    case XK_2:
                        key = HG_KEY_2;
                        break;
                    case XK_3:
                        key = HG_KEY_3;
                        break;
                    case XK_4:
                        key = HG_KEY_4;
                        break;
                    case XK_5:
                        key = HG_KEY_5;
                        break;
                    case XK_6:
                        key = HG_KEY_6;
                        break;
                    case XK_7:
                        key = HG_KEY_7;
                        break;
                    case XK_8:
                        key = HG_KEY_8;
                        break;
                    case XK_9:
                        key = HG_KEY_9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HG_KEY_Q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HG_KEY_W;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HG_KEY_E;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HG_KEY_R;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HG_KEY_T;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HG_KEY_Y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HG_KEY_U;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HG_KEY_I;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HG_KEY_O;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HG_KEY_P;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HG_KEY_A;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HG_KEY_S;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HG_KEY_D;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HG_KEY_F;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HG_KEY_G;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HG_KEY_H;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HG_KEY_J;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HG_KEY_K;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HG_KEY_L;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HG_KEY_Z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HG_KEY_X;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HG_KEY_C;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HG_KEY_V;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HG_KEY_B;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HG_KEY_N;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HG_KEY_M;
                        break;

                    case XK_semicolon:
                        key = HG_KEY_SEMICOLON;
                        break;
                    case XK_colon:
                        key = HG_KEY_COLON;
                        break;
                    case XK_apostrophe:
                        key = HG_KEY_APOSTROPHE;
                        break;
                    case XK_quotedbl:
                        key = HG_KEY_QUOTATION;
                        break;
                    case XK_comma:
                        key = HG_KEY_COMMA;
                        break;
                    case XK_period:
                        key = HG_KEY_PERIOD;
                        break;
                    case XK_question:
                        key = HG_KEY_QUESTION;
                        break;
                    case XK_grave:
                        key = HG_KEY_GRAVE;
                        break;
                    case XK_asciitilde:
                        key = HG_KEY_TILDE;
                        break;
                    case XK_exclam:
                        key = HG_KEY_EXCLAMATION;
                        break;
                    case XK_at:
                        key = HG_KEY_AT;
                        break;
                    case XK_numbersign:
                        key = HG_KEY_HASH;
                        break;
                    case XK_dollar:
                        key = HG_KEY_DOLLAR;
                        break;
                    case XK_percent:
                        key = HG_KEY_PERCENT;
                        break;
                    case XK_asciicircum:
                        key = HG_KEY_CAROT;
                        break;
                    case XK_ampersand:
                        key = HG_KEY_AMPERSAND;
                        break;
                    case XK_asterisk:
                        key = HG_KEY_ASTERISK;
                        break;
                    case XK_parenleft:
                        key = HG_KEY_LPAREN;
                        break;
                    case XK_parenright:
                        key = HG_KEY_RPAREN;
                        break;
                    case XK_bracketleft:
                        key = HG_KEY_LBRACKET;
                        break;
                    case XK_bracketright:
                        key = HG_KEY_RBRACKET;
                        break;
                    case XK_braceleft:
                        key = HG_KEY_LBRACE;
                        break;
                    case XK_braceright:
                        key = HG_KEY_RBRACE;
                        break;
                    case XK_equal:
                        key = HG_KEY_EQUAL;
                        break;
                    case XK_less:
                        key = HG_KEY_LESS;
                        break;
                    case XK_greater:
                        key = HG_KEY_GREATER;
                        break;
                    case XK_plus:
                        key = HG_KEY_PLUS;
                        break;
                    case XK_minus:
                        key = HG_KEY_MINUS;
                        break;
                    case XK_slash:
                        key = HG_KEY_SLASH;
                        break;
                    case XK_backslash:
                        key = HG_KEY_BACKSLASH;
                        break;
                    case XK_underscore:
                        key = HG_KEY_UNDERSCORE;
                        break;
                    case XK_bar:
                        key = HG_KEY_BAR;
                        break;

                    case XK_Up:
                        key = HG_KEY_UP;
                        break;
                    case XK_Down:
                        key = HG_KEY_DOWN;
                        break;
                    case XK_Left:
                        key = HG_KEY_LEFT;
                        break;
                    case XK_Right:
                        key = HG_KEY_RIGHT;
                        break;
                    case XK_Escape:
                        key = HG_KEY_ESCAPE;
                        break;
                    case XK_space:
                        key = HG_KEY_SPACE;
                        break;
                    case XK_Return:
                        key = HG_KEY_ENTER;
                        break;
                    case XK_BackSpace:
                        key = HG_KEY_BACKSPACE;
                        break;
                    case XK_Delete:
                        key = HG_KEY_DELETE;
                        break;
                    case XK_Insert:
                        key = HG_KEY_INSERT;
                        break;
                    case XK_Tab:
                        key = HG_KEY_TAB;
                        break;
                    case XK_Home:
                        key = HG_KEY_HOME;
                        break;
                    case XK_End:
                        key = HG_KEY_END;
                        break;

                    case XK_F1:
                        key = HG_KEY_F1;
                        break;
                    case XK_F2:
                        key = HG_KEY_F2;
                        break;
                    case XK_F3:
                        key = HG_KEY_F3;
                        break;
                    case XK_F4:
                        key = HG_KEY_F4;
                        break;
                    case XK_F5:
                        key = HG_KEY_F5;
                        break;
                    case XK_F6:
                        key = HG_KEY_F6;
                        break;
                    case XK_F7:
                        key = HG_KEY_F7;
                        break;
                    case XK_F8:
                        key = HG_KEY_F8;
                        break;
                    case XK_F9:
                        key = HG_KEY_F9;
                        break;
                    case XK_F10:
                        key = HG_KEY_F10;
                        break;
                    case XK_F11:
                        key = HG_KEY_F11;
                        break;
                    case XK_F12:
                        key = HG_KEY_F12;
                        break;

                    case XK_Shift_L:
                        key = HG_KEY_LSHIFT;
                        break;
                    case XK_Shift_R:
                        key = HG_KEY_RSHIFT;
                        break;
                    case XK_Control_L:
                        key = HG_KEY_LCTRL;
                        break;
                    case XK_Control_R:
                        key = HG_KEY_RCTRL;
                        break;
                    case XK_Meta_L:
                        key = HG_KEY_LMETA;
                        break;
                    case XK_Meta_R:
                        key = HG_KEY_RMETA;
                        break;
                    case XK_Alt_L:
                        key = HG_KEY_LALT;
                        break;
                    case XK_Alt_R:
                        key = HG_KEY_RALT;
                        break;
                    case XK_Super_L:
                        key = HG_KEY_LSUPER;
                        break;
                    case XK_Super_R:
                        key = HG_KEY_RSUPER;
                        break;
                    case XK_Caps_Lock:
                        key = HG_KEY_CAPSLOCK;
                        break;
                }
                if (event.type == KeyPress) {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                } else if (event.type == KeyRelease) {
                    window->input.keys_released[key] = true;
                    window->input.keys_down[key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HG_KEY_NONE;
                switch (event.xbutton.button) {
                    case Button1:
                        key = HG_KEY_MOUSE1;
                        break;
                    case Button2:
                        key = HG_KEY_MOUSE2;
                        break;
                    case Button3:
                        key = HG_KEY_MOUSE3;
                        break;
                    case Button4:
                        key = HG_KEY_MOUSE4;
                        break;
                    case Button5:
                        key = HG_KEY_MOUSE5;
                        break;
                }
                if (event.type == ButtonPress) {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                } else if (event.type == ButtonRelease) {
                    window->input.keys_released[key] = true;
                    window->input.keys_down[key] = false;
                }
            } break;
            case MotionNotify:
                window->input.mouse_pos_x = (f64)event.xmotion.x / (f64)window->input.height;
                window->input.mouse_pos_y = (f64)event.xmotion.y / (f64)window->input.height;
                break;
            default:
                break;
        }
    }

    if (window->input.width != old_window_width || window->input.height != old_window_height) {
        window->input.was_resized = true;
    }

    window->input.mouse_delta_x = window->input.mouse_pos_x - old_mouse_pos_x;
    window->input.mouse_delta_y = window->input.mouse_pos_y - old_mouse_pos_y;
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

HgPlatform *hg_platform_create(void) {
    return (HgPlatform *)GetModuleHandle(NULL);
}

void hg_platform_destroy(HgPlatform *platform) {
    (void)platform;
}

struct HgWindow {
    HgWindowInput input;
    HWND hwnd;
};

static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HgWindow *window = (HgWindow *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA *)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->input.was_closed = true;
            break;
        case WM_SIZE:
            window->input.width = LOWORD(lparam);
            window->input.height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            memset(window->input.keys_down, 0, sizeof(window->input.keys_down));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HG_KEY_NONE;
            HgKey shift_key = HG_KEY_NONE;

            switch (wparam) {
                case '0':
                    key = HG_KEY_0;
                    shift_key = HG_KEY_RPAREN;
                    break;
                case '1':
                    key = HG_KEY_1;
                    shift_key = HG_KEY_EXCLAMATION;
                    break;
                case '2':
                    key = HG_KEY_2;
                    shift_key = HG_KEY_AT;
                    break;
                case '3':
                    key = HG_KEY_3;
                    shift_key = HG_KEY_HASH;
                    break;
                case '4':
                    key = HG_KEY_4;
                    shift_key = HG_KEY_DOLLAR;
                    break;
                case '5':
                    key = HG_KEY_5;
                    shift_key = HG_KEY_PERCENT;
                    break;
                case '6':
                    key = HG_KEY_6;
                    shift_key = HG_KEY_CAROT;
                    break;
                case '7':
                    key = HG_KEY_7;
                    shift_key = HG_KEY_AMPERSAND;
                    break;
                case '8':
                    key = HG_KEY_8;
                    shift_key = HG_KEY_ASTERISK;
                    break;
                case '9':
                    key = HG_KEY_9;
                    shift_key = HG_KEY_LPAREN;
                    break;

                case 'A':
                    key = HG_KEY_A;
                    break;
                case 'B':
                    key = HG_KEY_B;
                    break;
                case 'C':
                    key = HG_KEY_C;
                    break;
                case 'D':
                    key = HG_KEY_D;
                    break;
                case 'E':
                    key = HG_KEY_E;
                    break;
                case 'F':
                    key = HG_KEY_F;
                    break;
                case 'G':
                    key = HG_KEY_G;
                    break;
                case 'H':
                    key = HG_KEY_H;
                    break;
                case 'I':
                    key = HG_KEY_I;
                    break;
                case 'J':
                    key = HG_KEY_J;
                    break;
                case 'K':
                    key = HG_KEY_K;
                    break;
                case 'L':
                    key = HG_KEY_L;
                    break;
                case 'M':
                    key = HG_KEY_M;
                    break;
                case 'N':
                    key = HG_KEY_N;
                    break;
                case 'O':
                    key = HG_KEY_O;
                    break;
                case 'P':
                    key = HG_KEY_P;
                    break;
                case 'Q':
                    key = HG_KEY_Q;
                    break;
                case 'R':
                    key = HG_KEY_R;
                    break;
                case 'S':
                    key = HG_KEY_S;
                    break;
                case 'T':
                    key = HG_KEY_T;
                    break;
                case 'U':
                    key = HG_KEY_U;
                    break;
                case 'V':
                    key = HG_KEY_V;
                    break;
                case 'W':
                    key = HG_KEY_W;
                    break;
                case 'X':
                    key = HG_KEY_X;
                    break;
                case 'Y':
                    key = HG_KEY_Y;
                    break;
                case 'Z':
                    key = HG_KEY_Z;
                    break;

                case VK_OEM_1:
                    key = HG_KEY_SEMICOLON;
                    shift_key = HG_KEY_COLON;
                    break;
                case VK_OEM_7:
                    key = HG_KEY_APOSTROPHE;
                    shift_key = HG_KEY_QUOTATION;
                    break;
                case VK_OEM_COMMA:
                    key = HG_KEY_COMMA;
                    shift_key = HG_KEY_LESS;
                    break;
                case VK_OEM_PERIOD:
                    key = HG_KEY_PERIOD;
                    shift_key = HG_KEY_GREATER;
                    break;
                case VK_OEM_2:
                    key = HG_KEY_SLASH;
                    shift_key = HG_KEY_QUESTION;
                    break;
                case VK_OEM_3:
                    key = HG_KEY_GRAVE;
                    shift_key = HG_KEY_TILDE;
                    break;
                case VK_OEM_4:
                    key = HG_KEY_LBRACKET;
                    shift_key = HG_KEY_LBRACE;
                    break;
                case VK_OEM_6:
                    key = HG_KEY_RBRACKET;
                    shift_key = HG_KEY_RBRACE;
                    break;
                case VK_OEM_5:
                    key = HG_KEY_BACKSLASH;
                    shift_key = HG_KEY_BAR;
                    break;
                case VK_OEM_PLUS:
                    key = HG_KEY_EQUAL;
                    shift_key = HG_KEY_PLUS;
                    break;
                case VK_OEM_MINUS:
                    key = HG_KEY_MINUS;
                    shift_key = HG_KEY_UNDERSCORE;
                    break;

                case VK_UP:
                    key = HG_KEY_UP;
                    break;
                case VK_DOWN:
                    key = HG_KEY_DOWN;
                    break;
                case VK_LEFT:
                    key = HG_KEY_LEFT;
                    break;
                case VK_RIGHT:
                    key = HG_KEY_RIGHT;
                    break;
                case VK_ESCAPE:
                    key = HG_KEY_ESCAPE;
                    break;
                case VK_SPACE:
                    key = HG_KEY_SPACE;
                    break;
                case VK_RETURN:
                    key = HG_KEY_ENTER;
                    break;
                case VK_BACK:
                    key = HG_KEY_BACKSPACE;
                    break;
                case VK_DELETE:
                    key = HG_KEY_DELETE;
                    break;
                case VK_INSERT:
                    key = HG_KEY_INSERT;
                    break;
                case VK_TAB:
                    key = HG_KEY_TAB;
                    break;
                case VK_HOME:
                    key = HG_KEY_HOME;
                    break;
                case VK_END:
                    key = HG_KEY_END;
                    break;

                case VK_F1:
                    key = HG_KEY_F1;
                    break;
                case VK_F2:
                    key = HG_KEY_F2;
                    break;
                case VK_F3:
                    key = HG_KEY_F3;
                    break;
                case VK_F4:
                    key = HG_KEY_F4;
                    break;
                case VK_F5:
                    key = HG_KEY_F5;
                    break;
                case VK_F6:
                    key = HG_KEY_F6;
                    break;
                case VK_F7:
                    key = HG_KEY_F7;
                    break;
                case VK_F8:
                    key = HG_KEY_F8;
                    break;
                case VK_F9:
                    key = HG_KEY_F9;
                    break;
                case VK_F10:
                    key = HG_KEY_F10;
                    break;
                case VK_F11:
                    key = HG_KEY_F11;
                    break;
                case VK_F12:
                    key = HG_KEY_F12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HG_KEY_RSHIFT;
                    else if (scancode == 0x2A)
                        key = HG_KEY_LSHIFT;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RALT;
                    else
                        key = HG_KEY_LALT;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RCTRL;
                    else
                        key = HG_KEY_LCTRL;
                    break;
                case VK_LWIN:
                    key = HG_KEY_LSUPER;
                    break;
                case VK_RWIN:
                    key = HG_KEY_RSUPER;
                    break;
                case VK_CAPITAL:
                    key = HG_KEY_CAPSLOCK;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
                if (shift_key != HG_KEY_NONE &&
                   (window->input.keys_down[HG_KEY_LSHIFT] ||
                    window->input.keys_down[HG_KEY_RSHIFT])
                ) {
                    window->input.keys_pressed[shift_key] = true;
                    window->input.keys_down[shift_key] = true;
                } else {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                }
            } else {
                window->input.keys_released[shift_key] = window->input.keys_down[shift_key];
                window->input.keys_down[shift_key] = false;
                window->input.keys_released[key] = window->input.keys_down[key];
                window->input.keys_down[key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = true;
            break;
        case WM_RBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = true;
            break;
        case WM_MBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = true;
            break;
        case WM_LBUTTONUP:
            window->input.keys_released[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = false;
            break;
        case WM_RBUTTONUP:
            window->input.keys_released[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = false;
            break;
        case WM_MBUTTONUP:
            window->input.keys_released[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = false;
            break;
        case WM_MOUSEMOVE:
            window->input.mouse_pos_x = (f64)LOWORD(lparam) / (f64)window->input.height;
            window->input.mouse_pos_y = (f64)HIWORD(lparam) / (f64)window->input.height;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow *hg_window_create(const HgPlatform *platform, const HgWindowConfig *config) {
    const char *title = config->title != NULL ? config->title : "Hurdy Gurdy";

    HgWindow *window = malloc(sizeof(*window));
    window->input = (HgWindowInput){
        .width = config->width,
        .height = config->height,
    };

    WNDCLASSA window_class = {
        .hInstance = (HINSTANCE)platform,
            .hIcon = LoadIcon(NULL, IDI_APPLICATION),
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = title,
            .lpfnWndProc = window_callback,
    };
    if (!RegisterClassA(&window_class))
        hg_error("Win32 failed to register window class for window: %s\n", config->title);

    if (config->windowed) {
        window->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window->input.width,
            window->input.height,
            NULL,
            NULL,
            (HINSTANCE)platform,
            window
        );
    } else {
        window->input.width = GetSystemMetrics(SM_CXSCREEN);
        window->input.height = GetSystemMetrics(SM_CYSCREEN);
        window->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window->input.width,
            window->input.height,
            NULL,
            NULL,
            (HINSTANCE)platform,
            window
        );
    }
    if (window->hwnd == NULL)
        hg_error("Win32 window creation failed\n");

    ShowWindow(window->hwnd, SW_SHOW);
    return window;
}

void hg_window_destroy(const HgPlatform *platform, HgWindow *window) {
    (void)platform;
    if (window != NULL)
        DestroyWindow(window->hwnd);
}

void hg_window_set_icon(const HgPlatform *platform, HgWindow *window, u32 *icon_data, u32 width, u32 height);

bool hg_window_get_fullscreen(const HgPlatform *platform, HgWindow *window);

void hg_window_set_fullscreen(const HgPlatform *platform, HgWindow *window, bool fullscreen);

void hg_window_set_cursor(const HgPlatform *platform, HgWindow *window, HgCursor cursor);

void hg_window_set_cursor_image(const HgPlatform *platform, HgWindow *window, u32 *data, u32 width, u32 height);

VkSurfaceKHR hg_vk_create_surface(
    VkInstance instance,
    const HgPlatform *platform,
    const HgWindow *window
) {
    hg_assert(instance != VK_NULL_HANDLE);
    hg_assert(platform != NULL);
    hg_assert(window != NULL);

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (pfn_vkCreateWin32SurfaceKHR == NULL)
        hg_error("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = (HINSTANCE)platform,
        .hwnd = window->hwnd,
    };
    hg_vk_check(pfn_vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface));

    hg_assert(surface != NULL);
    return surface;
}

void hg_window_process_events(const HgPlatform *platform, HgWindow **windows, u32 window_count) {
    (void)platform;

    for (usize i = 0; i < window_count; ++i) {
        memset(windows[i]->input.keys_pressed, 0, sizeof(windows[i]->input.keys_pressed));
        memset(windows[i]->input.keys_released, 0, sizeof(windows[i]->input.keys_released));
        windows[i]->input.was_resized = false;

        u32 old_window_width = windows[i]->input.width;
        u32 old_window_height = windows[i]->input.height;
        f64 old_mouse_pos_x = windows[i]->input.mouse_pos_x;
        f64 old_mouse_pos_y = windows[i]->input.mouse_pos_y;

        MSG msg;
        while (PeekMessageA(&msg, windows[i]->hwnd, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (windows[i]->input.width != old_window_width || windows[i]->input.height != old_window_height) {
            windows[i]->input.was_resized = true;
        }

        windows[i]->input.mouse_delta_x = windows[i]->input.mouse_pos_x - old_mouse_pos_x;
        windows[i]->input.mouse_delta_y = windows[i]->input.mouse_pos_y - old_mouse_pos_y;

        if (windows[i]->input.keys_down[HG_KEY_LSHIFT] && windows[i]->input.keys_down[HG_KEY_RSHIFT]) {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift) {
                windows[i]->input.keys_released[HG_KEY_LSHIFT] = true;
                windows[i]->input.keys_down[HG_KEY_LSHIFT] = false;
            }
            if (!rshift) {
                windows[i]->input.keys_released[HG_KEY_RSHIFT] = true;
                windows[i]->input.keys_down[HG_KEY_RSHIFT] = false;
            }
        }
    }
}

#else

#error "unsupported platform"

#endif

bool hg_window_was_closed(const HgWindow *window) {
    hg_assert(window != NULL);
    return window->input.was_closed;
}

bool hg_window_was_resized(const HgWindow *window) {
    hg_assert(window != NULL);
    return window->input.was_resized;
}

void hg_window_get_size(const HgWindow *window, u32 *width, u32 *height) {
    hg_assert(window != NULL);
    *width = window->input.width;
    *height = window->input.height;
}

void hg_window_get_mouse_pos(const HgWindow *window, f64 *x, f64 *y) {
    hg_assert(window != NULL);
    hg_assert(x != NULL);
    hg_assert(y != NULL);
    *x = window->input.mouse_pos_x;
    *y = window->input.mouse_pos_y;
}

void hg_window_get_mouse_delta(const HgWindow *window, f64 *x, f64 *y) {
    hg_assert(window != NULL);
    hg_assert(x != NULL);
    hg_assert(y != NULL);
    *x = window->input.mouse_delta_x;
    *y = window->input.mouse_delta_y;
}

bool hg_window_is_key_down(const HgWindow *window, HgKey key) {
    hg_assert(window != NULL);
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_down[key];
}

bool hg_window_was_key_pressed(const HgWindow *window, HgKey key) {
    hg_assert(window != NULL);
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_pressed[key];
}

bool hg_window_was_key_released(const HgWindow *window, HgKey key) {
    hg_assert(window != NULL);
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_released[key];
}

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name;

typedef struct hgVulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr)
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr)

    HG_MAKE_VULKAN_FUNC(vkCreateInstance)
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance)

    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT)
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT)

    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices)
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties)
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)

    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR)
    HG_MAKE_VULKAN_FUNC(vkCreateDevice)

    HG_MAKE_VULKAN_FUNC(vkDestroyDevice)
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle)

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR)
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR)
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR)
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR)
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR)
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR)

    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore)
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore)
    HG_MAKE_VULKAN_FUNC(vkCreateFence)
    HG_MAKE_VULKAN_FUNC(vkDestroyFence)
    HG_MAKE_VULKAN_FUNC(vkResetFences)
    HG_MAKE_VULKAN_FUNC(vkWaitForFences)

    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue)
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle)
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit)
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR)

    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool)
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool)
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers)
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers)

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool)
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool)
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool)
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets)
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets)

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout)
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout)
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout)
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout)
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule)
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule)
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines)
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines)
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline)

    HG_MAKE_VULKAN_FUNC(vkCreateBuffer)
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer)
    HG_MAKE_VULKAN_FUNC(vkCreateImage)
    HG_MAKE_VULKAN_FUNC(vkDestroyImage)
    HG_MAKE_VULKAN_FUNC(vkCreateImageView)
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView)
    HG_MAKE_VULKAN_FUNC(vkCreateSampler)
    HG_MAKE_VULKAN_FUNC(vkDestroySampler)

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties)
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements)
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements)

    HG_MAKE_VULKAN_FUNC(vkAllocateMemory)
    HG_MAKE_VULKAN_FUNC(vkFreeMemory)
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory)
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory)
    HG_MAKE_VULKAN_FUNC(vkMapMemory)
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory)
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges)
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges)

    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer)
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer)
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer)

    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer)
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage)
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage)
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage)
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer)
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2)

    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering)
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering)
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport)
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor)
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline)
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets)
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants)
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers)
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer)
    HG_MAKE_VULKAN_FUNC(vkCmdDraw)
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed)
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch)
} hgVulkanFuncs;

#undef HG_MAKE_VULKAN_FUNC

static void *hg_libvulkan = NULL;
static hgVulkanFuncs hg_vk_pfn = {0};

#define HG_LOAD_VULKAN_FUNC(name) \
    hg_vk_pfn. name = (PFN_##name)hg_vk_pfn.vkGetInstanceProcAddr(NULL, #name); \
    if (hg_vk_pfn. name == NULL) { \
        hg_error("Could not load " #name "\n"); \
    }

void hg_vk_load(void) {

#if defined(__unix__)

    hg_libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (hg_libvulkan == NULL)
        hg_error("Could not load vulkan dynamic lib: %s\n", dlerror());

    *(void **)&hg_vk_pfn.vkGetInstanceProcAddr = dlsym(hg_libvulkan, "vkGetInstanceProcAddr");
    if (hg_vk_pfn.vkGetInstanceProcAddr == NULL)
        hg_error("Could not load vkGetInstanceProcAddr: %s\n", dlerror());

#elif defined(_WIN32)

    hg_libvulkan = (void *)LoadLibraryA("vulkan-1.dll");
    if (hg_libvulkan == NULL)
        hg_error("Could not load vulkan dynamic lib\n");

    hg_vk_pfn.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress(hg_libvulkan, "vkGetInstanceProcAddr");
    if (hg_vk_pfn.vkGetInstanceProcAddr == NULL)
        hg_error("Could not load vkGetInstanceProcAddr\n");

#else

#error "unsupported platform"

#endif

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

#undef HG_LOAD_VULKAN_FUNC

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    hg_vk_pfn. name = (PFN_##name)hg_vk_pfn.vkGetInstanceProcAddr(instance, #name); \
    if (hg_vk_pfn. name == NULL) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_instance(VkInstance instance) {
    hg_assert(instance != VK_NULL_HANDLE);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyInstance);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDebugUtilsMessengerEXT);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyDebugUtilsMessengerEXT);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroySurfaceKHR)
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDevice)
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    hg_vk_pfn. name = (PFN_##name)hg_vk_pfn.vkGetDeviceProcAddr(device, #name); \
    if (hg_vk_pfn. name == NULL) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_device(VkDevice device) {
    hg_assert(device != VK_NULL_HANDLE);

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
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory);
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

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    return hg_vk_pfn.vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    return hg_vk_pfn.vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    return hg_vk_pfn.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyInstance(instance, pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) {
    return hg_vk_pfn.vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pCount, VkPhysicalDevice *pDevices) {
    return hg_vk_pfn.vkEnumeratePhysicalDevices(instance, pCount, pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProps) {
    return hg_vk_pfn.vkEnumerateDeviceExtensionProperties(device, pLayerName, pCount, pProps);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, uint32_t *pCount, VkQueueFamilyProperties *pProps) {
    hg_vk_pfn.vkGetPhysicalDeviceQueueFamilyProperties(device, pCount, pProps);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult vkCreateDevice(VkPhysicalDevice device, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    return hg_vk_pfn.vkCreateDevice(device, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyDevice(device, pAllocator);
}

VkResult vkDeviceWaitIdle(VkDevice device) {
    return hg_vk_pfn.vkDeviceWaitIdle(device);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkSurfaceFormatKHR *pFormats) {
    return hg_vk_pfn.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, pCount, pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkPresentModeKHR *pModes) {
    return hg_vk_pfn.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, pCount, pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pCaps) {
    return hg_vk_pfn.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, pCaps);
}

VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    return hg_vk_pfn.vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount, VkImage *pImages) {
    return hg_vk_pfn.vkGetSwapchainImagesKHR(device, swapchain, pCount, pImages);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t *pIndex) {
    return hg_vk_pfn.vkAcquireNextImageKHR(device, swapchain, timeout, sem, fence, pIndex);
}

VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    return hg_vk_pfn.vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroySemaphore(device, sem, pAllocator);
}

VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    return hg_vk_pfn.vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(VkDevice device, uint32_t count, const VkFence *pFences) {
    return hg_vk_pfn.vkResetFences(device, count, pFences);
}

VkResult vkWaitForFences(VkDevice device, uint32_t count, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
    return hg_vk_pfn.vkWaitForFences(device, count, pFences, waitAll, timeout);
}

void vkGetDeviceQueue(VkDevice device, uint32_t family, uint32_t index, VkQueue *pQueue) {
    hg_vk_pfn.vkGetDeviceQueue(device, family, index, pQueue);
}

VkResult vkQueueWaitIdle(VkQueue queue) {
    return hg_vk_pfn.vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo *pSubmits, VkFence fence) {
    return hg_vk_pfn.vkQueueSubmit(queue, count, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pInfo) {
    return hg_vk_pfn.vkQueuePresentKHR(queue, pInfo);
}

VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pPool) {
    return hg_vk_pfn.vkCreateCommandPool(device, pCreateInfo, pAllocator, pPool);
}

void vkDestroyCommandPool(VkDevice device, VkCommandPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyCommandPool(device, pool, pAllocator);
}

VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pInfo, VkCommandBuffer *pBufs) {
    return hg_vk_pfn.vkAllocateCommandBuffers(device, pInfo, pBufs);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer *pBufs) {
    hg_vk_pfn.vkFreeCommandBuffers(device, pool, count, pBufs);
}

VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pPool) {
    return hg_vk_pfn.vkCreateDescriptorPool(device, pInfo, pAllocator, pPool);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyDescriptorPool(device, pool, pAllocator);
}

VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool pool, uint32_t flags) {
    return hg_vk_pfn.vkResetDescriptorPool(device, pool, flags);
}

VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pInfo, VkDescriptorSet *pSets) {
    return hg_vk_pfn.vkAllocateDescriptorSets(device, pInfo, pSets);
}

void vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet *pWrites, uint32_t copyCount, const VkCopyDescriptorSet *pCopies) {
    hg_vk_pfn.vkUpdateDescriptorSets(device, writeCount, pWrites, copyCount, pCopies);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pLayout) {
    return hg_vk_pfn.vkCreateDescriptorSetLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyDescriptorSetLayout(device, layout, pAllocator);
}

VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pLayout) {
    return hg_vk_pfn.vkCreatePipelineLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyPipelineLayout(device, layout, pAllocator);
}

VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pModule) {
    return hg_vk_pfn.vkCreateShaderModule(device, pInfo, pAllocator, pModule);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyShaderModule(device, module, pAllocator);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_vk_pfn.vkCreateGraphicsPipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_vk_pfn.vkCreateComputePipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuf) {
    return hg_vk_pfn.vkCreateBuffer(device, pInfo, pAllocator, pBuf);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buf, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyBuffer(device, buf, pAllocator);
}

VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    return hg_vk_pfn.vkCreateImage(device, pInfo, pAllocator, pImage);
}

void vkDestroyImage(VkDevice device, VkImage img, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyImage(device, img, pAllocator);
}

VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    return hg_vk_pfn.vkCreateImageView(device, pInfo, pAllocator, pView);
}

void vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroyImageView(device, view, pAllocator);
}

VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    return hg_vk_pfn.vkCreateSampler(device, pInfo, pAllocator, pSampler);
}

void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkDestroySampler(device, sampler, pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice device, VkPhysicalDeviceMemoryProperties *pProps) {
    hg_vk_pfn.vkGetPhysicalDeviceMemoryProperties(device, pProps);
}

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pReq) {
    hg_vk_pfn.vkGetBufferMemoryRequirements(device, buffer, pReq);
}

void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pReq) {
    hg_vk_pfn.vkGetImageMemoryRequirements(device, image, pReq);
}

VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMem) {
    return hg_vk_pfn.vkAllocateMemory(device, pInfo, pAllocator, pMem);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    hg_vk_pfn.vkFreeMemory(device, mem, pAllocator);
}

VkResult vkBindBufferMemory(VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_vk_pfn.vkBindBufferMemory(device, buf, mem, offset);
}

VkResult vkBindImageMemory(VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_vk_pfn.vkBindImageMemory(device, img, mem, offset);
}

VkResult vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
    return hg_vk_pfn.vkMapMemory(device, mem, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory mem) {
    hg_vk_pfn.vkUnmapMemory(device, mem);
}

VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_vk_pfn.vkFlushMappedMemoryRanges(device, count, pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_vk_pfn.vkInvalidateMappedMemoryRanges(device, count, pRanges);
}

VkResult vkBeginCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferBeginInfo *pInfo) {
    return hg_vk_pfn.vkBeginCommandBuffer(cmd, pInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer cmd) {
    return hg_vk_pfn.vkEndCommandBuffer(cmd);
}

VkResult vkResetCommandBuffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags) {
    return hg_vk_pfn.vkResetCommandBuffer(cmd, flags);
}

void vkCmdCopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy *pRegions) {
    hg_vk_pfn.vkCmdCopyBuffer(cmd, src, dst, count, pRegions);
}

void vkCmdCopyImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy *pRegions) {
    hg_vk_pfn.vkCmdCopyImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions);
}

void vkCmdBlitImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit *pRegions, VkFilter filter) {
    hg_vk_pfn.vkCmdBlitImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter);
}

void vkCmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_vk_pfn.vkCmdCopyBufferToImage(cmd, src, dst, dstLayout, count, pRegions);
}

void vkCmdCopyImageToBuffer(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_vk_pfn.vkCmdCopyImageToBuffer(cmd, src, srcLayout, dst, count, pRegions);
}

void vkCmdPipelineBarrier2(VkCommandBuffer cmd, const VkDependencyInfo *pInfo) {
    hg_vk_pfn.vkCmdPipelineBarrier2(cmd, pInfo);
}

void vkCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo *pInfo) {
    hg_vk_pfn.vkCmdBeginRendering(cmd, pInfo);
}

void vkCmdEndRendering(VkCommandBuffer cmd) {
    hg_vk_pfn.vkCmdEndRendering(cmd);
}

void vkCmdSetViewport(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport *pViewports) {
    hg_vk_pfn.vkCmdSetViewport(cmd, first, count, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D *pScissors) {
    hg_vk_pfn.vkCmdSetScissor(cmd, first, count, pScissors);
}

void vkCmdBindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    hg_vk_pfn.vkCmdBindPipeline(cmd, bindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet *pSets, uint32_t dynCount, const uint32_t *pDyn) {
    hg_vk_pfn.vkCmdBindDescriptorSets(cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn);
}

void vkCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *pData) {
    hg_vk_pfn.vkCmdPushConstants(cmd, layout, stages, offset, size, pData);
}

void vkCmdBindVertexBuffers(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer *pBufs, const VkDeviceSize *pOffsets) {
    hg_vk_pfn.vkCmdBindVertexBuffers(cmd, first, count, pBufs, pOffsets);
}

void vkCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type) {
    hg_vk_pfn.vkCmdBindIndexBuffer(cmd, buf, offset, type);
}

void vkCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    hg_vk_pfn.vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    hg_vk_pfn.vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z) {
    hg_vk_pfn.vkCmdDispatch(cmd, x, y, z);
}

