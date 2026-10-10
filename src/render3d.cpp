#include "hg/render3d.hpp"

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

Renderer3D::Renderer3D(Format colorFormat, Format depthFormat)
{
}

} // namespace hg
