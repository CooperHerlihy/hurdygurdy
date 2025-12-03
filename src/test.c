#include "hurdygurdy.h"

#include "vk_mem_alloc.h"

#include "test.frag.spv.h"
#include "test.vert.spv.h"

typedef struct HgPipelineSprite {
    VkDescriptorSetLayout vp_layout;
    VkDescriptorSetLayout image_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSet vp_set;

    VkBuffer vp_buffer;
    VmaAllocation vp_buffer_allocation;
} HgPipelineSprite;

typedef struct HgPipelineSpriteVPUniform {
    HgMat4 proj;
    HgMat4 view;
} HgPipelineSpriteVPUniform;

typedef struct HgPipelineSpriteTexture {
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet set;
} HgPipelineSpriteTexture;

HgPipelineSprite hg_pipeline_sprite_create(
    VkDevice device,
    VmaAllocator allocator,
    VkFormat color_format,
    VkFormat depth_format
) {
    assert(device != VK_NULL_HANDLE);
    assert(color_format != VK_FORMAT_UNDEFINED);
    assert(depth_format != VK_FORMAT_UNDEFINED);

    HgPipelineSprite pipeline;

    VkDescriptorSetLayoutBinding vp_bindings[] = {{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    }};
    VkDescriptorSetLayoutCreateInfo vp_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = hg_countof(vp_bindings),
        .pBindings = vp_bindings,
    };
    vkCreateDescriptorSetLayout(device, &vp_layout_info, NULL, &pipeline.vp_layout);

    VkDescriptorSetLayoutBinding image_bindings[] = {{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    }};
    VkDescriptorSetLayoutCreateInfo image_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = hg_countof(image_bindings),
        .pBindings = image_bindings,
    };
    vkCreateDescriptorSetLayout(device, &image_layout_info, NULL, &pipeline.image_layout);

    VkDescriptorSetLayout set_layouts[] = {pipeline.vp_layout, pipeline.image_layout};
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(HgMat4),
    };
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = hg_countof(set_layouts),
        .pSetLayouts = set_layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };
    vkCreatePipelineLayout(device, &layout_info, NULL, &pipeline.pipeline_layout);

    VkShaderModuleCreateInfo vertex_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = test_vert_spv_size,
        .pCode = (u32 *)test_vert_spv,
    };
    VkShaderModule vertex_shader;
    vkCreateShaderModule(device, &vertex_shader_info, NULL, &vertex_shader);

    VkShaderModuleCreateInfo fragment_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = test_frag_spv_size,
        .pCode = (u32 *)test_frag_spv,
    };
    VkShaderModule fragment_shader;
    vkCreateShaderModule(device, &fragment_shader_info, NULL, &fragment_shader);

    VkPipelineShaderStageCreateInfo shader_stages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader,
        .pName = "main",
    }, {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader,
        .pName = "main",
    }};
    HgVkPipelineConfig pipeline_config = {
        .color_attachment_formats = &color_format,
        .color_attachment_count = 1,
        .depth_attachment_format = depth_format,
        .shader_stages = shader_stages,
        .shader_count = hg_countof(shader_stages),
        .layout = pipeline.pipeline_layout,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        .enable_color_blend = true,
    };
    pipeline.pipeline = hg_vk_create_graphics_pipeline(device, &pipeline_config);

    vkDestroyShaderModule(device, fragment_shader, NULL);
    vkDestroyShaderModule(device, vertex_shader, NULL);

    VkDescriptorPoolSize desc_pool_sizes[] = {{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
    }, {
        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = 255,
    }};
    VkDescriptorPoolCreateInfo desc_pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 256,
        .poolSizeCount = hg_countof(desc_pool_sizes),
        .pPoolSizes = desc_pool_sizes,
    };
    vkCreateDescriptorPool(device, &desc_pool_info, NULL, &pipeline.descriptor_pool);

    VkDescriptorSetAllocateInfo vp_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pipeline.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pipeline.vp_layout,
    };
    vkAllocateDescriptorSets(device, &vp_set_alloc_info, &pipeline.vp_set);

    VkBufferCreateInfo vp_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(HgPipelineSpriteVPUniform),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    };
    VmaAllocationCreateInfo vp_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    vmaCreateBuffer(
        allocator,
        &vp_buffer_info,
        &vp_alloc_info,
        &pipeline.vp_buffer,
        &pipeline.vp_buffer_allocation,
        NULL);

    HgPipelineSpriteVPUniform vp_data = {
        .proj = hg_smat4(1.0f),
        .view = hg_smat4(1.0f),
    };
    vmaCopyMemoryToAllocation(allocator, &vp_data, pipeline.vp_buffer_allocation, 0, sizeof(vp_data));

    return pipeline;
}

void hg_pipeline_sprite_destroy(VkDevice device, HgPipelineSprite *pipeline) {
    assert(device != VK_NULL_HANDLE);
    if (pipeline == NULL)
        return;

    vkFreeDescriptorSets(device, pipeline->descriptor_pool, 1, &pipeline->vp_set);
    vkDestroyDescriptorPool(device, pipeline->descriptor_pool, NULL);

    vkDestroyPipeline(device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device, pipeline->image_layout, NULL);
    vkDestroyDescriptorSetLayout(device, pipeline->vp_layout, NULL);
}

void hg_pipeline_sprite_update_projection(
    VkDevice device,
    HgPipelineSprite *pipeline,
    VmaAllocator allocator,
    HgMat4 *projection
) {
    (void)device;
    assert(device != VK_NULL_HANDLE);
    assert(pipeline != NULL);
    assert(allocator != VK_NULL_HANDLE);
    assert(projection != NULL);

    vmaCopyMemoryToAllocation(
        allocator,
        projection,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, proj),
        sizeof(*projection));
}

void hg_pipeline_sprite_update_view(
    HgPipelineSprite *pipeline,
    VmaAllocator allocator,
    HgMat4 *view
) {
    assert(pipeline != NULL);
    assert(allocator != VK_NULL_HANDLE);
    assert(view != NULL);

    vmaCopyMemoryToAllocation(
        allocator,
        view,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, view),
        sizeof(*view));
}

HgPipelineSpriteTexture hg_pipeline_sprite_create_texture(
    VkDevice device,
    HgPipelineSprite *pipeline,
    VmaAllocator allocator,
    void *tex_data,
    u32 width,
    u32 height,
    VkFormat pixel_format,
    VkFilter sampler_filter,
    VkSamplerAddressMode edge_mode
) {
    assert(device != VK_NULL_HANDLE);
    assert(pipeline != NULL);
    assert(allocator != VK_NULL_HANDLE);
    assert(tex_data != NULL);
    assert(width > 0);
    assert(height > 0);

    HgPipelineSpriteTexture tex;

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = pixel_format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    };
    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    vmaCreateImage(allocator, &image_info, &alloc_info, &tex.image, &tex.allocation, NULL);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = pixel_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    vkCreateImageView(device, &view_info, NULL, &tex.view);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = sampler_filter,
        .minFilter = sampler_filter,
        .addressModeU = edge_mode,
        .addressModeV = edge_mode,
        .addressModeW = edge_mode,
    };
    vkCreateSampler(device, &sampler_info, NULL, &tex.sampler);

    VkDescriptorSetAllocateInfo set_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pipeline->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pipeline->image_layout,
    };
    vkAllocateDescriptorSets(device, &set_info, &tex.set);

    VkDescriptorImageInfo desc_info = {
        .sampler = tex.sampler,
        .imageView = tex.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet desc_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = tex.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &desc_info,
    };
    vkUpdateDescriptorSets(device, 1, &desc_write, 0, NULL);

    return tex;
}

void hg_pipeline_sprite_bind(VkCommandBuffer cmd, HgPipelineSprite *pipeline) {
    assert(cmd != VK_NULL_HANDLE);
    assert(pipeline != NULL);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        0,
        1,
        &pipeline->vp_set,
        0,
        NULL);
}

void hg_pipeline_sprite_draw(
    VkCommandBuffer cmd,
    HgPipelineSprite *pipeline,
    HgPipelineSpriteTexture *texture,
    HgMat4 *model
) {
    assert(cmd != VK_NULL_HANDLE);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        1,
        1,
        &texture->set,
        0,
        NULL);

    vkCmdPushConstants(
        cmd,
        pipeline->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(HgPipelineSpriteVPUniform),
        model);

    vkCmdDraw(cmd, 4, 1, 0, 0);
}

typedef struct HgPipelineTest {
    VkPipelineLayout layout;
    VkPipeline pipeline;
} HgPipelineTest;

typedef struct HgPipelineTestTriangle {
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
} HgPipelineTestTriangle;

HgPipelineTest hg_pipeline_test_create(VkDevice device, VkFormat color_format) {
    assert(device != VK_NULL_HANDLE);
    assert(color_format != VK_FORMAT_UNDEFINED);

    HgPipelineTest renderer = {0};

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    vkCreatePipelineLayout(device, &layout_info, NULL, &renderer.layout);

    VkShaderModuleCreateInfo vertex_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = test_vert_spv_size,
        .pCode = (u32 *)test_vert_spv,
    };
    VkShaderModule vertex_shader;
    vkCreateShaderModule(device, &vertex_shader_info, NULL, &vertex_shader);

    VkShaderModuleCreateInfo fragment_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = test_frag_spv_size,
        .pCode = (u32 *)test_frag_spv,
    };
    VkShaderModule fragment_shader;
    vkCreateShaderModule(device, &fragment_shader_info, NULL, &fragment_shader);

    VkPipelineShaderStageCreateInfo shader_stages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader,
        .pName = "main",
    }, {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader,
        .pName = "main",
    }};

    VkVertexInputBindingDescription vertex_bindings[] = {
        {.binding = 0, .stride = sizeof(f32) * 2, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
    };
    VkVertexInputAttributeDescription vertex_attributes[] = {
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0},
    };

    HgVkPipelineConfig pipeline_config = {
        .color_attachment_formats = &color_format,
        .color_attachment_count = 1,
        .shader_stages = shader_stages,
        .shader_count = hg_countof(shader_stages),
        .layout = renderer.layout,
        .vertex_bindings = vertex_bindings,
        .vertex_binding_count = hg_countof(vertex_bindings),
        .vertex_attributes = vertex_attributes,
        .vertex_attribute_count = hg_countof(vertex_attributes),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    renderer.pipeline = hg_vk_create_graphics_pipeline(device, &pipeline_config);

    vkDestroyShaderModule(device, fragment_shader, NULL);
    vkDestroyShaderModule(device, vertex_shader, NULL);

    return renderer;
}

void hg_pipeline_test_destroy(VkDevice device, HgPipelineTest *renderer) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);

    vkDestroyPipeline(device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(device, renderer->layout, NULL);
}

HgPipelineTestTriangle hg_pipeline_test_triangle_create(
    VkDevice device,
    HgPipelineTest *renderer,
    VkPhysicalDevice gpu
) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);
    assert(gpu != VK_NULL_HANDLE);

    HgPipelineTestTriangle triangle = {0};

    f32 vertices[] = {
        0.0f, -0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f,
    };

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    };
    vkCreateBuffer(device, &buffer_info, NULL, &triangle.vertex_buffer);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, triangle.vertex_buffer, &mem_reqs);
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = hg_vk_find_memory_type_index(gpu, mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0)
    };
    vkAllocateMemory(device, &alloc_info, NULL, &triangle.vertex_buffer_memory);

    vkBindBufferMemory(device, triangle.vertex_buffer, triangle.vertex_buffer_memory, 0);

    void *memory_map;
    vkMapMemory(device, triangle.vertex_buffer_memory, 0, sizeof(vertices), 0, &memory_map);
    memcpy(memory_map, vertices, sizeof(vertices));
    vkUnmapMemory(device, triangle.vertex_buffer_memory);

    return triangle;
}

void hg_pipeline_test_triangle_destroy(VkDevice device, HgPipelineTest *renderer, HgPipelineTestTriangle *triangle) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);
    assert(triangle != NULL);

    vkDestroyBuffer(device, triangle->vertex_buffer, NULL);
    vkFreeMemory(device, triangle->vertex_buffer_memory, NULL);
}

void hg_pipeline_test_bind(VkCommandBuffer cmd, HgPipelineTest *renderer) {
    assert(cmd != VK_NULL_HANDLE);
    assert(renderer != NULL);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
}

void hg_pipeline_test_draw(VkCommandBuffer cmd, HgPipelineTestTriangle *triangle) {
    assert(cmd != VK_NULL_HANDLE);
    assert(triangle != NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &triangle->vertex_buffer, &(VkDeviceSize){0});
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

int main(void) {
    HgPlatform *platform = hg_platform_create();
    HgWindow *window = hg_window_create(platform, &(HgWindowConfig){
        .title = "Hg Test",
        .windowed = true,
        .width = 800,
        .height = 600,
    });

    hg_vk_load();

    VkInstance instance = hg_vk_create_instance("HurdyGurdy Test");
    hg_debug_mode(VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance));
    HgSingleQueueDeviceData device = hg_vk_create_single_queue_device(instance);

    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = device.gpu,
        .device = device.handle,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };
    VmaAllocator allocator;
    vmaCreateAllocator(&allocator_info, &allocator);

    VkSurfaceKHR surface = hg_vk_create_surface(instance, platform, window);
    HgSwapchainData swapchain = hg_vk_create_swapchain(device.handle, device.gpu, NULL, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);
    VkImage *swap_images = malloc(swapchain.image_count * sizeof(*swap_images));
    VkImageView *swap_views = malloc(swapchain.image_count * sizeof(*swap_views));
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swapchain.image_count, swap_images);
    for (usize i = 0; i < swapchain.image_count; ++i) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            }
        };
        vkCreateImageView(device.handle, &create_info, VK_NULL_HANDLE, &swap_views[i]);
    }

    HgFrameSync frame_sync = hg_frame_sync_create(device.handle, device.queue_family, swapchain.image_count);

    HgPipelineTest test_pipeline = hg_pipeline_test_create(device.handle, swapchain.format);
    HgPipelineTestTriangle triangle = hg_pipeline_test_triangle_create(device.handle, &test_pipeline, device.gpu);

    u32 frame_count = 0;
    f64 frame_time = 0.0f;
    HgClock hclock;
    hg_clock_tick(&hclock);

    while(true) {
        f64 delta = hg_clock_tick(&hclock);
        ++frame_count;
        frame_time += delta;
        if (frame_time > 1.0f) {
            hg_info("fps: %d, avg: %fms\n", frame_count, 1.0e3 / (f64)frame_count);
            frame_count = 0;
            frame_time -= 1.0f;
        }

        hg_window_process_events(platform, &window, 1);
        if (hg_window_was_closed(window) || hg_window_is_key_down(window, HG_KEY_ESCAPE))
            break;

        if (hg_window_was_resized(window)) {
            vkQueueWaitIdle(device.queue);

            u32 old_count = swapchain.image_count;
            VkSwapchainKHR old_swapchain = swapchain.handle;
            swapchain = hg_vk_create_swapchain(device.handle, device.gpu, old_swapchain, surface,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

            if (swapchain.handle != VK_NULL_HANDLE) {
                if (swapchain.image_count != old_count) {
                    swap_images = realloc(swap_images, swapchain.image_count * sizeof(*swap_images));
                    swap_views = realloc(swap_views, swapchain.image_count * sizeof(*swap_images));
                }

                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swapchain.image_count, swap_images);
                for (usize i = 0; i < swapchain.image_count; ++i) {
                    vkDestroyImageView(device.handle, swap_views[i], NULL);
                    VkImageViewCreateInfo create_info = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .image = swap_images[i],
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = swapchain.format,
                        .subresourceRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .levelCount = 1,
                            .layerCount = 1,
                        }
                    };
                    vkCreateImageView(device.handle, &create_info, VK_NULL_HANDLE, &swap_views[i]);
                }

                hg_frame_sync_destroy(device.handle, &frame_sync);
                frame_sync = hg_frame_sync_create(device.handle, device.queue_family, swapchain.image_count);
            }

            vkDestroySwapchainKHR(device.handle, old_swapchain, NULL);
            hg_info("window resized\n");
        }

        VkCommandBuffer cmd = hg_frame_sync_begin_frame(device.handle, &frame_sync, swapchain.handle);
        if (cmd != VK_NULL_HANDLE) {
            u32 image_index = frame_sync.current_image;

            VkImageMemoryBarrier2 color_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = swap_images[image_index],
                .subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &color_barrier,
            });

            VkRenderingAttachmentInfo color_attachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = swap_views[image_index],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            };
            VkRenderingInfo rendering_info = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {.extent = {swapchain.width, swapchain.height}},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_attachment,
            };
            vkCmdBeginRendering(cmd, &rendering_info);

            VkViewport viewport = {0.0f, 0.0f, (f32)swapchain.width, (f32)swapchain.height, 0.0f, 1.0f};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            VkRect2D scissor = {{0, 0}, {swapchain.width, swapchain.height}};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            hg_pipeline_test_bind(cmd, &test_pipeline);
            hg_pipeline_test_draw(cmd, &triangle);

            vkCmdEndRendering(cmd);

            VkImageMemoryBarrier2 present_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .image = swap_images[image_index],
                .subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &present_barrier,
            });

            hg_frame_sync_end_frame_and_present(device.queue, &frame_sync, swapchain.handle);
        }
    }

    vkDeviceWaitIdle(device.handle);

    hg_pipeline_test_triangle_destroy(device.handle, &test_pipeline, &triangle);
    hg_pipeline_test_destroy(device.handle, &test_pipeline);

    hg_frame_sync_destroy(device.handle, &frame_sync);

    for (usize i = 0; i < swapchain.image_count; ++i) {
        vkDestroyImageView(device.handle, swap_views[i], NULL);
    }
    free(swap_views);
    free(swap_images);
    vkDestroySwapchainKHR(device.handle, swapchain.handle, NULL);

    vkDestroyDevice(device.handle, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    hg_debug_mode(vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, NULL));
    vkDestroyInstance(instance, NULL);

    hg_window_destroy(platform, window);
    hg_platform_destroy(platform);

    hg_info("Tests complete\n");
}

