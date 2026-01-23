#include "hurdygurdy.hpp"

#include <emmintrin.h>

int main(void) {
    hg_defer(hg_info("Exited successfully\n"));

    hg_init();
    hg_defer(hg_exit());

    hg_run_tests();

    HgArenaScope arena = hg_get_scratch();

    HgWindow::Config window_config{};
    window_config.title = "Hg Test";
    window_config.windowed = true;
    window_config.width = 1600;
    window_config.height = 900;

    HgWindow window = HgWindow::create(arena, window_config);
    hg_defer(window.destroy());

    VkCommandPoolCreateInfo cmd_pool_info{};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = hg_vk_queue_family;

    VkCommandPool cmd_pool;
    vkCreateCommandPool(hg_vk_device, &cmd_pool_info, nullptr, &cmd_pool);
    hg_defer(vkDestroyCommandPool(hg_vk_device, cmd_pool, nullptr));

    VkSurfaceKHR surface = hg_vk_create_surface(hg_vk_instance, window);
    hg_defer(vkDestroySurfaceKHR(hg_vk_instance, surface, nullptr));

    HgSwapchainData swapchain = hg_vk_create_swapchain(nullptr, surface,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);
    hg_defer(vkDestroySwapchainKHR(hg_vk_device, swapchain.handle, nullptr));

    u32 swap_image_count;
    vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, nullptr);
    HgSpan<VkImage> swap_images = arena.alloc<VkImage>(swap_image_count);
    HgSpan<VkImageView> swap_views = arena.alloc<VkImageView>(swap_image_count);
    vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, swap_images.data);
    for (usize i = 0; i < swap_image_count; ++i) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain.format;
        create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(hg_vk_device, &create_info, nullptr, &swap_views[i]);
    }
    hg_defer(
        for (usize i = 0; i < swap_image_count; ++i) {
            vkDestroyImageView(hg_vk_device, swap_views[i], nullptr);
        }
    )
    HgSwapchainCommands swapchain_commands = swapchain_commands.create(arena, swapchain.handle, cmd_pool);
    hg_defer(swapchain_commands.destroy());

    HgPipeline2D pipeline2d = pipeline2d.create(arena, 255, swapchain.format, VK_FORMAT_UNDEFINED);
    hg_defer(pipeline2d.destroy());

    struct {u8 r, g, b, a;} tex_data[]{
        {0xff, 0x00, 0x00, 0xff}, {0x00, 0xff, 0x00, 0xff},
        {0x00, 0x00, 0xff, 0xff}, {0xff, 0xff, 0x00, 0xff},
    };

    HgTexture texture;
    texture.pixels = tex_data;
    texture.format = VK_FORMAT_R8G8B8A8_SRGB;
    texture.width = 2;
    texture.height = 2;
    texture.depth = 1;
    texture.location = HgTexture::CPU;

    texture.create_gpu(cmd_pool, VK_FILTER_NEAREST);
    hg_defer(texture.destroy_gpu());

    pipeline2d.add_texture(&texture);
    hg_defer(pipeline2d.remove_texture(&texture));

    hg_ecs->register_component<HgTransform>(arena, 1024);
    hg_ecs->register_component<HgSprite>(arena, 1024);

    HgEntity squares[] = {
        hg_ecs->spawn(),
        hg_ecs->spawn(),
    };

    for (HgEntity square : squares) {
        hg_ecs->add<HgTransform>(square) = {};
        pipeline2d.add_sprite(square, texture, {0.0f}, {1.0f});
    }

    {
        HgTransform& tf = hg_ecs->get<HgTransform>(squares[0]);
        tf.position.x = -0.3f;
        tf.position.z = 0.7f;
    }
    {
        HgTransform& tf = hg_ecs->get<HgTransform>(squares[1]);
        tf.position.x = 0.3f;
        tf.position.z = 1.3f;
    }

    HgTransform camera = {};

    f32 aspect = (f32)swapchain.width / (f32)swapchain.height;
    // HgMat4 proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
    HgMat4 proj = hg_projection_perspective((f32)hg_pi * 0.5f, aspect, 0.1f, 1000.0f);
    pipeline2d.update_projection(proj);

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

        static const f32 rot_speed = 2.0f;
        if (window.is_key_down(HG_KEY_LMOUSE)) {
            f64 x, y;
            window.get_mouse_delta(x, y);
            HgQuat rot_x = hg_axis_angle({0.0f, 1.0f, 0.0f}, (f32)x * rot_speed);
            HgQuat rot_y = hg_axis_angle({-1.0f, 0.0f, 0.0f}, (f32)y * rot_speed);
            camera.rotation = rot_x * camera.rotation * rot_y;
        }

        static const f32 move_speed = 1.5f;
        HgVec3 movement = {0.0f};
        if (window.is_key_down(HG_KEY_SPACE))
            movement.y -= 1.0f;
        if (window.is_key_down(HG_KEY_LSHIFT))
            movement.y += 1.0f;
        if (window.is_key_down(HG_KEY_W))
            movement.z += 1.0f;
        if (window.is_key_down(HG_KEY_S))
            movement.z -= 1.0f;
        if (window.is_key_down(HG_KEY_A))
            movement.x -= 1.0f;
        if (window.is_key_down(HG_KEY_D))
            movement.x += 1.0f;

        if (movement != HgVec3{0.0f}) {
            HgVec3 rotated = hg_rotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
            camera.position += hg_norm(HgVec3{rotated.x, movement.y, rotated.z}) * move_speed * deltaf;
        }

        pipeline2d.update_view(hg_view_matrix(camera.position, camera.scale, camera.rotation));

        if (window.was_resized()) {
            vkQueueWaitIdle(hg_vk_queue);

            VkSwapchainKHR old_swapchain = swapchain.handle;
            swapchain = hg_vk_create_swapchain(old_swapchain, surface,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_PRESENT_MODE_FIFO_KHR);

            for (usize i = 0; i < swap_views.count; ++i) {
                vkDestroyImageView(hg_vk_device, swap_views[i], nullptr);
            }
            swapchain_commands.destroy();

            if (swapchain.handle != nullptr) {
                vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, nullptr);
                if (swap_images.count != swap_image_count) {
                    swap_images = arena.realloc(swap_images, swap_image_count);
                    swap_views = arena.realloc(swap_views, swap_image_count);
                }
                vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, swap_images.data);
                for (usize i = 0; i < swap_image_count; ++i) {
                    VkImageViewCreateInfo create_info{};
                    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    create_info.image = swap_images[i];
                    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    create_info.format = swapchain.format;
                    create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                    vkCreateImageView(hg_vk_device, &create_info, nullptr, &swap_views[i]);
                }
                swapchain_commands = swapchain_commands.create(arena, swapchain.handle, cmd_pool);

                aspect = (f32)swapchain.width / (f32)swapchain.height;
                // proj = hg_projection_orthographic(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);
                proj = hg_projection_perspective((f32)hg_pi * 0.5f, aspect, 0.1f, 1000.0f);
                pipeline2d.update_projection(proj);
            }

            vkDestroySwapchainKHR(hg_vk_device, old_swapchain, nullptr);
            hg_info("window resized\n");
        }

        VkCommandBuffer cmd;
        cpu_time += cpu_clock.tick();
        if (swapchain.handle && (cmd = swapchain_commands.acquire_and_record())) {
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

            pipeline2d.draw(cmd);

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

            swapchain_commands.end_and_present(hg_vk_queue);
        }
    }

    vkDeviceWaitIdle(hg_vk_device);
}

hg_test(HgTest) {
    hg_test_assert(true);
    return true;
}

hg_test(HgMat) {
    HgMat2 mat{
        {1.0f, 0.0f},
        {1.0f, 0.0f},
    };
    HgVec2 vec{1.0f, 1.0f};

    HgMat2 identity{
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    hg_test_assert(identity * mat == mat);
    hg_test_assert(identity * vec == vec);

    HgMat2 mat_rotated{
        {0.0f, 1.0f},
        {0.0f, 1.0f},
    };
    HgVec2 vec_rotated{-1.0f, 1.0f};

    HgMat2 rotation{
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

hg_test(HgQuat) {
    HgMat3 identity_mat = 1.0f;
    HgVec3 up_vec{0.0f, -1.0f, 0.0f};
    HgQuat rotation = hg_axis_angle({0.0f, 0.0f, -1.0f}, -(f32)hg_pi * 0.5f);

    HgVec3 rotated_vec = hg_rotate(rotation, up_vec);
    HgMat3 rotated_mat = hg_rotate(rotation, identity_mat);

    HgVec3 mat_rotated_vec = rotated_mat * up_vec;

    hg_test_assert(std::abs(rotated_vec.x - 1.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON);

    hg_test_assert(std::abs(mat_rotated_vec.x - rotated_vec.x) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.y) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.z) < FLT_EPSILON);

    return true;
}

hg_test(HgArena) {
    void* block = std::malloc(1024);
    hg_defer(std::free(block));

    HgArena arena{block, 1024};

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(arena.memory != nullptr);
        hg_test_assert(arena.capacity == 1024);
        hg_test_assert(arena.head == 0);

        u32* alloc_u32 = arena.alloc<u32>();
        hg_test_assert(alloc_u32 == arena.memory);

        HgSpan<u64> alloc_u64 = arena.alloc<u64>(2);
        hg_test_assert((u8*)alloc_u64.data == (u8*)alloc_u32 + 8);

        u8* alloc_u8 = arena.alloc<u8>();
        hg_test_assert(alloc_u8 == (u8*)alloc_u32 + 24);

        struct Big {
            u8 data[32];
        };
        Big* alloc_big = arena.alloc<Big>();
        hg_test_assert((u8*)alloc_big == (u8*)alloc_u32 + 25);

        HgSpan<Big> realloc_big = arena.realloc(HgSpan<Big>{alloc_big, 1}, 2);
        hg_test_assert(realloc_big.data == alloc_big);

        memset(realloc_big.data, 2, realloc_big.size());

        HgSpan<Big> realloc_big2 = arena.realloc(realloc_big, 2);
        hg_test_assert(realloc_big2.data == realloc_big.data);
        hg_test_assert(memcmp(realloc_big.data, realloc_big2.data, realloc_big2.size()) == 0);

        arena.reset();
    }

    return true;
}

hg_test(HgPool) {
    HgArenaScope arena = hg_get_scratch();

    HgPool<u32> pool = pool.create(arena, 128);

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(pool.first == 0);

        u32* a = pool.alloc();
        hg_test_assert(a == (u32*)pool.slots.data);

        u32* b = pool.alloc();
        hg_test_assert(b != a);

        pool.free(a);
        u32* c = pool.alloc();
        hg_test_assert(c == a);

        pool.free(b);
        pool.free(c);

        u32* items[100]{};

        for (u32 j = 0; j < 100; ++j) {
            items[j] = pool.alloc();
            *items[j] = j + 1000;
        }

        for (u32 j = 0; j < 100; ++j) {
            hg_test_assert(items[j] != nullptr);
            hg_test_assert(*items[j] == j + 1000);
        }

        for (u32 j = 0; j < 100; ++j) {
            pool.free(items[j]);
        }

        for (u32 j = 0; j < 100; ++j) {
            hg_test_assert(*items[j] != j + 1000);
        }

        for (u32 j = 0; j < 100; ++j) {
            items[j] = pool.alloc();
        }

        pool.reset();
    }

    u32* items[256]{};

    for (u32 i = 0; i < 128; ++i) {
        items[i] = pool.alloc();
        *items[i] = i;
    }

    for (u32 i = 0; i < 128; ++i) {
        hg_test_assert(items[i] != nullptr);
        hg_test_assert(*items[i] == i);
    }

    pool.resize(arena, 256);

    for (u32 i = 128; i < 256; ++i) {
        items[i] = pool.alloc();
        *items[i] = i;
    }

    for (u32 i = 0; i < 256; ++i) {
        hg_test_assert(items[i] != nullptr);
        hg_test_assert(*items[i] == i);
    }

    return true;
}

hg_test(HgArray) {
    HgArenaScope arena = hg_get_scratch();

    HgArray<u32> arr = arr.create(arena, 0, 2);
    hg_test_assert(arr.items != nullptr);
    hg_test_assert(arr.capacity == 2);
    hg_test_assert(arr.count == 0);

    arr.push() = 2;
    hg_test_assert(arr[0] == 2);
    hg_test_assert(arr.count == 1);
    arr.push() = 4;
    hg_test_assert(arr[1] == 4);
    hg_test_assert(arr.count == 2);

    arr.grow(arena);
    hg_test_assert(arr.capacity == 4);

    arr.push() = 8;
    hg_test_assert(arr[2] == 8);
    hg_test_assert(arr.count == 3);

    arr.pop();
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr.capacity == 4);

    arr.insert(0) = 1;
    hg_test_assert(arr.count == 3);
    hg_test_assert(arr[0] == 1);
    hg_test_assert(arr[1] == 2);
    hg_test_assert(arr[2] == 4);

    arr.remove(1);
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr[0] == 1);
    hg_test_assert(arr[1] == 4);

    for (u32 i = 0; i < 100; ++i) {
        if (arr.is_full())
            arr.grow(arena);
        arr.push() = i;
    }
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr.capacity >= 102);

    arr.swap_remove(2);
    hg_test_assert(arr.count == 101);
    hg_test_assert(arr[2] == 99);
    hg_test_assert(arr[arr.count - 1] == 98);

    arr.swap_insert(0) = 42;
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr[0] == 42);
    hg_test_assert(arr[1] == 4);
    hg_test_assert(arr[2] == 99);
    hg_test_assert(arr[arr.count - 1] == 1);

    return true;
}

hg_test(HgArrayAny) {
    HgArenaScope arena = hg_get_scratch();

    HgArrayAny arr = arr.create(arena, sizeof(u32), alignof(u32), 0, 2);
    hg_test_assert(arr.items != nullptr);
    hg_test_assert(arr.capacity == 2);
    hg_test_assert(arr.count == 0);

    *(u32*)arr.push() = 2;
    hg_test_assert(*(u32*)arr[0] == 2);
    hg_test_assert(arr.count == 1);
    *(u32*)arr.push() = 4;
    hg_test_assert(*(u32*)arr[1] == 4);
    hg_test_assert(arr.count == 2);

    arr.grow(arena);
    hg_test_assert(arr.capacity == 4);

    *(u32*)arr.push() = 8;
    hg_test_assert(*(u32*)arr[2] == 8);
    hg_test_assert(arr.count == 3);

    arr.pop();
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr.capacity == 4);

    *(u32*)arr.insert(0) = 1;
    hg_test_assert(arr.count == 3);
    hg_test_assert(*(u32*)arr[0] == 1);
    hg_test_assert(*(u32*)arr[1] == 2);
    hg_test_assert(*(u32*)arr[2] == 4);

    arr.remove(1);
    hg_test_assert(arr.count == 2);
    hg_test_assert(*(u32*)arr[0] == 1);
    hg_test_assert(*(u32*)arr[1] == 4);

    for (u32 i = 0; i < 100; ++i) {
        if (arr.is_full())
            arr.grow(arena);
        *(u32*)arr.push() = i;
    }
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr.capacity >= 102);

    arr.swap_remove(2);
    hg_test_assert(arr.count == 101);
    hg_test_assert(*(u32*)arr[2] == 99);
    hg_test_assert(*(u32*)arr[arr.count - 1] == 98);

    *(u32*)arr.swap_insert(0) = 42;
    hg_test_assert(arr.count == 102);
    hg_test_assert(*(u32*)arr[0] == 42);
    hg_test_assert(*(u32*)arr[1] == 4);
    hg_test_assert(*(u32*)arr[2] == 99);
    hg_test_assert(*(u32*)arr[arr.count - 1] == 1);

    return true;
}

hg_test(HgQueue) {
    HgArenaScope arena = hg_get_scratch();

    HgRingBuffer<u32> queue = queue.create(arena, 8);

    hg_test_assert(queue.items != nullptr);
    hg_test_assert(queue.capacity == 8);
    hg_test_assert(queue.head == 0);
    hg_test_assert(queue.tail == 0);

    for (usize i = 0; i < 8; ++i) {
        for (u32 j = 0; j < 7; ++j) {
            hg_test_assert(queue.count() == j);
            queue.push() = j;
        }

        hg_test_assert(queue.pop() == 0);
        hg_test_assert(queue.pop() == 1);
        hg_test_assert(queue.pop() == 2);
        hg_test_assert(queue.pop() == 3);

        for (u32 j = 7; j < 10; ++j) {
            hg_test_assert(queue.count() == j - 4);
            queue.push() = j;
        }

        hg_test_assert(queue.pop() == 4);
        hg_test_assert(queue.pop() == 5);
        hg_test_assert(queue.pop() == 6);
        hg_test_assert(queue.pop() == 7);
        hg_test_assert(queue.pop() == 8);
        hg_test_assert(queue.pop() == 9);

        hg_test_assert(queue.count() == 0);
    }

    return true;
}

hg_test(HgHashMap) {
    {
        HgArenaScope arena = hg_get_scratch();

        constexpr usize count = 128;

        HgHashMap<u32, u32> map = map.create(arena, count);

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
    }

    {
        HgArenaScope arena = hg_get_scratch();

        using StrHash = usize;

        HgHashMap<StrHash, u32> map = map.create(arena, 128);

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
        HgArenaScope arena = hg_get_scratch();

        HgHashMap<const char*, u32> map = map.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

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
        HgArenaScope arena = hg_get_scratch();

        HgHashMap<HgString, u32> map = map.create(arena, 128);

        hg_test_assert(!map.has(HgString::create(arena, "a")));
        hg_test_assert(!map.has(HgString::create(arena, "b")));
        hg_test_assert(!map.has(HgString::create(arena, "ab")));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.insert(HgString::create(arena, "a"), 1);
        map.insert(HgString::create(arena, "b"), 2);
        map.insert(HgString::create(arena, "ab"), 3);
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"), 4);

        hg_test_assert(map.has(HgString::create(arena, "a")));
        hg_test_assert(map.get(HgString::create(arena, "a")) == 1);
        hg_test_assert(map.has(HgString::create(arena, "b")));
        hg_test_assert(map.get(HgString::create(arena, "b")) == 2);
        hg_test_assert(map.has(HgString::create(arena, "ab")));
        hg_test_assert(map.get(HgString::create(arena, "ab")) == 3);
        hg_test_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
        hg_test_assert(map.get(HgString::create(arena, "supercalifragilisticexpialidocious")) == 4);

        map.remove(HgString::create(arena, "a"));
        map.remove(HgString::create(arena, "b"));
        map.remove(HgString::create(arena, "ab"));
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_test_assert(!map.has(HgString::create(arena, "a")));
        hg_test_assert(!map.has(HgString::create(arena, "b")));
        hg_test_assert(!map.has(HgString::create(arena, "ab")));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArenaScope arena = hg_get_scratch();

        HgHashMap<HgStringView, u32> map = map.create(arena, 128);

        hg_test_assert(!map.has("a"));
        hg_test_assert(!map.has("b"));
        hg_test_assert(!map.has("ab"));
        hg_test_assert(!map.has("supercalifragilisticexpialidocious"));

        map.insert(HgString::create(arena, "a"), 1);
        map.insert(HgString::create(arena, "b"), 2);
        map.insert(HgString::create(arena, "ab"), 3);
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"), 4);

        hg_test_assert(map.has("a"));
        hg_test_assert(map.get("a") == 1);
        hg_test_assert(map.has("b"));
        hg_test_assert(map.get("b") == 2);
        hg_test_assert(map.has("ab"));
        hg_test_assert(map.get("ab") == 3);
        hg_test_assert(map.has("supercalifragilisticexpialidocious"));
        hg_test_assert(map.get("supercalifragilisticexpialidocious") == 4);

        map.remove("a");
        map.remove("b");
        map.remove("ab");
        map.remove("supercalifragilisticexpialidocious");

        hg_test_assert(!map.has("a"));
        hg_test_assert(!map.has("b"));
        hg_test_assert(!map.has("ab"));
        hg_test_assert(!map.has("supercalifragilisticexpialidocious"));
    }

    return true;
}

hg_test(HgHashSet) {
    {
        HgArenaScope arena = hg_get_scratch();

        u32 count = 128;

        HgHashSet<u32> set = set.create(arena, count);

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
    }

    {
        HgArenaScope arena = hg_get_scratch();

        using StrHash = usize;

        HgHashSet<StrHash> map = map.create(arena, 128);

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
        HgArenaScope arena = hg_get_scratch();

        HgHashSet<const char* > map = map.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

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
        HgArenaScope arena = hg_get_scratch();

        HgHashSet<HgString> map = map.create(arena, 128);

        HgString a = a.create(arena, "a");
        HgString b = b.create(arena, "b");
        HgString ab = ab.create(arena, "ab");
        HgString scf = scf.create(arena, "supercalifragilisticexpialidocious");

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
        HgArenaScope arena = hg_get_scratch();

        HgHashSet<HgString> map = map.create(arena, 128);

        hg_test_assert(!map.has(HgString::create(arena, "a")));
        hg_test_assert(!map.has(HgString::create(arena, "b")));
        hg_test_assert(!map.has(HgString::create(arena, "ab")));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.insert(HgString::create(arena, "a"));
        map.insert(HgString::create(arena, "b"));
        map.insert(HgString::create(arena, "ab"));
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_test_assert(map.has(HgString::create(arena, "a")));
        hg_test_assert(map.has(HgString::create(arena, "b")));
        hg_test_assert(map.has(HgString::create(arena, "ab")));
        hg_test_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.remove(HgString::create(arena, "a"));
        map.remove(HgString::create(arena, "b"));
        map.remove(HgString::create(arena, "ab"));
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_test_assert(!map.has(HgString::create(arena, "a")));
        hg_test_assert(!map.has(HgString::create(arena, "b")));
        hg_test_assert(!map.has(HgString::create(arena, "ab")));
        hg_test_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArenaScope arena = hg_get_scratch();

        HgHashSet<HgStringView> map = map.create(arena, 128);

        hg_test_assert(!map.has("a"));
        hg_test_assert(!map.has("b"));
        hg_test_assert(!map.has("ab"));
        hg_test_assert(!map.has("supercalifragilisticexpialidocious"));

        map.insert(HgString::create(arena, "a"));
        map.insert(HgString::create(arena, "b"));
        map.insert(HgString::create(arena, "ab"));
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_test_assert(map.has("a"));
        hg_test_assert(map.has("b"));
        hg_test_assert(map.has("ab"));
        hg_test_assert(map.has("supercalifragilisticexpialidocious"));

        map.remove("a");
        map.remove("b");
        map.remove("ab");
        map.remove("supercalifragilisticexpialidocious");

        hg_test_assert(!map.has("a"));
        hg_test_assert(!map.has("b"));
        hg_test_assert(!map.has("ab"));
        hg_test_assert(!map.has("supercalifragilisticexpialidocious"));
    }

    return true;
}

hg_test(HgFunction) {
    {
        HgFunction<void()> f{};
        HgFunction<void()> f_copy = f;
        f = f_copy;
        HgFunction<void()> f_move = std::move(f);
        f = std::move(f_move);
    }

    {
        HgArenaScope arena = hg_get_scratch();

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
        });

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
        HgFunction<u32(u32)> mul_2{};
        mul_2.fn = [](void*, u32 x) {
            return x * 2;
        };
        hg_test_assert(mul_2.capture == nullptr);

        hg_test_assert(mul_2(2) == 4);
    }

    return true;
}

hg_test(HgString) {
    {
        HgArenaScope arena = hg_get_scratch();

        HgString a = a.create(arena, "a");
        hg_test_assert(a[0] == 'a');
        hg_test_assert(a.chars.count == 1);
        hg_test_assert(a.length == 1);

        HgString abc = abc.create(arena, "abc");
        hg_test_assert(abc[0] == 'a');
        hg_test_assert(abc[1] == 'b');
        hg_test_assert(abc[2] == 'c');
        hg_test_assert(abc.length == 3);
        hg_test_assert(abc.chars.count == 3);

        a.append(arena, "bc");
        hg_test_assert(a == abc);

        HgString str = str.create(arena, 16);
        hg_test_assert(str == HgString::create(arena, 0));

        str.append(arena, "hello");
        hg_test_assert(str == HgString::create(arena, "hello"));

        str.append(arena, " there");
        hg_test_assert(str == HgString::create(arena, "hello there"));

        str.prepend(arena, "why ");
        hg_test_assert(str == HgString::create(arena, "why hello there"));

        str.insert(arena, 3, ",");
        hg_test_assert(str == HgString::create(arena, "why, hello there"));
    }

    return true;
}

hg_test(hg_string_utils) {
    HgArenaScope arena = hg_get_scratch();

    hg_test_assert(hg_is_whitespace(' '));
    hg_test_assert(hg_is_whitespace('\t'));
    hg_test_assert(hg_is_whitespace('\n'));

    hg_test_assert(hg_is_numeral_base10('0'));
    hg_test_assert(hg_is_numeral_base10('1'));
    hg_test_assert(hg_is_numeral_base10('2'));
    hg_test_assert(hg_is_numeral_base10('3'));
    hg_test_assert(hg_is_numeral_base10('4'));
    hg_test_assert(hg_is_numeral_base10('5'));
    hg_test_assert(hg_is_numeral_base10('5'));
    hg_test_assert(hg_is_numeral_base10('6'));
    hg_test_assert(hg_is_numeral_base10('7'));
    hg_test_assert(hg_is_numeral_base10('8'));
    hg_test_assert(hg_is_numeral_base10('9'));

    hg_test_assert(!hg_is_numeral_base10('0' - 1));
    hg_test_assert(!hg_is_numeral_base10('9' + 1));

    hg_test_assert(!hg_is_numeral_base10('x'));
    hg_test_assert(!hg_is_numeral_base10('a'));
    hg_test_assert(!hg_is_numeral_base10('b'));
    hg_test_assert(!hg_is_numeral_base10('c'));
    hg_test_assert(!hg_is_numeral_base10('d'));
    hg_test_assert(!hg_is_numeral_base10('e'));
    hg_test_assert(!hg_is_numeral_base10('f'));
    hg_test_assert(!hg_is_numeral_base10('X'));
    hg_test_assert(!hg_is_numeral_base10('A'));
    hg_test_assert(!hg_is_numeral_base10('B'));
    hg_test_assert(!hg_is_numeral_base10('C'));
    hg_test_assert(!hg_is_numeral_base10('D'));
    hg_test_assert(!hg_is_numeral_base10('E'));
    hg_test_assert(!hg_is_numeral_base10('F'));

    hg_test_assert(!hg_is_numeral_base10('.'));
    hg_test_assert(!hg_is_numeral_base10('+'));
    hg_test_assert(!hg_is_numeral_base10('-'));
    hg_test_assert(!hg_is_numeral_base10('*'));
    hg_test_assert(!hg_is_numeral_base10('/'));
    hg_test_assert(!hg_is_numeral_base10('='));
    hg_test_assert(!hg_is_numeral_base10('#'));
    hg_test_assert(!hg_is_numeral_base10('&'));
    hg_test_assert(!hg_is_numeral_base10('^'));
    hg_test_assert(!hg_is_numeral_base10('~'));

    hg_test_assert(hg_is_integer_base10("0"));
    hg_test_assert(hg_is_integer_base10("1"));
    hg_test_assert(hg_is_integer_base10("2"));
    hg_test_assert(hg_is_integer_base10("3"));
    hg_test_assert(hg_is_integer_base10("4"));
    hg_test_assert(hg_is_integer_base10("5"));
    hg_test_assert(hg_is_integer_base10("6"));
    hg_test_assert(hg_is_integer_base10("7"));
    hg_test_assert(hg_is_integer_base10("8"));
    hg_test_assert(hg_is_integer_base10("9"));
    hg_test_assert(hg_is_integer_base10("10"));

    hg_test_assert(hg_is_integer_base10("12"));
    hg_test_assert(hg_is_integer_base10("42"));
    hg_test_assert(hg_is_integer_base10("100"));
    hg_test_assert(hg_is_integer_base10("123456789"));
    hg_test_assert(hg_is_integer_base10("-12"));
    hg_test_assert(hg_is_integer_base10("-42"));
    hg_test_assert(hg_is_integer_base10("-100"));
    hg_test_assert(hg_is_integer_base10("-123456789"));
    hg_test_assert(hg_is_integer_base10("+12"));
    hg_test_assert(hg_is_integer_base10("+42"));
    hg_test_assert(hg_is_integer_base10("+100"));
    hg_test_assert(hg_is_integer_base10("+123456789"));

    hg_test_assert(!hg_is_integer_base10("hello"));
    hg_test_assert(!hg_is_integer_base10("not a number"));
    hg_test_assert(!hg_is_integer_base10("number"));
    hg_test_assert(!hg_is_integer_base10("integer"));
    hg_test_assert(!hg_is_integer_base10("0.0"));
    hg_test_assert(!hg_is_integer_base10("1.0"));
    hg_test_assert(!hg_is_integer_base10(".10"));
    hg_test_assert(!hg_is_integer_base10("1e2"));
    hg_test_assert(!hg_is_integer_base10("1f"));
    hg_test_assert(!hg_is_integer_base10("0xff"));
    hg_test_assert(!hg_is_integer_base10("--42"));
    hg_test_assert(!hg_is_integer_base10("++42"));
    hg_test_assert(!hg_is_integer_base10("42-"));
    hg_test_assert(!hg_is_integer_base10("42+"));
    hg_test_assert(!hg_is_integer_base10("4 2"));
    hg_test_assert(!hg_is_integer_base10("4+2"));

    hg_test_assert(hg_is_float_base10("0.0"));
    hg_test_assert(hg_is_float_base10("1."));
    hg_test_assert(hg_is_float_base10("2.0"));
    hg_test_assert(hg_is_float_base10("3."));
    hg_test_assert(hg_is_float_base10("4.0"));
    hg_test_assert(hg_is_float_base10("5."));
    hg_test_assert(hg_is_float_base10("6.0"));
    hg_test_assert(hg_is_float_base10("7."));
    hg_test_assert(hg_is_float_base10("8.0"));
    hg_test_assert(hg_is_float_base10("9."));
    hg_test_assert(hg_is_float_base10("10.0"));

    hg_test_assert(hg_is_float_base10("0.0"));
    hg_test_assert(hg_is_float_base10(".1"));
    hg_test_assert(hg_is_float_base10("0.2"));
    hg_test_assert(hg_is_float_base10(".3"));
    hg_test_assert(hg_is_float_base10("0.4"));
    hg_test_assert(hg_is_float_base10(".5"));
    hg_test_assert(hg_is_float_base10("0.6"));
    hg_test_assert(hg_is_float_base10(".7"));
    hg_test_assert(hg_is_float_base10("0.8"));
    hg_test_assert(hg_is_float_base10(".9"));
    hg_test_assert(hg_is_float_base10("0.10"));

    hg_test_assert(hg_is_float_base10("1.0"));
    hg_test_assert(hg_is_float_base10("+10.f"));
    hg_test_assert(hg_is_float_base10(".10"));
    hg_test_assert(hg_is_float_base10("-999.999f"));
    hg_test_assert(hg_is_float_base10("1e3"));
    hg_test_assert(hg_is_float_base10("1e3"));
    hg_test_assert(hg_is_float_base10("+1.e3f"));
    hg_test_assert(hg_is_float_base10(".1e3"));

    hg_test_assert(!hg_is_float_base10("hello"));
    hg_test_assert(!hg_is_float_base10("not a number"));
    hg_test_assert(!hg_is_float_base10("number"));
    hg_test_assert(!hg_is_float_base10("float"));
    hg_test_assert(!hg_is_float_base10("1.0ff"));
    hg_test_assert(!hg_is_float_base10("0x1.0"));
    hg_test_assert(!hg_is_float_base10("-0x1.0"));

    hg_test_assert(hg_str_to_int_base10("0") == 0);
    hg_test_assert(hg_str_to_int_base10("1") == 1);
    hg_test_assert(hg_str_to_int_base10("2") == 2);
    hg_test_assert(hg_str_to_int_base10("3") == 3);
    hg_test_assert(hg_str_to_int_base10("4") == 4);
    hg_test_assert(hg_str_to_int_base10("5") == 5);
    hg_test_assert(hg_str_to_int_base10("6") == 6);
    hg_test_assert(hg_str_to_int_base10("7") == 7);
    hg_test_assert(hg_str_to_int_base10("8") == 8);
    hg_test_assert(hg_str_to_int_base10("9") == 9);

    hg_test_assert(hg_str_to_int_base10("0000000") == 0);
    hg_test_assert(hg_str_to_int_base10("+0000001") == +1);
    hg_test_assert(hg_str_to_int_base10("0000002") == 2);
    hg_test_assert(hg_str_to_int_base10("-0000003") == -3);
    hg_test_assert(hg_str_to_int_base10("0000004") == 4);
    hg_test_assert(hg_str_to_int_base10("+0000005") == +5);
    hg_test_assert(hg_str_to_int_base10("0000006") == 6);
    hg_test_assert(hg_str_to_int_base10("-0000007") == -7);
    hg_test_assert(hg_str_to_int_base10("0000008") == 8);
    hg_test_assert(hg_str_to_int_base10("+0000009") == +9);

    hg_test_assert(hg_str_to_int_base10("0000000") == 0);
    hg_test_assert(hg_str_to_int_base10("1000000") == 1000000);
    hg_test_assert(hg_str_to_int_base10("2000000") == 2000000);
    hg_test_assert(hg_str_to_int_base10("3000000") == 3000000);
    hg_test_assert(hg_str_to_int_base10("4000000") == 4000000);
    hg_test_assert(hg_str_to_int_base10("5000000") == 5000000);
    hg_test_assert(hg_str_to_int_base10("6000000") == 6000000);
    hg_test_assert(hg_str_to_int_base10("7000000") == 7000000);
    hg_test_assert(hg_str_to_int_base10("8000000") == 8000000);
    hg_test_assert(hg_str_to_int_base10("9000000") == 9000000);
    hg_test_assert(hg_str_to_int_base10("1234567890") == 1234567890);

    hg_test_assert(hg_str_to_float_base10("0.0") == 0.0);
    hg_test_assert(hg_str_to_float_base10("1.0f") == 1.0);
    hg_test_assert(hg_str_to_float_base10("2.0") == 2.0);
    hg_test_assert(hg_str_to_float_base10("3.0f") == 3.0);
    hg_test_assert(hg_str_to_float_base10("4.0") == 4.0);
    hg_test_assert(hg_str_to_float_base10("5.0f") == 5.0);
    hg_test_assert(hg_str_to_float_base10("6.0") == 6.0);
    hg_test_assert(hg_str_to_float_base10("7.0f") == 7.0);
    hg_test_assert(hg_str_to_float_base10("8.0") == 8.0);
    hg_test_assert(hg_str_to_float_base10("9.0f") == 9.0);

    hg_test_assert(hg_str_to_float_base10("0e1") == 0.0);
    hg_test_assert(hg_str_to_float_base10("1e2f") == 1e2);
    hg_test_assert(hg_str_to_float_base10("2e3") == 2e3);
    hg_test_assert(hg_str_to_float_base10("3e4f") == 3e4);
    hg_test_assert(hg_str_to_float_base10("4e5") == 4e5);
    hg_test_assert(hg_str_to_float_base10("5e6f") == 5e6);
    hg_test_assert(hg_str_to_float_base10("6e7") == 6e7);
    hg_test_assert(hg_str_to_float_base10("7e8f") == 7e8);
    hg_test_assert(hg_str_to_float_base10("8e9") == 8e9);
    hg_test_assert(hg_str_to_float_base10("9e10f") == 9e10);

    hg_test_assert(hg_str_to_float_base10("0e1") == 0.0);
    hg_test_assert(hg_str_to_float_base10("1e2f") == 1e2);
    hg_test_assert(hg_str_to_float_base10("2e3") == 2e3);
    hg_test_assert(hg_str_to_float_base10("3e4f") == 3e4);
    hg_test_assert(hg_str_to_float_base10("4e5") == 4e5);
    hg_test_assert(hg_str_to_float_base10("5e6f") == 5e6);
    hg_test_assert(hg_str_to_float_base10("6e7") == 6e7);
    hg_test_assert(hg_str_to_float_base10("7e8f") == 7e8);
    hg_test_assert(hg_str_to_float_base10("8e9") == 8e9);
    hg_test_assert(hg_str_to_float_base10("9e10f") == 9e10);

    hg_test_assert(hg_str_to_float_base10(".1") == .1);
    hg_test_assert(hg_str_to_float_base10("+.1") == +.1);
    hg_test_assert(hg_str_to_float_base10("-.1") == -.1);
    hg_test_assert(hg_str_to_float_base10("+.1e5") == +.1e5);

    hg_test_assert(hg_int_to_str_base10(arena, 0) == "0");
    hg_test_assert(hg_int_to_str_base10(arena, -1) == "-1");
    hg_test_assert(hg_int_to_str_base10(arena, 2) == "2");
    hg_test_assert(hg_int_to_str_base10(arena, -3) == "-3");
    hg_test_assert(hg_int_to_str_base10(arena, 4) == "4");
    hg_test_assert(hg_int_to_str_base10(arena, -5) == "-5");
    hg_test_assert(hg_int_to_str_base10(arena, 6) == "6");
    hg_test_assert(hg_int_to_str_base10(arena, -7) == "-7");
    hg_test_assert(hg_int_to_str_base10(arena, 8) == "8");
    hg_test_assert(hg_int_to_str_base10(arena, -9) == "-9");

    hg_test_assert(hg_int_to_str_base10(arena, 0000000) == "0");
    hg_test_assert(hg_int_to_str_base10(arena, -1000000) == "-1000000");
    hg_test_assert(hg_int_to_str_base10(arena, 2000000) == "2000000");
    hg_test_assert(hg_int_to_str_base10(arena, -3000000) == "-3000000");
    hg_test_assert(hg_int_to_str_base10(arena, 4000000) == "4000000");
    hg_test_assert(hg_int_to_str_base10(arena, -5000000) == "-5000000");
    hg_test_assert(hg_int_to_str_base10(arena, 6000000) == "6000000");
    hg_test_assert(hg_int_to_str_base10(arena, -7000000) == "-7000000");
    hg_test_assert(hg_int_to_str_base10(arena, 8000000) == "8000000");
    hg_test_assert(hg_int_to_str_base10(arena, -9000000) == "-9000000");
    hg_test_assert(hg_int_to_str_base10(arena, 1234567890) == "1234567890");

    hg_test_assert(hg_float_to_str_base10(arena, 0.0, 10) == "0.0");
    hg_test_assert(hg_float_to_str_base10(arena, -1.0f, 1) == "-1.0");
    hg_test_assert(hg_float_to_str_base10(arena, 2.0, 2) == "2.00");
    hg_test_assert(hg_float_to_str_base10(arena, -3.0f, 3) == "-3.000");
    hg_test_assert(hg_float_to_str_base10(arena, 4.0, 4) == "4.0000");
    hg_test_assert(hg_float_to_str_base10(arena, -5.0f, 5) == "-5.00000");
    hg_test_assert(hg_float_to_str_base10(arena, 6.0, 6) == "6.000000");
    hg_test_assert(hg_float_to_str_base10(arena, -7.0f, 7) == "-7.0000000");
    hg_test_assert(hg_float_to_str_base10(arena, 8.0, 8) == "8.00000000");
    hg_test_assert(hg_float_to_str_base10(arena, -9.0f, 9) == "-9.000000000");

    hg_test_assert(hg_float_to_str_base10(arena, 0e0, 1) == "0.0");
    hg_test_assert(hg_float_to_str_base10(arena, -1e1f, 0) == "-10.");
    hg_test_assert(hg_float_to_str_base10(arena, 2e2, 1) == "200.0");
    hg_test_assert(hg_float_to_str_base10(arena, -3e3f, 0) == "-3000.");
    hg_test_assert(hg_float_to_str_base10(arena, 4e4, 1) == "40000.0");
    hg_test_assert(hg_float_to_str_base10(arena, -5e5f, 0) == "-500000.");
    hg_test_assert(hg_float_to_str_base10(arena, 6e6, 1) == "6000000.0");
    hg_test_assert(hg_float_to_str_base10(arena, -7e7f, 0) == "-70000000.");
    hg_test_assert(hg_float_to_str_base10(arena, 8e8, 1) == "800000000.0");
    hg_test_assert(hg_float_to_str_base10(arena, -9e9f, 0) == "-8999999488.");

    hg_test_assert(hg_float_to_str_base10(arena, -0e-0, 3) == "0.0");
    hg_test_assert(hg_float_to_str_base10(arena, 1e-1f, 3) == "0.100");
    hg_test_assert(hg_float_to_str_base10(arena, -2e-2, 3) == "-0.020");
    hg_test_assert(hg_float_to_str_base10(arena, 3e-3f, 3) == "0.003");
    hg_test_assert(hg_float_to_str_base10(arena, -4e-0, 3) == "-4.000");
    hg_test_assert(hg_float_to_str_base10(arena, 5e-1f, 3) == "0.500");
    hg_test_assert(hg_float_to_str_base10(arena, -6e-2, 3) == "-0.060");
    hg_test_assert(hg_float_to_str_base10(arena, 7e-3f, 3) == "0.007");
    hg_test_assert(hg_float_to_str_base10(arena, -8e-0, 3) == "-8.000");
    hg_test_assert(hg_float_to_str_base10(arena, 9e-1f, 3) == "0.899");

    return true;
}

hg_test(HgJsonParser) {
    {
        HgArenaScope arena = hg_get_scratch();

        HgStringView file = R"(
            {
                "resource": {
                    "id": 1234,
                    "path": "tex.png",
                },
                "num array": [1, 2, 3],
                "entities": [
                    {
                        "id": "player",
                        "pos": [0.0, 0.0, 0.0],
                        "is_target": true,
                    },
                    {
                        "id": "enemy",
                        "pos": [10.0, -5.0, 1.0],
                        "is_target": false,
                    },
                ],
            },
        )";

        HgJsonParser json = json.create(file);

        HgJsonParser::Token token;

        {
            token = json.next_token(arena);
            hg_test_assert(token.type == HgJsonParser::STRUCT_BEGIN);
            hg_test_assert(token.literal == HgJsonParser::EMPTY);

            token = json.next_token(arena);
            hg_test_assert(token.type == HgJsonParser::FIELD);
            hg_test_assert(token.literal == HgJsonParser::STRING);
            hg_test_assert(token.string == "resource");

            {
                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::STRUCT_BEGIN);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::FIELD);
                hg_test_assert(token.literal == HgJsonParser::STRING);
                hg_test_assert(token.string == "id");

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::LITERAL);
                hg_test_assert(token.literal == HgJsonParser::INTEGER);
                hg_test_assert(token.integer == 1234);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::FIELD);
                hg_test_assert(token.literal == HgJsonParser::STRING);
                hg_test_assert(token.string == "path");

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::LITERAL);
                hg_test_assert(token.literal == HgJsonParser::STRING);
                hg_test_assert(token.string == "tex.png");

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::STRUCT_END);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);
            }

            token = json.next_token(arena);
            hg_test_assert(token.type == HgJsonParser::FIELD);
            hg_test_assert(token.literal == HgJsonParser::STRING);
            hg_test_assert(token.string == "num array");

            {
                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::ARRAY_BEGIN);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::LITERAL);
                hg_test_assert(token.literal == HgJsonParser::INTEGER);
                hg_test_assert(token.integer == 1);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::LITERAL);
                hg_test_assert(token.literal == HgJsonParser::INTEGER);
                hg_test_assert(token.integer == 2);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::LITERAL);
                hg_test_assert(token.literal == HgJsonParser::INTEGER);
                hg_test_assert(token.integer == 3);

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::ARRAY_END);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);
            }

            token = json.next_token(arena);
            hg_test_assert(token.type == HgJsonParser::FIELD);
            hg_test_assert(token.literal == HgJsonParser::STRING);
            hg_test_assert(token.string == "entities");

            {
                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::ARRAY_BEGIN);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);

                {
                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::STRUCT_BEGIN);
                    hg_test_assert(token.literal == HgJsonParser::EMPTY);

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "id");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::LITERAL);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "player");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "pos");

                    {
                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::ARRAY_BEGIN);
                        hg_test_assert(token.literal == HgJsonParser::EMPTY);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == 0.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == 0.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == 0.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::ARRAY_END);
                        hg_test_assert(token.literal == HgJsonParser::EMPTY);
                    }

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "is_target");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::LITERAL);
                    hg_test_assert(token.literal == HgJsonParser::BOOLEAN);
                    hg_test_assert(token.boolean == true);

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::STRUCT_END);
                    hg_test_assert(token.literal == HgJsonParser::EMPTY);
                }

                {
                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::STRUCT_BEGIN);
                    hg_test_assert(token.literal == HgJsonParser::EMPTY);

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "id");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::LITERAL);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "enemy");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "pos");

                    {
                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::ARRAY_BEGIN);
                        hg_test_assert(token.literal == HgJsonParser::EMPTY);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == 10.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == -5.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::LITERAL);
                        hg_test_assert(token.literal == HgJsonParser::FLOATING);
                        hg_test_assert(token.floating == 1.0);

                        token = json.next_token(arena);
                        hg_test_assert(token.type == HgJsonParser::ARRAY_END);
                        hg_test_assert(token.literal == HgJsonParser::EMPTY);
                    }

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::FIELD);
                    hg_test_assert(token.literal == HgJsonParser::STRING);
                    hg_test_assert(token.string == "is_target");

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::LITERAL);
                    hg_test_assert(token.literal == HgJsonParser::BOOLEAN);
                    hg_test_assert(token.boolean == false);

                    token = json.next_token(arena);
                    hg_test_assert(token.type == HgJsonParser::STRUCT_END);
                    hg_test_assert(token.literal == HgJsonParser::EMPTY);
                }

                token = json.next_token(arena);
                hg_test_assert(token.type == HgJsonParser::ARRAY_END);
                hg_test_assert(token.literal == HgJsonParser::EMPTY);
            }

            token = json.next_token(arena);
            hg_test_assert(token.type == HgJsonParser::STRUCT_END);
            hg_test_assert(token.literal == HgJsonParser::EMPTY);
        }

        token = json.next_token(arena);
        hg_test_assert(token.type == HgJsonParser::END_OF_FILE);
        hg_test_assert(token.literal == HgJsonParser::EMPTY);
    }

    return true;
}

hg_test(HgMPSCQueue) {
    {
        HgArenaScope arena = hg_get_scratch();

        HgMPSCQueue<u32> queue = HgMPSCQueue<u32>::create(arena, 8);

        hg_test_assert(queue.items != nullptr);
        hg_test_assert(queue.capacity == 8);
        hg_test_assert(queue.head->load() == 0);
        hg_test_assert(queue.tail->load() == 0);

        u32 item;

        for (usize i = 0; i < 3; ++i) {
            for (u32 j = 0; j < 7; ++j) {
                queue.push(j);
            }

            hg_test_assert(queue.pop(item) && item == 0);
            hg_test_assert(queue.pop(item) && item == 1);
            hg_test_assert(queue.pop(item) && item == 2);
            hg_test_assert(queue.pop(item) && item == 3);

            for (u32 j = 7; j < 10; ++j) {
                queue.push(j);
            }

            hg_test_assert(queue.pop(item) && item == 4);
            hg_test_assert(queue.pop(item) && item == 5);
            hg_test_assert(queue.pop(item) && item == 6);
            hg_test_assert(queue.pop(item) && item == 7);
            hg_test_assert(queue.pop(item) && item == 8);
            hg_test_assert(queue.pop(item) && item == 9);
        }
    }

    {
        HgArenaScope arena = hg_get_scratch();

        HgMPSCQueue<u32> queue = HgMPSCQueue<u32>::create(arena, 128);

        hg_test_assert(queue.items != nullptr);
        hg_test_assert(queue.capacity == 128);
        hg_test_assert(queue.head->load() == 0);
        hg_test_assert(queue.tail->load() == 0);

        for (u32 n = 0; n < 3; ++n) {

            std::atomic_bool start{false};
            std::thread producers[4];
            std::thread consumer;

            bool vals[100] = {};

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    queue.push(i);
                }
            };
            for (u32 j = 0; j < hg_countof(producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            auto cons_fn = [&]() {
                while (!start) {
                    _mm_pause();
                }
                usize count = 0;
                while (count < hg_countof(vals)) {
                    u32 idx;
                    if (queue.pop(idx)) {
                        vals[idx] = !vals[idx];
                        ++count;
                    }
                }
            };
            consumer = std::thread(cons_fn);

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }
            consumer.join();

            for (auto val : vals) {
                hg_test_assert(val == true);
            }
        }
    }

    return true;
}

hg_test(HgMPMCQueue) {
    {
        HgArenaScope arena = hg_get_scratch();

        HgMPMCQueue<u32> queue = HgMPMCQueue<u32>::create(arena, 8);

        hg_test_assert(queue.items != nullptr);
        hg_test_assert(queue.capacity == 8);
        hg_test_assert(queue.head->load() == 0);
        hg_test_assert(queue.tail->load() == 0);

        u32 item;

        for (usize i = 0; i < 3; ++i) {
            for (u32 j = 0; j < 7; ++j) {
                queue.push(j);
            }

            hg_test_assert(queue.pop(item) && item == 0);
            hg_test_assert(queue.pop(item) && item == 1);
            hg_test_assert(queue.pop(item) && item == 2);
            hg_test_assert(queue.pop(item) && item == 3);

            for (u32 j = 7; j < 10; ++j) {
                queue.push(j);
            }

            hg_test_assert(queue.pop(item) && item == 4);
            hg_test_assert(queue.pop(item) && item == 5);
            hg_test_assert(queue.pop(item) && item == 6);
            hg_test_assert(queue.pop(item) && item == 7);
            hg_test_assert(queue.pop(item) && item == 8);
            hg_test_assert(queue.pop(item) && item == 9);
        }
    }

    {
        HgArenaScope arena = hg_get_scratch();

        HgMPMCQueue<u32> queue = HgMPMCQueue<u32>::create(arena, 128);

        hg_test_assert(queue.items != nullptr);
        hg_test_assert(queue.capacity == 128);
        hg_test_assert(queue.head->load() == 0);
        hg_test_assert(queue.tail->load() == 0);

        for (u32 n = 0; n < 3; ++n) {

            std::atomic_bool start{false};
            std::thread producers[4];
            std::thread consumers[4];

            bool vals[100] = {};

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    queue.push(i);
                }
            };
            for (u32 j = 0; j < hg_countof(producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            std::atomic<usize> count = 0;
            auto cons_fn = [&]() {
                while (!start) {
                    _mm_pause();
                }
                while (count < hg_countof(vals)) {
                    usize count_internal = 0;
                    for (usize j = 0; j < 20; ++j) {
                        u32 idx;
                        if (queue.pop(idx)) {
                            vals[idx] = !vals[idx];
                            ++count_internal;
                        }
                    }
                    count.fetch_add(count_internal);
                }
            };
            for (u32 j = 0; j < hg_countof(consumers); ++j) {
                consumers[j] = std::thread(cons_fn);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }
            for (auto& thread : consumers) {
                thread.join();
            }

            for (auto val : vals) {
                hg_test_assert(val == true);
            }
        }
    }

    return true;
}

hg_test(HgThreadPool) {
    HgArenaScope arena = hg_get_scratch();

    HgThreadPool* threads = HgThreadPool::create(arena, std::thread::hardware_concurrency() - 1, 128);
    hg_defer(threads->destroy());

    hg_assert(threads != nullptr);

    HgFence fence;
    {
        bool a = false;
        bool b = false;

        threads->call_par(&fence, &a, [](void *pa) {
            *(bool*)pa = true;
        });
        threads->call_par(&fence, &b, [](void *pb) {
            *(bool*)pb = true;
        });

        hg_test_assert(fence.wait(2.0));

        hg_test_assert(a == true);
        hg_test_assert(b == true);
    }

    {
        bool vals[100] = {};
        for (bool& val : vals) {
            threads->call_par(&fence, &val, [](void* data) {
                *(bool*)data = true;
            });
        }

        hg_test_assert(threads->help(fence, 2.0));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        bool vals[100] = {};
        for (u32 i = 0; i < hg_countof(vals); ++i) {
            auto fn_obj = hg_function<void()>(arena, [&vals, i]() {
                vals[i] = true;
            });

            threads->call_par(&fence, fn_obj.capture, fn_obj.fn);
        }

        hg_test_assert(threads->help(fence, 2.0));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        bool vals[100] = {};

        threads->for_par(hg_countof(vals), 16, [&](usize begin, usize end) {
            hg_assert(begin < end && end <= hg_countof(vals));
            for (; begin < end; ++begin) {
                vals[begin] = true;
            }
        });

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    return true;
}

hg_test(HgIOThread) {
    HgArenaScope arena = hg_get_scratch();

    HgIOThread* io = HgIOThread::create(arena, 128);
    hg_defer(io->destroy());

    hg_test_assert(io != nullptr);

    HgFence fence;
    {
        bool vals[100] = {};

        HgIOThread::Request request;
        request.fence = &fence;
        request.resource = vals;
        request.fn = [](void*, void* pval, HgStringView) {
            HgSpan<bool> span = {(bool*)pval, hg_countof(vals)};
            for (auto& val : span) {
                val = true;
            }
        };
        io->request(request);

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < hg_countof(vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        bool vals[100] = {};

        for (usize i = 0; i < hg_countof(vals); ++i) {
            HgIOThread::Request request;
            request.fence = &fence;
            request.resource = &vals[i];
            request.fn = [](void*, void* pval, HgStringView) {
                *(bool*)pval = true;
            };
            io->request(request);
        }

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < hg_countof(vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        bool vals[100] = {};

        vals[0] = true;

        for (usize i = 1; i < hg_countof(vals); ++i) {
            HgIOThread::Request request;
            request.fence = &fence;
            request.resource = &vals[i];
            request.fn = [](void*, void* pval, HgStringView) {
                *(bool*)pval = *((bool*)pval - 1);
            };
            io->request(request);
        }

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < hg_countof(vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    return true;
}

hg_test(HgFileBinary) {
    HgArenaScope arena = hg_get_scratch();

    u32 save_data[] = {12, 42, 100, 128};

    const char* file_path = "hg_test_dir/file_bin_test.bin";
    HgBinary file{};

    HgFence fence;
    {
        file.load(&fence, "file_does_not_exist.bin");
        hg_test_assert(fence.wait(2.0));

        hg_test_assert(file.data == nullptr);
        hg_test_assert(file.count == 0);
    }

    {
        file.data = save_data;
        file.count = sizeof(save_data);

        file.store(&fence, "dir/does/not/exist.bin");
        hg_test_assert(fence.wait(2.0));

        FILE* file_handle = std::fopen("dir/does/not/exist.bin", "rb");
        hg_test_assert(file_handle == nullptr);
    }

    {
        file.data = save_data;
        file.count = sizeof(save_data);

        file.store(&fence, file_path);
        file.load(&fence, file_path);
        hg_test_assert(fence.wait(2.0));

        hg_test_assert(file.data != nullptr);
        hg_test_assert(file.data != save_data);
        hg_test_assert(file.count == sizeof(save_data));
        hg_test_assert(std::memcmp(save_data, file.data, file.count) == 0);

        file.unload(&fence);
    }
    hg_test_assert(fence.wait(2.0));

    return true;
}

hg_test(HgTexture) {
    HgArenaScope arena = hg_get_scratch();

    struct color {
        u8 r, g, b, a;

        operator u32() { return *(u32*)this; }
    };

    u32 red =   color{0xff, 0x00, 0x00, 0xff};
    u32 green = color{0x00, 0xff, 0x00, 0xff};
    u32 blue =  color{0x00, 0x00, 0xff, 0xff};

    u32 save_data[3][3] = {
        {red, green, blue},
        {blue, red, green},
        {green, blue, red},
    };
    VkFormat save_format = VK_FORMAT_R8G8B8A8_SRGB;
    u32 save_width = 3;
    u32 save_height = 3;
    u32 save_depth = 1;

    const char* file_path = "hg_test_dir/file_image_test.png";
    HgTexture texture;

    HgFence fence;
    {
        texture.pixels = save_data;
        texture.format = save_format;
        texture.width = save_width;
        texture.height = save_height;
        texture.depth = save_depth;
        texture.location = HgTexture::CPU;

        texture.store_png(&fence, file_path);
        texture.load_png(&fence, file_path);
        hg_test_assert(fence.wait(2.0));

        hg_test_assert(texture.pixels != nullptr);
        hg_test_assert(texture.pixels != save_data);
        hg_test_assert(texture.format == save_format);
        hg_test_assert(texture.width == save_width);
        hg_test_assert(texture.height == save_height);
        hg_test_assert(texture.depth == save_depth);
        hg_test_assert(texture.width
                     * texture.height
                     * texture.depth
                     * hg_vk_format_to_size(texture.format)
                    == sizeof(save_data));
        hg_test_assert(std::memcmp(save_data, texture.pixels, sizeof(save_data)) == 0);

        texture.unload(&fence);
    }
    hg_test_assert(fence.wait(2.0));

    hg_test_assert(texture.location == HgTexture::UNLOADED);

    return true;
}

hg_test(HgECS) {
    HgArenaScope arena = hg_get_scratch();

    HgECS ecs = ecs.create(arena, 512);

    ecs.register_component<u32>(arena, 512);
    ecs.register_component<u64>(arena, 512);

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

    {
        bool has_unknown = false;
        ecs.for_each<u32>([&](HgEntity, u32&) {
            has_unknown = true;
        });
        hg_test_assert(!has_unknown);

        hg_test_assert(ecs.component_count<u32>() == 0);
        hg_test_assert(ecs.component_count<u64>() == 0);
    }

    {
        ecs.add<u32>(e1) = 12;
        ecs.add<u32>(e2) = 42;
        ecs.add<u32>(e3) = 100;
        hg_test_assert(ecs.component_count<u32>() == 3);
        hg_test_assert(ecs.component_count<u64>() == 0);

        bool has_unknown = false;
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
    }

    {
        ecs.add<u64>(e2) = 2042;
        ecs.add<u64>(e3) = 2100;
        hg_test_assert(ecs.component_count<u32>() == 3);
        hg_test_assert(ecs.component_count<u64>() == 2);

        bool has_unknown = false;
        bool has_12 = false;
        bool has_42 = false;
        bool has_100 = false;
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
    }

    {
        ecs.despawn(e1);
        hg_test_assert(ecs.component_count<u32>() == 2);
        hg_test_assert(ecs.component_count<u64>() == 2);

        bool has_unknown = false;
        bool has_12 = false;
        bool has_42 = false;
        bool has_100 = false;
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
    }

    {
        ecs.despawn(e2);
        hg_test_assert(ecs.component_count<u32>() == 1);
        hg_test_assert(ecs.component_count<u64>() == 1);
    }

    ecs.reset();
    hg_test_assert(ecs.component_count<u32>() == 0);
    hg_test_assert(ecs.component_count<u64>() == 0);

    {
        for (u32 i = 0; i < 300; ++i) {
            HgEntity e = ecs.spawn();
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
    }

    ecs.reset();

    auto comparison = [](void* pecs, HgEntity lhs, HgEntity rhs) {
        return (*(HgECS*)pecs).get<u32>(lhs) < (*(HgECS*)pecs).get<u32>(rhs);
    };

    {
        ecs.add<u32>(ecs.spawn()) = 42;

        ecs.sort<u32>(&ecs, comparison);

        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void) e;
            hg_test_assert(c == 42);
        }

        ecs.reset();
    }

    {
        u32 small_scramble_1[] = {1, 0};
        for (u32 i = 0; i < hg_countof(small_scramble_1); ++i) {
            ecs.add<u32>(ecs.spawn()) = small_scramble_1[i];
        }

        ecs.sort<u32>(&ecs, comparison);

        u32 elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem);
            ++elem;
        }

        ecs.sort<u32>(&ecs, comparison);

        elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem);
            ++elem;
        }

        ecs.reset();
    }

    {
        u32 medium_scramble_1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
        for (u32 i = 0; i < hg_countof(medium_scramble_1); ++i) {
            ecs.add<u32>(ecs.spawn()) = medium_scramble_1[i];
        }
        ecs.sort<u32>(&ecs, comparison);

        u32 elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem);
            ++elem;
        }

        ecs.reset();
    }

    {
        u32 medium_scramble_2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
        for (u32 i = 0; i < hg_countof(medium_scramble_2); ++i) {
            ecs.add<u32>(ecs.spawn()) = medium_scramble_2[i];
        }
        ecs.sort<u32>(&ecs, comparison);
        ecs.sort<u32>(&ecs, comparison);

        u32 elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem);
            ++elem;
        }

        ecs.reset();
    }

    {
        for (u32 i = 127; i < 128; --i) {
            ecs.add<u32>(ecs.spawn()) = i;
        }
        ecs.sort<u32>(&ecs, comparison);

        u32 elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem);
            ++elem;
        }

        ecs.reset();
    }

    {
        for (u32 i = 127; i < 128; --i) {
            ecs.add<u32>(ecs.spawn()) = i / 2;
        }
        ecs.sort<u32>(&ecs, comparison);
        ecs.sort<u32>(&ecs, comparison);

        u32 elem = 0;
        for (auto [e, c] : ecs.component_iter<u32>()) {
            (void)e;
            hg_test_assert(c == elem / 2);
            ++elem;
        }

        ecs.reset();
    }

    return true;
}

