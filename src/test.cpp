#include "hurdygurdy.hpp"

int main(void) {
    hg_defer(hg_info("Exited successfully\n"));

    hg_run_tests();

    hg_init();
    hg_defer(hg_exit());

    HgStdAllocator mem;

    HgWindow::Config window_config{};
    window_config.title = "Hg Test";
    window_config.windowed = true;
    window_config.width = 800;
    window_config.height = 600;

    HgWindow window = HgWindow::create(window_config);
    hg_defer(window.destroy());

    VkInstance instance = hg_vk_create_instance("HurdyGurdy Test");
    hg_defer(vkDestroyInstance(instance, nullptr));
    hg_debug_mode(
        VkDebugUtilsMessengerEXT debug_messenger = hg_vk_create_debug_messenger(instance);
        hg_defer(vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr));
    )
    HgSingleQueueDeviceData device = hg_vk_create_single_queue_device(instance);
    hg_defer(vkDestroyDevice(device.handle, nullptr));

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = device.gpu;
    allocator_info.device = device.handle;
    allocator_info.instance = instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma;
    vmaCreateAllocator(&allocator_info, &vma);
    hg_defer(vmaDestroyAllocator(vma));

    VkCommandPoolCreateInfo cmd_pool_info{};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = device.queue_family;

    VkCommandPool cmd_pool;
    vkCreateCommandPool(device.handle, &cmd_pool_info, nullptr, &cmd_pool);
    hg_defer(vkDestroyCommandPool(device.handle, cmd_pool, nullptr));

    VkSurfaceKHR surface = hg_vk_create_surface(instance, window);
    hg_defer(vkDestroySurfaceKHR(instance, surface, nullptr));

    HgSwapchainData swapchain = hg_vk_create_swapchain(device.handle, device.gpu, nullptr, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);
    hg_defer(vkDestroySwapchainKHR(device.handle, swapchain.handle, nullptr));

    u32 swap_image_count;
    vkGetSwapchainImagesKHR(device.handle, swapchain.handle, &swap_image_count, nullptr);
    HgSpan<VkImage> swap_images = mem.alloc<VkImage>(swap_image_count);
    hg_defer(mem.free(swap_images));
    HgSpan<VkImageView> swap_views = mem.alloc<VkImageView>(swap_image_count);
    hg_defer(mem.free(swap_views));
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
    hg_defer(
        for (usize i = 0; i < swap_image_count; ++i) {
            vkDestroyImageView(device.handle, swap_views[i], nullptr);
        }
    )
    HgSwapchainCommands swapchain_commands = HgSwapchainCommands::create(
        device.handle, swapchain.handle, cmd_pool);
    hg_defer(swapchain_commands.destroy(device.handle));

    HgPipelineSprite sprite_pipeline = hg_pipeline_sprite_create(
        device.handle, vma, swapchain.format, VK_FORMAT_UNDEFINED);
    hg_defer(hg_pipeline_sprite_destroy(&sprite_pipeline));

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
    hg_defer(hg_pipeline_sprite_destroy_texture(&sprite_pipeline, &texture));

    HgVec3 position{0.0f, 0.0f, 0.0f};

    f32 aspect = (f32)swapchain.width / (f32)swapchain.height;
    HgMat4 proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
    hg_pipeline_sprite_update_projection(&sprite_pipeline, &proj);

    u32 frame_count = 0;
    f64 frame_time = 0.0f;
    f64 cpu_time = 0.0f;
    HgClock game_clock{};
    HgClock cpu_clock{};

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

        hg_process_window_events({&window, 1});
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
            push.model = hg_model_matrix_2d(position, 0.5f, 0.0f);
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
}

hg_test(hg_init_and_exit) {
    for (usize i = 0; i < 16; ++i) {
        hg_init();
        hg_init();
        hg_exit();
        hg_exit();
    }
    return true;
}

hg_test(hg_test) {
    hg_test_assert(true);
    return true;
}

hg_test(hg_matrix_mul) {
    HgMat2f mat{
        {1.0f, 0.0f},
        {1.0f, 0.0f},
    };
    HgVec2f vec{1.0f, 1.0f};

    HgMat2f identity{
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    hg_test_assert(identity * mat == mat);
    hg_test_assert(identity * vec == vec);

    HgMat2f mat_rotated{
        {0.0f, 1.0f},
        {0.0f, 1.0f},
    };
    HgVec2f vec_rotated{-1.0f, 1.0f};

    HgMat2f rotation{
        {0.0f, 1.0f},
        {-1.0f, 0.0f},
    };
    hg_test_assert(rotation * mat == mat_rotated);
    hg_test_assert(rotation * vec == vec_rotated);

    hg_test_assert((identity * rotation) * mat == identity * (rotation * mat));
    hg_test_assert((identity * rotation) * vec == identity * (rotation * vec));
    hg_test_assert((rotation * rotation) * mat == rotation * (rotation * mat));
    hg_test_assert((rotation * rotation) * vec == rotation * (rotation * vec));

    return true;
}

hg_test(hg_quat) {
    HgMat3f identity_mat = 1.0f;
    HgVec3f up_vec{0.0f, -1.0f, 0.0f};
    HgQuatf rotation = hg_axis_angle<f32>({0.0f, 0.0f, -1.0f}, -(f32)hg_pi * 0.5f);

    HgVec3f rotated_vec = hg_rotate(rotation, up_vec);
    HgMat3f rotated_mat = hg_rotate(rotation, identity_mat);

    HgVec3f mat_rotated_vec = rotated_mat * up_vec;

    hg_test_assert(std::abs(rotated_vec.x - 1.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON);

    hg_test_assert(std::abs(mat_rotated_vec.x - rotated_vec.x) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.y) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.z) < FLT_EPSILON);

    return true;
}

hg_test(hg_arena) {
    HgStdAllocator mem;

    HgArena arena = arena.create(mem, 1024).value();
    hg_defer(arena.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(arena.memory != nullptr);
        hg_test_assert(arena.memory.count == 1024);
        hg_test_assert(arena.head == arena.memory.data);

        u32 *alloc_u32 = arena.alloc<u32>();
        hg_test_assert(alloc_u32 == arena.memory.data);

        void *head = arena.head;
        arena.free(alloc_u32);
        hg_test_assert(alloc_u32 == arena.memory.data);
        hg_test_assert(head == arena.head);

        HgSpan<u64> alloc_u64 = arena.alloc<u64>(2);
        hg_test_assert((u8 *)alloc_u64.data == (u8 *)alloc_u32 + 8);

        u8 *alloc_u8 = arena.alloc<u8>();
        hg_test_assert(alloc_u8 == (u8 *)alloc_u32 + 24);

        struct Big {
            u8 data[32];
        };
        Big *alloc_big = arena.alloc<Big>();
        hg_test_assert((u8 *)alloc_big == (u8 *)alloc_u32 + 25);

        HgSpan<Big> realloc_big = arena.realloc(HgSpan<Big>{alloc_big, 1}, 2);
        hg_test_assert(realloc_big.data == alloc_big);

        memset(realloc_big.data, 2, realloc_big.size());

        HgSpan<Big> realloc_big2 = arena.realloc(realloc_big, 2);
        hg_test_assert(realloc_big2.data == realloc_big.data);
        hg_test_assert(memcmp(realloc_big.data, realloc_big2.data, realloc_big2.size()) == 0);

        u8 *alloc_little = arena.alloc<u8>();
        arena.free(alloc_little);

        realloc_big2 = arena.realloc(realloc_big, 2);
        hg_test_assert(realloc_big2.data != realloc_big.data);
        hg_test_assert(memcmp(realloc_big.data, realloc_big2.data, realloc_big2.size()) == 0);

        HgSpan<Big> big_alloc_big = arena.alloc<Big>(100);
        hg_test_assert(big_alloc_big == nullptr);

        arena.reset();
    }

    return true;
}

hg_test(hg_stack) {
    HgStdAllocator mem;

    HgStack stack = stack.create(mem, 1024).value();
    hg_defer(stack.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(stack.memory != nullptr);
        hg_test_assert(stack.memory.count == 1024);
        hg_test_assert(stack.head == 0);

        u8 *alloc_u8_1 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_1 == (u8 *)stack.memory.data);

        u8 *alloc_u8_2 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_2 == alloc_u8_1 + 16);

        stack.free(alloc_u8_2);
        u8 *alloc_u8_3 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_3 == alloc_u8_2);

        HgSpan<u64> alloc_u64 = stack.alloc<u64>(2);
        hg_test_assert((u8 *)alloc_u64.data == alloc_u8_3 + 16);

        HgSpan<u64> realloc_u64 = stack.realloc(alloc_u64, 3);
        hg_test_assert(realloc_u64.data = alloc_u64.data);

        std::fill_n(realloc_u64.data, realloc_u64.count, 2);
        (void)stack.alloc<u8>();
        HgSpan<u64> realloc_u64_2 = stack.realloc(realloc_u64, 3);
        hg_test_assert(realloc_u64_2.data != realloc_u64.data);
        hg_test_assert(memcmp(realloc_u64_2.data, realloc_u64.data, realloc_u64.size()) == 0);

        stack.reset();
    }

    return true;
}

hg_test(hg_array) {
    HgStdAllocator mem;

    HgArray<u32> arr = arr.create(mem, 0, 2).value();
    hg_defer(arr.destroy(mem));
    hg_test_assert(arr.items != nullptr);
    hg_test_assert(arr.capacity == 2);
    hg_test_assert(arr.count == 0);

    arr.push((u32)2);
    hg_test_assert(arr[0] == 2);
    hg_test_assert(arr.count == 1);
    arr.push((u32)4);
    hg_test_assert(arr[1] == 4);
    hg_test_assert(arr.count == 2);

    arr.grow(mem);
    hg_test_assert(arr.capacity == 4);

    arr.push((u32)8);
    hg_test_assert(arr[2] == 8);
    hg_test_assert(arr.count == 3);

    arr.pop();
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr.capacity == 4);

    arr.insert(0, (u32)1);
    hg_test_assert(arr.count == 3);
    hg_test_assert(arr[0] == 1);
    hg_test_assert(arr[1] == 2);
    hg_test_assert(arr[2] == 4);

    arr.remove(1);
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr[0] == 1);
    hg_test_assert(arr[1] == 4);

    for (usize i = 0; i < 100; ++i) {
        if (arr.is_full())
            arr.grow(mem);
        arr.push((u32)i);
    }
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr.capacity >= 102);

    arr.swap_remove(2);
    hg_test_assert(arr.count == 101);
    hg_test_assert(arr[2] == 99);
    hg_test_assert(arr[arr.count - 1] == 98);

    arr.swap_insert(0, (u32)42);
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr[0] == 42);
    hg_test_assert(arr[1] == 4);
    hg_test_assert(arr[2] == 99);
    hg_test_assert(arr[arr.count - 1] == 1);

    return true;
}

hg_test(hg_array_any) {
    HgStdAllocator mem;

    HgArrayAny arr = arr.create(mem, sizeof(u32), alignof(u32), 0, 2).value();
    hg_defer(arr.destroy(mem));
    hg_test_assert(arr.items != nullptr);
    hg_test_assert(arr.capacity == 2);
    hg_test_assert(arr.count == 0);

    *(u32 *)arr.push() = 2;
    hg_test_assert(*(u32 *)arr[0] == 2);
    hg_test_assert(arr.count == 1);
    *(u32 *)arr.push() = 4;
    hg_test_assert(*(u32 *)arr[1] == 4);
    hg_test_assert(arr.count == 2);

    arr.grow(mem);
    hg_test_assert(arr.capacity == 4);

    *(u32 *)arr.push() = 8;
    hg_test_assert(*(u32 *)arr[2] == 8);
    hg_test_assert(arr.count == 3);

    arr.pop();
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr.capacity == 4);

    *(u32 *)arr.insert(0) = 1;
    hg_test_assert(arr.count == 3);
    hg_test_assert(*(u32 *)arr[0] == 1);
    hg_test_assert(*(u32 *)arr[1] == 2);
    hg_test_assert(*(u32 *)arr[2] == 4);

    arr.remove(1);
    hg_test_assert(arr.count == 2);
    hg_test_assert(*(u32 *)arr[0] == 1);
    hg_test_assert(*(u32 *)arr[1] == 4);

    for (u32 i = 0; i < 100; ++i) {
        if (arr.is_full())
            arr.grow(mem);
        *(u32 *)arr.push() = i;
    }
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr.capacity >= 102);

    arr.swap_remove(2);
    hg_test_assert(arr.count == 101);
    hg_test_assert(*(u32 *)arr[2] == 99);
    hg_test_assert(*(u32 *)arr[arr.count - 1] == 98);

    *(u32 *)arr.swap_insert(0) = 42;
    hg_test_assert(arr.count == 102);
    hg_test_assert(*(u32 *)arr[0] == 42);
    hg_test_assert(*(u32 *)arr[1] == 4);
    hg_test_assert(*(u32 *)arr[2] == 99);
    hg_test_assert(*(u32 *)arr[arr.count - 1] == 1);

    return true;
}

hg_test(hg_string) {
    HgStdAllocator mem;
    HgArena arena = arena.create(mem, 4096).value();
    hg_defer(arena.destroy(mem));

    HgString a = a.create(arena, "a").value();
    hg_test_assert(a[0] == 'a');
    hg_test_assert(a.chars.count == 1);
    hg_test_assert(a.length == 1);

    HgString abc = abc.create(arena, "abc").value();
    hg_test_assert(abc[0] == 'a');
    hg_test_assert(abc[1] == 'b');
    hg_test_assert(abc[2] == 'c');
    hg_test_assert(abc.length == 3);
    hg_test_assert(abc.chars.count == 3);

    a.append(arena, "bc");
    hg_test_assert(a == abc);

    HgString str = str.create(arena, 16).value();
    hg_test_assert(str == HgString::create(arena, 0));

    str.append(arena, "hello");
    hg_test_assert(str == HgString::create(arena, "hello"));

    str.append(arena, " there");
    hg_test_assert(str == HgString::create(arena, "hello there"));

    str.prepend(arena, "why ");
    hg_test_assert(str == HgString::create(arena, "why hello there"));

    str.insert(arena, 3, ",");
    hg_test_assert(str == HgString::create(arena, "why, hello there"));

    return true;
}

hg_test(hg_hash_map_int) {
    HgStdAllocator mem;

    constexpr usize count = 128;

    HgHashMap<u32, u32> map = map.create(mem, count).value();
    hg_defer(map.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(map.load == 0);
        hg_test_assert(!map.has(0));
        hg_test_assert(!map.has(1));
        hg_test_assert(!map.has(12));
        hg_test_assert(!map.has(42));
        hg_test_assert(!map.has(100000));

        map.insert(1, 1);
        hg_test_assert(map.load == 1);
        hg_test_assert(map.has(1));
        hg_test_assert(map.get(1) == 1);
        hg_test_assert(map.try_get(1) != nullptr);
        hg_test_assert(*map.try_get(1) == 1);

        map.remove(1);
        hg_test_assert(map.load == 0);
        hg_test_assert(!map.has(1));
        hg_test_assert(map.try_get(1) == nullptr);

        hg_test_assert(!map.has(12));
        hg_test_assert(!map.has(12 + count));

        map.insert(12, 42);
        hg_test_assert(map.load == 1);
        hg_test_assert(map.has(12) && map.get(12) == 42);
        hg_test_assert(!map.has(12 + count));

        map.insert(12 + count, 100);
        hg_test_assert(map.load == 2);
        hg_test_assert(map.has(12) && map.get(12) == 42);
        hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);

        map.insert(12 + count * 2, 200);
        hg_test_assert(map.load == 3);
        hg_test_assert(map.has(12) && map.get(12) == 42);
        hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);
        hg_test_assert(map.has(12 + count * 2) && map.get(12 + count * 2) == 200);

        map.remove(12);
        hg_test_assert(map.load == 2);
        hg_test_assert(!map.has(12));
        hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);

        map.insert(42, 12);
        hg_test_assert(map.load == 3);
        hg_test_assert(map.has(42) && map.get(42) == 12);

        map.remove(12 + count);
        hg_test_assert(map.load == 2);
        hg_test_assert(!map.has(12));
        hg_test_assert(!map.has(12 + count));

        map.remove(42);
        hg_test_assert(map.load == 1);
        hg_test_assert(!map.has(42));

        map.remove(12 + count * 2);
        hg_test_assert(map.load == 0);
        hg_test_assert(!map.has(12));
        hg_test_assert(!map.has(12 + count));
        hg_test_assert(!map.has(12 + count * 2));

        map.reset();
    }

    return true;
}

hg_test(hg_hash_map_str) {
    HgStdAllocator mem;

    {
        using StrHash = usize;

        HgHashMap<StrHash, u32> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        StrHash a = hg_hash("a");
        StrHash b = hg_hash("b");
        StrHash ab = hg_hash("ab");
        StrHash scf = hg_hash("supercalifragilisticexpialidocious");

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a, 1);
        map.insert(b, 2);
        map.insert(ab, 3);
        map.insert(scf, 4);

        hg_test_assert(map.has(a) && map.get(a) == 1);
        hg_test_assert(map.has(b) && map.get(b) == 2);
        hg_test_assert(map.has(ab) && map.get(ab) == 3);
        hg_test_assert(map.has(scf) && map.get(scf) == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgHashMap<const char *, u32> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        const char *a = "a";
        const char *b = "b";
        const char *ab = "ab";
        const char *scf = "supercalifragilisticexpialidocious";

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a, 1);
        map.insert(b, 2);
        map.insert(ab, 3);
        map.insert(scf, 4);

        hg_test_assert(map.has(a) && map.get(a) == 1);
        hg_test_assert(map.has(b) && map.get(b) == 2);
        hg_test_assert(map.has(ab) && map.get(ab) == 3);
        hg_test_assert(map.has(scf) && map.get(scf) == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgHashMap<HgString, u32> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        HgString a = a.create(mem, "a").value();
        hg_defer(a.destroy(mem));
        HgString b = b.create(mem, "b").value();
        hg_defer(b.destroy(mem));
        HgString ab = ab.create(mem, "ab").value();
        hg_defer(ab.destroy(mem));
        HgString scf = scf.create(mem, "supercalifragilisticexpialidocious").value();
        hg_defer(scf.destroy(mem));

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a, 1);
        map.insert(b, 2);
        map.insert(ab, 3);
        map.insert(scf, 4);

        hg_test_assert(map.has(a) && map.get(a) == 1);
        hg_test_assert(map.has(b) && map.get(b) == 2);
        hg_test_assert(map.has(ab) && map.get(ab) == 3);
        hg_test_assert(map.has(scf) && map.get(scf) == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgArena arena = arena.create(mem, 1 << 16).value();
        hg_defer(arena.destroy(mem));

        HgHashMap<HgString, u32> map = map.create(arena, 128).value();

        hg_test_assert(!map.has(HgString::create(arena, "a").value()));
        hg_test_assert(!map.has(HgString::create(arena, "b").value()));
        hg_test_assert(!map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));

        map.insert(HgString::create(arena, "a").value(), 1);
        map.insert(HgString::create(arena, "b").value(), 2);
        map.insert(HgString::create(arena, "ab").value(), 3);
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious").value(), 4);

        hg_test_assert(map.has(HgString::create(arena, "a").value()));
        hg_test_assert(map.get(HgString::create(arena, "a").value()) == 1);
        hg_test_assert(map.has(HgString::create(arena, "b").value()));
        hg_test_assert(map.get(HgString::create(arena, "b").value()) == 2);
        hg_test_assert(map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(map.get(HgString::create(arena, "ab").value()) == 3);
        hg_test_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));
        hg_test_assert(map.get(HgString::create(arena, "supercalifragilisticexpialidocious").value()) == 4);

        map.remove(HgString::create(arena, "a").value());
        map.remove(HgString::create(arena, "b").value());
        map.remove(HgString::create(arena, "ab").value());
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious").value());

        hg_test_assert(!map.has(HgString::create(arena, "a").value()));
        hg_test_assert(!map.has(HgString::create(arena, "b").value()));
        hg_test_assert(!map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));
    }

    return true;
}

hg_test(hg_hash_set_int) {
    HgStdAllocator mem;

    u32 count = 128;

    HgHashSet<u32> set = set.create(mem, count).value();
    hg_defer(set.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(set.load == 0);
        hg_test_assert(!set.has(0));
        hg_test_assert(!set.has(1));
        hg_test_assert(!set.has(12));
        hg_test_assert(!set.has(42));
        hg_test_assert(!set.has(100000));

        set.insert(1);
        hg_test_assert(set.load == 1);
        hg_test_assert(set.has(1));

        set.remove(1);
        hg_test_assert(set.load == 0);
        hg_test_assert(!set.has(1));

        hg_test_assert(!set.has(12));
        hg_test_assert(!set.has(12 + count));

        set.insert(12);
        hg_test_assert(set.load == 1);
        hg_test_assert(set.has(12));
        hg_test_assert(!set.has(12 + count));

        set.insert(12 + count);
        hg_test_assert(set.load == 2);
        hg_test_assert(set.has(12));
        hg_test_assert(set.has(12 + count));

        set.insert(12 + count * 2);
        hg_test_assert(set.load == 3);
        hg_test_assert(set.has(12));
        hg_test_assert(set.has(12 + count));
        hg_test_assert(set.has(12 + count * 2));

        set.remove(12);
        hg_test_assert(set.load == 2);
        hg_test_assert(!set.has(12));
        hg_test_assert(set.has(12 + count));

        set.insert(42);
        hg_test_assert(set.load == 3);
        hg_test_assert(set.has(42));

        set.remove(12 + count);
        hg_test_assert(set.load == 2);
        hg_test_assert(!set.has(12));
        hg_test_assert(!set.has(12 + count));

        set.remove(42);
        hg_test_assert(set.load == 1);
        hg_test_assert(!set.has(42));

        set.remove(12 + count * 2);
        hg_test_assert(set.load == 0);
        hg_test_assert(!set.has(12));
        hg_test_assert(!set.has(12 + count));
        hg_test_assert(!set.has(12 + count * 2));

        for (u32 j = 0; j < 100; ++j) {
            hg_test_assert(!set.has(j * 3));
            set.insert(j * 3);
            hg_test_assert(set.has(j * 3));
        }
        for (u32 j = 0; j < 100; ++j) {
            hg_test_assert(set.has(j * 3));
            set.remove(j * 3);
            hg_test_assert(!set.has(j * 3));
        }

        set.reset();
    }

    return true;
}

hg_test(hg_hash_set_str) {
    HgStdAllocator mem;

    {
        using StrHash = usize;

        HgHashSet<StrHash> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        StrHash a = hg_hash("a");
        StrHash b = hg_hash("b");
        StrHash ab = hg_hash("ab");
        StrHash scf = hg_hash("supercalifragilisticexpialidocious");

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a);
        map.insert(b);
        map.insert(ab);
        map.insert(scf);

        hg_test_assert(map.has(a));
        hg_test_assert(map.has(b));
        hg_test_assert(map.has(ab));
        hg_test_assert(map.has(scf));

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgHashSet<const char *> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        const char *a = "a";
        const char *b = "b";
        const char *ab = "ab";
        const char *scf = "supercalifragilisticexpialidocious";

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a);
        map.insert(b);
        map.insert(ab);
        map.insert(scf);

        hg_test_assert(map.has(a));
        hg_test_assert(map.has(b));
        hg_test_assert(map.has(ab));
        hg_test_assert(map.has(scf));

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgHashSet<HgString> map = map.create(mem, 128).value();
        hg_defer(map.destroy(mem));

        HgString a = a.create(mem, "a").value();
        hg_defer(a.destroy(mem));
        HgString b = b.create(mem, "b").value();
        hg_defer(b.destroy(mem));
        HgString ab = ab.create(mem, "ab").value();
        hg_defer(ab.destroy(mem));
        HgString scf = scf.create(mem, "supercalifragilisticexpialidocious").value();
        hg_defer(scf.destroy(mem));

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));

        map.insert(a);
        map.insert(b);
        map.insert(ab);
        map.insert(scf);

        hg_test_assert(map.has(a));
        hg_test_assert(map.has(b));
        hg_test_assert(map.has(ab));
        hg_test_assert(map.has(scf));

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_test_assert(!map.has(a));
        hg_test_assert(!map.has(b));
        hg_test_assert(!map.has(ab));
        hg_test_assert(!map.has(scf));
    }

    {
        HgArena arena = arena.create(mem, 1 << 16).value();
        hg_defer(arena.destroy(mem));

        HgHashSet<HgString> map = map.create(arena, 128).value();

        hg_test_assert(!map.has(HgString::create(arena, "a").value()));
        hg_test_assert(!map.has(HgString::create(arena, "b").value()));
        hg_test_assert(!map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));

        map.insert(HgString::create(arena, "a").value());
        map.insert(HgString::create(arena, "b").value());
        map.insert(HgString::create(arena, "ab").value());
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious").value());

        hg_test_assert(map.has(HgString::create(arena, "a").value()));
        hg_test_assert(map.has(HgString::create(arena, "b").value()));
        hg_test_assert(map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));

        map.remove(HgString::create(arena, "a").value());
        map.remove(HgString::create(arena, "b").value());
        map.remove(HgString::create(arena, "ab").value());
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious").value());

        hg_test_assert(!map.has(HgString::create(arena, "a").value()));
        hg_test_assert(!map.has(HgString::create(arena, "b").value()));
        hg_test_assert(!map.has(HgString::create(arena, "ab").value()));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious").value()));
    }

    return true;
}

hg_test(hg_function) {
    {
        HgFunction<void()> f{};
        HgFunction<void()> f_copy = f;
        f = f_copy;
        HgFunction<void()> f_move = std::move(f);
        f = std::move(f_move);
    }

    {
        HgStdAllocator mem;

        HgArena arena = arena.create(mem, 4096).value();
        hg_defer(arena.destroy(mem));

        enum State {
            state_no_mul = 0,
            state_mul_2 = 1,
            state_mul_3 = 2,
        } state;

        HgFunction<u32(u32)> mul_maybe = mul_maybe.create(arena, [&](u32 in) -> u32 {
            switch (state) {
                case state_no_mul: return in;
                case state_mul_2: return in * 2;
                case state_mul_3: return in * 3;
                default: return 0;
            }
        }).value();

        state = state_no_mul;
        hg_test_assert(mul_maybe(2) == 2);
        state = state_mul_2;
        hg_test_assert(mul_maybe(3) == 6);
        state = state_mul_3;
        hg_test_assert(mul_maybe(4) == 12);
        state = (State)3;
        hg_test_assert(mul_maybe(5) == 0);
    }

    {
        HgFunction<u32(u32)> mul_2 = [](void *, u32 x) {
            return x * 2;
        };
        hg_test_assert(mul_2.capture == nullptr);

        hg_test_assert(mul_2(2) == 4);
    }

    return true;
}

hg_test(hg_function_view) {
    {
        HgFunctionView<void()> f{};
        HgFunctionView<void()> f_copy = f;
        f = f_copy;
        HgFunctionView<void()> f_move = std::move(f);
        f = std::move(f_move);
    }

    {
        HgFunctionView<u32(u32)> mul_2 = [](void *, u32 x) {
            return x * 2;
        };
        hg_test_assert(mul_2.capture == nullptr);

        hg_test_assert(mul_2(2) == 4);
    }

    {
        bool called = false;

        HgFunctionView<u32(u32)> mul_2{&called, [](void *pcalled, u32 x) {
            *(bool *)pcalled = true;
            return x * 2;
        }};

        hg_test_assert(called == false);
        hg_test_assert(mul_2(2) == 4);
        hg_test_assert(called == true);
    }

    return true;
}

hg_test(hg_thread_pool) {
    HgStdAllocator mem;

    hg_threads_init(std::thread::hardware_concurrency() - 1, 128);
    hg_defer(hg_threads_deinit());

    hg_test_assert(hg_get_thread_pool() != nullptr);

    {
        HgFence fence;

        bool a = false;
        auto a_fn = [&] {
            a = true;
        };
        bool b = false;
        auto b_fn = [&] {
            b = true;
        };

        hg_call_par(&fence, hg_function_view<void()>(a_fn));
        hg_call_par(&fence, hg_function_view<void()>(b_fn));

        hg_test_assert(fence.wait(2.0));

        hg_test_assert(a == true);
        hg_test_assert(b == true);
    }

    {
        HgFence fence;

        bool vals[100] = {};
        for (bool& val : vals) {
            hg_call_par(&fence, {&val, [](void *data) {
                *(bool *)data = true;
            }});
        }

        hg_test_assert(fence.wait(2.0));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        HgFence fence;

        HgArena arena = arena.create(mem, 1 << 16).value();
        hg_defer(arena.destroy(mem));

        bool vals[100] = {};
        for (u32 i = 0; i < hg_countof(vals); ++i) {
            hg_call_par(&fence, hg_function<void()>(arena, [&vals, i]() {
                vals[i] = true;
            }).value());
        }

        hg_test_assert(fence.wait(2.0));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        bool vals[100] = {};

        auto iter = [](void *pvals, usize begin, usize end) {
            hg_assert(begin < end && end <= hg_countof(vals));
            for (; begin < end; ++begin) {
                (*(decltype(vals) *)pvals)[begin] = true;
            }
        };

        hg_for_par(hg_countof(vals), 16, {&vals, iter});

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        bool vals[100] = {};

        auto iter = [&](usize begin, usize end) {
            hg_assert(begin < end && end <= hg_countof(vals));
            for (; begin < end; ++begin) {
                vals[begin] = true;
            }
        };

        hg_for_par(hg_countof(vals), 16, hg_function_view<void(usize, usize)>(iter));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        HgArena arena = arena.create(mem, 1 << 16).value();
        hg_defer(arena.destroy(mem));

        bool vals[100] = {};

        hg_for_par(hg_countof(vals), 16, hg_function<void(usize, usize)>(arena, [&](usize begin, usize end) {
            hg_assert(begin < end && end <= hg_countof(vals));
            for (; begin < end; ++begin) {
                vals[begin] = true;
            }
        }).value());

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    return true;
}

hg_test(hg_ecs) {
    HgStdAllocator mem;

    HgECS ecs = ecs.create(mem, 1 << 16).value();
    hg_defer(ecs.destroy(mem));

    hg_test_assert(ecs.register_component<u32>(mem, 4096));
    hg_test_assert(ecs.register_component<u64>(mem, 4096));

    HgEntity e1 = ecs.spawn();
    HgEntity e2 = ecs.spawn();
    HgEntity e3;
    hg_test_assert(e1 == 0);
    hg_test_assert(e2 == 1);
    hg_test_assert(ecs.is_alive(e1));
    hg_test_assert(ecs.is_alive(e2));
    hg_test_assert(!ecs.is_alive(e3));

    ecs.despawn(e1);
    hg_test_assert(!ecs.is_alive(e1));
    e3 = ecs.spawn();
    hg_test_assert(ecs.is_alive(e3));
    hg_test_assert(e3 == e1);

    e1 = ecs.spawn();
    hg_test_assert(ecs.is_alive(e1));
    hg_test_assert(e1 == 2);

    bool has_unknown = false;
    ecs.for_each<u32>([&](HgEntity, u32&) {
        has_unknown = true;
    });
    hg_test_assert(!has_unknown);

    hg_test_assert(ecs.component_count<u32>() == 0);
    hg_test_assert(ecs.component_count<u64>() == 0);

    ecs.add<u32>(e1) = 12;
    ecs.add<u32>(e2) = 42;
    ecs.add<u32>(e3) = 100;
    hg_test_assert(ecs.component_count<u32>() == 3);
    hg_test_assert(ecs.component_count<u64>() == 0);

    bool has_12 = false;
    bool has_42 = false;
    bool has_100 = false;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        switch (c) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    }
    hg_test_assert(has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(!has_unknown);

    ecs.add<u64>(e2) = 2042;
    ecs.add<u64>(e3) = 2100;
    hg_test_assert(ecs.component_count<u32>() == 3);
    hg_test_assert(ecs.component_count<u64>() == 2);

    has_12 = false;
    has_42 = false;
    has_100 = false;
    bool has_2042 = false;
    bool has_2100 = false;
    ecs.for_each<u32, u64>([&](HgEntity e, u32& comp32, u64& comp64) {
        switch (comp32) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
        switch (comp64) {
            case 2042:
                has_2042 = e == e2;
                break;
            case 2100:
                has_2100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    });
    hg_test_assert(!has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(has_2042);
    hg_test_assert(has_2100);
    hg_test_assert(!has_unknown);

    ecs.despawn(e1);
    hg_test_assert(ecs.component_count<u32>() == 2);
    hg_test_assert(ecs.component_count<u64>() == 2);

    has_12 = false;
    has_42 = false;
    has_100 = false;
    ecs.for_each<u32>([&](HgEntity e, u32& c) {
        switch (c) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    });
    hg_test_assert(!has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(!has_unknown);

    ecs.despawn(e2);
    hg_test_assert(ecs.component_count<u32>() == 1);
    hg_test_assert(ecs.component_count<u64>() == 1);

    ecs.reset();
    hg_test_assert(ecs.component_count<u32>() == 0);
    hg_test_assert(ecs.component_count<u64>() == 0);

    {
        hg_threads_init(std::thread::hardware_concurrency() - 1, 1024);
        hg_defer(hg_threads_deinit());

        HgArena arena = arena.create(mem, 4096).value();
        hg_defer(arena.destroy(mem));

        HgArray<HgEntity> entities = entities.create(arena, 0, 300).value();

        for (u32 i = 0; i < 300; ++i) {
            HgEntity e = entities.push(ecs.spawn());
            switch (i % 3) {
                case 0:
                    ecs.add<u32>(e) = 12;
                    ecs.add<u64>(e) = 42;
                    break;
                case 1:
                    ecs.add<u32>(e) = 12;
                    break;
                case 2:
                    ecs.add<u64>(e) = 42;
                    break;
            }
        }

        ecs.for_each_par<u32>(16, [&](HgEntity, u32& c) {
            c += 4;
        });
        for (auto [e, c] : ecs.component_iter<u32>()) {
            hg_test_assert(c == 16);
        }

        ecs.for_each_par<u64>(16, [&](HgEntity, u64& c) {
            c += 3;
        });
        for (auto [e, c] : ecs.component_iter<u64>()) {
            hg_test_assert(c == 45);
        }

        ecs.for_each_par<u32, u64>(16, [&](HgEntity, u32& c32, u64& c64) {
            c64 -= c32;
        });
        for (auto [e, c] : ecs.component_iter<u64>()) {
            if (ecs.has<u32>(e))
                hg_test_assert(c == 29);
            else
                hg_test_assert(c == 45);
        }

        ecs.reset();
    }

    return true;
}

hg_test(hg_ecs_sort) {
    HgStdAllocator mem;

    HgECS ecs = ecs.create(mem, 128).value();
    hg_defer(ecs.destroy(mem));

    u32 elem;

    hg_test_assert(ecs.register_component<u32>(mem, 128));

    auto comparison = [](u32 lhs, u32 rhs) {
        return lhs < rhs;
    };

    ecs.add<u32>(ecs.spawn()) = 42;
    ecs.sort<u32>(comparison);

    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void) e;
        hg_test_assert(c == 42);
    }

    ecs.reset();

    u32 small_scramble_1[] = {1, 0};
    for (u32 i = 0; i < hg_countof(small_scramble_1); ++i) {
        ecs.add<u32>(ecs.spawn()) = small_scramble_1[i];
    }
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    u32 medium_scramble_1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
    for (u32 i = 0; i < hg_countof(medium_scramble_1); ++i) {
        ecs.add<u32>(ecs.spawn()) = medium_scramble_1[i];
    }
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    u32 medium_scramble_2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
    for (u32 i = 0; i < hg_countof(medium_scramble_2); ++i) {
        ecs.add<u32>(ecs.spawn()) = medium_scramble_2[i];
    }
    ecs.sort<u32>(comparison);
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    for (u32 i = 127; i < 128; --i) {
        ecs.add<u32>(ecs.spawn()) = i;
    }
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    for (u32 i = 127; i < 128; --i) {
        ecs.add<u32>(ecs.spawn()) = i / 2;
    }
    ecs.sort<u32>(comparison);
    ecs.sort<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem / 2);
        ++elem;
    }

    return true;
}

hg_test(hg_file_binary) {
    HgStdAllocator mem;

    u32 save_data[] = {12, 42, 100, 128};

    hg_test_assert(!hg_file_save_binary({save_data, sizeof(save_data), alignof(u32)}, "dir/does/not/exist.bin"));
    hg_test_assert(hg_file_load_binary(mem, "file_does_not_exist.bin") == nullptr);

    hg_test_assert(hg_file_save_binary({save_data, sizeof(save_data), alignof(u32)}, "hg_file_test.bin"));
    HgSpan<void> load_data = hg_file_load_binary(mem, "hg_file_test.bin");
    hg_defer(hg_file_unload_binary(mem, load_data));

    hg_test_assert(load_data != nullptr);
    hg_test_assert(load_data.size() == sizeof(save_data));
    hg_test_assert(memcmp(save_data, load_data.data, load_data.size()) == 0);

    return true;
}

