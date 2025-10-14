#ifndef HG_MATH_H
#define HG_MATH_H

#include "hg_utils.h"

#define HG_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HG_MIN(a, b) ((a) < (b) ? (a) : (b))

#define HG_PI      3.1415926535897932
#define HG_TAU     6.2831853071795864
#define HG_E       2.7182818284590452
#define HG_ROOT2   1.4142135623730951
#define HG_ROOT3   1.7320508075688772
#define HG_EPSILON 1.0e-6

typedef struct HgVec2 {
    f32 x;
    f32 y;
} HgVec2;

typedef struct HgVec3 {
    f32 x;
    f32 y;
    f32 z;
} HgVec3;

typedef struct HgVec4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} HgVec4;

typedef struct HgMat2 {
    HgVec2 x;
    HgVec2 y;
} HgMat2;

typedef struct HgMat3 {
    HgVec3 x;
    HgVec3 y;
    HgVec3 z;
} HgMat3;

typedef struct HgMat4 {
    HgVec4 x;
    HgVec4 y;
    HgVec4 z;
    HgVec4 w;
} HgMat4;

typedef struct HgComplex {
    f32 r;
    f32 i;
} HgComplex;

typedef struct HgQuat {
    f32 r;
    f32 i;
    f32 j;
    f32 k;
} HgQuat;

HgVec2 hg_svec2(f32 scalar);
HgVec3 hg_svec3(f32 scalar);
HgVec4 hg_svec4(f32 scalar);
HgMat2 hg_smat2(f32 scalar);
HgMat3 hg_smat3(f32 scalar);
HgMat4 hg_smat4(f32 scalar);

HgVec3 hg_vec2to3(HgVec2 lhs);
HgVec4 hg_vec2to4(HgVec2 lhs);
HgVec4 hg_vec3to4(HgVec3 lhs);
HgMat3 hg_mat2to3(HgMat2 lhs);
HgMat4 hg_mat2to4(HgMat2 lhs);
HgMat4 hg_mat3to4(HgMat3 lhs);

void hg_vadd(u32 size, f32* dst, f32* lhs, f32* rhs);
HgVec2 hg_vadd2(HgVec2 lhs, HgVec2 rhs);
HgVec3 hg_vadd3(HgVec3 lhs, HgVec3 rhs);
HgVec4 hg_vadd4(HgVec4 lhs, HgVec4 rhs);

void hg_vsub(u32 size, f32* dst, f32* lhs, f32* rhs);
HgVec2 hg_vsub2(HgVec2 lhs, HgVec2 rhs);
HgVec3 hg_vsub3(HgVec3 lhs, HgVec3 rhs);
HgVec4 hg_vsub4(HgVec4 lhs, HgVec4 rhs);

void hg_vmul(u32 size, f32* dst, f32* lhs, f32* rhs);
HgVec2 hg_vmul2(HgVec2 lhs, HgVec2 rhs);
HgVec3 hg_vmul3(HgVec3 lhs, HgVec3 rhs);
HgVec4 hg_vmul4(HgVec4 lhs, HgVec4 rhs);

void hg_svmul(u32 size, f32* dst, f32 scalar, f32* rhs);
HgVec2 hg_svmul2(f32 scalar, HgVec2 vec);
HgVec3 hg_svmul3(f32 scalar, HgVec3 vec);
HgVec4 hg_svmul4(f32 scalar, HgVec4 vec);

void hg_vdiv(u32 size, f32* dst, f32* lhs, f32* rhs);
HgVec2 hg_vdiv2(HgVec2 lhs, HgVec2 rhs);
HgVec3 hg_vdiv3(HgVec3 lhs, HgVec3 rhs);
HgVec4 hg_vdiv4(HgVec4 lhs, HgVec4 rhs);

void hg_svdiv(u32 size, f32* dst, f32 scalar, f32* rhs);
HgVec2 hg_svdiv2(f32 scalar, HgVec2 vec);
HgVec3 hg_svdiv3(f32 scalar, HgVec3 vec);
HgVec4 hg_svdiv4(f32 scalar, HgVec4 vec);

void hg_vdot(u32 size, f32* dst, f32* lhs, f32* rhs);
float hg_vdot2(HgVec2 lhs, HgVec2 rhs);
float hg_vdot3(HgVec3 lhs, HgVec3 rhs);
float hg_vdot4(HgVec4 lhs, HgVec4 rhs);

void hg_vlen(u32 size, f32* dst, f32* lhs);
float hg_vlen2(HgVec2 lhs);
float hg_vlen3(HgVec3 lhs);
float hg_vlen4(HgVec4 lhs);

void hg_vnorm(u32 size, f32* dst, f32* vec);
HgVec2 hg_vnorm2(HgVec2 vec);
HgVec3 hg_vnorm3(HgVec3 vec);
HgVec4 hg_vnorm4(HgVec4 vec);

void hg_vcross(f32* dst, f32* lhs, f32* rhs);
HgVec3 hg_vcross3(HgVec3 lhs, HgVec3 rhs);

void hg_madd(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);
HgMat2 hg_madd2(HgMat2 lhs, HgMat2 rhs);
HgMat3 hg_madd3(HgMat3 lhs, HgMat3 rhs);
HgMat4 hg_madd4(HgMat4 lhs, HgMat4 rhs);

void hg_msub(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);
HgMat2 hg_msub2(HgMat2 lhs, HgMat2 rhs);
HgMat3 hg_msub3(HgMat3 lhs, HgMat3 rhs);
HgMat4 hg_msub4(HgMat4 lhs, HgMat4 rhs);

void hg_mmul(f32* dst, u32 wl, u32 hl, f32* lhs, u32 wr, u32 hr, f32* rhs);
HgMat2 hg_mmul2(HgMat2 lhs, HgMat2 rhs);
HgMat3 hg_mmul3(HgMat3 lhs, HgMat3 rhs);
HgMat4 hg_mmul4(HgMat4 lhs, HgMat4 rhs);

void hg_mvmul(f32* dst, u32 width, u32 height, f32* mat, f32* vec);
HgVec2 hg_mvmul2(HgMat2 lhs, HgVec2 rhs);
HgVec3 hg_mvmul3(HgMat3 lhs, HgVec3 rhs);
HgVec4 hg_mvmul4(HgMat4 lhs, HgVec4 rhs);

HgComplex hg_cadd(HgComplex lhs, HgComplex rhs);
HgComplex hg_csub(HgComplex lhs, HgComplex rhs);
HgComplex hg_cmul(HgComplex lhs, HgComplex rhs);

HgQuat hg_qadd(HgQuat lhs, HgQuat rhs);
HgQuat hg_qsub(HgQuat lhs, HgQuat rhs);
HgQuat hg_qmul(HgQuat lhs, HgQuat rhs);
HgQuat hg_qconj(HgQuat quat);

HgQuat hg_axis_angle(HgVec3 axis, f32 angle);
HgVec3 hg_rotate_vec3(HgQuat lhs, HgVec3 rhs);
HgMat3 hg_rotate_mat3(HgQuat lhs, HgMat3 rhs);

HgVec3 hg_move_first_person(HgVec3 position, HgQuat rotation, HgVec3 direction, f32 distance);

HgMat4 hg_model_matrix_2d(HgVec3 position, HgVec2 scale, f32 rotation);
HgMat4 hg_model_matrix_3d(HgVec3 position, HgVec3 scale, HgQuat rotation);
HgMat4 hg_view_matrix(HgVec3 position, f32 zoom, HgQuat rotation);
HgMat4 hg_projection_matrix_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);
HgMat4 hg_projection_matrix_perspective(f32 fov, f32 aspect, f32 near, f32 far);

#endif // HG_MATH_H
