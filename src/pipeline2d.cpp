#include "hurdygurdy.hpp"

#include "sprite.frag.spv.h"
#include "sprite.vert.spv.h"

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

struct Push {
    HgMat4 model;
    HgVec2 uv_pos;
    HgVec2 uv_size;
};

VkDescriptorSetLayout vp_layout;
VkDescriptorSetLayout texture_layout;
VkPipelineLayout pipeline_layout;
VkPipeline pipeline;

VkDescriptorPool descriptor_pool;
VkDescriptorSet vp_set;

VkBuffer vp_buffer;
VmaAllocation vp_buffer_allocation;

HgHashMap<HgResource, VkDescriptorSet> texture_sets;

void hg_pipeline_2d_init(
    HgArena& arena,
    u32 max_textures,
    VkFormat color_format,
    VkFormat depth_format
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(color_format != VK_FORMAT_UNDEFINED);

    texture_sets = texture_sets.create(arena, max_textures);

    vp_layout = HgVkDescriptorSetLayoutBuilder()
        .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
        .create();

    texture_layout = HgVkDescriptorSetLayoutBuilder()
        .add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        .create();

    pipeline_layout = HgVkPipelineLayoutBuilder()
        .add_descriptor_set(vp_layout)
        .add_descriptor_set(texture_layout)
        .set_push_range(VK_SHADER_STAGE_VERTEX_BIT, sizeof(Push))
        .create();

    VkShaderModule vertex_shader = hg_vk_create_shader_module(sprite_vert_spv, sprite_vert_spv_size);
    VkShaderModule fragment_shader = hg_vk_create_shader_module(sprite_frag_spv, sprite_frag_spv_size);
    hg_defer(vkDestroyShaderModule(hg_vk_device, vertex_shader, nullptr));
    hg_defer(vkDestroyShaderModule(hg_vk_device, fragment_shader, nullptr));

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    HgVkPipelineConfig pipeline_config{};
    pipeline_config.color_attachment_formats = &color_format;
    pipeline_config.color_attachment_format_count = 1;
    pipeline_config.depth_attachment_format = depth_format;
    pipeline_config.stencil_attachment_format = VK_FORMAT_UNDEFINED;
    pipeline_config.shader_stages = shader_stages;
    pipeline_config.shader_count = sizeof(shader_stages) / sizeof(*shader_stages);
    pipeline_config.layout = pipeline_layout;
    pipeline_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipeline_config.enable_color_blend = true;

    pipeline = hg_vk_create_graphics_pipeline(pipeline_config);

    descriptor_pool = HgVkDescriptorPoolBuilder()
        .add_descriptor_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
        .add_descriptor_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        .create(1 + max_textures, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    vp_set = hg_vk_allocate_descriptor_set(descriptor_pool, vp_layout);
    hg_assert(vp_set != nullptr);

    hg_vk_create_buffer(
        &vp_buffer,
        &vp_buffer_allocation,
        sizeof(VPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    VPUniform vp_data{};
    vp_data.proj = 1.0f;
    vp_data.view = 1.0f;

    vmaCopyMemoryToAllocation(hg_vk_vma, &vp_data, vp_buffer_allocation, 0, sizeof(vp_data));

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = vp_buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(VPUniform);

    hg_vk_update_descriptor_set(vp_set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer_info);
}

void hg_pipeline_2d_deinit() {
    vmaDestroyBuffer(hg_vk_vma, vp_buffer, vp_buffer_allocation);
    vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, &vp_set);
    vkDestroyDescriptorPool(hg_vk_device, descriptor_pool, nullptr);
    vkDestroyPipeline(hg_vk_device, pipeline, nullptr);
    vkDestroyPipelineLayout(hg_vk_device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, texture_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, vp_layout, nullptr);
}

void hg_pipeline_2d_add_texture(HgResource texture_id) {
    hg_assert(hg_is_gpu_resource_loaded(texture_id));

    HgGpuTexture& texture = *hg_get_gpu_texture(texture_id);
    if (texture_sets.has(texture_id))
        return;

    VkDescriptorSet set = hg_vk_allocate_descriptor_set(descriptor_pool, texture_layout);
    hg_assert(set != nullptr);

    VkDescriptorImageInfo image_info{};
    image_info.sampler = texture.sampler;
    image_info.imageView = texture.view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_update_descriptor_set(set, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_info);

    texture_sets.insert(texture_id, set);
}

void hg_pipeline_2d_remove_texture(HgResource texture_id) {
    VkDescriptorSet set;
    if (texture_sets.remove(texture_id, &set))
        vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, &set);
}

bool hg_pipeline_2d_has_texture(HgResource texture_id) {
    return texture_sets.has(texture_id);
}

void hg_pipeline_2d_update_projection(const HgMat4& projection) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &projection,
        vp_buffer_allocation,
        offsetof(VPUniform, proj),
        sizeof(projection));
}

void hg_pipeline_2d_update_view(const HgMat4& view) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &view,
        vp_buffer_allocation,
        offsetof(VPUniform, view),
        sizeof(view));
}

void hg_draw_2d(HgECS& ecs, VkCommandBuffer cmd) {
    hg_assert(cmd != nullptr);

    ecs.sort<HgSprite>(nullptr, [](void*, HgECS& pecs, HgEntity lhs, HgEntity rhs) -> bool {
        hg_assert(pecs.has<HgTransform>(lhs));
        hg_assert(pecs.has<HgTransform>(rhs));
        return pecs.get<HgTransform>(lhs).position.z > pecs.get<HgTransform>(rhs).position.z;
    });

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        1,
        &vp_set,
        0,
        nullptr);

    ecs.for_each<HgSprite, HgTransform>([&](HgEntity, HgSprite& sprite, HgTransform& transform) {
        hg_assert(texture_sets.has(sprite.texture));
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            1,
            1,
            texture_sets.get(sprite.texture),
            0,
            nullptr);

        Push push{};
        push.model = hg_model_matrix_3d(transform.position, transform.scale, transform.rotation);
        push.uv_pos = sprite.uv_pos;
        push.uv_size = sprite.uv_size;

        vkCmdPushConstants(
            cmd,
            pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(push),
            &push);

        vkCmdDraw(cmd, 4, 1, 0, 0);
    });
}

