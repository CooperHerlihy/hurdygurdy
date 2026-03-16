#include "hurdygurdy.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

HgResourceManager HgResourceManager::create(usize resource_width, usize capacity) {
    hg_assert(capacity > 0);

    HgResourceManager rm;
    rm.ref_counts = (u32*)malloc(capacity * sizeof(u32));
    rm.ids = (HgResource*)malloc(capacity * sizeof(HgResource));
    rm.resources = malloc(capacity * resource_width);
    rm.resource_width = resource_width;
    rm.capacity = capacity;
    rm.reset();
    return rm;
}

void HgResourceManager::destroy() {
    free(ref_counts);
    free(ids);
    free(resources);
}

void HgResourceManager::reset() {
    for (usize i = 0; i < capacity; ++i) {
        ref_counts[i] = (u32)-1;
    }
    count = 0;
}

void HgResourceManager::resize(usize new_size) {
    HgResourceManager old = *this;
    hg_defer(old.destroy());

    *this = create(resource_width, new_size);
    for (usize i = 0; i < old.capacity; ++i) {
        if (old.ref_counts[i] != (u32)-1) {
            usize idx = add(old.ids[i]);
            ref_counts[idx] = old.ref_counts[i];
            memcpy(
                (u8*)resources + idx * resource_width,
                (u8*)old.resources + i * resource_width,
                resource_width);
        }
    }
}

usize HgResourceManager::add(HgResource id) {
    if ((f32)(count + 1) >= (f32)capacity * 0.5f)
        resize(capacity * 2);

    usize idx = id % capacity;
    while (ref_counts[idx] != (u32)-1) {
        if (ids[idx] == id)
            return idx;
        idx = (idx + 1) % capacity;
    }

    ref_counts[idx] = 0;
    ids[idx] = id;
    ++count;

    return idx;
}

void HgResourceManager::remove(HgResource id) {
    usize idx = get_idx(id);
    if (idx == (usize)-1)
        return;

    usize next = (idx + 1) % capacity;
    while (ref_counts[next] % capacity != (u32)-1) {
        if (ids[next] % capacity != idx) {
            ref_counts[idx] = ref_counts[next];
            ids[idx] = ids[next];
            memcpy(
                (u8*)resources + idx * resource_width,
                (u8*)resources + next * resource_width,
                resource_width);
            idx = next;
        }
        next = (next + 1) % capacity;
    }
    ref_counts[idx] = (u32)-1;
    --count;
}

bool HgResourceManager::load(HgResource id) {
    return ref_counts[add(id)]++ == 0;
}

bool HgResourceManager::unload(HgResource id) {
    usize idx = get_idx(id);
    return idx == (usize)-1 || ref_counts[idx] == 0
        ? false
        : --ref_counts[idx] == 0;
}

usize HgResourceManager::get_idx(HgResource id) {
    for (usize idx = id % capacity; ref_counts[idx] != (u32)-1; idx = (idx + 1) % capacity) {
        if (ids[idx] == id)
            return idx;
    }
    return (usize)-1;
}

void* HgResourceManager::get(HgResource id) {
    usize idx = get_idx(id);
    return idx == (usize)-1 ? nullptr : (u8*)resources + idx * resource_width;
}

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

HgResourceManager bin_resources{};

void hg_resources_init() {
    bin_resources = bin_resources.create(sizeof(HgBinary*));
}

void hg_resources_deinit() {
    bin_resources.for_each([](HgResource, void* pres) {
        HgBinary* bin = (HgBinary*)pres;
        free(bin->data);
        free(bin);
    });
    bin_resources.destroy();
}

void hg_load_empty_resource(HgResource id) {
    if (bin_resources.load(id)) {
        HgBinary** bin = (HgBinary**)bin_resources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};
    }
}

void hg_load_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    if (bin_resources.load(id)) {
        HgBinary** bin = (HgBinary**)bin_resources.get(id);
        *bin = (HgBinary*)malloc(sizeof(HgBinary));
        **bin = {};

        hg_io_request(fences, fence_count, *bin, path, [](void* pres, HgStringView fpath) {
            HgBinary& bin = *(HgBinary*)pres;

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
            bin.size = (usize)ftell(file_handle);
            bin.data = malloc(bin.size);
            rewind(file_handle);

            if (fread(bin.data, 1, bin.size, file_handle) != bin.size) {
                hg_warn("Failed to read binary from file: %s\n", cpath);
                free(bin.data);
                bin.size = 0;
                bin.data = nullptr;
                return;
            }
        });
    }
}

void hg_unload_resource(HgFence* fences, usize fence_count, HgResource id) {
    if (bin_resources.unload(id)) {
        HgBinary** bin = (HgBinary**)bin_resources.get(id);

        hg_io_request(fences, fence_count, *bin, {}, [](void* pres, HgStringView) {
            HgBinary& bin = *(HgBinary*)pres;
            free(bin.data);
            bin = {};
        });
    }
}

void hg_store_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    HgBinary** res = (HgBinary**)bin_resources.get(id);
    if (res == nullptr || *res == nullptr) {
        HgArena& scratch = hg_get_scratch();
        HgArenaScope scratch_scope{scratch};
        hg_warn("Could not store resource to \"%s\" because the resource does not exist\n",
            hg_c_string(scratch, path));
        return;
    }

    hg_io_request(fences, fence_count, *res, path, [](void* pres, HgStringView fpath) {
        (*(HgBinary*)pres).store(fpath);
    });
}

HgBinary* hg_get_resource(HgResource id) {
    HgBinary** res = (HgBinary**)bin_resources.get(id);
    return res == nullptr ? nullptr : *res;
}

bool HgTextureData::get_info(VkFormat* format, u32* width, u32* height, u32* depth) {
    if (bin.size >= sizeof(Info) && memcmp(bin.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        bin.read(offsetof(Info, format), format, sizeof(*format));
        bin.read(offsetof(Info, width), width, sizeof(*width));
        bin.read(offsetof(Info, height), height, sizeof(*height));
        bin.read(offsetof(Info, depth), depth, sizeof(*depth));
        return true;
    }
    return false;
}

void* HgTextureData::get_pixels() {
    if (bin.size >= sizeof(Info) && memcmp(bin.data, texture_identifier, sizeof(texture_identifier)) == 0) {
        return (u8*)bin.data + bin.read<usize>(offsetof(Info, pixels_begin));
    }
    return bin.data;
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

        bin.size = sizeof(HgTextureData::Info) + (usize)width * (usize)height * sizeof(u32);
        bin.data = malloc(bin.size);

        HgTextureData::Info info;
        memcpy(info.identifier, HgTextureData::texture_identifier, sizeof(HgTextureData::texture_identifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = (u32)width;
        info.height = (u32)height;
        info.depth = 1;
        info.pixels_begin = sizeof(info);

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

        HgTextureData tex = *(HgBinary*)ptex;
        void* pixels = tex.get_pixels();
        if (pixels == nullptr) {
            hg_warn("Cannot export empty image %s\n", cpath);
            return;
        }

        VkFormat format;
        u32 width, height, depth;
        if (!tex.get_info(&format, &width, &height, &depth)) {
            hg_warn("Could not get info from image %s to export\n", cpath);
            return;
        }
        if (depth > 1)
            hg_warn("Cannot export 3d image %s, exporting only the first layer\n", cpath);
        stbi_write_png(cpath, (int)width, (int)height, 4, pixels, (int)(width * sizeof(u32)));
    });
}

bool HgModelData::get_info(u32* vertex_count, u32* vertex_width, u32* index_count, VkPrimitiveTopology* topology) {
    if (file.size >= sizeof(Info) && memcmp(file.data, model_identifier, sizeof(model_identifier)) == 0) {
        file.read(offsetof(Info, vertex_count), vertex_count, sizeof(vertex_count));
        file.read(offsetof(Info, vertex_width), vertex_width, sizeof(vertex_width));
        file.read(offsetof(Info, index_count), index_count, sizeof(index_count));
        file.read(offsetof(Info, topology), topology, sizeof(topology));
        return true;
    }
    return false;
}

void* HgModelData::get_vertex_data() {
    if (file.size >= sizeof(Info) && memcmp(file.data, model_identifier, sizeof(model_identifier)) == 0) {
        return (u8*)file.data + file.read<usize>(offsetof(Info, vertices_begin));
    }
    return file.data;
}

void* HgModelData::get_index_data() {
    if (file.size >= sizeof(Info) && memcmp(file.data, model_identifier, sizeof(model_identifier)) == 0) {
        return (u8*)file.data + file.read<usize>(offsetof(Info, indices_begin));
    }
    return file.data;
}

void hg_import_gltf(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    (void)fences;
    (void)fence_count;
    (void)id;
    (void)path;
    hg_error("hg_import_gltf : TODO\n");
}

void hg_export_gltf(HgFence* fences, usize fence_count, HgResource id, HgStringView path) {
    (void)fences;
    (void)fence_count;
    (void)id;
    (void)path;
    hg_error("hg_export_gltf : TODO\n");
}

HgResourceManager gpu_textures{};

void hg_gpu_textures_init() {
    gpu_textures = gpu_textures.create(sizeof(HgGpuTexture));
}

void hg_gpu_textures_deinit() {
    gpu_textures.for_each([](HgResource, void* ptex) {
        HgGpuTexture& tex = *(HgGpuTexture*)ptex;
        vkDestroySampler(hg_vk_device, tex.sampler, nullptr);
        vkDestroyImageView(hg_vk_device, tex.view, nullptr);
        vmaDestroyImage(hg_vk_vma, tex.image, tex.allocation);
    });
    gpu_textures.destroy();
}

void hg_load_gpu_texture(HgResource id, VkFilter filter) {
    if (gpu_textures.load(id)) {
        hg_assert(hg_get_resource(id) != nullptr);
        hg_assert(hg_get_resource(id)->data != nullptr);
        HgTextureData data = *hg_get_resource(id);

        HgGpuTexture& tex = *(HgGpuTexture*)gpu_textures.get(id);;
        if (!data.get_info(&tex.format, &tex.width, &tex.height, &tex.depth)) {
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

        tex.view = hg_vk_create_image_view(view_info);
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
}

void hg_unload_gpu_texture(HgResource id) {
    if (gpu_textures.unload(id)) {
        HgGpuTexture& tex = *(HgGpuTexture*)gpu_textures.get(id);
        vkDestroySampler(hg_vk_device, tex.sampler, nullptr);
        vkDestroyImageView(hg_vk_device, tex.view, nullptr);
        vmaDestroyImage(hg_vk_vma, tex.image, tex.allocation);
        tex = {};
    }
}

HgGpuTexture* hg_get_gpu_texture(HgResource id) {
    return (HgGpuTexture*)gpu_textures.get(id);
}

HgResourceManager gpu_models{};

void hg_gpu_models_init() {
    gpu_models = gpu_models.create(sizeof(HgGpuModel));
}

void hg_gpu_models_deinit() {
    gpu_models.for_each([](HgResource, void* pmodel) {
        HgGpuModel& model = *(HgGpuModel*)pmodel;
        vmaDestroyBuffer(hg_vk_vma, model.vertex_buffer, model.vertex_alloc);
        vmaDestroyBuffer(hg_vk_vma, model.index_buffer, model.index_alloc);
    });
    gpu_models.destroy();
}

void hg_load_gpu_model(HgResource id) {
    if (gpu_models.load(id)) {
        hg_assert(hg_get_resource(id) != nullptr);
        hg_assert(hg_get_resource(id)->data != nullptr);
        HgModelData data = *hg_get_resource(id);

        VkPrimitiveTopology topology;
        HgGpuModel& model = *(HgGpuModel*)gpu_models.get(id);
        if (!data.get_info(
            &model.vertex_count,
            &model.vertex_width,
            &model.index_count,
            &topology)
        ) {
            hg_warn("Could not get info to load model\n");
            return;
        }

        hg_vk_create_buffer(
            &model.vertex_buffer,
            &model.vertex_alloc,
            model.vertex_count * model.vertex_width,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hg_vk_create_buffer(
            &model.index_buffer,
            &model.index_alloc,
            model.index_count * sizeof(u32),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        hg_vk_buffer_staging_write(
            model.vertex_buffer, 0,
            data.get_vertex_data(), model.vertex_count * model.vertex_width);

        hg_vk_buffer_staging_write(
            model.index_buffer, 0,
            data.get_index_data(), model.index_count * sizeof(u32));
    }
}

void hg_unload_gpu_model(HgResource id) {
    if (gpu_models.unload(id)) {
        HgGpuModel& model = *(HgGpuModel*)gpu_models.get(id);        vmaDestroyBuffer(hg_vk_vma, model.vertex_buffer, model.vertex_alloc);
        vmaDestroyBuffer(hg_vk_vma, model.index_buffer, model.index_alloc);
    }
}

HgGpuModel* hg_get_gpu_model(HgResource id) {
    return (HgGpuModel*)gpu_models.get(id);
}

void hg_gpu_resources_init() {
    hg_gpu_textures_init();
    hg_gpu_models_init();
}

void hg_gpu_resources_deinit() {
    hg_gpu_models_deinit();
    hg_gpu_textures_deinit();
}

