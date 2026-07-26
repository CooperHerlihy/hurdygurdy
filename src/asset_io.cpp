#include "hurdygurdy.hpp"
#include "internal.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <cstdio>

namespace hg {

template<>
void assetLoadImpl(AssetData<TextureData>* data)
{
    ArenaScope scratch = getScratch();
    char* cpath = cString(scratch, data->path);

    int x, y, channels;
    data->asset.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->asset.pixels == nullptr)
    {
        setError("Could not load image: %s", cpath);
        return;
    }
    data->asset.width = static_cast<u32>(x);
    data->asset.height = static_cast<u32>(y);
    data->asset.depth = 1;
    data->asset.format = Format_r8g8b8a8_srgb;
}

TextureData::~TextureData() noexcept
{
    std::free(pixels);
}

bool textureStorePng(TextureData* texture, StringView path)
{
    ArenaScope scratch = getScratch();

    const char* cpath = cString(scratch, path);

    if (!stbi_write_png(
         cpath,
         static_cast<int>(texture->width),
         static_cast<int>(texture->height),
         4,
         texture->pixels,
         static_cast<int>(texture->width * sizeof(u32))))
    {
        setError("Could not store image: %s", cpath);
        return false;
    }
    return true;
}

template<>
void assetLoadImpl(AssetData<Texture>* data)
{
    Asset<TextureData> tex = load<TextureData>(data->path);
    if (tex->pixels == nullptr)
        return;

    GpuImageCreateInfo imageInfo{};
    imageInfo.format = tex->format;
    imageInfo.width = tex->width;
    imageInfo.height = tex->height;
    imageInfo.depth = tex->depth;
    imageInfo.usage = GpuImageUsage_transferDst | GpuImageUsage_sampled;

    data->asset.image = GpuImage::createEx(imageInfo);
    data->asset.view = GpuView::create(data->asset.image, GpuAspect_color, GpuFilter_nearest);

    data->asset.view.write(tex->pixels);
}

template<>
void assetLoadImpl(AssetData<MeshData>* data)
{
    static_cast<void>(data);
    HG_PANIC("load gltf file : TODO\n");
}

void meshStoreGltf(MeshData* data, StringView path, Fence* fence)
{
    static_cast<void>(data);
    static_cast<void>(path);
    static_cast<void>(fence);
    HG_PANIC("store gltf file : TODO\n");
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
void assetLoadImpl(AssetData<Binary>* data)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        setError("Could not find file to read binary: %s", cpath);
        return;
    }
    HG_DEFER(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        setError("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = static_cast<u64>(ftell(fileHandle));
    data->asset.data = heapAlloc(data->asset.size, 1);

    rewind(fileHandle);
    if (fread(const_cast<void*>(data->asset.data), 1, data->asset.size, fileHandle) != data->asset.size)
    {
        heapFree(const_cast<void*>(data->asset.data), data->asset.size);
        setError("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

bool binaryStore(BinaryView bin, StringView path)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        setError("Failed to create file to write binary: %s", cpath);
        return false;
    }
    HG_DEFER(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        setError("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

} // namespace hg

