#ifndef HG_MATH_H
#define HG_MATH_H

#include "hg_utils.h"

#define HG_PI      3.1415926535897932
#define HG_TAU     6.2831853071795864
#define HG_E       2.7182818284590452
#define HG_ROOT2   1.4142135623730951
#define HG_ROOT3   1.7320508075688772
#define HG_EPSILON 1.0e-6

/**
 * A 2D vector
 */
typedef struct HgVec2 {
    f32 x;
    f32 y;
} HgVec2;

/**
 * A 3D vector
 */
typedef struct HgVec3 {
    f32 x;
    f32 y;
    f32 z;
} HgVec3;

/**
 * A 4D vector
 */
typedef struct HgVec4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} HgVec4;

/**
 * A 2x2 matrix
 */
typedef struct HgMat2 {
    HgVec2 x;
    HgVec2 y;
} HgMat2;

/**
 * A 3x3 matrix
 */
typedef struct HgMat3 {
    HgVec3 x;
    HgVec3 y;
    HgVec3 z;
} HgMat3;

/**
 * A 4x4 matrix
 */
typedef struct HgMat4 {
    HgVec4 x;
    HgVec4 y;
    HgVec4 z;
    HgVec4 w;
} HgMat4;

/**
 * A complex number
 */
typedef struct HgComplex {
    f32 r;
    f32 i;
} HgComplex;

/**
 * A quaternion
 *
 * r is the real part
 */
typedef struct HgQuat {
    f32 r;
    f32 i;
    f32 j;
    f32 k;
} HgQuat;

/**
 * Creates a 2D vector with the given scalar
 *
 * \param scalar The scalar to create the vector with
 * \return The created vector
 */
HgVec2 hg_svec2(f32 scalar);

/**
 * Creates a 3D vector with the given scalar
 *
 * \param scalar The scalar to create the vector with
 * \return The created vector
 */
HgVec3 hg_svec3(f32 scalar);

/**
 * Creates a 4D vector with the given scalar
 *
 * \param scalar The scalar to create the vector with
 * \return The created vector
 */
HgVec4 hg_svec4(f32 scalar);

/**
 * Creates a 2x2 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * \param scalar The scalar to create the matrix with
 * \return The created matrix
 */
HgMat2 hg_smat2(f32 scalar);

/**
 * Creates a 3x3 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * \param scalar The scalar to create the matrix with
 * \return The created matrix
 */
HgMat3 hg_smat3(f32 scalar);

/**
 * Creates a 4x4 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * \param scalar The scalar to create the matrix with
 * \return The created matrix
 */
HgMat4 hg_smat4(f32 scalar);

/**
 * Creates a 3D vector from a 2D vector and 0 for the z component
 *
 * \param lhs The vector to convert
 * \return The converted vector
 */
HgVec3 hg_vec2to3(HgVec2 lhs);

/**
 * Creates a 4D vector from a 2D vector and 0 for the z and w components
 *
 * \param lhs The vector to convert
 * \return The converted vector
 */
HgVec4 hg_vec2to4(HgVec2 lhs);

/**
 * Creates a 3D vector from a 3D vector and 0 for the w component
 *
 * \param lhs The vector to convert
 * \return The converted vector
 */
HgVec4 hg_vec3to4(HgVec3 lhs);

/**
 * Scales a 2x2 matrix to a 3x3 matrix with 1 on the diagonal
 *
 * \param lhs The matrix to convert
 * \return The converted matrix
 */
HgMat3 hg_mat2to3(HgMat2 lhs);

/**
 * Scales a 2x2 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * \param lhs The matrix to convert
 * \return The converted matrix
 */
HgMat4 hg_mat2to4(HgMat2 lhs);

/**
 * Scales a 3x3 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * \param lhs The matrix to convert
 * \return The converted matrix
 */
HgMat4 hg_mat3to4(HgMat3 lhs);

/**
 * Adds two arbitrary size vectors
 *
 * \param size The size of the vectors
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vadd(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Adds two 2D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The added vector
 */
HgVec2 hg_vadd2(HgVec2 lhs, HgVec2 rhs);

/**
 * Adds two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The added vector
 */
HgVec3 hg_vadd3(HgVec3 lhs, HgVec3 rhs);

/**
 * Adds two 4D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The added vector
 */
HgVec4 hg_vadd4(HgVec4 lhs, HgVec4 rhs);

/**
 * Subtracts two arbitrary size vectors
 *
 * \param size The size of the vectors
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vsub(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Subtracts two 2D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The subtracted vector
 */
HgVec2 hg_vsub2(HgVec2 lhs, HgVec2 rhs);

/**
 * Subtracts two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The subtracted vector
 */
HgVec3 hg_vsub3(HgVec3 lhs, HgVec3 rhs);

/**
 * Subtracts two 4D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The subtracted vector
 */
HgVec4 hg_vsub4(HgVec4 lhs, HgVec4 rhs);

/**
 * Multiplies pairwise two arbitrary size vectors
 *
 * \param size The size of the vectors
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vmul(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Multiplies pairwise two 2D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The multiplied vector
 */
HgVec2 hg_vmul2(HgVec2 lhs, HgVec2 rhs);

/**
 * Multiplies pairwise two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The multiplied vector
 */
HgVec3 hg_vmul3(HgVec3 lhs, HgVec3 rhs);

/**
 * Multiplies pairwise two 4D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The multiplied vector
 */
HgVec4 hg_vmul4(HgVec4 lhs, HgVec4 rhs);

/**
 * Multiplies a scalar and a vector
 *
 * \param size The size of the vector
 * \param dst The destination vector
 * \param scalar The scalar to multiply with
 * \param vec The vector to multiply with
 */
void hg_svmul(u32 size, f32* dst, f32 scalar, f32* vec);

/**
 * Multiplies a scalar and a 2D vector
 *
 * \param scalar The scalar to multiply with
 * \param vec The vector to multiply with
 * \return The multiplied vector
 */
HgVec2 hg_svmul2(f32 scalar, HgVec2 vec);

/**
 * Multiplies a scalar and a 3D vector
 *
 * \param scalar The scalar to multiply with
 * \param vec The vector to multiply with
 * \return The multiplied vector
 */
HgVec3 hg_svmul3(f32 scalar, HgVec3 vec);

/**
 * Multiplies a scalar and a 4D vector
 *
 * \param scalar The scalar to multiply with
 * \param vec The vector to multiply with
 * \return The multiplied vector
 */
HgVec4 hg_svmul4(f32 scalar, HgVec4 vec);

/**
 * Divides pairwise two arbitrary size vectors
 *
 * \param size The size of the vectors
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vdiv(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Divides pairwise two 2D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The divided vector
 */
HgVec2 hg_vdiv2(HgVec2 lhs, HgVec2 rhs);

/**
 * Divides pairwise two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The divided vector
 */
HgVec3 hg_vdiv3(HgVec3 lhs, HgVec3 rhs);

/**
 * Divides pairwise two 4D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The divided vector
 */
HgVec4 hg_vdiv4(HgVec4 lhs, HgVec4 rhs);

/**
 * Divides a vector by a scalar
 *
 * \param size The size of the vector
 * \param dst The destination vector
 * \param scalar The scalar to divide by
 * \param vec The vector to divide
 */
void hg_svdiv(u32 size, f32* dst, f32 scalar, f32* vec);

/**
 * Divides a 2D vector by a scalar
 *
 * \param scalar The scalar to divide by
 * \param vec The vector to divide
 * \return The divided vector
 */
HgVec2 hg_svdiv2(f32 scalar, HgVec2 vec);

/**
 * Divides a 3D vector by a scalar
 *
 * \param scalar The scalar to divide by
 * \param vec The vector to divide
 * \return The divided vector
 */
HgVec3 hg_svdiv3(f32 scalar, HgVec3 vec);

/**
 * Divides a 4D vector by a scalar
 *
 * \param scalar The scalar to divide by
 * \param vec The vector to divide
 * \return The divided vector
 */
HgVec4 hg_svdiv4(f32 scalar, HgVec4 vec);

/**
 * Computes the dot product of two arbitrary size vectors
 *
 * \param size The size of the vectors
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vdot(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Computes the dot product of two 2D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The dot product
 */
float hg_vdot2(HgVec2 lhs, HgVec2 rhs);

/**
 * Computes the dot product of two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The dot product
 */
float hg_vdot3(HgVec3 lhs, HgVec3 rhs);

/**
 * Computes the dot product of two 4D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The dot product
 */
float hg_vdot4(HgVec4 lhs, HgVec4 rhs);

/**
 * Computes the length of a vector
 *
 * \param size The size of the vector
 * \param dst The destination vector
 * \param vec The vector to compute the length of
 */
void hg_vlen(u32 size, f32* dst, f32* lhs);

/**
 * Computes the length of a 2D vector
 *
 * \param lhs The vector to compute the length of
 * \return The length of the vector
 */
float hg_vlen2(HgVec2 lhs);

/**
 * Computes the length of a 3D vector
 *
 * \param lhs The vector to compute the length of
 * \return The length of the vector
 */
float hg_vlen3(HgVec3 lhs);

/**
 * Computes the length of a 4D vector
 *
 * \param lhs The vector to compute the length of
 * \return The length of the vector
 */
float hg_vlen4(HgVec4 lhs);

/**
 * Normalizes a vector
 *
 * \param size The size of the vector
 * \param dst The destination vector
 * \param vec The vector to normalize
 */
void hg_vnorm(u32 size, f32* dst, f32* vec);

/**
 * Normalizes a 2D vector
 *
 * \param vec The vector to normalize
 * \return The normalized vector
 */
HgVec2 hg_vnorm2(HgVec2 vec);

/**
 * Normalizes a 3D vector
 *
 * \param vec The vector to normalize
 * \return The normalized vector
 */
HgVec3 hg_vnorm3(HgVec3 vec);

/**
 * Normalizes a 4D vector
 *
 * \param vec The vector to normalize
 * \return The normalized vector
 */
HgVec4 hg_vnorm4(HgVec4 vec);

/**
 * Computes the cross product of two 3D vectors
 *
 * \param dst The destination vector
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 */
void hg_vcross(f32* dst, f32* lhs, f32* rhs);

/**
 * Computes the cross product of two 3D vectors
 *
 * \param lhs The left-hand side vector
 * \param rhs The right-hand side vector
 * \return The cross product
 */
HgVec3 hg_vcross3(HgVec3 lhs, HgVec3 rhs);

/**
 * Adds two arbitrary size matrices
 *
 * \param width The width of the matrices
 * \param height The height of the matrices
 * \param dst The destination matrix
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 */
void hg_madd(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);

/**
 * Adds two 2x2 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The added matrix
 */
HgMat2 hg_madd2(HgMat2 lhs, HgMat2 rhs);

/**
 * Adds two 3x3 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The added matrix
 */
HgMat3 hg_madd3(HgMat3 lhs, HgMat3 rhs);

/**
 * Adds two 4x4 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The added matrix
 */
HgMat4 hg_madd4(HgMat4 lhs, HgMat4 rhs);

/**
 * Subtracts two arbitrary size matrices
 *
 * \param width The width of the matrices
 * \param height The height of the matrices
 * \param dst The destination matrix
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 */
void hg_msub(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);

/**
 * Subtracts two 2x2 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The subtracted matrix
 */
HgMat2 hg_msub2(HgMat2 lhs, HgMat2 rhs);

/**
 * Subtracts two 3x3 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The subtracted matrix
 */
HgMat3 hg_msub3(HgMat3 lhs, HgMat3 rhs);

/**
 * Subtracts two 4x4 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The subtracted matrix
 */
HgMat4 hg_msub4(HgMat4 lhs, HgMat4 rhs);

/**
 * Multiplies two arbitrary size matrices
 *
 * \param dst The destination matrix
 * \param wl The width of the left-hand side matrix
 * \param hl The height of the left-hand side matrix
 * \param lhs The left-hand side matrix
 * \param wr The width of the right-hand side matrix
 * \param hr The height of the right-hand side matrix
 * \param rhs The right-hand side matrix
 */
void hg_mmul(f32* dst, u32 wl, u32 hl, f32* lhs, u32 wr, u32 hr, f32* rhs);

/**
 * Multiplies two 2x2 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The multiplied matrix
 */
HgMat2 hg_mmul2(HgMat2 lhs, HgMat2 rhs);

/**
 * Multiplies two 3x3 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The multiplied matrix
 */
HgMat3 hg_mmul3(HgMat3 lhs, HgMat3 rhs);

/**
 * Multiplies two 4x4 matrices
 *
 * \param lhs The left-hand side matrix
 * \param rhs The right-hand side matrix
 * \return The multiplied matrix
 */
HgMat4 hg_mmul4(HgMat4 lhs, HgMat4 rhs);

/**
 * Multiplies a matrix and a vector
 *
 * \param dst The destination vector
 * \param width The width of the matrix
 * \param height The height of the matrix
 * \param mat The matrix to multiply with
 * \param vec The vector to multiply with
 */
void hg_mvmul(f32* dst, u32 width, u32 height, f32* mat, f32* vec);

/**
 * Multiplies a 2x2 matrix and a 2D vector
 *
 * \param lhs The matrix to multiply with
 * \param rhs The vector to multiply with
 * \return The multiplied vector
 */
HgVec2 hg_mvmul2(HgMat2 lhs, HgVec2 rhs);

/**
 * Multiplies a 3x3 matrix and a 3D vector
 *
 * \param lhs The matrix to multiply with
 * \param rhs The vector to multiply with
 * \return The multiplied vector
 */
HgVec3 hg_mvmul3(HgMat3 lhs, HgVec3 rhs);

/**
 * Multiplies a 4x4 matrix and a 4D vector
 *
 * \param lhs The matrix to multiply with
 * \param rhs The vector to multiply with
 * \return The multiplied vector
 */
HgVec4 hg_mvmul4(HgMat4 lhs, HgVec4 rhs);

/**
 * Adds two complex numbers
 *
 * \param lhs The left-hand side complex number
 * \param rhs The right-hand side complex number
 * \return The added complex number
 */
HgComplex hg_cadd(HgComplex lhs, HgComplex rhs);

/**
 * Subtracts two complex numbers
 *
 * \param lhs The left-hand side complex number
 * \param rhs The right-hand side complex number
 * \return The subtracted complex number
 */
HgComplex hg_csub(HgComplex lhs, HgComplex rhs);

/**
 * Multiplies two complex numbers
 *
 * \param lhs The left-hand side complex number
 * \param rhs The right-hand side complex number
 * \return The multiplied complex number
 */
HgComplex hg_cmul(HgComplex lhs, HgComplex rhs);

/**
 * Adds two quaternions
 *
 * \param lhs The left-hand side quaternion
 * \param rhs The right-hand side quaternion
 * \return The added quaternion
 */
HgQuat hg_qadd(HgQuat lhs, HgQuat rhs);

/**
 * Subtracts two quaternions
 *
 * \param lhs The left-hand side quaternion
 * \param rhs The right-hand side quaternion
 * \return The subtracted quaternion
 */
HgQuat hg_qsub(HgQuat lhs, HgQuat rhs);

/**
 * Multiplies two quaternions
 *
 * \param lhs The left-hand side quaternion
 * \param rhs The right-hand side quaternion
 * \return The multiplied quaternion
 */
HgQuat hg_qmul(HgQuat lhs, HgQuat rhs);

/**
 * Computes the conjugate of a quaternion
 *
 * \param quat The quaternion to compute the conjugate of
 * \return The conjugate of the quaternion
 */
HgQuat hg_qconj(HgQuat quat);

/**
 * Creates a rotation quaternion from an axis and angle
 *
 * \param axis The axis of the rotation
 * \param angle The angle of the rotation
 * \return The created quaternion
 */
HgQuat hg_axis_angle(HgVec3 axis, f32 angle);

/**
 * Rotates a 3D vector using a quaternion
 *
 * \param lhs The quaternion to rotate with
 * \param rhs The vector to rotate
 * \return The rotated vector
 */
HgVec3 hg_rotate_vec3(HgQuat lhs, HgVec3 rhs);

/**
 * Rotates a 3x3 matrix using a quaternion
 *
 * \param lhs The quaternion to rotate with
 * \param rhs The matrix to rotate
 * \return The rotated matrix
 */
HgMat3 hg_rotate_mat3(HgQuat lhs, HgMat3 rhs);

/**
 * Moves a camera position from a first person perspective
 *
 * \param position The position to move
 * \param rotation The rotation to move with
 * \param direction The direction to move in
 * \param distance The distance to move
 * \return The moved position
 */
HgVec3 hg_move_camera_first_person(HgVec3 position, HgQuat rotation, HgVec3 direction, f32 distance);

/**
 * Creates a model matrix for 2D graphics
 *
 * \param position The position of the model
 * \param scale The scale of the model
 * \param rotation The rotation of the model
 * \return The created matrix
 */
HgMat4 hg_model_matrix_2d(HgVec3 position, HgVec2 scale, f32 rotation);

/**
 * Creates a model matrix for 3D graphics
 *
 * \param position The position of the model
 * \param scale The scale of the model
 * \param rotation The rotation of the model
 * \return The created matrix
 */
HgMat4 hg_model_matrix_3d(HgVec3 position, HgVec3 scale, HgQuat rotation);

/**
 * Creates a view matrix
 *
 * \param position The position of the camera
 * \param zoom The zoom of the camera
 * \param rotation The rotation of the camera
 * \return The created matrix
 */
HgMat4 hg_view_matrix(HgVec3 position, f32 zoom, HgQuat rotation);

/**
 * Creates an orthographic projection matrix
 *
 * \param left The left-hand side of the view frustum
 * \param right The right-hand side of the view frustum
 * \param top The top of the view frustum
 * \param bottom The bottom of the view frustum
 * \param near The near plane of the view frustum
 * \param far The far plane of the view frustum
 * \return The created matrix
 */
HgMat4 hg_projection_matrix_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

/**
 * Creates a perspective projection matrix
 *
 * \param fov The field of view of the projection in radians
 * \param aspect The aspect ratio of the projection
 * \param near The near plane of the projection
 * \param far The far plane of the projection
 * \return The created matrix
 */
HgMat4 hg_projection_matrix_perspective(f32 fov, f32 aspect, f32 near, f32 far);

#endif // HG_MATH_H
