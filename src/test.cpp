#include "hurdygurdy.hpp"

/**
 * The transform component for all entities with a position
 *
 * Note, 2D entities can just ignore scale.z and rotation.i,j,k
 */
struct HgTransform {
    /**
     * The 3D position
     */
    HgVec3 pos;
    /**
     * The 3D scale
     */
    HgVec3 scale;
    /**
     * The 3D rotation
     */
    HgQuat rotation;
};

struct HgSpriteSystem {
    struct Texture {
        VmaAllocation allocation;
        VkImage image;
        VkImageView view;
        VkSampler sampler;
        VkDescriptorSet set;
    };

    HgSpan<Texture> texture_pool;

};

struct HgSprite {
    u32 texture;
    HgVec2 uv_begin;
    HgVec2 uv_end;
};

int main(void) {
    hg_init();
    HgAllocator& mem = hg_persistent_allocator();

    HgArray<u32> arr = HgArray<u32>::create(mem, 1, 1);

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    arr.push((u32)12);
    arr.push((u32)42);
    arr.push((u32)100);
    arr.push((u32)1000);
    arr.push((u32)999999999);

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    arr.remove(2);
    arr.pop();

    for (usize i = 0; i < arr.count; ++i) {
        hg_info("elem %d: %d\n", (int)i, arr[i]);
    }

    {
        HgECS ecs = HgECS::create(mem, 1 << 16, 2);
        HgSystemID<void, u32> u32_system = ecs.add_system<void, u32>(1 << 16);
        HgSystemID<u8, u64> u64_system = ecs.add_system<u8, u64>(1 << 16);
        ecs.get_system(u64_system) = 5;

        HgEntityID e1 = ecs.create_entity();
        HgEntityID e2 = ecs.create_entity();
        HgEntityID e3;
        hg_info("e1: %" PRIx64 ", e2: %" PRIx64 "\n", e1, e2);
        ecs.destroy_entity(e1);
        ecs.destroy_entity(e2);
        e1 = ecs.create_entity();
        e2 = ecs.create_entity();
        e3 = ecs.create_entity();
        hg_info("e1: %" PRIx64 ", e2: %" PRIx64 ", e3: %" PRIx64 "\n", e1, e2, e3);

        hg_info("u32_system first iteration\n");
        for (HgEntityID *e = nullptr; ecs.iterate_system(u32_system, &e);) {
            u32& comp = ecs.get_component(*e, u32_system);
            hg_info("iterator: %" PRIu32 "\n", comp);
        }

        ecs.add_component(e1, u32_system) = 12;
        ecs.add_component(e2, u32_system) = 42;
        ecs.add_component(e3, u32_system) = 100;

        hg_info("u32_system second iteration\n");
        for (HgEntityID *e = nullptr; ecs.iterate_system(u32_system, &e);) {
            hg_info("iterator: %" PRIu32 "\n", ecs.get_component(*e, u32_system));
        }

        ecs.destroy_entity(e1);

        hg_info("u32_system third iteration\n");
        for (HgEntityID *e = nullptr; ecs.iterate_system(u32_system, &e);) {
            hg_info("iterator: %" PRIu32 "\n", ecs.get_component(*e, u32_system));
        }

        ecs.flush_system(u32_system);

        hg_info("u32_system fourth iteration\n");
        for (HgEntityID *e = nullptr; ecs.iterate_system(u32_system, &e);) {
            hg_info("iterator: %" PRIu32 "\n", ecs.get_component(*e, u32_system));
        }

        ecs.add_component(e2, u64_system) = 2042;
        ecs.add_component(e3, u64_system) = 2100;

        hg_info("u64_system first iteration\n");
        for (HgEntityID *e = nullptr; ecs.iterate_system(u64_system, &e);) {
            hg_info("sys u64_system: %" PRIu64 ", sys u32_system: %" PRIu32 "\n",
                    ecs.get_component(*e, u64_system),
                    ecs.get_component(*e, u32_system));
        }

        hg_info("u64_system data: %d\n", ecs.get_system(u64_system));

        ecs.destroy();
    }

    // HgECS ecs = HgECS::create(mem, 10000, 16);
    // auto transform_system = ecs.add_system<void, HgTransform>(10000);
    // auto sprite_system = ecs.add_system<HgSpriteSystem, HgSprite>(1000);

    HgWindow::Config window_config{};
    window_config.title = "Hg Test";
    window_config.windowed = true;
    window_config.width = 800;
    window_config.height = 600;

    HgWindow window = HgWindow::create(window_config);

    VkInstance instance = hg_vk_create_instance("HurdyGurdy Test");
    hg_debug_mode(VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance));
    HgSingleQueueDeviceData device = hg_vk_create_single_queue_device(instance);

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = device.gpu;
    allocator_info.device = device.handle;
    allocator_info.instance = instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma;
    vmaCreateAllocator(&allocator_info, &vma);

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
    HgSpan<VkImage> swap_images = mem.alloc<VkImage>(swap_image_count);
    HgSpan<VkImageView> swap_views = mem.alloc<VkImageView>(swap_image_count);
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images.data);
    for (usize i = 0; i < swap_image_count; ++i) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain.format;
        create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(device.handle, &create_info, nullptr, &swap_views[i]);
    }
    HgSwapchainCommands swapchain_commands = HgSwapchainCommands::create(
        device.handle, swapchain.handle, cmd_pool);

    HgPipelineSprite sprite_pipeline = hg_pipeline_sprite_create(
        device.handle, vma, swapchain.format, VK_FORMAT_UNDEFINED);

    struct {u8 r, g, b, a;} tex_data[]{
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

    HgVec3 position{0.0f, 0.0f, 0.0f};

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
        if (window.was_closed() || window.is_key_down(HG_KEY_ESCAPE))
            break;

        static const f32 speed = 1.0f;
        if (window.is_key_down(HG_KEY_W))
            position.y -= speed * deltaf;
        if (window.is_key_down(HG_KEY_A))
            position.x -= speed * deltaf;
        if (window.is_key_down(HG_KEY_S))
            position.y += speed * deltaf;
        if (window.is_key_down(HG_KEY_D))
            position.x += speed * deltaf;

        if (window.was_resized()) {
            vkQueueWaitIdle(device.queue);

            VkSwapchainKHR old_swapchain = swapchain.handle;
            swapchain = hg_vk_create_swapchain(device.handle, device.gpu, old_swapchain, surface,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

            for (usize i = 0; i < swap_views.count; ++i) {
                vkDestroyImageView(device.handle, swap_views[i], nullptr);
            }
            swapchain_commands.destroy(device.handle);

            if (swapchain.handle != nullptr) {
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, nullptr);
                if (swap_images.count != swap_image_count) {
                    swap_images = mem.realloc(swap_images, swap_image_count);
                    swap_views = mem.realloc(swap_views, swap_image_count);
                }
                vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, swap_images.data);
                for (usize i = 0; i < swap_image_count; ++i) {
                    VkImageViewCreateInfo create_info{};
                    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    create_info.image = swap_images[i];
                    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    create_info.format = swapchain.format;
                    create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                    vkCreateImageView(device.handle, &create_info, nullptr, &swap_views[i]);
                }
                swapchain_commands = HgSwapchainCommands::create(
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
        if (swapchain.handle && (cmd = swapchain_commands.acquire_and_record(device.handle))) {
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

            VkViewport viewport{0.0f, 0.0f, (f32)swapchain.width, (f32)swapchain.height, 0.0f, 1.0f};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            VkRect2D scissor{{0, 0}, {swapchain.width, swapchain.height}};
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

            swapchain_commands.end_and_present(device.queue);
        }
    }

    vkDeviceWaitIdle(device.handle);

    hg_pipeline_sprite_destroy_texture(&sprite_pipeline, &texture);
    hg_pipeline_sprite_destroy(&sprite_pipeline);

    swapchain_commands.destroy(device.handle);
    for (usize i = 0; i < swap_image_count; ++i) {
        vkDestroyImageView(device.handle, swap_views[i], nullptr);
    }
    mem.free(swap_views);
    mem.free(swap_images);
    vkDestroySwapchainKHR(device.handle, swapchain.handle, nullptr);

    vkDestroyCommandPool(device.handle, cmd_pool, nullptr);
    vmaDestroyAllocator(vma);
    vkDestroyDevice(device.handle, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    hg_debug_mode(vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr));
    vkDestroyInstance(instance, nullptr);

    window.destroy();

    hg_exit();

    hg_info("Tests complete\n");
}

