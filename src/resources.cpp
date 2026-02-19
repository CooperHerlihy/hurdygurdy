#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

HgBinary HgBinary::load(HgArena& arena, HgStringView path) {
    hg_arena_scope(scratch, hg_get_scratch(arena));

    HgBinary bin;

    HgString cpath = cpath.create(scratch, path).append(scratch, 0);

    FILE* file_handle = std::fopen(cpath.chars, "rb");
    if (file_handle == nullptr) {
        hg_warn("Could not find file to read binary: %s\n", cpath.chars);
        return {};
    }
    hg_defer(std::fclose(file_handle));

    if (std::fseek(file_handle, 0, SEEK_END) != 0) {
        hg_warn("Failed to read binary from file: %s\n", cpath.chars);
        return {};
    }

    bin.resize(arena, (usize)std::ftell(file_handle));
    std::rewind(file_handle);

    if (std::fread(bin.data, 1, bin.size, file_handle) != bin.size) {
        hg_warn("Failed to read binary from file: %s\n", cpath.chars);
        return {};
    }

    return bin;
}

void HgBinary::store(HgStringView path) {
    hg_arena_scope(scratch, hg_get_scratch());

    HgString cpath = cpath.create(scratch, path).append(scratch, 0);

    FILE* file_handle = std::fopen(cpath.chars, "wb");
    if (file_handle == nullptr) {
        hg_warn("Failed to create file to write binary: %s\n", cpath.chars);
        return;
    }
    hg_defer(std::fclose(file_handle));

    if (std::fwrite(data, 1, size, file_handle) != size) {
        hg_warn("Failed to write binary data to file: %s\n", cpath.chars);
        return;
    }
}

bool HgTexture::get_info(VkFormat& format, u32& width, u32& height, u32& depth) {
    if (file.size >= sizeof(Info) && std::memcmp(file.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        file.read(offsetof(Info, format), &format, sizeof(format));
        file.read(offsetof(Info, width), &width, sizeof(width));
        file.read(offsetof(Info, height), &height, sizeof(height));
        file.read(offsetof(Info, depth), &depth, sizeof(depth));
        return true;
    }
    return false;
}

void* HgTexture::get_pixels() {
    if (file.size >= sizeof(Info) && std::memcmp(file.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        return (u8*)file.data + sizeof(Info);
    }
    return file.data;
}

HgResourceManager HgResourceManager::create(HgArena& arena, usize capacity) {
    HgResourceManager rm;
    rm.pool = arena.alloc<HgBinary>(capacity);
    rm.free_list = arena.alloc<usize>(capacity);
    rm.capacity = capacity;
    rm.first = 0;
    rm.map = rm.map.create(arena, capacity);
    rm.reset();
    return rm;
}

void HgResourceManager::reset() {
    map.for_each([&](HgResourceID id, Resource& res) {
        if (res.ref_count != 0) {
            res.ref_count = 1;
            unload(nullptr, 0, id);
        }
    });
    map.reset();
    for (usize i = 0; i < capacity; ++i) {
        free_list[i] = i + 1;
    }
    first = 0;
}

void HgResourceManager::register_resource(HgResourceID id) {
    if (!map.has(id)) {
        usize idx = first;
        HgBinary* bin = pool + idx;
        *bin = {};

        map.insert(id, {bin, 0});
        first = free_list[idx];
        free_list[idx] = idx;
    }
}

void HgResourceManager::unregister_resource(HgResourceID id) {
    Resource* r = map.get(id);
    if (r != nullptr) {
        if (r->ref_count > 0) {
            r->ref_count = 1;
            unload(nullptr, 0, id);
        }
        usize idx = (uptr)(r->file - pool);
        free_list[idx] = first;
        first = idx;

        map.remove(id);
    }
}

void HgResourceManager::load(HgFence* fences, usize fence_count, HgStringView path) {
    auto fn = [](void* pres, HgStringView fpath) {
        Resource& res = *(Resource*)pres;

        hg_arena_scope(scratch, hg_get_scratch());
        HgString cpath = cpath.create(scratch, fpath).append(scratch, 0);

        FILE* file_handle = std::fopen(cpath.chars, "rb");
        if (file_handle == nullptr) {
            hg_warn("Could not find file to read binary: %s\n", cpath.chars);
            return;
        }
        hg_defer(std::fclose(file_handle));

        if (std::fseek(file_handle, 0, SEEK_END) != 0) {
            hg_warn("Failed to read binary from file: %s\n", cpath.chars);
            return;
        }
        res.file->size = (usize)std::ftell(file_handle);
        res.file->data = std::malloc(res.file->size);
        std::rewind(file_handle);

        if (std::fread(res.file->data, 1, res.file->size, file_handle) != res.file->size) {
            hg_warn("Failed to read binary from file: %s\n", cpath.chars);
            std::free(res.file->data);
            res.file->size = 0;
            res.file->data = nullptr;
            return;
        }
    };

    HgResourceID id = hg_resource_id(path);
    hg_assert(is_registered(id));

    Resource& res = *map.get(id);
    if (res.ref_count++ > 0)
        return;

    hg_io_request(fences, fence_count, &res, path, fn);
}

void HgResourceManager::unload(HgFence* fences, usize fence_count, HgResourceID id) {
    hg_assert(is_registered(id));
    Resource& res = *map.get(id);
    if (--res.ref_count > 0)
        return;

    hg_io_request(fences, fence_count, &res, {}, [](void* pres, HgStringView) {
        Resource& rres = *(Resource*)pres;
        std::free(rres.file->data);
        rres.file->size = 0;
        rres.file->data = nullptr;
    });
}

void HgResourceManager::store(HgFence* fences, usize fence_count, HgResourceID id, HgStringView path) {
    hg_assert(is_registered(id));
    hg_io_request(fences, fence_count, map.get(id), path, [](void* pres, HgStringView fpath) {
        (*(Resource*)pres).file->store(fpath);
    });
}

void hg_import_png(HgFence* fences, usize fence_count, HgStringView path) {
    HgResourceID id = hg_resource_id(path);
    hg_assert(hg_resources->is_registered(id));

    hg_resources->load(fences, fence_count, path);

    hg_io_request(fences, fence_count, &hg_resources->get(id), path, [](void* ptex, HgStringView fpath) {
        HgBinary& bin = *(HgBinary*)ptex;

        int width, height, channels;
        u8* pixels = stbi_load_from_memory((u8*)bin.data, (int)bin.size, &width, &height, &channels, 4);
        if (pixels == nullptr) {
            hg_arena_scope(scratch, hg_get_scratch());
            hg_warn("Failed to decode image file: %s\n", HgString::create(scratch, fpath).append(scratch, 0).chars);
            return;
        }

        HgBinary new_bin;
        new_bin.size = sizeof(HgTexture::Info) + (usize)width * (usize)height * sizeof(u32);
        new_bin.data = std::malloc(new_bin.size);

        HgTexture::Info info;
        std::memcpy(info.identifier, HgTexture::texture_identifier, sizeof(HgTexture::texture_identifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;

        new_bin.overwrite(0, info);
        new_bin.overwrite(sizeof(info), pixels, (usize)width * (usize)height * sizeof(u32));

        std::free(pixels);
        std::free(bin.data);
        bin = new_bin;
    });
}

void hg_export_png(HgFence* fences, usize fence_count, HgResourceID id, HgStringView path) {
    hg_assert(hg_resources->is_registered(id));
    hg_io_request(fences, fence_count, &hg_resources->get(id), path, [](void* ptexture, HgStringView fpath) {
        HgTexture tex = *(HgBinary*)ptexture;

        hg_arena_scope(scratch, hg_get_scratch());
        HgString cpath = cpath.create(scratch, fpath).append(scratch, 0);

        void* pixels = tex.get_pixels();
        if (pixels == nullptr) {
            hg_warn("Cannot export emprty image %s\n", cpath.chars);
            return;
        }

        VkFormat format;
        u32 width, height, depth;
        if (!tex.get_info(format, width, height, depth)) {
            hg_warn("Could not get info from image %s to export\n", cpath.chars);
            return;
        }
        if (depth > 1)
            hg_warn("Cannot export 3d image %s, exporting only the first layer\n", cpath.chars);
        stbi_write_png(cpath.chars, (int)width, (int)height, 4, pixels, (int)(width * sizeof(u32)));
    });
}

HgGpuResourceManager HgGpuResourceManager::create(HgArena& arena, usize max_resources) {
    HgGpuResourceManager rm;
    rm.map = rm.map.create(arena, max_resources);
    return rm;
}

void HgGpuResourceManager::reset() {
    map.for_each([&](HgResourceID id, Resource& r) {
        if (r.ref_count != 0) {
            r.ref_count = 1;
            unload(id);
        }
    });
    map.reset();
}

void HgGpuResourceManager::register_buffer(HgResourceID id) {
    if (!map.has(id)) {
        Resource* r = map.get(id);
        if (r != nullptr) {
            if (r->type != Resource::Type::buffer)
                hg_warn("Attempted to register gpu resource not of type buffer as a buffer\n");
        } else {
            r = &map.insert(id, {});
            r->type = Resource::Type::buffer;
        }
    }
}

void HgGpuResourceManager::register_texture(HgResourceID id) {
    if (!map.has(id)) {
        Resource* r = map.get(id);
        if (r != nullptr) {
            if (r->type != Resource::Type::texture)
                hg_warn("Attempted to register gpu resource not of type texture as a texture\n");
        } else {
            r = &map.insert(id, {});
            r->type = Resource::Type::texture;
        }
    }
}

void HgGpuResourceManager::unregister_resource(HgResourceID id) {
    Resource* r = map.get(id);
    if (r != nullptr) {
        if (r->ref_count > 0) {
            r->ref_count = 1;
            unload(id);
        }
        map.remove(id);
    }
}

HgGpuBuffer& HgGpuResourceManager::get_buffer(HgResourceID id) {
    Resource* r = map.get(id);
    hg_assert(r != nullptr);
    if (r->type != Resource::Type::buffer)
        hg_warn("Accessing non buffer as gpu buffer\n");
    return r->buffer;
}

HgGpuTexture& HgGpuResourceManager::get_texture(HgResourceID id) {
    Resource* r = map.get(id);
    hg_assert(r != nullptr);
    if (r->type != Resource::Type::texture)
        hg_warn("Accessing non texture as gpu texture\n");
    return r->texture;
}

void HgGpuResourceManager::load_from_cpu(VkCommandPool cmd_pool, HgResourceID id, VkFilter filter) {
    hg_assert(is_registered(id));

    Resource& r = *map.get(id);
    if (r.ref_count++ > 0)
        return;

    switch (r.type) {
        case Resource::Type::buffer: // : TODO
            break;
        case Resource::Type::texture: {
            HgTexture data = hg_resources->get(id);
            hg_assert(data.file.data != nullptr);

            HgGpuTexture& tex = get_texture(id);
            if (!data.get_info(tex.format, tex.width, tex.height, tex.depth)) {
                hg_warn("Could not get info to load texture\n");
                return;
            }
            hg_assert(tex.format != 0);
            hg_assert(tex.width != 0);
            hg_assert(tex.height != 0);
            hg_assert(tex.depth != 0);

            VkImageCreateInfo image_info{};
            image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.format = tex.format;
            image_info.extent = {tex.width, tex.height, tex.depth};
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            VmaAllocationCreateInfo alloc_info{};
            alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

            vmaCreateImage(hg_vk_vma, &image_info, &alloc_info, &tex.image, &tex.allocation, nullptr);
            hg_assert(tex.allocation != nullptr);
            hg_assert(tex.image != nullptr);

            HgVkImageStagingWriteConfig staging_config{};
            staging_config.dst_image = tex.image;
            staging_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            staging_config.subresource.layerCount = 1;
            staging_config.src_data = data.get_pixels();
            staging_config.width = tex.width;
            staging_config.height = tex.height;
            staging_config.depth = tex.depth;
            staging_config.format = tex.format;
            staging_config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            hg_vk_image_staging_write(hg_vk_queue, cmd_pool, staging_config);

            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = tex.image;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = tex.format;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;

            vkCreateImageView(hg_vk_device, &view_info, nullptr, &tex.view);
            hg_assert(tex.view != nullptr);

            VkSamplerCreateInfo sampler_info{};
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter = filter;
            sampler_info.minFilter = filter;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            vkCreateSampler(hg_vk_device, &sampler_info, nullptr, &tex.sampler);
            hg_assert(tex.sampler != nullptr);
        } break;
        default:
            hg_assert(false);
    }
}

void HgGpuResourceManager::load_from_disc(VkCommandPool cmd_pool, HgStringView path, VkFilter filter) {
    HgResourceID id = hg_resource_id(path);
    hg_assert(is_registered(id));

    if (!is_loaded(id)) {
        HgFence fence;
        hg_resources->register_resource(id);
        hg_resources->load(&fence, 1, path);
        fence.wait(INFINITY);
        load_from_cpu(cmd_pool, id, filter);
        hg_resources->unload(nullptr, 0, id);
    }
}

void HgGpuResourceManager::unload(HgResourceID id) {
    hg_assert(is_registered(id));

    Resource& r = *map.get(id);
    if (--r.ref_count > 0)
        return;

    switch (r.type) {
        case Resource::Type::buffer:
            vmaDestroyBuffer(hg_vk_vma, r.buffer.buffer, r.buffer.allocation);
            r.buffer = {};
            break;
        case Resource::Type::texture:
            vkDestroySampler(hg_vk_device, r.texture.sampler, nullptr);
            vkDestroyImageView(hg_vk_device, r.texture.view, nullptr);
            vmaDestroyImage(hg_vk_vma, r.texture.image, r.texture.allocation);
            r.texture = {};
            break;
        default:
            hg_assert(false);
    }
}

