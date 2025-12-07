#include "hurdygurdy.h"

#include "vk_mem_alloc.h"

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

    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device.queue_family,
    };
    VkCommandPool cmd_pool;
    vkCreateCommandPool(device.handle, &cmd_pool_info, NULL, &cmd_pool);

    VkSurfaceKHR surface = hg_vk_create_surface(instance, platform, window);
    HgSwapchainData swapchain = hg_vk_create_swapchain(device.handle, device.gpu, NULL, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

    u32 swap_image_count;
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, NULL);
    VkImage *swap_images = malloc(swap_image_count * sizeof(*swap_images));
    VkImageView *swap_views = malloc(swap_image_count * sizeof(*swap_views));
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images);
    for (usize i = 0; i < swap_image_count; ++i) {
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
        vkCreateImageView(device.handle, &create_info, NULL, &swap_views[i]);
    }
    HgSwapchainCommands swapchain_commands = hg_swapchain_commands_create(
        device.handle, swapchain.handle, cmd_pool);

    HgPipelineSprite sprite_pipeline = hg_pipeline_sprite_create(
        device.handle, allocator, swapchain.format, 0);

    struct {u8 r, g, b, a;} tex_data[] = {
        {0xff, 0x00, 0x00, 0xff}, {0x00, 0xff, 0x00, 0xff},
        {0x00, 0x00, 0xff, 0xff}, {0xff, 0xff, 0x00, 0xff},
    };
    HgPipelineSpriteTextureConfig tex_config = {
        .tex_data = tex_data,
        .width = 2,
        .height = 2,
        .pixel_width = 4,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .filter = VK_FILTER_NEAREST,
        .edge_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };
    HgPipelineSpriteTexture texture = hg_pipeline_sprite_create_texture(
        &sprite_pipeline, cmd_pool, device.queue, &tex_config);

    HgVec3 position = {0.0f, 0.0f, 0.0f};

    f32 aspect = (f32)swapchain.width / (f32)swapchain.height;
    HgMat4 proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
    hg_pipeline_sprite_update_projection(&sprite_pipeline, &proj);

    u32 frame_count = 0;
    f64 frame_time = 0.0f;
    f64 cpu_time = 0.0f;
    HgClock game_clock;
    HgClock cpu_clock;
    hg_clock_tick(&game_clock);
    hg_clock_tick(&cpu_clock);

    while(true) {
        f64 delta = hg_clock_tick(&game_clock);
        f32 deltaf = (f32)delta;
        ++frame_count;
        frame_time += delta;
        if (frame_time > 1.0f) {
            hg_info("fps: %d, total avg: %fms, cpu avg: %fms\n",
                frame_count,
                1.0e3 / (f64)frame_count,
                cpu_time * 1.0e3 / (f64)frame_count);
            frame_count = 0;
            frame_time -= 1.0;
            cpu_time = 0.0;
        }

        hg_window_process_events(platform, &window, 1);
        if (hg_window_was_closed(window) || hg_window_is_key_down(window, HG_KEY_ESCAPE))
            break;

        static const f32 speed = 1.0f;
        if (hg_window_is_key_down(window, HG_KEY_W))
            position.y -= speed * deltaf;
        if (hg_window_is_key_down(window, HG_KEY_A))
            position.x -= speed * deltaf;
        if (hg_window_is_key_down(window, HG_KEY_S))
            position.y += speed * deltaf;
        if (hg_window_is_key_down(window, HG_KEY_D))
            position.x += speed * deltaf;

        if (hg_window_was_resized(window)) {
            vkQueueWaitIdle(device.queue);

            u32 old_count = swap_image_count;
            VkSwapchainKHR old_swapchain = swapchain.handle;
            swapchain = hg_vk_create_swapchain(device.handle, device.gpu, old_swapchain, surface,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

            for (usize i = 0; i < old_count; ++i) {
                vkDestroyImageView(device.handle, swap_views[i], NULL);
            }
            hg_swapchain_commands_destroy(device.handle, &swapchain_commands);

            if (swapchain.handle != NULL) {
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, NULL);
                if (swap_image_count != old_count) {
                    swap_images = realloc(swap_images, swap_image_count * sizeof(*swap_images));
                    swap_views = realloc(swap_views, swap_image_count * sizeof(*swap_images));
                }
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images);
                for (usize i = 0; i < swap_image_count; ++i) {
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
                    vkCreateImageView(device.handle, &create_info, NULL, &swap_views[i]);
                }
                swapchain_commands = hg_swapchain_commands_create(
                    device.handle, swapchain.handle, cmd_pool);

                aspect = (f32)swapchain.width / (f32)swapchain.height;
                proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
                hg_pipeline_sprite_update_projection(&sprite_pipeline, &proj);
            }

            vkDestroySwapchainKHR(device.handle, old_swapchain, NULL);
            hg_info("window resized\n");
        }

        cpu_time += hg_clock_tick(&cpu_clock);
        VkCommandBuffer cmd = hg_swapchain_commands_acquire_and_begin(device.handle, &swapchain_commands);
        hg_clock_tick(&cpu_clock);
        if (cmd != NULL) {
            u32 image_index = swapchain_commands.current_image;

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

            hg_pipeline_sprite_bind(&sprite_pipeline, cmd);

            HgPipelineSpritePush push = {
                .model = hg_model_matrix_2d(position, hg_svec2(0.5f), 0.0f),
                .uv_pos = {0.0f, 0.0f},
                .uv_size = {1.0f, 1.0f},
            };
            hg_pipeline_sprite_draw(&sprite_pipeline, cmd, &texture, &push);

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

            hg_swapchain_commands_end_and_present(device.queue, &swapchain_commands);
        }
    }

    vkDeviceWaitIdle(device.handle);

    hg_pipeline_sprite_destroy_texture(&sprite_pipeline, &texture);
    hg_pipeline_sprite_destroy(&sprite_pipeline);

    hg_swapchain_commands_destroy(device.handle, &swapchain_commands);
    for (usize i = 0; i < swap_image_count; ++i) {
        vkDestroyImageView(device.handle, swap_views[i], NULL);
    }
    free(swap_views);
    free(swap_images);
    vkDestroySwapchainKHR(device.handle, swapchain.handle, NULL);

    vkDestroyCommandPool(device.handle, cmd_pool, NULL);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device.handle, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    hg_debug_mode(vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, NULL));
    vkDestroyInstance(instance, NULL);

    hg_window_destroy(platform, window);
    hg_platform_destroy(platform);

    hg_info("Tests complete\n");
}

