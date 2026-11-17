#include "hg/math.hpp"

namespace hg {

Mat4 matModel2D(Vec3 position, Vec2 scale, f32 rotation)
{
    Mat2 m2{scale.x(), 0.0f, 0.0f, scale.y()};
    f32 rotSin = std::sin(rotation);
    f32 rotCos = std::cos(rotation);
    Mat2 rot{rotCos, rotSin, -rotSin, rotCos};
    Mat4 m4 = Mat4{rot * m2};
    m4.w().x() = position.x();
    m4.w().y() = position.y();
    m4.w().z() = position.z();
    return m4;
}

Mat4 matModel3D(const Vec3& position, const Vec3& scale, const Quat& rotation)
{
    Mat3 m3{1.0f};
    m3.x().x() = scale.x();
    m3.y().y() = scale.y();
    m3.z().z() = scale.z();
    m3 = rotate(rotation, m3);
    Mat4 m4 = Mat4{m3};
    m4.w().x() = position.x();
    m4.w().y() = position.y();
    m4.w().z() = position.z();
    return m4;
}

Mat4 matView(const Vec3& position, const Vec3& zoom, const Quat& rotation)
{
    Mat4 rot{rotate(conjugate(rotation), Mat3{1.0f})};
    Mat4 pos{1.0f};
    pos.w() = Vec4{-position, 1.0f};
    Mat4 zoomM{1.0f};
    zoomM.x().x() = zoom.x();
    zoomM.y().y() = zoom.y();
    zoomM.z().z() = zoom.z();
    return zoomM * (rot * pos);
}

Mat4 matModelToView(const Mat4& model)
{
    if (Vec3{model.x()} == Vec3{0} || Vec3{model.y()} == Vec3{0} || Vec3{model.z()} == Vec3{0})
        return Mat4{Mat3{0}};

    Mat3 inv3 = transpose(Mat3{
        normalize(Vec3{model.x()}),
        normalize(Vec3{model.y()}),
        normalize(Vec3{model.z()}),
    });
    Mat4 inv4{inv3};
    inv4.w() = Vec4{inv3 * Vec3{model.w()} * -1, 1};
    return inv4;
}

Mat4 matOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return Mat4{
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f / (far - near), 0.0f,
        -(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f,
    };
}

Mat4 matPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    HG_ASSERT(near > 0.0f);
    HG_ASSERT(far > near);
    f32 scale = 1.0f / std::tan(fov * 0.5f);
    return Mat4{
        scale / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, -scale, 0.0f, 0.0f,
        0.0f, 0.0f, far / (far - near), 1.0f,
        0.0f, 0.0f, -(far * near) / (far - near), 0.0f,
    };
}

} // namespace hg
