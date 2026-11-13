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

Mesh uploadMeshToGpu(const MeshData& data)
{
    Mesh mesh{};

    mesh.vertexCount = static_cast<u32>(data.vertices.size());
    mesh.vertexWidth = sizeof(MeshVertex);
    mesh.indexCount = static_cast<u32>(data.indices.size());

    mesh.vertexBuffer = GpuBuffer::create(
        mesh.vertexCount * mesh.vertexWidth, GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    mesh.indexBuffer = GpuBuffer::create(
        mesh.indexCount * sizeof(u32), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    mesh.vertexBuffer.write(std::as_bytes(std::span{data.vertices}), 0);
    mesh.indexBuffer.write(std::as_bytes(std::span{data.indices}), 0);

    return mesh;
}

MeshData downloadMeshFromGpu(const Mesh& mesh)
{
    MeshData data{};

    u64 vCount = mesh.vertexBuffer.size() / sizeof(MeshVertex);
    data.vertices.resize(vCount);

    u64 iCount = mesh.indexBuffer.size() / sizeof(u32);
    data.indices.resize(iCount);

    mesh.vertexBuffer.read(std::as_writable_bytes(std::span{data.vertices}), 0);
    mesh.indexBuffer.read(std::as_writable_bytes(std::span{data.indices}), 0);

    return data;
}

template<>
bool assetLoadImpl(AssetData<Mesh>* data)
{
    Asset<MeshData> mesh = load<MeshData>(data->path);
    data->asset = uploadMeshToGpu(*mesh);
    return true;
}

template<>
bool serialize(Serializer* s, Material3D* material)
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

Renderer3D::Renderer3D(Format colorFormat, Format depthFormat)
{
    GpuGraphicsPipelineCreateInfo conf{};
    conf.vertexShader = shaders_render3d_vert_spv;
    conf.fragmentShader = shaders_render3d_frag_spv;
    conf.pushConstantSize = sizeof(Push);
    Format colorFormats[] = {colorFormat};
    conf.colorAttachmentFormats = colorFormats;
    conf.depthAttachmentFormat = depthFormat;
    conf.cullMode = GpuCull_none;
    conf.enableDepthRead = true;
    conf.enableDepthWrite = true;

    pipeline = GpuPipeline::graphics(conf);
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

void Renderer3D::drawModelMaterial(const Material3D& material, const Mat4& transform)
{
    drawModel(
        *material.mesh,
        *material.colorMap,
        *material.normalMap,
        material.diffuse,
        material.shininess,
        transform);
}

void Renderer3D::renderScene(GpuCmd* cmd, Camera& camera)
{
    if (dirLightBuffer.size() < dirLights.size() * sizeof(LightData))
    {
        gpuWaitIdle();
        dirLightBuffer = GpuBuffer::create(
                2 * dirLights.size() * sizeof(LightData),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);
    }

    if (pointLightBuffer.size() < pointLights.size() * sizeof(LightData))
    {
        gpuWaitIdle();
        pointLightBuffer = GpuBuffer::create(
                2 * pointLights.size() * sizeof(LightData),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);
    }

    struct Light {
        Vec4 pos;
        Vec4 color;
    };

    Mat4 view = matView(camera.position, Vec3{1.0f}, camera.rotation);

    if (dirLights.size() > 0)
    {
        ArenaScope scratch = getScratch();
        vector_temp<Light> dirBuf{scratch};
        dirBuf.reserve(dirLights.size());

        for (LightData& light : dirLights)
        {
            dirBuf.emplace_back(view * Vec4{light.pos, 0}, Vec4{light.color, 1});
        }
        dirLightBuffer.write(std::as_bytes(std::span{dirBuf}), 0);
    }

    if (pointLights.size() > 0)
    {
        ArenaScope scratch = getScratch();
        vector_temp<Light> pointBuf{scratch};
        pointBuf.reserve(pointLights.size());

        for (LightData& light : pointLights)
        {
            pointBuf.emplace_back(view * Vec4{light.pos, 1}, Vec4{light.color, 1});
        }

        pointLightBuffer.write(std::as_bytes(std::span{pointBuf}), 0);
    }

    gpuBindPipeline(cmd, pipeline);

    for (ModelData& model : models)
    {
        Push push{};
        push.model = model.transform;
        if (dirLights.size() > 0)
        {
            push.dirLights = dirLightBuffer.storageDescriptor();
            push.dirLightCount = static_cast<u32>(dirLights.size());
        }
        if (pointLights.size() > 0)
        {
            push.pointLights = pointLightBuffer.storageDescriptor();
            push.pointLightCount = static_cast<u32>(pointLights.size());
        }
        push.colorMap = model.colorMap->view.samplerDescriptor();
        push.normalMap = model.normalMap->view.samplerDescriptor();
        push.diffuse = model.diffuse;
        push.shininess = model.shininess;
        push.verts = model.mesh->vertexBuffer.storageDescriptor();
        push.inds = model.mesh->indexBuffer.storageDescriptor();
        push.vp = camera.vpBuffer.uniformDescriptor();

        gpuPushConstants(cmd, pipeline, push);
        gpuDraw(cmd, 0, model.mesh->indexCount, 0, 1);
    }

    dirLights.clear();
    pointLights.clear();
    models.clear();
}

} // namespace hg
