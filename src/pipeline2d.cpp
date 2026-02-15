#include "hurdygurdy.hpp"

#include "sprite.frag.spv.h"
#include "sprite.vert.spv.h"

HgPipeline2D HgPipeline2D::create(
    HgArena& arena,
    usize max_textures,
    VkFormat color_format,
    VkFormat depth_format
) {
    hg_assert(hg_vk_device != nullptr);
    hg_assert(color_format != VK_FORMAT_UNDEFINED);

    HgPipeline2D pipeline{};
    pipeline.texture_sets = pipeline.texture_sets.create(arena, max_textures);

    VkDescriptorSetLayoutBinding vp_bindings[1]{};
    vp_bindings[0].binding = 0;
    vp_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vp_bindings[0].descriptorCount = 1;
    vp_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo vp_layout_info{};
    vp_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vp_layout_info.bindingCount = hg_countof(vp_bindings);
    vp_layout_info.pBindings = vp_bindings;

    vkCreateDescriptorSetLayout(hg_vk_device, &vp_layout_info, nullptr, &pipeline.vp_layout);
    hg_assert(pipeline.vp_layout != nullptr);

    VkDescriptorSetLayoutBinding texture_bindings[1]{};
    texture_bindings[0].binding = 0;
    texture_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_bindings[0].descriptorCount = 1;
    texture_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texture_layout_info{};
    texture_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_layout_info.bindingCount = hg_countof(texture_bindings);
    texture_layout_info.pBindings = texture_bindings;

    vkCreateDescriptorSetLayout(hg_vk_device, &texture_layout_info, nullptr, &pipeline.texture_layout);
    hg_assert(pipeline.texture_layout != nullptr);

    VkDescriptorSetLayout set_layouts[]{pipeline.vp_layout, pipeline.texture_layout};
    VkPushConstantRange push_ranges[1]{};
    push_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_ranges[0].size = sizeof(Push);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = hg_countof(set_layouts);
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = hg_countof(push_ranges);
    layout_info.pPushConstantRanges = push_ranges;

    vkCreatePipelineLayout(hg_vk_device, &layout_info, nullptr, &pipeline.pipeline_layout);
    hg_assert(pipeline.pipeline_layout != nullptr);

    VkShaderModuleCreateInfo vertex_shader_info{};
    vertex_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_info.codeSize = sprite_vert_spv_size;
    vertex_shader_info.pCode = (u32*)sprite_vert_spv;

    VkShaderModule vertex_shader = nullptr;
    vkCreateShaderModule(hg_vk_device, &vertex_shader_info, nullptr, &vertex_shader);
    hg_assert(vertex_shader != nullptr);

    VkShaderModuleCreateInfo fragment_shader_info{};
    fragment_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_info.codeSize = sprite_frag_spv_size;
    fragment_shader_info.pCode = (u32*)sprite_frag_spv;

    VkShaderModule fragment_shader = nullptr;
    vkCreateShaderModule(hg_vk_device, &fragment_shader_info, nullptr, &fragment_shader);
    hg_assert(fragment_shader != nullptr);

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
    pipeline_config.shader_count = hg_countof(shader_stages);
    pipeline_config.layout = pipeline.pipeline_layout;
    pipeline_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipeline_config.enable_color_blend = true;

    pipeline.pipeline = hg_vk_create_graphics_pipeline(pipeline_config);

    vkDestroyShaderModule(hg_vk_device, fragment_shader, nullptr);
    vkDestroyShaderModule(hg_vk_device, vertex_shader, nullptr);

    VkDescriptorPoolSize desc_pool_sizes[2]{};
    desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_pool_sizes[0].descriptorCount = 1;
    desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_pool_sizes[1].descriptorCount = (u32)max_textures;

    VkDescriptorPoolCreateInfo desc_pool_info{};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc_pool_info.maxSets = 1 + (u32)max_textures;
    desc_pool_info.poolSizeCount = hg_countof(desc_pool_sizes);
    desc_pool_info.pPoolSizes = desc_pool_sizes;

    vkCreateDescriptorPool(hg_vk_device, &desc_pool_info, nullptr, &pipeline.descriptor_pool);
    hg_assert(pipeline.descriptor_pool != nullptr);

    VkDescriptorSetAllocateInfo vp_set_alloc_info{};
    vp_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vp_set_alloc_info.descriptorPool = pipeline.descriptor_pool;
    vp_set_alloc_info.descriptorSetCount = 1;
    vp_set_alloc_info.pSetLayouts = &pipeline.vp_layout;

    vkAllocateDescriptorSets(hg_vk_device, &vp_set_alloc_info, &pipeline.vp_set);
    hg_assert(pipeline.vp_set != nullptr);

    VkBufferCreateInfo vp_buffer_info{};
    vp_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vp_buffer_info.size = sizeof(VPUniform);
    vp_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo vp_alloc_info{};
    vp_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vp_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(
        hg_vk_vma,
        &vp_buffer_info,
        &vp_alloc_info,
        &pipeline.vp_buffer,
        &pipeline.vp_buffer_allocation,
        nullptr);
    hg_assert(pipeline.vp_buffer != nullptr);
    hg_assert(pipeline.vp_buffer_allocation != nullptr);

    VPUniform vp_data{};
    vp_data.proj = 1.0f;
    vp_data.view = 1.0f;

    vmaCopyMemoryToAllocation(hg_vk_vma, &vp_data, pipeline.vp_buffer_allocation, 0, sizeof(vp_data));

    VkDescriptorBufferInfo desc_info{};
    desc_info.buffer = pipeline.vp_buffer;
    desc_info.offset = 0;
    desc_info.range = sizeof(VPUniform);

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = pipeline.vp_set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_write.pBufferInfo = &desc_info;

    vkUpdateDescriptorSets(hg_vk_device, 1, &desc_write, 0, nullptr);

    return pipeline;
}

void HgPipeline2D::destroy() {
    vmaDestroyBuffer(hg_vk_vma, vp_buffer, vp_buffer_allocation);
    vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, &vp_set);
    vkDestroyDescriptorPool(hg_vk_device, descriptor_pool, nullptr);
    vkDestroyPipeline(hg_vk_device, pipeline, nullptr);
    vkDestroyPipelineLayout(hg_vk_device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, texture_layout, nullptr);
    vkDestroyDescriptorSetLayout(hg_vk_device, vp_layout, nullptr);
}

void HgPipeline2D::add_texture(HgResourceID texture_id) {
    hg_assert(hg_resources->is_registered(texture_id));

    HgGpuTexture& texture = hg_gpu_resources->get_texture(texture_id);
    if (texture_sets.has(texture_id))
        return;

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &texture_layout;

    VkDescriptorSet set = nullptr;
    vkAllocateDescriptorSets(hg_vk_device, &set_info, &set);
    hg_assert(set != nullptr);

    VkDescriptorImageInfo desc_info{};
    desc_info.sampler = texture.sampler;
    desc_info.imageView = texture.view;
    desc_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_write.pImageInfo = &desc_info;

    vkUpdateDescriptorSets(hg_vk_device, 1, &desc_write, 0, nullptr);

    texture_sets.insert(texture_id, set);
}

void HgPipeline2D::remove_texture(HgResourceID texture_id) {
    hg_assert(hg_resources->is_registered(texture_id));

    VkDescriptorSet* set = texture_sets.get(texture_id);
    if (set != nullptr) {
        texture_sets.remove(texture_id);
        vkFreeDescriptorSets(hg_vk_device, descriptor_pool, 1, set);
    }
}

void HgPipeline2D::update_projection(const HgMat4& projection) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &projection,
        vp_buffer_allocation,
        offsetof(VPUniform, proj),
        sizeof(projection));
}

void HgPipeline2D::update_view(const HgMat4& view) {
    vmaCopyMemoryToAllocation(
        hg_vk_vma,
        &view,
        vp_buffer_allocation,
        offsetof(VPUniform, view),
        sizeof(view));
}

void HgPipeline2D::draw(VkCommandBuffer cmd) {
    hg_assert(cmd != nullptr);
    hg_assert(hg_ecs->is_registered<HgSprite>());

    hg_ecs->sort<HgSprite>(nullptr, [](void*, HgEntity lhs, HgEntity rhs) -> bool {
        hg_assert(hg_ecs->has<HgTransform>(lhs));
        hg_assert(hg_ecs->has<HgTransform>(rhs));
        return hg_ecs->get<HgTransform>(lhs).position.z > hg_ecs->get<HgTransform>(rhs).position.z;
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

    hg_ecs->for_each<HgSprite, HgTransform>([&](HgEntity, HgSprite& sprite, HgTransform& transform) {
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

        Push push;
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

