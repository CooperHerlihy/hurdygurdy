#include "hurdygurdy.h"

#include "test.frag.spv.h"
#include "test.vert.spv.h"

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
    hg_vk_load_instance(instance);
    hg_debug_mode(VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance));
    VkSurfaceKHR surface = hg_vk_create_surface(instance, platform, window);

    u32 queue_family;
    VkPhysicalDevice gpu = hg_vk_find_single_queue_gpu(instance, &queue_family);
    VkDevice device = hg_vk_create_single_queue_device(gpu, queue_family);
    hg_vk_load_device(device);
    VkQueue queue = hg_vk_get_queue(device, queue_family, 0);

    u32 swap_width;
    u32 swap_height;
    VkFormat swap_format;
    u32 swap_image_count;
    VkSwapchainKHR swapchain = hg_vk_create_swapchain(device, gpu, NULL, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR,
        &swap_width, &swap_height, &swap_format, &swap_image_count);

    VkImage *swap_images = malloc(swap_image_count * sizeof(*swap_images));
    hg_vk_get_swapchain_images(device, swapchain, swap_images, swap_image_count);

    VkImageView *swap_views = malloc(swap_image_count * sizeof(*swap_views));
    for (usize i = 0; i < swap_image_count; ++i) {
        swap_views[i] = hg_vk_create_image_view(
            device, swap_images[i], swap_format, VK_IMAGE_VIEW_TYPE_2D,
            (VkImageSubresourceRange){.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT});
    }

    VkPipelineLayout pipeline_layout = hg_vk_create_pipeline_layout(device, NULL, 0, NULL, 0);

    VkShaderModule vertex_shader = hg_vk_create_shader_module(device, test_vert_spv, test_vert_spv_size);
    VkShaderModule fragment_shader = hg_vk_create_shader_module(device, test_frag_spv, test_frag_spv_size);

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
        .color_attachment_formats = &swap_format,
        .color_attachment_count = 1,
        .layout = pipeline_layout,
        .shader_stages = shader_stages,
        .shader_count = hg_countof(shader_stages),
        .vertex_bindings = vertex_bindings,
        .vertex_binding_count = hg_countof(vertex_bindings),
        .vertex_attributes = vertex_attributes,
        .vertex_attribute_count = hg_countof(vertex_attributes),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    VkPipeline pipeline = hg_vk_create_graphics_pipeline(device, &pipeline_config);

    hg_vk_destroy_shader_module(device, fragment_shader);
    hg_vk_destroy_shader_module(device, vertex_shader);

    f32 vertices[] = {
        0.0f, -0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f,
    };

    VkBuffer vertex_buffer = hg_vk_create_buffer(device, &(VkBufferCreateInfo){
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    });

    VkMemoryRequirements vertex_buffer_mem_reqs = hg_vk_get_buffer_mem_reqs(device, vertex_buffer);
    VkDeviceMemory vertex_buffer_memory = hg_vk_allocate_memory(device, gpu, &vertex_buffer_mem_reqs,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);

    hg_vk_bind_buffer_memory(device, vertex_buffer, vertex_buffer_memory, 0);

    u8 *vertex_memory_map = hg_vk_map_memory(device, vertex_buffer_memory, 0, sizeof(vertices));
    memcpy(vertex_memory_map, vertices, sizeof(vertices));
    hg_vk_unmap_memory(device, vertex_buffer_memory);

    HgFrameSync frame_sync = hg_frame_sync_create(device, queue_family, swap_image_count);

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
            u32 old_count = swap_image_count;
            VkSwapchainKHR old_swapchain = swapchain;
            swapchain = hg_vk_create_swapchain(device, gpu, NULL, surface,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR,
                &swap_width, &swap_height, &swap_format, &swap_image_count);

            if (swapchain != VK_NULL_HANDLE) {
                if (swap_image_count != old_count) {
                    swap_images = realloc(swap_images, swap_image_count * sizeof(*swap_images));
                    swap_views = realloc(swap_views, swap_image_count * sizeof(*swap_images));
                }
                hg_vk_get_swapchain_images(device, swapchain, swap_images, swap_image_count);

                for (usize i = 0; i < swap_image_count; ++i) {
                    hg_vk_destroy_image_view(device, swap_views[i]);
                    swap_views[i] = hg_vk_create_image_view(
                        device, swap_images[i], swap_format, VK_IMAGE_VIEW_TYPE_2D,
                        (VkImageSubresourceRange){.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT});
                }
            }

            hg_vk_destroy_swapchain(device, old_swapchain);
            hg_info("window resized\n");
        }

        if (swapchain != NULL) {
            VkCommandBuffer cmd = hg_frame_sync_begin_frame(&frame_sync, device, swapchain);
            u32 image_index = frame_sync.current_image;

            VkImageMemoryBarrier2 color_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = swap_images[image_index],
                .subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            hg_vk_pipeline_barrier(cmd, &(VkDependencyInfo){
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
                .renderArea = {.extent = {swap_width, swap_height}},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_attachment,
            };
            hg_vk_begin_rendering(cmd, &rendering_info);

            hg_vk_bind_pipeline(cmd, pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);

            hg_vk_set_viewport(cmd, 0.0f, 0.0f, (f32)swap_width, (f32)swap_height, 0.0f, 1.0f);
            hg_vk_set_scissor(cmd, 0, 0, swap_width, swap_height);

            hg_vk_bind_vertex_buffer(cmd, vertex_buffer);
            hg_vk_draw(cmd, 0, 3, 0, 1);

            hg_vk_end_rendering(cmd);

            VkImageMemoryBarrier2 present_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .image = swap_images[image_index],
                .subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            hg_vk_pipeline_barrier(cmd, &(VkDependencyInfo){
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &present_barrier,
            });

            hg_frame_sync_end_frame_and_present(&frame_sync, queue, swapchain);
        }
    }

    hg_vk_wait_for_device(device);

    hg_frame_sync_destroy(&frame_sync, device);

    hg_vk_destroy_buffer(device, vertex_buffer);
    hg_vk_free_memory(device, vertex_buffer_memory);

    hg_vk_destroy_pipeline(device, pipeline);
    hg_vk_destroy_pipeline_layout(device, pipeline_layout);

    for (usize i = 0; i < swap_image_count; ++i) {
        hg_vk_destroy_image_view(device, swap_views[i]);
    }
    free(swap_views);
    free(swap_images);
    hg_vk_destroy_swapchain(device, swapchain);

    hg_vk_destroy_device(device);
    hg_vk_destroy_surface(instance, surface);
    hg_debug_mode(hg_vk_destroy_debug_messenger(instance, debug_messenger));
    hg_vk_destroy_instance(instance);

    hg_window_destroy(platform, window);
    hg_platform_destroy(platform);

    hg_info("Tests complete\n");
}
