#include "hurdygurdy.h"

#include "test.frag.spv.h"
#include "test.vert.spv.h"

typedef struct HgTestRenderer {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} HgTestRenderer;

typedef struct HgTestRendererTriangle {
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
} HgTestRendererTriangle;

HgTestRenderer hg_test_renderer_create(VkDevice device, VkFormat target_format) {
    assert(device != VK_NULL_HANDLE);
    assert(target_format != VK_FORMAT_UNDEFINED);

    HgTestRenderer renderer = {0};

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    vkCreatePipelineLayout(device, &layout_info, NULL, &renderer.pipeline_layout);

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
        .color_attachment_formats = &target_format,
        .color_attachment_count = 1,
        .layout = renderer.pipeline_layout,
        .shader_stages = shader_stages,
        .shader_count = hg_countof(shader_stages),
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

void hg_test_renderer_destroy(VkDevice device, HgTestRenderer *renderer) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);

    vkDestroyPipeline(device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(device, renderer->pipeline_layout, NULL);
}

HgTestRendererTriangle hg_test_renderer_triangle_create(
    VkDevice device,
    HgTestRenderer *renderer,
    VkPhysicalDevice gpu
) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);
    assert(gpu != VK_NULL_HANDLE);

    HgTestRendererTriangle triangle = {0};

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

void hg_test_renderer_triangle_destroy(VkDevice device, HgTestRenderer *renderer, HgTestRendererTriangle *triangle) {
    assert(device != VK_NULL_HANDLE);
    assert(renderer != NULL);
    assert(triangle != NULL);

    vkDestroyBuffer(device, triangle->vertex_buffer, NULL);
    vkFreeMemory(device, triangle->vertex_buffer_memory, NULL);
}

void hg_test_renderer_draw(
    VkCommandBuffer cmd,
    HgTestRenderer *renderer,
    HgTestRendererTriangle *triangle,
    VkImageView target_image,
    u32 target_width,
    u32 target_height
) {
    assert(cmd != VK_NULL_HANDLE);
    assert(renderer != NULL);
    assert(triangle != NULL);
    assert(target_image != VK_NULL_HANDLE);
    assert(target_width > 0);
    assert(target_height > 0);

    VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = target_image,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.extent = {target_width, target_height}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
    };
    vkCmdBeginRendering(cmd, &rendering_info);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);

    vkCmdSetViewport(cmd, 0, 1, &(VkViewport){
        0.0f, 0.0f, (f32)target_width, (f32)target_height, 0.0f, 1.0f});
    vkCmdSetScissor(cmd, 0, 1, &(VkRect2D){
        {0, 0}, {target_width, target_height}});

    vkCmdBindVertexBuffers(cmd, 0, 1, &triangle->vertex_buffer, &(VkDeviceSize){0});
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

int main(void) {
    HgPlatform *platform = hg_platform_create();
    HgWindow *window = hg_window_create(platform, &(HgWindowConfig){
        .title = "Hg Test",
        .windowed = true,
        .width = 800,
        .height = 600,
    });

    VkInstance instance = hg_vk_create_instance("HurdyGurdy Test");
    hg_debug_mode(VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance));
    HgSingleQueueDeviceData device = hg_vk_create_single_queue_device(instance);

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
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
        };
        swap_views[i] = hg_vk_create_image_view(device.handle, &create_info);
    }

    HgFrameSync frame_sync = hg_frame_sync_create(device.handle, device.queue_family, swapchain.image_count);

    HgTestRenderer renderer = hg_test_renderer_create(device.handle, swapchain.format);
    HgTestRendererTriangle triangle = hg_test_renderer_triangle_create(device.handle, &renderer, device.gpu);

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
            u32 old_count = swapchain.image_count;
            VkSwapchainKHR old_swapchain = swapchain.handle;
            swapchain = hg_vk_create_swapchain(device.handle, device.gpu, NULL, surface,
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
                        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
                    };
                    swap_views[i] = hg_vk_create_image_view(device.handle, &create_info);
                }
            }

            vkDestroySwapchainKHR(device.handle, old_swapchain, NULL);
            hg_info("window resized\n");
        }

        if (swapchain.handle != NULL) {
            VkCommandBuffer cmd = hg_frame_sync_begin_frame(device.handle, &frame_sync, swapchain.handle);
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

            hg_test_renderer_draw(cmd, &renderer, &triangle, swap_views[image_index], swapchain.width, swapchain.height);

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

    hg_test_renderer_triangle_destroy(device.handle, &renderer, &triangle);
    hg_test_renderer_destroy(device.handle, &renderer);

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

