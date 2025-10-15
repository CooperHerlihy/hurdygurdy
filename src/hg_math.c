#include "hg_math.h"

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
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
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
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
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
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
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

void hg_svmul(u32 size, f32* dst, f32 scalar, f32* rhs) {
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar * rhs[i];
    }
}

HgVec2 hg_svmul2(f32 scalar, HgVec2 vec) {
    return (HgVec2){scalar * vec.x, scalar * vec.y};
}

HgVec3 hg_svmul3(f32 scalar, HgVec3 vec) {
    return (HgVec3){scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

HgVec4 hg_svmul4(f32 scalar, HgVec4 vec) {
    return (HgVec4){scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

void hg_vdiv(u32 size, f32* dst, f32* lhs, f32* rhs){
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] / rhs[i];
    }
}

HgVec2 hg_vdiv2(HgVec2 lhs, HgVec2 rhs){
    return (HgVec2){lhs.x / rhs.x, lhs.y / rhs.y};
}

HgVec3 hg_vdiv3(HgVec3 lhs, HgVec3 rhs){
    return (HgVec3){lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

HgVec4 hg_vdiv4(HgVec4 lhs, HgVec4 rhs){
    return (HgVec4){lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

void hg_svdiv(u32 size, f32* dst, f32 scalar, f32* rhs) {
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(rhs != NULL);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar / rhs[i];
    }
}

HgVec2 hg_svdiv2(f32 scalar, HgVec2 vec) {
    return (HgVec2){scalar / vec.x, scalar / vec.y};
}

HgVec3 hg_svdiv3(f32 scalar, HgVec3 vec) {
    return (HgVec3){scalar / vec.x, scalar / vec.y, scalar / vec.z};
}

HgVec4 hg_svdiv4(f32 scalar, HgVec4 vec) {
    return (HgVec4){scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w};
}

void hg_vdot(u32 size, f32* dst, f32* lhs, f32* rhs){
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    *dst = 0.0f;
    for (u32 i = 0; i < size; ++i) {
        *dst += lhs[i] * rhs[i];
    }
}

float hg_vdot2(HgVec2 lhs, HgVec2 rhs){
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

float hg_vdot3(HgVec3 lhs, HgVec3 rhs){
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

float hg_vdot4(HgVec4 lhs, HgVec4 rhs){
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

void hg_vlen(u32 size, f32* dst, f32* lhs){
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    hg_vdot(size, dst, lhs, lhs);
    *dst = sqrtf(*dst);
}

float hg_vlen2(HgVec2 vec){
    return sqrtf(hg_vdot2(vec, vec));
}

float hg_vlen3(HgVec3 vec){
    return sqrtf(hg_vdot3(vec, vec));
}

float hg_vlen4(HgVec4 vec){
    return sqrtf(hg_vdot4(vec, vec));
}

void hg_vnorm(u32 size, f32* dst, f32* vec){
    HG_ASSERT(size > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(vec != NULL);
    f32 len;
    hg_vlen(size, &len, vec);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hg_vnorm2(HgVec2 vec){
    f32 len = hg_vlen2(vec);
    return (HgVec2){vec.x / len, vec.y / len};
}

HgVec3 hg_vnorm3(HgVec3 vec){
    f32 len = hg_vlen3(vec);
    return (HgVec3){vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hg_vnorm4(HgVec4 vec){
    f32 len = hg_vlen4(vec);
    return (HgVec4){vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hg_vcross(f32* dst, f32* lhs, f32* rhs) {
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

HgVec3 hg_vcross3(HgVec3 lhs, HgVec3 rhs){
    return (HgVec3){lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x};
}

void hg_madd(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs){
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

HgMat2 hg_madd2(HgMat2 lhs, HgMat2 rhs){
    HgMat2 result;
    hg_madd(2, 2, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat3 hg_madd3(HgMat3 lhs, HgMat3 rhs){
    HgMat3 result;
    hg_madd(3, 3, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat4 hg_madd4(HgMat4 lhs, HgMat4 rhs){
    HgMat4 result;
    hg_madd(4, 4, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

void hg_msub(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs){
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

HgMat2 hg_msub2(HgMat2 lhs, HgMat2 rhs){
    HgMat2 result;
    hg_msub(2, 2, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat3 hg_msub3(HgMat3 lhs, HgMat3 rhs){
    HgMat3 result;
    hg_msub(3, 3, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgMat4 hg_msub4(HgMat4 lhs, HgMat4 rhs){
    HgMat4 result;
    hg_msub(4, 4, (f32*)&result, (f32*)&lhs, (f32*)&rhs);
    return result;
}

void hg_mmul(f32* dst, u32 wl, u32 hl, f32* lhs, u32 wr, u32 hr, f32* rhs) {
    HG_ASSERT(dst != NULL);
    HG_ASSERT(lhs != NULL);
    HG_ASSERT(rhs != NULL);
    HG_ASSERT(wl > 0);
    HG_ASSERT(hl > 0);
    HG_ASSERT(wr > 0);
    HG_ASSERT(hr > 0);
    HG_ASSERT(wr == hl);
    for (u32 i = 0; i < wl; ++i) {
        for (u32 j = 0; j < wr; ++j) {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k) {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

HgMat2 hg_mmul2(HgMat2 lhs, HgMat2 rhs){
    HgMat2 result;
    hg_mmul((f32*)&result, 2, 2, (f32*)&lhs, 2, 2, (f32*)&rhs);
    return result;
}

HgMat3 hg_mmul3(HgMat3 lhs, HgMat3 rhs){
    HgMat3 result;
    hg_mmul((f32*)&result, 3, 3, (f32*)&lhs, 3, 3, (f32*)&rhs);
    return result;
}

HgMat4 hg_mmul4(HgMat4 lhs, HgMat4 rhs){
    HgMat4 result;
    hg_mmul((f32*)&result, 4, 4, (f32*)&lhs, 4, 4, (f32*)&rhs);
    return result;
}


void hg_mvmul(f32* dst, u32 width, u32 height, f32* mat, f32* vec) {
    HG_ASSERT(dst != NULL);
    HG_ASSERT(mat != NULL);
    HG_ASSERT(vec != NULL);
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);
    for (u32 i = 0; i < height; ++i) {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j) {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

HgVec2 hg_mvmul2(HgMat2 lhs, HgVec2 rhs) {
    HgVec2 result;
    hg_mvmul((f32*)&result, 2, 2, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgVec3 hg_mvmul3(HgMat3 lhs, HgVec3 rhs) {
    HgVec3 result;
    hg_mvmul((f32*)&result, 3, 3, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgVec4 hg_mvmul4(HgMat4 lhs, HgVec4 rhs) {
    HgVec4 result;
    hg_mvmul((f32*)&result, 4, 4, (f32*)&lhs, (f32*)&rhs);
    return result;
}

HgComplex hg_cadd(HgComplex lhs, HgComplex rhs){
    return (HgComplex){lhs.r + rhs.r, lhs.i + rhs.i};
}

HgComplex hg_csub(HgComplex lhs, HgComplex rhs){
    return (HgComplex){lhs.r - rhs.r, lhs.i - rhs.i};
}

HgComplex hg_cmul(HgComplex lhs, HgComplex rhs){
    return (HgComplex){lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

HgQuat hg_qadd(HgQuat lhs, HgQuat rhs){
    return (HgQuat){lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

HgQuat hg_qsub(HgQuat lhs, HgQuat rhs){
    return (HgQuat){lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

HgQuat hg_qmul(HgQuat lhs, HgQuat rhs){
    return (HgQuat){
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

HgQuat hg_qconj(HgQuat quat) {
    return (HgQuat){quat.r, -quat.i, -quat.j, -quat.k};
}

HgQuat hg_axis_angle(HgVec3 axis, f32 angle) {
    f32 half_angle = angle / 2.0f;
    f32 sin_half_angle = sinf(half_angle);
    return (HgQuat){
        cosf(half_angle),
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
    };
}

HgVec3 hg_rotate_vec3(HgQuat lhs, HgVec3 rhs) {
    HgQuat q = hg_qmul(lhs, hg_qmul((HgQuat){0.0f, rhs.x, rhs.y, rhs.z}, hg_qconj(lhs)));
    return (HgVec3){q.i, q.j, q.k};
}

HgMat3 hg_rotate_mat3(HgQuat lhs, HgMat3 rhs) {
    return (HgMat3){
        hg_rotate_vec3(lhs, rhs.x),
        hg_rotate_vec3(lhs, rhs.y),
        hg_rotate_vec3(lhs, rhs.z),
    };
}

HgVec3 hg_move_first_person(HgVec3 position, HgQuat rotation, HgVec3 direction, f32 distance) {
    HgVec3 d = hg_rotate_vec3(rotation, (HgVec3){direction.x, 0.0f, direction.z});
    d.y = direction.y;
    return hg_vadd3(position, hg_svmul3(distance, hg_vnorm3(d)));
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
    return hg_mmul4(m4, hg_mat2to4(m2));
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
    return hg_mmul4(m4, hg_mat3to4(m3));
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

HgMat4 hg_projection_matrix_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return (HgMat4){
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, -2.0f / (far - near), 0.0f},
        {(left + right) / (left - right), (bottom + top) / (bottom - top), near / (near - far), 1.0f},
    };
}

HgMat4 hg_projection_matrix_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    f32 scale = 1 / tanf(fov / 2);
    return (HgMat4){
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

