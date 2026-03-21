#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

HgResourceManager HgResourceManager::create(u32 resourceWidth, u32 capacity)
{
    hgAssert(capacity > 0);

    HgResourceManager rm;
    rm.refCounts = (u32*)malloc(capacity * sizeof(u32));
    rm.ids = (HgResource*)malloc(capacity * sizeof(HgResource));
    rm.resources = malloc(capacity * resourceWidth);
    rm.width = resourceWidth;
    rm.capacity = capacity;
    rm.reset();
    return rm;
}

void HgResourceManager::destroy()
{
    free(refCounts);
    free(ids);
    free(resources);
}

void HgResourceManager::resize(u32 newSize)
{
    HgResourceManager old = *this;
    hgDefer(old.destroy());

    *this = create(width, newSize);
    for (u32 i = 0; i < old.capacity; ++i)
    {
        if (old.refCounts[i] != (u32)-1)
        {
            u32 idx = add(old.ids[i]);
            refCounts[idx] = old.refCounts[i];
            memcpy(
                (u8*)resources + idx * width,
                (u8*)old.resources + i * width,
                width);
        }
    }
}

void HgResourceManager::reset()
{
    for (u32 i = 0; i < capacity; ++i)
    {
        refCounts[i] = (u32)-1;
    }
    count = 0;
}

u32 HgResourceManager::add(HgResource id)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    if ((f32)(count + 1) >= (f32)capacity * 0.5f)
        resize(capacity * 2);

    void* resource = hgAlloc(scratch, width, 16);
    void* resourceTmp = hgAlloc(scratch, width, 16);

    u32 idx = id % capacity;
    for (u32 dist = 0; refCounts[idx] != (u32)-1; ++dist)
    {
        u32 otherDist = ids[idx] % capacity - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            HgResource tmpID = ids[idx];
            ids[idx] = id;
            id = tmpID;
            memcpy(resourceTmp, (u8*)resources + idx * width, width);
            memcpy((u8*)resources + idx * width, resource, width);
            memcpy(resourceTmp, resource, width);
            dist = otherDist;
        }

        if (ids[idx] == id)
            return idx;
        idx = (idx + 1) % capacity;
    }

    refCounts[idx] = 0;
    ids[idx] = id;
    ++count;

    return idx;
}

void HgResourceManager::remove(HgResource id)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    if (refCounts[idx] == (u32)-1)
        return;

    u32 next = (idx + 1) % capacity;
    while (refCounts[next] % capacity != (u32)-1)
    {
        if (ids[next] % capacity != next)
        {
            refCounts[idx] = refCounts[next];
            ids[idx] = ids[next];
            memcpy(
                (u8*)resources + idx * width,
                (u8*)resources + next * width,
                width);
            idx = next;
        }
        next = (next + 1) % capacity;
    }
    refCounts[idx] = (u32)-1;
    --count;
}

bool HgResourceManager::load(HgResource id)
{
    u32 idx = add(id);
    return refCounts[idx]++ == 0;
}

bool HgResourceManager::unload(HgResource id, void* resource)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    if (refCounts[idx] == (u32)-1 || refCounts[idx] == 0)
        return false;
    if (--refCounts[idx] != 0)
        return false;
    memcpy(resource, (u8*)resources + idx * width, width);
    return true;
}

void* HgResourceManager::get(HgResource id)
{
    u32 idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    return refCounts[idx] == (u32)-1 || refCounts[idx] == 0
        ? nullptr
        : (u8*)resources + idx * width;
}

HgBinary hgLoadBinary(HgArena* arena, HgStringView path)
{
    HgArena* scratch = hgGetScratch(&arena, 1);
    HgArenaScope scratchScope{scratch};

    HgBinary bin;

    char* cpath = hgCString(scratch, path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        hgWarn("Could not find file to read binary: %s\n", cpath);
        return {};
    }
    hgDefer(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    bin.resize(arena, (u32)ftell(fileHandle));
    rewind(fileHandle);

    if (fread(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    return bin;
}

void hgStoreBinary(HgBinary bin, HgStringView path)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    char* cpath = hgCString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        hgWarn("Failed to create file to write binary: %s\n", cpath);
        return;
    }
    hgDefer(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        hgWarn("Failed to write binary data to file: %s\n", cpath);
        return;
    }
}

HgResourceManager binResources{};

void hgInitResources()
{
    binResources = binResources.create(sizeof(HgBinary*));
}

void hgDeinitResource()
{
    binResources.forEach([](HgResource, void* pres)
            {
        HgBinary* bin = (HgBinary*)pres;
        free(bin->data);
        free(bin);
    });
    binResources.destroy();
}

void hgLoadEmptyResource(HgResource id)
{
    if (binResources.load(id))
    {
        HgBinary** bin = (HgBinary**)binResources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};
    }
}

void hgLoadResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    if (binResources.load(id))
    {
        HgBinary** bin = (HgBinary**)binResources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};

        hgRequestIO(fences, fenceCount, *bin, path, [](void* pres, HgStringView fpath)
                {
            HgBinary& bin = *(HgBinary*)pres;

            HgArena* scratch = hgGetScratch();
            HgArenaScope scratchScope{scratch};
            char* cpath = hgCString(scratch, fpath);

            FILE* fileHandle = fopen(cpath, "rb");
            if (fileHandle == nullptr)
            {
                hgWarn("Could not find file to read binary: %s\n", cpath);
                return;
            }
            hgDefer(fclose(fileHandle));

            if (fseek(fileHandle, 0, SEEK_END) != 0)
            {
                hgWarn("Failed to read binary from file: %s\n", cpath);
                return;
            }
            bin.size = (u32)ftell(fileHandle);
            bin.data = malloc(bin.size);
            rewind(fileHandle);

            if (fread(bin.data, 1, bin.size, fileHandle) != bin.size)
            {
                hgWarn("Failed to read binary from file: %s\n", cpath);
                free(bin.data);
                bin.size = 0;
                bin.data = nullptr;
                return;
            }
        });
    }
}

void hgUnloadResource(HgFence* fences, u32 fenceCount, HgResource id)
{
    HgBinary* bin;
    if (binResources.unload(id, &bin))
    {
        hgRequestIO(fences, fenceCount, bin, {}, [](void* pres, HgStringView)
        {
            HgBinary& bin = *(HgBinary*)pres;
            free(bin.data);
            bin = {};
        });
    }
}

void hgStoreResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    HgBinary** res = (HgBinary**)binResources.get(id);
    if (res == nullptr || *res == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not store resource to \"%.*s\" because the resource does not exist\n",
            (int)path.length, path.chars);
        return;
    }

    hgRequestIO(fences, fenceCount, *res, path, [](void* pres, HgStringView fpath)
    {
        hgStoreBinary((*(HgBinary*)pres), fpath);
    });
}

HgBinary* hgGetResource(HgResource id)
{
    HgBinary** res = (HgBinary**)binResources.get(id);
    return res == nullptr ? nullptr : *res;
}

bool HgImageData::getInfo(VkFormat* format, u32* width, u32* height, u32* depth)
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, imageIdentifier, sizeof(imageIdentifier)) == 0)
    {
        bin.read(offsetof(Info, format), format, sizeof(*format));
        bin.read(offsetof(Info, width), width, sizeof(*width));
        bin.read(offsetof(Info, height), height, sizeof(*height));
        bin.read(offsetof(Info, depth), depth, sizeof(*depth));
        return true;
    }
    return false;
}

void* HgImageData::getPixels()
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, imageIdentifier, sizeof(imageIdentifier)) == 0)
    {
        return (u8*)bin.data + bin.read<u64>(offsetof(Info, pixelsBegin));
    }
    return bin.data;
}

void hgImportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    hgLoadResource(fences, fenceCount, id, path);

    hgRequestIO(fences, fenceCount, hgGetResource(id), path, [](void* pbin, HgStringView fpath)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgBinary& bin = *(HgBinary*)pbin;

        int width, height, channels;
        u8* pixels = stbi_load_from_memory((u8*)bin.data, (i32)bin.size, &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            hgWarn("Failed to decode image file: %.*s\n", (int)fpath.length, fpath.chars);
            return;
        }
        free(bin.data);
        hgDefer(free(pixels));

        bin.size = sizeof(HgImageData::Info) + (u32)width * (u32)height * sizeof(u32);
        bin.data = malloc(bin.size);

        HgImageData::Info info;
        memcpy(info.identifier, HgImageData::imageIdentifier, sizeof(HgImageData::imageIdentifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;
        info.pixelsBegin = sizeof(info);

        bin.overwrite(0, info);
        bin.overwrite(sizeof(info), pixels, (u32)width * (u32)height * sizeof(u32));
    });
}

void hgExportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    HgBinary* resource = hgGetResource(id);
    if (resource == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not export png resource to \"%.*s\" because the resource does not exist\n",
            (int)path.length, path.chars);
        return;
    }

    hgRequestIO(fences, fenceCount, resource, path, [](void* ptex, HgStringView fpath)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        char* cpath = hgCString(scratch, fpath);

        HgImageData tex = *(HgBinary*)ptex;
        void* pixels = tex.getPixels();
        if (pixels == nullptr)
        {
            hgWarn("Cannot export empty image %s\n", cpath);
            return;
        }

        VkFormat format;
        u32 width, height, depth;
        if (!tex.getInfo(&format, &width, &height, &depth))
        {
            hgWarn("Could not get info from image %s to export\n", cpath);
            return;
        }
        if (depth > 1)
            hgWarn("Cannot export 3d image %s, exporting only the first layer\n", cpath);
        stbi_write_png(cpath, (int)width, (int)height, 4, pixels, (int)(width * sizeof(u32)));
    });
}

bool HgModelData::getInfo(u32* vertexCount, u32* vertexWidth, u32* indexCount, VkPrimitiveTopology* topology)
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        file.read(offsetof(Info, vertexCount), vertexCount, sizeof(vertexCount));
        file.read(offsetof(Info, vertexWidth), vertexWidth, sizeof(vertexWidth));
        file.read(offsetof(Info, indexCount), indexCount, sizeof(indexCount));
        file.read(offsetof(Info, topology), topology, sizeof(topology));
        return true;
    }
    return false;
}

void* HgModelData::getVertexData()
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        return (u8*)file.data + file.read<u64>(offsetof(Info, verticesBegin));
    }
    return file.data;
}

void* HgModelData::getIndexData()
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        return (u8*)file.data + file.read<u64>(offsetof(Info, indicesBegin));
    }
    return nullptr;
}

void hgImportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgImportGltf : TODO\n");
}

void hgExportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgExportGltf : TODO\n");
}

void hgInitGpuResources()
{
    HgInitTextures();
    hgInitModels();
}

void hgDeinitGpuResources()
{
    hgDeinitModels();
    hgDeinitTextures();
}

HgResourceManager gpuTextures{};

void HgInitTextures()
{
    gpuTextures = gpuTextures.create(sizeof(HgTextureResource));
}

void hgDeinitTextures()
{
    gpuTextures.forEach([](HgResource, void* ptex)
    {
        HgTextureResource& tex = *(HgTextureResource*)ptex;
        hgDestroyImageView(tex.view);
        hgDestroyImage(tex.image);
    });
    gpuTextures.destroy();
}

void hgLoadTexture(HgResource id, VkSampler sampler)
{
    if (gpuTextures.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgImageData data = *hgGetResource(id);

        VkFormat format;
        u32 width, height, depth;
        if (!data.getInfo(&format, &width, &height, &depth))
        {
            hgWarn("Could not get info to load texture\n");
            return;
        }
        hgAssert(format != 0);
        hgAssert(width != 0);
        hgAssert(height != 0);
        hgAssert(depth != 0);

        HgTextureResource& tex = *(HgTextureResource*)gpuTextures.get(id);;

        HgCreateImageEx imageInfo{};
        imageInfo.format = format;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.depth = depth;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        tex.image = hgCreateImageEx(&imageInfo);

        hgWriteImage(
            tex.image,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            data.getPixels(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tex.view = hgCreateImageView(tex.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        hgAssert(sampler != nullptr);
        tex.sampler = sampler;

        tex.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

        VkDescriptorImageInfo descInfo = {tex.sampler, tex.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        hgUpdateDescriptor(tex.descriptor, nullptr, &descInfo);
    }
}

void hgUnloadTexture(HgResource id)
{
    HgTextureResource tex;
    if (gpuTextures.unload(id, &tex))
    {
        hgDestroyDescriptor(tex.descriptor);
        hgDestroyImageView(tex.view);
        hgDestroyImage(tex.image);
        tex = {};
    }
}

HgTextureResource* hgGetTexture(HgResource id)
{
    return (HgTextureResource*)gpuTextures.get(id);
}

HgResourceManager gpuModels{};

void hgInitModels()
{
    gpuModels = gpuModels.create(sizeof(HgModelResource));
}

void hgDeinitModels()
{
    gpuModels.forEach([](HgResource, void* pmodel)
    {
        HgModelResource& model = *(HgModelResource*)pmodel;
        hgDestroyBuffer(model.vertexBuffer);
        hgDestroyBuffer(model.indexBuffer);
    });
    gpuModels.destroy();
}

void hgLoadModel(HgResource id)
{
    if (gpuModels.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgModelData data = *hgGetResource(id);

        VkPrimitiveTopology topology;
        HgModelResource& model = *(HgModelResource*)gpuModels.get(id);
        if (!data.getInfo(
            &model.vertexCount,
            &model.vertexWidth,
            &model.indexCount,
            &topology))
        {
            hgWarn("Could not get info to load model\n");
            return;
        }

        model.vertexBuffer = hgCreateBuffer(
            model.vertexCount * model.vertexWidth,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        model.indexBuffer = hgCreateBuffer(
            model.indexCount * sizeof(u32),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hgWriteBuffer(
            model.vertexBuffer,
            0,
            data.getVertexData(),
            model.vertexCount * model.vertexWidth);

        hgWriteBuffer(
            model.indexBuffer,
            0,
            data.getIndexData(),
            model.indexCount * sizeof(u32));
    }
}

void hgUnloadModel(HgResource id)
{
    HgModelResource model;
    if (gpuModels.unload(id, &model))
    {
        hgDestroyBuffer(model.vertexBuffer);
        hgDestroyBuffer(model.indexBuffer);
    }
}

HgModelResource* hgGetModel(HgResource id)
{
    return (HgModelResource*)gpuModels.get(id);
}

