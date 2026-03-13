#include "hurdygurdy.hpp"

#include "model.vert.spv.h"
#include "model.frag.spv.h"

struct GlobalUniform {
    HgMat4 proj;
    HgMat4 view;
    u32 dir_light_count;
    u32 point_light_count;
};

struct Push {
    HgMat4 model;
};

struct DirLightData {
    HgVec4 dir;
    HgVec4 color;
};

struct PointLightData {
    HgVec4 pos;
    HgVec4 color;
};

static VkDescriptorSetLayout global_set_layout;
static VkDescriptorSetLayout texture_set_layout;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

static VkDescriptorPool descriptor_pool;
static VkDescriptorSet global_set;

static GlobalUniform global_data;

static VkBuffer global_buffer;
static VmaAllocation global_buffer_alloc;

static HgHashMap<HgResource, VkDescriptorSet> model_map_sets;

static u32 dir_light_capacity;
static u32 point_light_capacity;
static VkBuffer dir_light_buffer;
static VkBuffer point_light_buffer;
static VmaAllocation dir_light_buffer_alloc;
static VmaAllocation point_light_buffer_alloc;

static VkImage default_color_map_image;
static VmaAllocation default_color_map_alloc;
static VkImageView default_color_map_view;

static VkImage default_normal_map_image;
static VkImageView default_normal_map_view;
static VmaAllocation default_normal_map_alloc;

static VkSampler default_map_sampler;

static VkBuffer default_model_vertex_buffer;
static VmaAllocation default_model_vertex_alloc;

static VkBuffer default_model_index_buffer;
static VmaAllocation default_model_index_alloc;

void hg_pipeline_3d_init(
    HgArena& arena,
    u32 max_models,
    VkFormat color_format,
    VkFormat depth_format
) {
    hg_assert(color_format != VK_FORMAT_UNDEFINED);
    hg_assert(depth_format != VK_FORMAT_UNDEFINED);

    model_map_sets = model_map_sets.create(arena, max_models);

    VkDescriptorSetLayoutBinding global_set_bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    global_set_layout = hg_vk_create_descriptor_set_layout(
        global_set_bindings, sizeof(global_set_bindings) / sizeof(*global_set_bindings));

    VkDescriptorSetLayoutBinding texture_set_bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    texture_set_layout = hg_vk_create_descriptor_set_layout(
        texture_set_bindings, sizeof(texture_set_bindings) / sizeof(*texture_set_bindings));

    VkDescriptorSetLayout set_layouts[] = {global_set_layout, texture_set_layout};
    VkPushConstantRange push_range = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push)};
    pipeline_layout = hg_vk_create_pipeline_layout(
        set_layouts, sizeof(set_layouts) / sizeof(*set_layouts),
        &push_range, 1);

    VkShaderModule vertex_shader = hg_vk_create_shader_module(model_vert_spv, model_vert_spv_size);
    VkShaderModule fragment_shader = hg_vk_create_shader_module(model_frag_spv, model_frag_spv_size);
    hg_defer(vkDestroyShaderModule(hg_vk_device, vertex_shader, nullptr));
    hg_defer(vkDestroyShaderModule(hg_vk_device, fragment_shader, nullptr));

    VkVertexInputBindingDescription vertex_bindings[] = {
        {0, sizeof(HgVertex), VK_VERTEX_INPUT_RATE_VERTEX},
    };
    VkVertexInputAttributeDescription vertex_attributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgVertex, norm)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(HgVertex, tan)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(HgVertex, uv)},
    };

    HgVkPipelineConfig pipeline_config{};
    pipeline_config.color_attachment_formats = &color_format;
    pipeline_config.color_attachment_format_count = 1;
    pipeline_config.depth_attachment_format = depth_format;
    pipeline_config.vertex_shader = vertex_shader;
    pipeline_config.fragment_shader = fragment_shader;
    pipeline_config.layout = pipeline_layout;
    pipeline_config.vertex_bindings = vertex_bindings;
    pipeline_config.vertex_binding_count = sizeof(vertex_bindings) / sizeof(*vertex_bindings);
    pipeline_config.vertex_attributes = vertex_attributes;
    pipeline_config.vertex_attribute_count = sizeof(vertex_attributes) / sizeof(*vertex_attributes);
    pipeline_config.cull_mode = VK_CULL_MODE_BACK_BIT;

    pipeline = hg_vk_create_graphics_pipeline(pipeline_config);

    VkDescriptorPoolSize descriptor_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_models * 2}
    };
    descriptor_pool = hg_vk_create_descriptor_pool(
        max_models + 1,
        descriptor_sizes,
        sizeof(descriptor_sizes) / sizeof(*descriptor_sizes),
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    global_set = hg_vk_allocate_descriptor_set(descriptor_pool, global_set_layout);
    hg_assert(global_set != nullptr);

    hg_vk_create_buffer(&global_buffer, &global_buffer_alloc, sizeof(GlobalUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    global_data.proj = 1.0f;
    global_data.view = 1.0f;
    global_data.dir_light_count = 0;
    global_data.point_light_count = 0;

    vmaCopyMemoryToAllocation(hg_vk_vma, &global_data, global_buffer_alloc, 0, sizeof(global_data));

    dir_light_capacity = 1;
    hg_vk_create_buffer(&dir_light_buffer, &dir_light_buffer_alloc, sizeof(DirLightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    point_light_capacity = 1;
    hg_vk_create_buffer(&point_light_buffer, &point_light_buffer_alloc, sizeof(PointLightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    VkDescriptorBufferInfo dir_light_buffer_info = {dir_light_buffer, 0, VK_WHOLE_SIZE};
    hg_vk_update_descriptor_set(global_set, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &dir_light_buffer_info, 1);

    VkDescriptorBufferInfo point_light_buffer_info = {point_light_buffer, 0, VK_WHOLE_SIZE};
    hg_vk_update_descriptor_set(global_set, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &point_light_buffer_info, 1);

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = global_buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(GlobalUniform);

    hg_vk_update_descriptor_set(global_set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info, 1);

    struct Color {
        u8 r, g, b, a;
    };

    static const Color default_colors[] = {
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    HgVkImageConfig color_image_config{};
    color_image_config.width = 3;
    color_image_config.height = 3;
    color_image_config.format = VK_FORMAT_R8G8B8A8_SRGB;
    color_image_config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    hg_vk_create_image(&default_color_map_image, &default_color_map_alloc, color_image_config);

    HgVkImageViewConfig color_view_config{};
    color_view_config.image = default_color_map_image;
    color_view_config.format = color_image_config.format;
    color_view_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    default_color_map_view = hg_vk_create_image_view(color_view_config);

    HgVkImageStagingWriteConfig color_write{};
    color_write.dst_image = default_color_map_image;
    color_write.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_write.src_data = default_colors;
    color_write.width = color_image_config.width;
    color_write.height = color_image_config.height;
    color_write.format = color_image_config.format;
    color_write.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(color_write);

    static const HgVec4 default_normals[] = {
        {0, 0, -1, 0}, {0, 0, -1, 0},
        {0, 0, -1, 0}, {0, 0, -1, 0},
    };

    HgVkImageConfig normal_image_config{};
    normal_image_config.width = 2;
    normal_image_config.height = 2;
    normal_image_config.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    normal_image_config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    hg_vk_create_image(&default_normal_map_image, &default_normal_map_alloc, normal_image_config);

    HgVkImageViewConfig normal_view_config{};
    normal_view_config.image = default_normal_map_image;
    normal_view_config.format = normal_image_config.format;
    normal_view_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    default_normal_map_view = hg_vk_create_image_view(normal_view_config);

    HgVkImageStagingWriteConfig normal_write{};
    normal_write.dst_image = default_normal_map_image;
    normal_write.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    normal_write.src_data = default_normals;
    normal_write.width = normal_image_config.width;
    normal_write.height = normal_image_config.height;
    normal_write.format = normal_image_config.format;
    normal_write.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(normal_write);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkResult sampler_result = vkCreateSampler(hg_vk_device, &sampler_info, nullptr, &default_map_sampler);
    if (sampler_result != VK_SUCCESS)
        hg_error("Could not create sampler: %s\n", hg_vk_result_to_string(sampler_result));

    static const HgVertex cube_vertices[] = {
        {{ 0.5f,-0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,0}},
        {{ 0.5f, 0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,1}},
        {{-0.5f,-0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,1}},

        {{-0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,0}},
        {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,1}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,1}},

        {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,1}},
    };

    static u32 cube_indices[] = {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    hg_vk_create_buffer(&default_model_vertex_buffer, &default_model_vertex_alloc, sizeof(cube_vertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    hg_vk_create_buffer(&default_model_index_buffer, &default_model_index_alloc, sizeof(cube_indices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    hg_vk_buffer_staging_write(default_model_vertex_buffer, 0, cube_vertices, sizeof(cube_vertices));
    hg_vk_buffer_staging_write(default_model_index_buffer, 0, cube_indices, sizeof(cube_indices));
}

void hg_pipeline_3d_deinit() {
    vmaDestroyBuffer(hg_vk_vma, default_model_index_buffer, default_model_index_alloc);
    vmaDestroyBuffer(hg_vk_vma, default_model_vertex_buffer, default_model_vertex_alloc);
    vkDestroySampler(hg_vk_device, default_map_sampler, nullptr);
    vkDestroyImageView(hg_vk_device, default_normal_map_view, nullptr);
    vmaDestroyImage(hg_vk_vma, default_normal_map_image, default_normal_map_alloc);
    vkDestroyImageView(hg_vk_device, default_color_map_view, nullptr);
    vmaDestroyImage(hg_vk_vma, default_color_map_image, default_color_map_alloc);
    vmaDestroyBuffer(hg_vk_vma, point_light_buffer, point_light_buffer_alloc);
    vmaDestroyBuffer(hg_vk_vma, dir_light_buffer, dir_light_buffer_alloc);
    vmaDestroyBuffer(hg_vk_vma, global_buffer, global_buffer_alloc);
    vkDestroyDescriptorPool(hg_vk_device, descriptor_pool, nullptr);
    vkDestroyPipeline(hg_vk_device, pipeline, nullptr);
    vkDestroyPipelineLayout(hg_vk_device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, texture_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, global_set_layout, nullptr);
}

void hg_pipeline_3d_add_model(HgResource model_id, HgResource color_id, HgResource normal_id) {
    HgGpuTexture* color_map = hg_get_gpu_texture(color_id);
    HgGpuTexture* normal_map = hg_get_gpu_texture(normal_id);

    VkDescriptorSet set = hg_vk_allocate_descriptor_set(descriptor_pool, texture_set_layout);

    VkDescriptorImageInfo image_infos[2]{};
    image_infos[0].sampler = color_map != nullptr ? color_map->sampler : default_map_sampler;
    image_infos[0].imageView = color_map != nullptr ? color_map->view : default_color_map_view;
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_infos[1].sampler = normal_map != nullptr ? normal_map->sampler : default_map_sampler;
    image_infos[1].imageView = normal_map != nullptr ? normal_map->view : default_normal_map_view;
    image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_update_descriptor_set(set, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_infos, 2);

    model_map_sets.insert(model_id, set);
}

void hg_pipeline_3d_remove_model(HgResource model_id) {
    VkDescriptorSet set;
    if (model_map_sets.remove(model_id, &set))
        vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, &set);
}

void hg_pipeline_3d_update_projection(const HgMat4& projection) {
    global_data.proj = projection;
}

void hg_pipeline_3d_update_view(const HgMat4& view) {
    global_data.view = view;
}

void hg_draw_3d(HgECS& ecs, VkCommandBuffer cmd) {
    hg_assert(cmd != nullptr);

    HgArena& scratch = hg_get_scratch();
    HgArenaScope scratch_scope{scratch};

    global_data.dir_light_count = ecs.count<HgDirLight3D>();
    global_data.point_light_count = ecs.count<HgPointLight3D>();

    if (global_data.dir_light_count > dir_light_capacity) {
        u32 new_capacity = dir_light_capacity == 0 ? 1 : dir_light_capacity * 2;
        vmaDestroyBuffer(hg_vk_vma, dir_light_buffer, dir_light_buffer_alloc);
        hg_vk_create_buffer(&dir_light_buffer, &dir_light_buffer_alloc, sizeof(DirLightData) * new_capacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        dir_light_capacity = new_capacity;
    }

    if (global_data.point_light_count > point_light_capacity) {
        u32 new_capacity = point_light_capacity == 0 ? 1 : point_light_capacity * 2;
        vmaDestroyBuffer(hg_vk_vma, point_light_buffer, point_light_buffer_alloc);
        hg_vk_create_buffer(&point_light_buffer, &point_light_buffer_alloc, sizeof(PointLightData) * new_capacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        point_light_capacity = new_capacity;
    }

    DirLightData* dir_lights = scratch.alloc<DirLightData>(global_data.dir_light_count);
    PointLightData* point_lights = scratch.alloc<PointLightData>(global_data.point_light_count);

    u32 i = 0;
    ecs.for_each<HgDirLight3D>([&](HgEntity, HgDirLight3D& light) {
        dir_lights[i].dir = light.dir;
        dir_lights[i].dir.w = 0.0f;
        dir_lights[i].color = light.color;
        dir_lights[i].color.w = 1.0f;
        ++i;
    });

    i = 0;
    ecs.for_each<HgPointLight3D, HgTransform>([&](HgEntity, HgPointLight3D& light, HgTransform& transform) {
        point_lights[i].pos = transform.position;
        point_lights[i].pos.w = 0.0f;
        point_lights[i].color = light.color;
        point_lights[i].color.w = 1.0f;
        ++i;
    });

    if (global_data.dir_light_count > 0)
        vmaCopyMemoryToAllocation(
            hg_vk_vma,
            dir_lights,
            dir_light_buffer_alloc,
            0,
            sizeof(*dir_lights) * global_data.dir_light_count);

    if (global_data.point_light_count > 0)
        vmaCopyMemoryToAllocation(
            hg_vk_vma,
            point_lights,
            point_light_buffer_alloc,
            0,
            sizeof(*point_lights) * global_data.point_light_count);

    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &global_data,
        global_buffer_alloc,
        0,
        sizeof(global_data));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &global_set, 0, nullptr);

    ecs.for_each<HgModel3D, HgTransform>([&](HgEntity, HgModel3D& model, HgTransform& transform) {
        hg_assert(model_map_sets.has(model.model));

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            1,
            1,
            model_map_sets.get(model.model),
            0,
            nullptr);

        Push push{};
        push.model = hg_model_matrix_3d(transform.position, transform.scale, transform.rotation);

        HgGpuBuffer* vertex_buffer = hg_get_gpu_buffer(model.model);
        if (vertex_buffer != nullptr) {
            // VkBuffer buffers[] = {vertex_buffer->buffer};
            // VkDeviceSize offsets[] = {0};
            // vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
            hg_error("3d model vertex buffer : TODO\n");
        } else {
            vkCmdBindIndexBuffer(cmd, default_model_index_buffer, 0, VK_INDEX_TYPE_UINT32);

            VkBuffer buffers[] = {default_model_vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        }

        vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
    });
}

