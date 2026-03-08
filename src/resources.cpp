#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

HgBinary HgBinary::load(HgArena& arena, HgStringView path) {
    HgArena& scratch = hg_get_scratch(arena);
    HgArenaScope scratch_scope{scratch};

    HgBinary bin;

    char* cpath = hg_c_string(scratch, path);

    FILE* file_handle = fopen(cpath, "rb");
    if (file_handle == nullptr) {
        hg_warn("Could not find file to read binary: %s\n", cpath);
        return {};
    }
    hg_defer(fclose(file_handle));

    if (fseek(file_handle, 0, SEEK_END) != 0) {
        hg_warn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    bin.resize(arena, (usize)ftell(file_handle));
    rewind(file_handle);

    if (fread(bin.data, 1, bin.size, file_handle) != bin.size) {
        hg_warn("Failed to read binary from file: %s\n", cpath);
        return {};
    }

    return bin;
}

void HgBinary::store(HgStringView path) {
    HgArena& scratch = hg_get_scratch();
    HgArenaScope scratch_scope{scratch};

    char* cpath = hg_c_string(scratch, path);

    FILE* file_handle = fopen(cpath, "wb");
    if (file_handle == nullptr) {
        hg_warn("Failed to create file to write binary: %s\n", cpath);
        return;
    }
    hg_defer(fclose(file_handle));

    if (fwrite(data, 1, size, file_handle) != size) {
        hg_warn("Failed to write binary data to file: %s\n", cpath);
        return;
    }
}

bool HgTexture::get_info(VkFormat& format, u32& width, u32& height, u32& depth) {
    if (file.size >= sizeof(Info) && memcmp(file.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        file.read(offsetof(Info, format), &format, sizeof(format));
        file.read(offsetof(Info, width), &width, sizeof(width));
        file.read(offsetof(Info, height), &height, sizeof(height));
        file.read(offsetof(Info, depth), &depth, sizeof(depth));
        return true;
    }
    return false;
}

void* HgTexture::get_pixels() {
    if (file.size >= sizeof(Info) && memcmp(file.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        return (u8*)file.data + sizeof(Info);
    }
    return file.data;
}

struct Resource {
    HgBinary* file;
    u32 ref_count;
};

HgBinary* resource_pool = nullptr;
usize* resource_free_list = nullptr;
usize resource_capacity = 0;
usize resource_first = 0;

HgHashMap<HgResource, Resource> resource_map;

void hg_resources_init(HgArena& arena, usize max_resources) {
    resource_pool = arena.realloc(resource_pool, resource_capacity, max_resources);
    resource_free_list = arena.realloc(resource_free_list, resource_capacity, max_resources);
    resource_capacity = max_resources;
    resource_map = resource_map.create(arena, max_resources);
    hg_resources_reset();
}

void hg_resources_reset() {
    resource_map.for_each([&](HgResource id, Resource& res) {
        if (res.ref_count != 0) {
            res.ref_count = 1;
            hg_unload_resource(nullptr, 0, id);
        }
    });
    resource_map.reset();

    for (usize i = 0; i < resource_capacity; ++i) {
        resource_free_list[i] = i + 1;
    }
    resource_first = 0;
}

void hg_alloc_resource(HgResource id) {
    if (!resource_map.has(id)) {
        usize idx = resource_first;
        HgBinary* bin = resource_pool + idx;
        *bin = {};

        resource_map.insert(id, {bin, 0});
        resource_first = resource_free_list[idx];
        resource_free_list[idx] = idx;
    }
}

void hg_dealloc_resource(HgResource id) {
    Resource* r = resource_map.get(id);
    if (r != nullptr) {
        if (r->ref_count > 0) {
            r->ref_count = 1;
            hg_unload_resource(nullptr, 0, id);
        }
        usize idx = (uptr)(r->file - resource_pool);
        resource_free_list[idx] = resource_first;
        resource_first = idx;

        resource_map.remove(id);
    }
}

void hg_load_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    Resource* resource = resource_map.get(id);
    if (resource == nullptr) {
        hg_alloc_resource(id);
        resource = resource_map.get(id);
    }
    if (resource->ref_count++ > 0)
        return;

    hg_io_request(fences, fence_count, resource, path, [](void* pres, HgStringView fpath) {
        Resource& res = *(Resource*)pres;

        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};
        char* cpath = hg_c_string(scratch, fpath);

        FILE* file_handle = fopen(cpath, "rb");
        if (file_handle == nullptr) {
            hg_warn("Could not find file to read binary: %s\n", cpath);
            return;
        }
        hg_defer(fclose(file_handle));

        if (fseek(file_handle, 0, SEEK_END) != 0) {
            hg_warn("Failed to read binary from file: %s\n", cpath);
            return;
        }
        res.file->size = (usize)ftell(file_handle);
        res.file->data = malloc(res.file->size);
        rewind(file_handle);

        if (fread(res.file->data, 1, res.file->size, file_handle) != res.file->size) {
            hg_warn("Failed to read binary from file: %s\n", cpath);
            free(res.file->data);
            res.file->size = 0;
            res.file->data = nullptr;
            return;
        }
    });
}

void hg_unload_resource(HgFence* fences, usize fence_count, HgResource id) {
    Resource* resource = resource_map.get(id);
    if (resource == nullptr || resource->ref_count == 0 || --resource->ref_count > 0)
        return;

    hg_io_request(fences, fence_count, resource, {}, [](void* pres, HgStringView) {
        Resource& res = *(Resource*)pres;
        free(res.file->data);
        res.file->size = 0;
        res.file->data = nullptr;
    });
}

void hg_store_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    Resource* resource = resource_map.get(id);
    if (resource == nullptr) {
        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};
        hg_warn("Could not store resource to \"%s\" because the resource does not exist\n",
            hg_c_string(scratch, path));
        return;
    }

    hg_io_request(fences, fence_count, resource, path, [](void* pres, HgStringView fpath) {
        (*(Resource*)pres).file->store(fpath);
    });
}

HgBinary* hg_get_resource(HgResource id) {
    Resource* r = resource_map.get(id);
    return r == nullptr ? nullptr : r->file;
}

void hg_import_png(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    hg_load_resource(fences, fence_count, id, path);

    hg_io_request(fences, fence_count, hg_get_resource(id), path, [](void* pbin, HgStringView fpath) {
        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};

        HgBinary& bin = *(HgBinary*)pbin;

        int width, height, channels;
        u8* pixels = stbi_load_from_memory((u8*)bin.data, (i32)bin.size, &width, &height, &channels, 4);
        if (pixels == nullptr) {
            hg_warn("Failed to decode image file: %s\n", hg_c_string(scratch, fpath));
            return;
        }
        free(bin.data);
        hg_defer(free(pixels));

        bin.size = sizeof(HgTexture::Info) + (usize)width * (usize)height * sizeof(u32);
        bin.data = malloc(bin.size);

        HgTexture::Info info;
        memcpy(info.identifier, HgTexture::texture_identifier, sizeof(HgTexture::texture_identifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;

        bin.overwrite(0, info);
        bin.overwrite(sizeof(info), pixels, (usize)width * (usize)height * sizeof(u32));
    });
}

void hg_export_png(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    HgBinary* resource = hg_get_resource(id);
    if (resource == nullptr) {
        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};
        hg_warn("Could not export png resource to \"%s\" because the resource does not exist\n",
            hg_c_string(scratch, path));
        return;
    }

    hg_io_request(fences, fence_count, resource, path, [](void* ptex, HgStringView fpath) {
        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};
        char* cpath = hg_c_string(scratch, fpath);

        HgTexture tex = *(HgBinary*)ptex;
        void* pixels = tex.get_pixels();
        if (pixels == nullptr) {
            hg_warn("Cannot export emprty image %s\n", cpath);
            return;
        }

        VkFormat format;
        u32 width, height, depth;
        if (!tex.get_info(format, width, height, depth)) {
            hg_warn("Could not get info from image %s to export\n", cpath);
            return;
        }
        if (depth > 1)
            hg_warn("Cannot export 3d image %s, exporting only the first layer\n", cpath);
        stbi_write_png(cpath, (int)width, (int)height, 4, pixels, (int)(width * sizeof(u32)));
    });
}

enum class GpuResourceType : u32 {
    buffer,
    texture,
};

struct GpuResource {
    union {
        HgGpuBuffer buffer;
        HgGpuTexture texture;
    };
    GpuResourceType type;
    u32 ref_count;
};

HgHashMap<HgResource, GpuResource> gpu_map;

void hg_gpu_resources_init(HgArena& arena, usize max_resources) {
    gpu_map = gpu_map.create(arena, max_resources);
}

void hg_gpu_resources_reset() {
    gpu_map.for_each([&](HgResource id, GpuResource& r) {
        if (r.ref_count != 0) {
            r.ref_count = 1;
            hg_unload_gpu_resource(id);
        }
    });
    gpu_map.reset();
}

void hg_alloc_gpu_buffer(HgResource id) {
    if (!gpu_map.has(id)) {
        GpuResource* r = gpu_map.get(id);
        if (r != nullptr) {
            if (r->type != GpuResourceType::buffer)
                hg_warn("Attempted to register gpu resource not of type buffer as a buffer\n");
        } else {
            r = &gpu_map.insert(id, {});
            r->type = GpuResourceType::buffer;
        }
    }
}

void hg_alloc_gpu_texture(HgResource id) {
    if (!gpu_map.has(id)) {
        GpuResource* r = gpu_map.get(id);
        if (r != nullptr) {
            if (r->type != GpuResourceType::texture)
                hg_warn("Attempted to register gpu resource not of type texture as a texture\n");
        } else {
            r = &gpu_map.insert(id, {});
            r->type = GpuResourceType::texture;
        }
    }
}

void hg_dealloc_gpu_resource(HgResource id) {
    GpuResource* r = gpu_map.get(id);
    if (r != nullptr) {
        if (r->ref_count > 0) {
            r->ref_count = 1;
            hg_unload_gpu_resource(id);
        }
        gpu_map.remove(id);
    }
}

void hg_load_gpu_buffer(HgResource id) {
    if (!gpu_map.has(id))
        hg_alloc_gpu_buffer(id);

    GpuResource* r = gpu_map.get(id);
    hg_assert(r != nullptr);
    hg_assert(r->type == GpuResourceType::buffer);
    if (r->ref_count++ > 0)
        return;

    // load gpu buffer : TODO
    hg_error("load gpu buffer not implements : TODO\n");
}

void hg_load_gpu_texture(HgResource id, VkFilter filter) {
    if (!gpu_map.has(id))
        hg_alloc_gpu_texture(id);

    GpuResource* r = gpu_map.get(id);
    hg_assert(r != nullptr);
    hg_assert(r->type == GpuResourceType::texture);
    if (r->ref_count++ > 0)
        return;

    HgTexture data = *hg_get_resource(id);
    hg_assert(data.file.data != nullptr);

    HgGpuTexture& tex = *hg_get_gpu_texture(id);
    if (!data.get_info(tex.format, tex.width, tex.height, tex.depth)) {
        hg_warn("Could not get info to load texture\n");
        return;
    }
    hg_assert(tex.width != 0);
    hg_assert(tex.height != 0);
    hg_assert(tex.depth != 0);
    hg_assert(tex.format != 0);

    HgVkImageConfig image_info{};
    image_info.width = tex.width;
    image_info.height = tex.height;
    image_info.depth = tex.depth;
    image_info.format = tex.format;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    hg_vk_create_image(&tex.image, &tex.allocation, image_info);

    HgVkImageStagingWriteConfig staging_config{};
    staging_config.dst_image = tex.image;
    staging_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    staging_config.src_data = data.get_pixels();
    staging_config.width = tex.width;
    staging_config.height = tex.height;
    staging_config.depth = tex.depth;
    staging_config.format = tex.format;
    staging_config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(staging_config);

    HgVkImageViewConfig view_info{};
    view_info.image = tex.image;
    view_info.format = tex.format;
    view_info.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    hg_vk_create_image_view(&tex.view, view_info);
    hg_assert(tex.view != nullptr);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    vkCreateSampler(hg_vk_device, &sampler_info, nullptr, &tex.sampler);
    hg_assert(tex.sampler != nullptr);
}

void hg_unload_gpu_resource(HgResource id) {
    GpuResource* r = gpu_map.get(id);
    if (r == nullptr)
        return;
    if (--r->ref_count > 0)
        return;

    switch (r->type) {
        case GpuResourceType::buffer:
            vmaDestroyBuffer(hg_vk_vma, r->buffer.buffer, r->buffer.allocation);
            r->buffer = {};
            break;
        case GpuResourceType::texture:
            vkDestroySampler(hg_vk_device, r->texture.sampler, nullptr);
            vkDestroyImageView(hg_vk_device, r->texture.view, nullptr);
            vmaDestroyImage(hg_vk_vma, r->texture.image, r->texture.allocation);
            r->texture = {};
            break;
        default:
            hg_assert(false);
    }
}

bool hg_is_gpu_resource_loaded(HgResource id) {
    GpuResource* r = gpu_map.get(id);
    return r != nullptr && r->ref_count > 0;
}

HgGpuBuffer* hg_get_gpu_buffer(HgResource id) {
    GpuResource* r = gpu_map.get(id);
    if (r == nullptr)
        return nullptr;
    if (r->type != GpuResourceType::buffer) {
        hg_warn("Attempted to access non buffer as gpu buffer\n");
        return nullptr;
    }
    return &r->buffer;
}

HgGpuTexture* hg_get_gpu_texture(HgResource id) {
    GpuResource* r = gpu_map.get(id);
    if (r == nullptr)
        return nullptr;
    if (r->type != GpuResourceType::texture) {
        hg_warn("Attempted to access non texture as gpu texture\n");
        return nullptr;
    }
    return &r->texture;
}

