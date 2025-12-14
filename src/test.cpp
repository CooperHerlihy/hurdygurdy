#include "hurdygurdy.hpp"

int main(void) {

    HgArray<u32> arr = HgArray<u32>::create(hg_persistent_allocator(), 1, 1);

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    arr.push((u32)12);
    arr.push((u32)42);
    arr.push((u32)100);
    arr.push((u32)1000);
    arr.push((u32)999999999999);

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    arr.remove(2);
    arr.pop();

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    {
        HgSystemDescription systems[2]{};
        systems[0].max_components = 1 << 16;
        systems[0].component_size = sizeof(u32);
        systems[0].component_alignment = alignof(u32);
        systems[1].max_components = 1 << 16;
        systems[1].component_size = sizeof(u64);
        systems[1].component_alignment = alignof(u64);

        HgECS ecs = hg_ecs_create(hg_persistent_allocator(), 1 << 16, {systems, hg_countof(systems)});

        HgEntityID e1 = hg_entity_create(&ecs);
        HgEntityID e2 = hg_entity_create(&ecs);
        HgEntityID e3;
        hg_info("e1: %" PRIx64 ", e2: %" PRIx64 "\n", e1, e2);
        hg_entity_destroy(&ecs, e1);
        hg_entity_destroy(&ecs, e2);
        e1 = hg_entity_create(&ecs);
        e2 = hg_entity_create(&ecs);
        e3 = hg_entity_create(&ecs);
        hg_info("e1: %" PRIx64 ", e2: %" PRIx64 ", e3: %" PRIx64 "\n", e1, e2, e3);

        hg_info("0 first iteration\n");
        for (HgEntityID *e = nullptr; hg_ecs_iterate_system(&ecs, 0, &e);) {
            u32 *comp = (u32 *)hg_entity_get_component(&ecs, *e, 0);
            hg_info("iterator: %" PRIu32 "\n", *comp);
        }

        u32 *e1comp0 = (u32 *)hg_entity_add_component(&ecs, e1, 0);
        *e1comp0 = 12;
        u32 *e2comp0 = (u32 *)hg_entity_add_component(&ecs, e2, 0);
        *e2comp0 = 42;
        u32 *e3comp0 = (u32 *)hg_entity_add_component(&ecs, e3, 0);
        *e3comp0 = 100;

        hg_info("0 second iteration\n");
        for (HgEntityID *e = nullptr; hg_ecs_iterate_system(&ecs, 0, &e);) {
            u32 *comp = (u32 *)hg_entity_get_component(&ecs, *e, 0);
            hg_info("iterator: %" PRIu32 "\n", *comp);
        }

        hg_entity_destroy(&ecs, e1);

        hg_info("0 third iteration\n");
        for (HgEntityID *e = nullptr; hg_ecs_iterate_system(&ecs, 0, &e);) {
            u32 *comp = (u32 *)hg_entity_get_component(&ecs, *e, 0);
            hg_info("iterator: %" PRIu32 "\n", *comp);
        }

        hg_ecs_flush_system(&ecs, 0);

        hg_info("0 fourth iteration\n");
        for (HgEntityID *e = nullptr; hg_ecs_iterate_system(&ecs, 0, &e);) {
            u32 *comp = (u32 *)hg_entity_get_component(&ecs, *e, 0);
            hg_info("iterator: %" PRIu32 "\n", *comp);
        }

        u64 *e2comp1 = (u64 *)hg_entity_add_component(&ecs, e2, 1);
        *e2comp1 = 2042;
        u64 *e3comp1 = (u64 *)hg_entity_add_component(&ecs, e3, 1);
        *e3comp1 = 2100;

        hg_info("1 first iteration\n");
        for (HgEntityID *e = nullptr; hg_ecs_iterate_system(&ecs, 1, &e);) {
            u32 *comp0 = (u32 *)hg_entity_get_component(&ecs, *e, 0);
            u64 *comp1 = (u64 *)hg_entity_get_component(&ecs, *e, 1);
            hg_info("sys 1: %" PRIu64 ", sys 0: %" PRIu32 "\n", *comp1, *comp0);
        }

        hg_ecs_destroy(&ecs);
    }

    hg_init();

    HgWindowConfig window_config{};
    window_config.title = "Hg Test";
    window_config.windowed = true;
    window_config.width = 800;
    window_config.height = 600;

    HgWindow *window = hg_window_create(&window_config);

    VkInstance instance = hg_vk_create_instance("HurdyGurdy Test");
    hg_debug_mode(VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance));
    HgSingleQueueDeviceData device = hg_vk_create_single_queue_device(instance);

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = device.gpu;
    allocator_info.device = device.handle;
    allocator_info.instance = instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator allocator;
    vmaCreateAllocator(&allocator_info, &allocator);

    VkCommandPoolCreateInfo cmd_pool_info{};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = device.queue_family;

    VkCommandPool cmd_pool;
    vkCreateCommandPool(device.handle, &cmd_pool_info, nullptr, &cmd_pool);

    VkSurfaceKHR surface = hg_vk_create_surface(instance, window);
    HgSwapchainData swapchain = hg_vk_create_swapchain(device.handle, device.gpu, nullptr, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

    u32 swap_image_count;
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, nullptr);
    VkImage *swap_images = (VkImage *)malloc(swap_image_count * sizeof(*swap_images));
    VkImageView *swap_views = (VkImageView *)malloc(swap_image_count * sizeof(*swap_views));
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images);
    for (usize i = 0; i < swap_image_count; ++i) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        vkCreateImageView(device.handle, &create_info, nullptr, &swap_views[i]);
    }
    HgSwapchainCommands swapchain_commands = hg_swapchain_commands_create(
        device.handle, swapchain.handle, cmd_pool);

    HgPipelineSprite sprite_pipeline = hg_pipeline_sprite_create(
        device.handle, allocator, swapchain.format, VK_FORMAT_UNDEFINED);

    struct {u8 r, g, b, a;} tex_data[] = {
        {0xff, 0x00, 0x00, 0xff}, {0x00, 0xff, 0x00, 0xff},
        {0x00, 0x00, 0xff, 0xff}, {0xff, 0xff, 0x00, 0xff},
    };
    HgPipelineSpriteTextureConfig tex_config{};
    tex_config.tex_data = tex_data;
    tex_config.width = 2;
    tex_config.height = 2;
    tex_config.format = VK_FORMAT_R8G8B8A8_UNORM;
    tex_config.filter = VK_FILTER_NEAREST;
    tex_config.edge_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

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
    game_clock.tick();
    HgClock cpu_clock;
    cpu_clock.tick();

    while(true) {
        f64 delta = game_clock.tick();
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

        hg_window_process_events({&window, 1});
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
                vkDestroyImageView(device.handle, swap_views[i], nullptr);
            }
            hg_swapchain_commands_destroy(device.handle, &swapchain_commands);

            if (swapchain.handle != nullptr) {
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, nullptr);
                if (swap_image_count != old_count) {
                    swap_images = (VkImage *)realloc(swap_images, swap_image_count * sizeof(*swap_images));
                    swap_views = (VkImageView *)realloc(swap_views, swap_image_count * sizeof(*swap_images));
                }
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images);
                for (usize i = 0; i < swap_image_count; ++i) {
                    VkImageViewCreateInfo create_info{};
                    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    create_info.image = swap_images[i];
                    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    create_info.format = swapchain.format;
                    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    create_info.subresourceRange.baseMipLevel = 0;
                    create_info.subresourceRange.levelCount = 1;
                    create_info.subresourceRange.baseArrayLayer = 0;
                    create_info.subresourceRange.layerCount = 1;

                    vkCreateImageView(device.handle, &create_info, nullptr, &swap_views[i]);
                }
                swapchain_commands = hg_swapchain_commands_create(
                    device.handle, swapchain.handle, cmd_pool);

                aspect = (f32)swapchain.width / (f32)swapchain.height;
                proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
                hg_pipeline_sprite_update_projection(&sprite_pipeline, &proj);
            }

            vkDestroySwapchainKHR(device.handle, old_swapchain, nullptr);
            hg_info("window resized\n");
        }

        VkCommandBuffer cmd;
        cpu_time += cpu_clock.tick();
        if (swapchain.handle && (cmd = hg_swapchain_commands_record(device.handle, &swapchain_commands))) {
            cpu_clock.tick();
            u32 image_index = swapchain_commands.current_image;

            VkImageMemoryBarrier2 color_barrier{};
            color_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            color_barrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            color_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            color_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_barrier.image = swap_images[image_index];
            color_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkDependencyInfo color_dependency{};
            color_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            color_dependency.imageMemoryBarrierCount = 1;
            color_dependency.pImageMemoryBarriers = &color_barrier;

            vkCmdPipelineBarrier2(cmd, &color_dependency);

            VkRenderingAttachmentInfo color_attachment{};
            color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            color_attachment.imageView = swap_views[image_index];
            color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            VkRenderingInfo rendering_info{};
            rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            rendering_info.pNext = nullptr;
            rendering_info.renderArea.extent = {swapchain.width, swapchain.height};
            rendering_info.layerCount = 1;
            rendering_info.colorAttachmentCount = 1;
            rendering_info.pColorAttachments = &color_attachment;

            vkCmdBeginRendering(cmd, &rendering_info);

            VkViewport viewport = {0.0f, 0.0f, (f32)swapchain.width, (f32)swapchain.height, 0.0f, 1.0f};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            VkRect2D scissor = {{0, 0}, {swapchain.width, swapchain.height}};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            hg_pipeline_sprite_bind(&sprite_pipeline, cmd);

            HgPipelineSpritePush push{};
            push.model = hg_model_matrix_2d(position, hg_svec2(0.5f), 0.0f);
            push.uv_pos = {0.0f, 0.0f};
            push.uv_size = {1.0f, 1.0f};

            hg_pipeline_sprite_draw(&sprite_pipeline, cmd, &texture, &push);

            vkCmdEndRendering(cmd);

            VkImageMemoryBarrier2 present_barrier{};
            present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            present_barrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            present_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            present_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            present_barrier.image = swap_images[image_index];
            present_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkDependencyInfo present_dependency{};
            present_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            present_dependency.imageMemoryBarrierCount = 1;
            present_dependency.pImageMemoryBarriers = &present_barrier;

            vkCmdPipelineBarrier2(cmd, &present_dependency);

            hg_swapchain_commands_present(device.queue, &swapchain_commands);
        }
    }

    vkDeviceWaitIdle(device.handle);

    hg_pipeline_sprite_destroy_texture(&sprite_pipeline, &texture);
    hg_pipeline_sprite_destroy(&sprite_pipeline);

    hg_swapchain_commands_destroy(device.handle, &swapchain_commands);
    for (usize i = 0; i < swap_image_count; ++i) {
        vkDestroyImageView(device.handle, swap_views[i], nullptr);
    }
    free(swap_views);
    free(swap_images);
    vkDestroySwapchainKHR(device.handle, swapchain.handle, nullptr);

    vkDestroyCommandPool(device.handle, cmd_pool, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device.handle, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    hg_debug_mode(vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr));
    vkDestroyInstance(instance, nullptr);

    hg_window_destroy(window);

    hg_exit();

    hg_info("Tests complete\n");
}

