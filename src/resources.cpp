#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

HgResourceManager HgResourceManager::create(usize resourceWidth, usize capacity)
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

void HgResourceManager::resize(usize newSize)
{
    HgResourceManager old = *this;
    hgDefer(old.destroy());

    *this = create(width, newSize);
    for (usize i = 0; i < old.capacity; ++i)
    {
        if (old.refCounts[i] != (u32)-1)
        {
            usize idx = add(old.ids[i]);
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
    for (usize i = 0; i < capacity; ++i)
    {
        refCounts[i] = (u32)-1;
    }
    count = 0;
}

usize HgResourceManager::add(HgResource id)
{
    if ((f32)(count + 1) >= (f32)capacity * 0.5f)
        resize(capacity * 2);

    usize idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
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
    usize idx = id % capacity;
    while (refCounts[idx] != (u32)-1)
    {
        if (ids[idx] == id)
            break;
        idx = (idx + 1) % capacity;
    }
    if (refCounts[idx] == (u32)-1)
        return;

    usize next = (idx + 1) % capacity;
    while (refCounts[next] % capacity != (u32)-1)
    {
        if (ids[next] % capacity != idx)
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
    usize idx = add(id);
    return refCounts[idx]++ == 0;
}

bool HgResourceManager::unload(HgResource id, void* resource)
{
    usize idx = id % capacity;
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
    usize idx = id % capacity;
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

    bin.resize(arena, (usize)ftell(fileHandle));
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

void hgLoadResource(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
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
            bin.size = (usize)ftell(fileHandle);
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

void hgUnloadResource(HgFence* fences, usize fenceCount, HgResource id)
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

void hgStoreResource(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
{
    HgBinary** res = (HgBinary**)binResources.get(id);
    if (res == nullptr || *res == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not store resource to \"%s\" because the resource does not exist\n",
            hgCString(scratch, path));
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

bool HgTextureData::getInfo(VkFormat* format, u32* width, u32* height, u32* depth)
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, textureIdentifier, sizeof(textureIdentifier)) == 0)
    {
        bin.read(offsetof(Info, format), format, sizeof(*format));
        bin.read(offsetof(Info, width), width, sizeof(*width));
        bin.read(offsetof(Info, height), height, sizeof(*height));
        bin.read(offsetof(Info, depth), depth, sizeof(*depth));
        return true;
    }
    return false;
}

void* HgTextureData::getPixels()
{
    if (bin.size >= sizeof(Info) && memcmp(bin.data, textureIdentifier, sizeof(textureIdentifier)) == 0)
    {
        return (u8*)bin.data + bin.read<usize>(offsetof(Info, pixelsBegin));
    }
    return bin.data;
}

void hgImportPng(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
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
            hgWarn("Failed to decode image file: %s\n", hgCString(scratch, fpath));
            return;
        }
        free(bin.data);
        hgDefer(free(pixels));

        bin.size = sizeof(HgTextureData::Info) + (usize)width * (usize)height * sizeof(u32);
        bin.data = malloc(bin.size);

        HgTextureData::Info info;
        memcpy(info.identifier, HgTextureData::textureIdentifier, sizeof(HgTextureData::textureIdentifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;
        info.pixelsBegin = sizeof(info);

        bin.overwrite(0, info);
        bin.overwrite(sizeof(info), pixels, (usize)width * (usize)height * sizeof(u32));
    });
}

void hgExportPng(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
{
    HgBinary* resource = hgGetResource(id);
    if (resource == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        hgWarn("Could not export png resource to \"%s\" because the resource does not exist\n",
            hgCString(scratch, path));
        return;
    }

    hgRequestIO(fences, fenceCount, resource, path, [](void* ptex, HgStringView fpath)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};
        char* cpath = hgCString(scratch, fpath);

        HgTextureData tex = *(HgBinary*)ptex;
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
        return (u8*)file.data + file.read<usize>(offsetof(Info, verticesBegin));
    }
    return file.data;
}

void* HgModelData::getIndexData()
{
    if (file.size >= sizeof(Info) && memcmp(file.data, modelIdentifier, sizeof(modelIdentifier)) == 0)
    {
        return (u8*)file.data + file.read<usize>(offsetof(Info, indicesBegin));
    }
    return nullptr;
}

void hgImportGltf(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgImportGltf : TODO\n");
}

void hgExportGltf(HgFence* fences, usize fenceCount, HgResource id, HgStringView path)
{
    (void)fences;
    (void)fenceCount;
    (void)id;
    (void)path;
    hgError("hgExportGltf : TODO\n");
}

void hgInitGpuResources()
{
    HgInitGpuTextures();
    hgInitGpuModels();
}

void hgDeinitGpuResources()
{
    hgDeinitGpuModels();
    hgDeinitGpuTextures();
}

HgResourceManager gpuTextures{};

void HgInitGpuTextures()
{
    gpuTextures = gpuTextures.create(sizeof(HgGpuTexture));
}

void hgDeinitGpuTextures()
{
    gpuTextures.forEach([](HgResource, void* ptex)
    {
        HgGpuTexture& tex = *(HgGpuTexture*)ptex;
        vkDestroySampler(hgVkDevice, tex.sampler, nullptr);
        vkDestroyImageView(hgVkDevice, tex.view, nullptr);
        vmaDestroyImage(hgVkVma, tex.image, tex.allocation);
    });
    gpuTextures.destroy();
}

void hgLoadGpuTexture(HgResource id, VkFilter filter)
{
    if (gpuTextures.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgTextureData data = *hgGetResource(id);

        HgGpuTexture& tex = *(HgGpuTexture*)gpuTextures.get(id);;
        if (!data.getInfo(&tex.format, &tex.width, &tex.height, &tex.depth))
        {
            hgWarn("Could not get info to load texture\n");
            return;
        }
        hgAssert(tex.width != 0);
        hgAssert(tex.height != 0);
        hgAssert(tex.depth != 0);
        hgAssert(tex.format != 0);

        HgVkImageConfig imageInfo{};
        imageInfo.width = tex.width;
        imageInfo.height = tex.height;
        imageInfo.depth = tex.depth;
        imageInfo.format = tex.format;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        hgVkCreateImage(&tex.image, &tex.allocation, imageInfo);

        HgVkWriteImageStagingConfig stagingConfig{};
        stagingConfig.dstImage = tex.image;
        stagingConfig.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        stagingConfig.srcData = data.getPixels();
        stagingConfig.width = tex.width;
        stagingConfig.height = tex.height;
        stagingConfig.depth = tex.depth;
        stagingConfig.format = tex.format;
        stagingConfig.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        hgWriteVkImageStaging(stagingConfig);

        HgVkImageViewConfig viewInfo{};
        viewInfo.image = tex.image;
        viewInfo.format = tex.format;
        viewInfo.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        tex.view = hgVkCreateImageView(viewInfo);
        hgAssert(tex.view != nullptr);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        vkCreateSampler(hgVkDevice, &samplerInfo, nullptr, &tex.sampler);
        hgAssert(tex.sampler != nullptr);
    }
}

void hgUnloadGpuTexture(HgResource id)
{
    HgGpuTexture tex;
    if (gpuTextures.unload(id, &tex))
    {
        vkDestroySampler(hgVkDevice, tex.sampler, nullptr);
        vkDestroyImageView(hgVkDevice, tex.view, nullptr);
        vmaDestroyImage(hgVkVma, tex.image, tex.allocation);
        tex = {};
    }
}

HgGpuTexture* hgGetGpuTexture(HgResource id)
{
    return (HgGpuTexture*)gpuTextures.get(id);
}

HgResourceManager gpuModels{};

void hgInitGpuModels()
{
    gpuModels = gpuModels.create(sizeof(HgGpuModel));
}

void hgDeinitGpuModels()
{
    gpuModels.forEach([](HgResource, void* pmodel)
    {
        HgGpuModel& model = *(HgGpuModel*)pmodel;
        vmaDestroyBuffer(hgVkVma, model.vertexBuffer, model.vertexAlloc);
        vmaDestroyBuffer(hgVkVma, model.indexBuffer, model.indexAlloc);
    });
    gpuModels.destroy();
}

void hgLoadGpuModel(HgResource id)
{
    if (gpuModels.load(id))
    {
        hgAssert(hgGetResource(id) != nullptr);
        hgAssert(hgGetResource(id)->data != nullptr);
        HgModelData data = *hgGetResource(id);

        VkPrimitiveTopology topology;
        HgGpuModel& model = *(HgGpuModel*)gpuModels.get(id);
        if (!data.getInfo(
            &model.vertexCount,
            &model.vertexWidth,
            &model.indexCount,
            &topology))
        {
            hgWarn("Could not get info to load model\n");
            return;
        }

        hgVkCreateBuffer(
            &model.vertexBuffer,
            &model.vertexAlloc,
            model.vertexCount * model.vertexWidth,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hgVkCreateBuffer(
            &model.indexBuffer,
            &model.indexAlloc,
            model.indexCount * sizeof(u32),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hgVkWriteBufferStaging(
            model.vertexBuffer, 0,
            data.getVertexData(), model.vertexCount * model.vertexWidth);

        hgVkWriteBufferStaging(
            model.indexBuffer, 0,
            data.getIndexData(), model.indexCount * sizeof(u32));
    }
}

void hgUnloadGpuModel(HgResource id)
{
    HgGpuModel model;
    if (gpuModels.unload(id, &model))
    {
        vmaDestroyBuffer(hgVkVma, model.indexBuffer, model.indexAlloc);
    }
}

HgGpuModel* hgGetGpuModel(HgResource id)
{
    return (HgGpuModel*)gpuModels.get(id);
}

