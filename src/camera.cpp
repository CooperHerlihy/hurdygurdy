#include "hg_render2d.hpp"
#include <cstdio>

namespace hg {

struct VPUniform {
    Mat4 proj = {};
    Mat4 view = {};
};

Camera Camera::create()
{
    Camera camera{};

    camera.vpBuffer = GpuBuffer::create(
        sizeof(VPUniform), GpuBufferUsage_uniformBuffer, GpuMemoryUsage_frequentUpdate);

    camera.rotation = Quat{1.0f};
    camera.position = Vec3{0.0f};

    return camera;
}

void Camera::setPerspective(f32 aspect, f32 fov, f32 near, f32 far)
{
    projection = CameraPerspective{aspect, fov, near, far};
}

void Camera::setOrthographic(f32 width, f32 height, f32 actualAspect)
{
    projection = CameraOrthographic{0, width, 0, height, 0, 1};

    if (actualAspect != 0.0)
    {
        CameraOrthographic& ortho = projection.get<CameraOrthographic>();
        if (actualAspect > static_cast<f32>(width) / static_cast<f32>(height))
        {
            f32 margin = actualAspect - static_cast<f32>(width) / static_cast<f32>(height);
            ortho.left -= margin * width / 2.0f;
            ortho.right += margin * width / 2.0f;
        }
        else
        {
            f32 margin = 1.0f / actualAspect - static_cast<f32>(height) / static_cast<f32>(width);
            ortho.top -= margin * height / 2.0f;
            ortho.bottom += margin * height / 2.0f;
        }
    }
}

void Camera::update()
{
    VPUniform vp{};
    vp.view = matView(position, Vec3{1.0f}, rotation);
    projection.match(
        [&](CameraPerspective& persp)
        {
            vp.proj = matPerspective(
                persp.fov,
                persp.aspect,
                persp.near,
                persp.far);
        },
        [&](CameraOrthographic& ortho)
        {
            vp.proj = matOrthographic(
                ortho.left,
                ortho.right,
                ortho.top,
                ortho.bottom,
                ortho.near,
                ortho.far);
        });
    vpBuffer.write(&vp, 0, sizeof(vp));
}

template<>
void serialize(Serializer* s, CameraPerspective* camera)
{
    serializeObject(s,
        &camera->aspect,
        &camera->fov,
        &camera->near,
        &camera->far);
}

template<>
void serialize(Serializer* s, CameraOrthographic* camera)
{
    serializeObject(s,
        &camera->left,
        &camera->right,
        &camera->top,
        &camera->bottom,
        &camera->near,
        &camera->far);
}

template<>
void serialize(Serializer* s, Camera* camera)
{
    serializeBegin(s);
    serialize(s, &camera->rotation);
    serialize(s, &camera->position);
    serialize(s, &camera->projection);
    serializeEnd(s);
}

} // namespace hg
