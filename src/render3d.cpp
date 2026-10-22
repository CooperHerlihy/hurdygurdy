#include "hg/render3d.hpp"
#include "hg/gpu.hpp"

namespace hg {

void meshStoreGltf(MeshData* data, StringView path)
{
    static_cast<void>(data);
    static_cast<void>(path);
    HG_PANIC("store gltf file : TODO\n");
}

template<>
void assetLoadImpl(AssetData<MeshData>* data)
{
    static_cast<void>(data);
    HG_PANIC("load gltf file : TODO\n");
}

template<>
void assetLoadImpl(AssetData<Mesh>* data)
{
    Asset<MeshData> mesh = load<MeshData>(data->path);

    data->asset.vertexCount = static_cast<u32>(mesh->vertices.count);
    data->asset.vertexWidth = sizeof(MeshVertex);
    data->asset.indexCount = static_cast<u32>(mesh->indices.count);

    data->asset.vertexBuffer = GpuBuffer::create(
        data->asset.vertexCount * data->asset.vertexWidth, GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    data->asset.indexBuffer = GpuBuffer::create(
        data->asset.indexCount * sizeof(u32), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    data->asset.vertexBuffer.write(mesh->vertices.vals, 0, data->asset.vertexCount * data->asset.vertexWidth);
    data->asset.indexBuffer.write(mesh->indices.vals, 0, data->asset.indexCount * sizeof(u32));
}

template<>
void serialize<>(Serializer* s, Material3D* material)
{
    serializeObject(s,
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
    dirLights.push(LightData{{dir.x, dir.y, dir.z}, {color.x, color.y, color.z}});
}

void Renderer3D::drawPointLight(Vec3 color, Vec3 pos)
{
    pointLights.push(LightData{{pos.x, pos.y, pos.z}, {color.x, color.y, color.z}});
}

void Renderer3D::drawModel(
        const Mesh& mesh,
        const Texture& colorMap,
        const Texture& normalMap,
        f32 diffuse,
        f32 shininess,
        const Mat4& transform)
{
    models.push({
        transform,
        &mesh,
        &colorMap,
        &normalMap,
        diffuse,
        shininess,
    });
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
    if (dirLightBuffer.size() < dirLights.count * sizeof(LightData))
    {
        gpuWaitIdle();
        dirLightBuffer = GpuBuffer::create(
                2 * dirLights.count * sizeof(LightData),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);
    }

    if (pointLightBuffer.size() < pointLights.count * sizeof(LightData))
    {
        gpuWaitIdle();
        pointLightBuffer = GpuBuffer::create(
                2 * pointLights.count * sizeof(LightData),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);
    }

    struct Light {
        Vec4 pos;
        Vec4 color;
    };

    if (dirLights.count > 0)
    {
        ArenaScope scratch = getScratch();
        ArrayTemp<Light> dirBuf{scratch, 0, dirLights.count};

        for (LightData& light : dirLights)
        {
            dirBuf.push({Vec4{light.pos, 0}, Vec4{light.color, 1}});
        }
        dirLightBuffer.write(dirBuf.vals, 0, dirBuf.count * sizeof(Light));
    }

    if (pointLights.count > 0)
    {
        ArenaScope scratch = getScratch();
        ArrayTemp<Light> pointBuf{scratch, 0, pointLights.count};

        Mat4 view = matView(camera.position, Vec3{1.0f}, camera.rotation);
        for (LightData& light : pointLights)
        {
            pointBuf.push({view * Vec4{light.pos, 1}, Vec4{light.color, 1}});
        }

        pointLightBuffer.write(pointBuf.vals, 0, pointBuf.count * sizeof(Light));
    }

    gpuBindPipeline(cmd, pipeline);

    for (ModelData& model : models)
    {
        Push push{};
        push.model = model.transform;
        if (dirLights.count > 0)
        {
            push.dirLights = dirLightBuffer.storageDescriptor();
            push.dirLightCount = static_cast<u32>(dirLights.count);
        }
        if (pointLights.count > 0)
        {
            push.pointLights = pointLightBuffer.storageDescriptor();
            push.pointLightCount = static_cast<u32>(pointLights.count);
        }
        push.colorMap = model.colorMap->view.samplerDescriptor();
        push.normalMap = model.normalMap->view.samplerDescriptor();
        push.diffuse = model.diffuse;
        push.shininess = model.shininess;
        push.verts = model.mesh->vertexBuffer.storageDescriptor();
        push.inds = model.mesh->indexBuffer.storageDescriptor();
        push.vp = camera.vpBuffer.uniformDescriptor();

        gpuPushConstants(cmd, pipeline, &push, sizeof(push));
        gpuDraw(cmd, 0, model.mesh->indexCount, 0, 1);
    }

    dirLights.reset();
    pointLights.reset();
    models.reset();
}

} // namespace hg
