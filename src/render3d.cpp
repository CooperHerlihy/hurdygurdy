#include "hg/render3d.hpp"

#include "hg/gpu.hpp"
#include "hg/memory.hpp"

namespace hg {

template<>
bool serialize(Serializer* s, MeshData* mesh)
{
    return serializeObject(s,
        &mesh->vertices,
        &mesh->indices);
}

Mesh uploadMeshToGpu(Gpu* gpu, const MeshData& data)
{
    Mesh mesh{};

    mesh.vertexCount = assertCast<u32>(data.vertices.size());
    mesh.vertexWidth = sizeof(MeshVertex);
    mesh.indexCount = assertCast<u32>(data.indices.size());

    mesh.vertexBuffer = gpu->createBuffer(mesh.vertexCount * mesh.vertexWidth,
        GpuBufferUsage::storageBuffer | GpuBufferUsage::transferDst);

    mesh.indexBuffer = gpu->createBuffer(mesh.indexCount * sizeof(u32),
        GpuBufferUsage::storageBuffer | GpuBufferUsage::transferDst);

    mesh.vertexBuffer->write(std::as_bytes(std::span{data.vertices}), 0);
    mesh.indexBuffer->write(std::as_bytes(std::span{data.indices}), 0);

    return mesh;
}

MeshData downloadMeshFromGpu(const Mesh& mesh)
{
    MeshData data{};

    u64 vCount = mesh.vertexBuffer->size() / sizeof(MeshVertex);
    data.vertices.resize(vCount);

    u64 iCount = mesh.indexBuffer->size() / sizeof(u32);
    data.indices.resize(iCount);

    mesh.vertexBuffer->read(std::as_writable_bytes(std::span{data.vertices}), 0);
    mesh.indexBuffer->read(std::as_writable_bytes(std::span{data.indices}), 0);

    return data;
}

std::optional<Mesh> meshLoadToGpu(Gpu* gpu, std::string_view path)
{
    std::optional<MeshData> mesh = serialLoad<MeshData>(path);
    if (!mesh.has_value())
        return std::nullopt;
    return uploadMeshToGpu(gpu, *mesh);
}

template<>
bool serialize(Serializer* s, Model3D* material)
{
    return serializeObject(s,
        &material->mesh,
        &material->colorMap,
        &material->normalMap,
        &material->diffuse,
        &material->shininess);
}

#include "shaders/render3d.vert.spv.h"
#include "shaders/render3d.frag.spv.h"

namespace {

struct ViewProj {
    Mat4 proj{};
    Mat4 view{};
};

struct Push {
    Mat4 model;
    u32 dirLights;
    u32 dirLightCount;
    u32 pointLights;
    u32 pointLightCount;
    u32 colorMap;
    u32 normalMap;
    f32 diffuse;
    f32 shininess;
    u32 verts;
    u32 inds;
    u32 vp;
};

} // namespace

Renderer3D::Renderer3D(Gpu* gpu, Format colorFormat, Format depthFormat)
    : gpu{gpu}
{
    GpuGraphicsPipelineCreateInfo conf{};
    conf.vertexShader = std::as_bytes(std::span{render3d_vert_spv});
    conf.fragmentShader = std::as_bytes(std::span{render3d_frag_spv});
    conf.pushConstantSize = sizeof(Push);
    Format colorFormats[] = {colorFormat};
    conf.colorAttachmentFormats = colorFormats;
    conf.depthAttachmentFormat = depthFormat;
    conf.cullMode = GpuCullMode::back;
    conf.enableDepthRead = true;
    conf.enableDepthWrite = true;

    pipeline = gpu->createGraphicsPipeline(conf);

    vpBuffer = gpu->createBuffer(sizeof(ViewProj),
        GpuBufferUsage::uniformBuffer, GpuMemoryUsage::frequentUpdate);

    ViewProj vp{
        matView(Vec3{0.0f}, Vec3{1.0f}, 1.0f),
        matOrthographic(-1, 1, -1, 1, 0, 1),
    };
    vpBuffer->write(std::as_bytes(std::span{&vp, 1}), 0);

    dirLightBuffer = gpu->createBuffer(128 * sizeof(LightData),
        GpuBufferUsage::transferDst | GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    pointLightBuffer = gpu->createBuffer(128 * sizeof(LightData),
        GpuBufferUsage::transferDst | GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
}

void Renderer3D::drawDirectionalLight(Vec3 color, Vec3 dir)
{
    dirLights.emplace_back(dir, color);
}

void Renderer3D::drawPointLight(Vec3 color, Vec3 pos)
{
    pointLights.emplace_back(pos, color);
}

void Renderer3D::drawModel(
        const Mesh& mesh,
        const Texture& colorMap,
        const Texture& normalMap,
        f32 diffuse,
        f32 shininess,
        const Mat4& transform)
{
    models.emplace_back(
        transform,
        &mesh,
        &colorMap,
        &normalMap,
        diffuse,
        shininess);
}

void Renderer3D::drawModelMaterial(const Model3D& material, const Mat4& transform)
{
    drawModel(
        *material.mesh,
        *material.colorMap,
        *material.normalMap,
        material.diffuse,
        material.shininess,
        transform);
}

void Renderer3D::renderScene(GpuCommandBuffer* cmd, Camera& camera)
{
    ViewProj vp{};
    match(camera.projection,
        [&](const Perspective& persp)
        {
            vp.proj = matPerspective(
                persp.fov,
                persp.aspect,
                persp.near,
                persp.far);
        },
        [&](const Orthographic& ortho)
        {
            vp.proj = matOrthographic(
                ortho.left,
                ortho.right,
                ortho.top,
                ortho.bottom,
                ortho.near,
                ortho.far);
        });
    vp.view = matView(camera.position, Vec3{1.0f}, camera.rotation);
    vpBuffer->write(std::as_bytes(std::span{&vp, 1}), 0);

    if (dirLightBuffer->size() < dirLights.size() * sizeof(LightData))
    {
        gpu->waitIdle();
        dirLightBuffer = gpu->createBuffer(2 * dirLights.size() * sizeof(LightData),
            GpuBufferUsage::transferDst | GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    }

    if (pointLightBuffer->size() < pointLights.size() * sizeof(LightData))
    {
        gpu->waitIdle();
        pointLightBuffer = gpu->createBuffer(2 * pointLights.size() * sizeof(LightData),
            GpuBufferUsage::transferDst | GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    }

    struct Light {
        Vec4 pos;
        Vec4 color;
    };

    Mat4 view = matView(camera.position, Vec3{1.0f}, camera.rotation);

    if (dirLights.size() > 0)
    {
        ArenaScope scratch = getScratch();
        VectorTemp<Light> dirBuf{scratch};
        dirBuf.reserve(dirLights.size());

        for (LightData& light : dirLights)
        {
            dirBuf.emplace_back(view * Vec4{light.pos, 0}, Vec4{light.color, 1});
        }
        dirLightBuffer->write(std::as_bytes(std::span{dirBuf}), 0);
    }

    if (pointLights.size() > 0)
    {
        ArenaScope scratch = getScratch();
        VectorTemp<Light> pointBuf{scratch};
        pointBuf.reserve(pointLights.size());

        for (LightData& light : pointLights)
        {
            pointBuf.emplace_back(view * Vec4{light.pos, 1}, Vec4{light.color, 1});
        }

        pointLightBuffer->write(std::as_bytes(std::span{pointBuf}), 0);
    }

    cmd->bindPipeline(pipeline.get());

    for (ModelData& model : models)
    {
        Push push{};
        push.model = model.transform;
        if (dirLights.size() > 0)
        {
            push.dirLights = dirLightBuffer->storageDescriptor();
            push.dirLightCount = assertCast<u32>(dirLights.size());
        }
        if (pointLights.size() > 0)
        {
            push.pointLights = pointLightBuffer->storageDescriptor();
            push.pointLightCount = assertCast<u32>(pointLights.size());
        }
        push.colorMap = model.colorMap->view->samplerDescriptor();
        push.normalMap = model.normalMap->view->samplerDescriptor();
        push.diffuse = model.diffuse;
        push.shininess = model.shininess;
        push.verts = model.mesh->vertexBuffer->storageDescriptor();
        push.inds = model.mesh->indexBuffer->storageDescriptor();
        push.vp = vpBuffer->uniformDescriptor();

        cmd->pushConstants(pipeline.get(), push);
        cmd->draw(0, model.mesh->indexCount, 0, 1);
    }

    dirLights.clear();
    pointLights.clear();
    models.clear();
}

} // namespace hg
