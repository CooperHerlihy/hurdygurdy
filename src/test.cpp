#include "hurdygurdy.hpp"

#include <emmintrin.h>

int main(void) {
    hg_defer(hg_info("Exited successfully\n"));

    hg_init();
    hg_defer(hg_exit());

    hg_run_tests();

    hg_arena_scope(arena, hg_get_scratch());

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
    VkImage* swap_images = arena.alloc<VkImage>(swap_image_count);
    VkImageView* swap_views = arena.alloc<VkImageView>(swap_image_count);
    vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, swap_images);
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

    HgResourceID texture_id = hg_resource_id("sprite_texture");
    hg_resources->register_resource(HgResource::texture, texture_id);

    HgTexture& texture = hg_resources->get<HgTexture>(texture_id);
    texture.pixels = tex_data;
    texture.format = VK_FORMAT_R8G8B8A8_SRGB;
    texture.width = 2;
    texture.height = 2;
    texture.depth = 1;
    texture.location = (u32)HgTexture::Location::cpu;
    texture.transfer_to_gpu(cmd_pool, VK_FILTER_NEAREST);
    hg_defer(texture.free_from_gpu());

    pipeline2d.add_texture(texture_id);
    hg_defer(pipeline2d.remove_texture(texture_id));

    hg_ecs->register_component<HgTransform>(arena, 1024);
    hg_ecs->register_component<HgSprite>(arena, 1024);

    HgEntity squares[] = {
        hg_ecs->spawn(),
        hg_ecs->spawn(),
    };

    for (HgEntity square : squares) {
        hg_ecs->add(square, HgTransform{});
        hg_ecs->add(square, HgSprite{texture_id, {0.0f}, {1.0f}});
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

        hg_arena_scope(frame, hg_get_scratch(arena));

        hg_process_window_events(&window, 1);
        if (window.was_closed() || window.is_key_down(HgKey::escape))
            break;

        static const f32 rot_speed = 2.0f;
        if (window.is_key_down(HgKey::lmouse)) {
            f64 x, y;
            window.get_mouse_delta(x, y);
            HgQuat rot_x = hg_axis_angle({0.0f, 1.0f, 0.0f}, (f32)x * rot_speed);
            HgQuat rot_y = hg_axis_angle({-1.0f, 0.0f, 0.0f}, (f32)y * rot_speed);
            camera.rotation = rot_x * camera.rotation * rot_y;
        }

        static const f32 move_speed = 1.5f;
        HgVec3 movement = {0.0f};
        if (window.is_key_down(HgKey::space))
            movement.y -= 1.0f;
        if (window.is_key_down(HgKey::lshift))
            movement.y += 1.0f;
        if (window.is_key_down(HgKey::w))
            movement.z += 1.0f;
        if (window.is_key_down(HgKey::s))
            movement.z -= 1.0f;
        if (window.is_key_down(HgKey::a))
            movement.x -= 1.0f;
        if (window.is_key_down(HgKey::d))
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

            for (usize i = 0; i < swap_image_count; ++i) {
                vkDestroyImageView(hg_vk_device, swap_views[i], nullptr);
            }
            swapchain_commands.destroy();

            if (swapchain.handle != nullptr) {
                usize old_count = swap_image_count;
                vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, nullptr);
                if (old_count != swap_image_count) {
                    swap_images = arena.realloc(swap_images, old_count, swap_image_count);
                    swap_views = arena.realloc(swap_views, old_count, swap_image_count);
                }
                vkGetSwapchainImagesKHR(hg_vk_device, swapchain.handle, &swap_image_count, swap_images);
                for (usize i = 0; i < swap_image_count; ++i) {
                    VkImageViewCreateInfo create_info{};
                    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    create_info.image = swap_images[i];
                    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    create_info.format = swapchain.format;
                    create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                    vkCreateImageView(hg_vk_device, &create_info, nullptr, &swap_views[i]);
                }
                swapchain_commands.recreate(arena, swapchain.handle, cmd_pool);

                aspect = (f32)swapchain.width / (f32)swapchain.height;
                proj = hg_projection_perspective((f32)hg_pi * 0.5f, aspect, 0.1f, 1000.0f);
                pipeline2d.update_projection(proj);
            }

            vkDestroySwapchainKHR(hg_vk_device, old_swapchain, nullptr);
            hg_info("window resized\n");
        }

        cpu_time += cpu_clock.tick();
        VkCommandBuffer cmd = swapchain_commands.acquire_and_record();
        if (cmd != nullptr) {
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

        u32* alloc_u32 = arena.alloc<u32>(1);
        hg_test_assert(alloc_u32 == arena.memory);

        u64* alloc_u64 = arena.alloc<u64>(2);
        hg_test_assert((u8*)alloc_u64 == (u8*)alloc_u32 + 8);

        u8* alloc_u8 = arena.alloc<u8>(1);
        hg_test_assert(alloc_u8 == (u8*)alloc_u32 + 24);

        struct Big {
            u8 data[32];
        };
        Big* alloc_big = arena.alloc<Big>(1);
        hg_test_assert((u8*)alloc_big == (u8*)alloc_u32 + 25);

        Big* realloc_big = arena.realloc(alloc_big, 1, 2);
        hg_test_assert(realloc_big == alloc_big);

        Big* realloc_big_same = arena.realloc(realloc_big, 2, 2);
        hg_test_assert(realloc_big_same == realloc_big);

        memset(realloc_big, 2, 2 * sizeof(*realloc_big));
        u8* alloc_interrupt = arena.alloc<u8>(1);
        (void)alloc_interrupt;

        Big* realloc_big2 = arena.realloc(realloc_big, 2, 4);
        hg_test_assert(realloc_big2 != realloc_big);
        hg_test_assert(memcmp(realloc_big, realloc_big2, 2 * sizeof(*realloc_big)) == 0);

        arena.reset();
    }

    return true;
}

hg_test(HgString) {
    {
        hg_arena_scope(arena, hg_get_scratch());

        HgString a = a.create(arena, "a");
        hg_test_assert(a[0] == 'a');
        hg_test_assert(a.capacity == 1);
        hg_test_assert(a.length == 1);

        HgString abc = abc.create(arena, "abc");
        hg_test_assert(abc[0] == 'a');
        hg_test_assert(abc[1] == 'b');
        hg_test_assert(abc[2] == 'c');
        hg_test_assert(abc.length == 3);
        hg_test_assert(abc.capacity == 3);

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
    hg_arena_scope(arena, hg_get_scratch());

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

hg_test(HgJson) {
    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->message == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->message == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf":
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next != nullptr);
        hg_test_assert(error->message == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->message == "on line 4, found unexpected token \"}\"\n");

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": true
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::boolean);
        hg_test_assert(field->value->boolean == true);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": false
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::boolean);
        hg_test_assert(field->value->boolean == false);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": asdf
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next != nullptr);
        hg_test_assert(error->message == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->message == "on line 3, found unexpected token \"asdf\"\n");

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::string);
        hg_test_assert(field->value->string == "asdf");
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": 1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::integer);
        hg_test_assert(field->value->integer == 1234);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": 1234.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::floating);
        hg_test_assert(field->value->floating == 1234.0);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": 1234.0,
                "hjkl": 5678.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next != nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::floating);
        hg_test_assert(field->value->floating == 1234.0);

        field = field->next;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "hjkl");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::floating);
        hg_test_assert(field->value->floating == 5678.0);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": [1, 2, 3, 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        for (auto e = json.errors; e != nullptr; e = e->next) {
            hg_info("e: %s", HgString::create(arena, e->message).append(arena, 0).chars);
        }
        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 3);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 4);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": [1 2 3 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = node->field.value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 3);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 4);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": [1, 2, "3", 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->message ==
            "on line 3, array has element which is not the same type as the first valid element\n");

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = node->field.value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 4);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "asdf": {
                    "a": 1,
                    "s": 2.0,
                    "d": 3,
                    "f": 4.0,
                }
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* node = json.first;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::jstruct);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Field* sub_field = node->field.value->jstruct.fields;
        hg_test_assert(sub_field->next != nullptr);
        hg_test_assert(sub_field->name == "a");
        hg_test_assert(sub_field->value != nullptr);
        hg_test_assert(sub_field->value->type == HgJson::integer);
        hg_test_assert(sub_field->value->integer == 1);

        sub_field = sub_field->next;
        hg_test_assert(sub_field->next != nullptr);
        hg_test_assert(sub_field->name == "s");
        hg_test_assert(sub_field->value != nullptr);
        hg_test_assert(sub_field->value->type == HgJson::floating);
        hg_test_assert(sub_field->value->floating == 2.0);

        sub_field = sub_field->next;
        hg_test_assert(sub_field->next != nullptr);
        hg_test_assert(sub_field->name == "d");
        hg_test_assert(sub_field->value != nullptr);
        hg_test_assert(sub_field->value->type == HgJson::integer);
        hg_test_assert(sub_field->value->integer == 3);

        sub_field = sub_field->next;
        hg_test_assert(sub_field->next == nullptr);
        hg_test_assert(sub_field->name == "f");
        hg_test_assert(sub_field->value != nullptr);
        hg_test_assert(sub_field->value->type == HgJson::floating);
        hg_test_assert(sub_field->value->floating == 4.0);
    }

    {
        hg_arena_scope(arena, hg_get_scratch());

        HgStringView file = R"(
            {
                "player": {
                    "transform": {
                        "position": [1.0, 0.0, -1.0],
                        "scale": [1.0, 1.0, 1.0],
                        "rotation": [1.0, 0.0, 0.0, 0.0]
                    },
                    "sprite": {
                        "texture": "tex.png",
                        "uv_pos": [0.0, 0.0],
                        "uv_size": [1.0, 1.0]
                    }
                }
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.first != nullptr);

        HgJson::Node* main_struct = json.first;
        hg_test_assert(main_struct->type == HgJson::jstruct);
        hg_test_assert(main_struct->jstruct.fields != nullptr);

        HgJson::Field* player = main_struct->jstruct.fields;
        hg_test_assert(player->next == nullptr);
        hg_test_assert(player->name == "player");
        hg_test_assert(player->value != nullptr);
        hg_test_assert(player->value->type == HgJson::jstruct);
        hg_test_assert(player->value->jstruct.fields != nullptr);

        HgJson::Field* component = player->value->jstruct.fields;
        hg_test_assert(component->next != nullptr);
        hg_test_assert(component->name == "transform");
        hg_test_assert(component->value != nullptr);
        hg_test_assert(component->value->type == HgJson::jstruct);
        hg_test_assert(component->value->jstruct.fields != nullptr);

        HgJson::Field* field = component->value->jstruct.fields;
        hg_test_assert(field->next != nullptr);
        hg_test_assert(field->name == "position");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == -1.0);

        field = field->next;
        hg_test_assert(field->next != nullptr);
        hg_test_assert(field->name == "scale");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        field = field->next;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "rotation");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        component = component->next;
        hg_test_assert(component->next == nullptr);
        hg_test_assert(component->name == "sprite");
        hg_test_assert(component->value != nullptr);
        hg_test_assert(component->value->type == HgJson::jstruct);
        hg_test_assert(component->value->jstruct.fields != nullptr);

        field = component->value->jstruct.fields;
        hg_test_assert(field->next != nullptr);
        hg_test_assert(field->name == "texture");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::string);
        hg_test_assert(field->value->string == "tex.png");

        field = field->next;
        hg_test_assert(field->next != nullptr);
        hg_test_assert(field->name == "uv_pos");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 0.0);

        field = field->next;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "uv_size");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::array);
        hg_test_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_test_assert(elem->next != nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::floating);
        hg_test_assert(elem->value->floating == 1.0);
    }

    return true;
}

hg_test(HgArrayAny) {
    hg_arena_scope(arena, hg_get_scratch());

    HgAnyArray arr = arr.create(arena, sizeof(u32), alignof(u32), 0, 2);
    hg_test_assert(arr.items != nullptr);
    hg_test_assert(arr.capacity == 2);
    hg_test_assert(arr.count == 0);

    *(u32*)arr.push() = 2;
    hg_test_assert(*(u32*)arr.get(0) == 2);
    hg_test_assert(arr.count == 1);
    *(u32*)arr.push() = 4;
    hg_test_assert(*(u32*)arr.get(1) == 4);
    hg_test_assert(arr.count == 2);

    arr.grow(arena);
    hg_test_assert(arr.capacity == 4);

    *(u32*)arr.push() = 8;
    hg_test_assert(*(u32*)arr.get(2) == 8);
    hg_test_assert(arr.count == 3);

    arr.pop();
    hg_test_assert(arr.count == 2);
    hg_test_assert(arr.capacity == 4);

    *(u32*)arr.insert(0) = 1;
    hg_test_assert(arr.count == 3);
    hg_test_assert(*(u32*)arr.get(0) == 1);
    hg_test_assert(*(u32*)arr.get(1) == 2);
    hg_test_assert(*(u32*)arr.get(2) == 4);

    arr.remove(1);
    hg_test_assert(arr.count == 2);
    hg_test_assert(*(u32*)arr.get(0) == 1);
    hg_test_assert(*(u32*)arr.get(1) == 4);

    for (u32 i = 0; i < 100; ++i) {
        if (arr.is_full())
            arr.grow(arena);
        *(u32*)arr.push() = i;
    }
    hg_test_assert(arr.count == 102);
    hg_test_assert(arr.capacity >= 102);

    arr.swap_remove(2);
    hg_test_assert(arr.count == 101);
    hg_test_assert(*(u32*)arr.get(2) == 99);
    hg_test_assert(*(u32*)arr.get(arr.count - 1) == 98);

    *(u32*)arr.swap_insert(0) = 42;
    hg_test_assert(arr.count == 102);
    hg_test_assert(*(u32*)arr.get(0) == 42);
    hg_test_assert(*(u32*)arr.get(1) == 4);
    hg_test_assert(*(u32*)arr.get(2) == 99);
    hg_test_assert(*(u32*)arr.get(arr.count - 1) == 1);

    return true;
}

hg_test(HgHashMap) {
    {
        hg_arena_scope(arena, hg_get_scratch());

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
            hg_test_assert(*map.get(1) == 1);

            map.remove(1);
            hg_test_assert(map.load == 0);
            hg_test_assert(!map.has(1));
            hg_test_assert(map.get(1) == nullptr);

            hg_test_assert(!map.has(12));
            hg_test_assert(!map.has(12 + count));

            map.insert(12, 42);
            hg_test_assert(map.load == 1);
            hg_test_assert(map.has(12) && *map.get(12) == 42);
            hg_test_assert(!map.has(12 + count));

            map.insert(12 + count, 100);
            hg_test_assert(map.load == 2);
            hg_test_assert(map.has(12) && *map.get(12) == 42);
            hg_test_assert(map.has(12 + count) && *map.get(12 + count) == 100);

            map.insert(12 + count * 2, 200);
            hg_test_assert(map.load == 3);
            hg_test_assert(map.has(12) && *map.get(12) == 42);
            hg_test_assert(map.has(12 + count) && *map.get(12 + count) == 100);
            hg_test_assert(map.has(12 + count * 2) && *map.get(12 + count * 2) == 200);

            map.remove(12);
            hg_test_assert(map.load == 2);
            hg_test_assert(!map.has(12));
            hg_test_assert(map.has(12 + count) && *map.get(12 + count) == 100);

            map.insert(42, 12);
            hg_test_assert(map.load == 3);
            hg_test_assert(map.has(42) && *map.get(42) == 12);

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
        hg_arena_scope(arena, hg_get_scratch());

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

        hg_test_assert(map.has(a) && *map.get(a) == 1);
        hg_test_assert(map.has(b) && *map.get(b) == 2);
        hg_test_assert(map.has(ab) && *map.get(ab) == 3);
        hg_test_assert(map.has(scf) && *map.get(scf) == 4);

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
        hg_arena_scope(arena, hg_get_scratch());

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

        hg_test_assert(map.has(a) && *map.get(a) == 1);
        hg_test_assert(map.has(b) && *map.get(b) == 2);
        hg_test_assert(map.has(ab) && *map.get(ab) == 3);
        hg_test_assert(map.has(scf) && *map.get(scf) == 4);

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
        hg_arena_scope(arena, hg_get_scratch());

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
        hg_test_assert(*map.get(HgString::create(arena, "a")) == 1);
        hg_test_assert(map.has(HgString::create(arena, "b")));
        hg_test_assert(*map.get(HgString::create(arena, "b")) == 2);
        hg_test_assert(map.has(HgString::create(arena, "ab")));
        hg_test_assert(*map.get(HgString::create(arena, "ab")) == 3);
        hg_test_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
        hg_test_assert(*map.get(HgString::create(arena, "supercalifragilisticexpialidocious")) == 4);

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
        hg_arena_scope(arena, hg_get_scratch());

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
        hg_test_assert(*map.get("a") == 1);
        hg_test_assert(map.has("b"));
        hg_test_assert(*map.get("b") == 2);
        hg_test_assert(map.has("ab"));
        hg_test_assert(*map.get("ab") == 3);
        hg_test_assert(map.has("supercalifragilisticexpialidocious"));
        hg_test_assert(*map.get("supercalifragilisticexpialidocious") == 4);

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
        hg_arena_scope(arena, hg_get_scratch());

        constexpr usize count = 128;

        HgHashSet<u32> map = map.create(arena, count);

        for (usize i = 0; i < 3; ++i) {
            hg_test_assert(map.load == 0);
            hg_test_assert(!map.has(0));
            hg_test_assert(!map.has(1));
            hg_test_assert(!map.has(12));
            hg_test_assert(!map.has(42));
            hg_test_assert(!map.has(100000));

            map.insert(1);
            hg_test_assert(map.load == 1);
            hg_test_assert(map.has(1));

            map.remove(1);
            hg_test_assert(map.load == 0);
            hg_test_assert(!map.has(1));

            hg_test_assert(!map.has(12));
            hg_test_assert(!map.has(12 + count));

            map.insert(12);
            hg_test_assert(map.load == 1);
            hg_test_assert(map.has(12));
            hg_test_assert(!map.has(12 + count));

            map.insert(12 + count);
            hg_test_assert(map.load == 2);
            hg_test_assert(map.has(12));
            hg_test_assert(map.has(12 + count));

            map.insert(12 + count * 2);
            hg_test_assert(map.load == 3);
            hg_test_assert(map.has(12));
            hg_test_assert(map.has(12 + count));
            hg_test_assert(map.has(12 + count * 2));

            map.remove(12);
            hg_test_assert(map.load == 2);
            hg_test_assert(!map.has(12));
            hg_test_assert(map.has(12 + count));

            map.insert(42);
            hg_test_assert(map.load == 3);
            hg_test_assert(map.has(42));

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
        hg_arena_scope(arena, hg_get_scratch());

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
        hg_arena_scope(arena, hg_get_scratch());

        HgHashSet<const char*> map = map.create(arena, 128);

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
        hg_arena_scope(arena, hg_get_scratch());

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
        hg_arena_scope(arena, hg_get_scratch());

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

hg_test(HgThreadPool) {
    hg_arena_scope(arena, hg_get_scratch());

    HgThreadPool* threads = HgThreadPool::create(arena, std::thread::hardware_concurrency() - 1, 128);
    hg_defer(threads->destroy());

    hg_assert(threads != nullptr);

    HgFence fence;
    {
        bool a = false;
        bool b = false;

        threads->push(&fence, 1, &a, [](void *pa) {
            *(bool*)pa = true;
        });
        threads->push(&fence, 1, &b, [](void *pb) {
            *(bool*)pb = true;
        });

        fence.wait(2.0);

        hg_test_assert(fence.wait(2.0));

        hg_test_assert(a == true);
        hg_test_assert(b == true);
    }

    {
        bool vals[100] = {};
        for (bool& val : vals) {
            threads->push(&fence, 1, &val, [](void* data) {
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

    {
        for (u32 n = 0; n < 3; ++n) {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

            auto fn = [](void* pval) {
                *((bool*)pval) = !*((bool*)pval);
            };

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    threads->push(&fence, 1, vals + i, fn);
                }
            };
            for (u32 j = 0; j < hg_countof(producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }

            hg_test_assert(threads->help(fence, 2.0));
            for (auto val : vals) {
                hg_test_assert(val == true);
            }
        }
    }

    return true;
}

hg_test(HgIOThread) {
    hg_arena_scope(arena, hg_get_scratch());

    HgIOThread* io = HgIOThread::create(arena, 128);
    hg_defer(io->destroy());

    hg_test_assert(io != nullptr);

    HgFence fence;
    {
        bool vals[100] = {};

        HgIOThread::Request request;
        request.fences = &fence;
        request.fence_count = 1;
        request.resource = vals;
        request.fn = [](void*, void* pvals, HgStringView) {
            for (usize i = 0; i < hg_countof(vals); ++i) {
                ((bool*)pvals)[i] = true;
            }
        };
        io->push(request);

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < hg_countof(vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        bool vals[100] = {};

        for (usize i = 0; i < hg_countof(vals); ++i) {
            HgIOThread::Request request;
            request.fences = &fence;
            request.fence_count = 1;
            request.resource = &vals[i];
            request.fn = [](void*, void* pval, HgStringView) {
                *(bool*)pval = true;
            };
            io->push(request);
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
            request.fences = &fence;
            request.fence_count = 1;
            request.resource = &vals[i];
            request.fn = [](void*, void* pval, HgStringView) {
                *(bool*)pval = *((bool*)pval - 1);
            };
            io->push(request);
        }

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < hg_countof(vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        for (u32 n = 0; n < 3; ++n) {

            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

            auto req_fn = [](void*, void* pval, HgStringView) {
                *(bool*)pval = !*(bool*)pval;
            };

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    HgIOThread::Request r{};
                    r.fences = &fence;
                    r.fence_count = 1;
                    r.resource = &vals[i];
                    r.fn = req_fn;
                    io->push(r);
                }
            };
            for (u32 j = 0; j < hg_countof(producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }

            hg_test_assert(fence.wait(2.0));
            for (auto val : vals) {
                hg_test_assert(val == true);
            }
        }
    }

    return true;
}

hg_test(HgFileBinary) {
    hg_arena_scope(arena, hg_get_scratch());

    u32 save_data[] = {12, 42, 100, 128};

    const char* file_path = "hg_test_dir/file_bin_test.bin";
    HgBinary bin{};

    HgFence fence;
    {
        bin.load(&fence, 1, "file_does_not_exist.bin");
        hg_test_assert(fence.wait(2.0));

        hg_test_assert(bin.file == nullptr);
        hg_test_assert(bin.size == 0);
    }

    {
        bin.file = save_data;
        bin.size = sizeof(save_data);

        bin.store(&fence, 1, "dir/does/not/exist.bin");
        hg_test_assert(fence.wait(2.0));

        FILE* file_handle = std::fopen("dir/does/not/exist.bin", "rb");
        hg_test_assert(file_handle == nullptr);
    }

    {
        bin.file = save_data;
        bin.size = sizeof(save_data);

        bin.store(&fence, 1, file_path);
        bin.load(&fence, 1, file_path);
        hg_test_assert(fence.wait(2.0));

        hg_test_assert(bin.file != nullptr);
        hg_test_assert(bin.file != save_data);
        hg_test_assert(bin.size == sizeof(save_data));
        hg_test_assert(std::memcmp(save_data, bin.file, bin.size) == 0);

        bin.unload(&fence, 1);
    }
    hg_test_assert(fence.wait(2.0));

    return true;
}

hg_test(HgTexture) {
    hg_arena_scope(arena, hg_get_scratch());

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
        texture.location = (u32)HgTexture::Location::cpu;

        texture.store_png(&fence, 1, file_path);
        texture.load_png(&fence, 1, file_path);
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

        texture.unload(&fence, 1);
    }
    hg_test_assert(fence.wait(2.0));

    hg_test_assert(texture.location == (u32)HgTexture::Location::none);

    return true;
}

hg_test(HgResourceManager) {
    hg_arena_scope(arena, hg_get_scratch());

    HgResourceManager rm = rm.create(arena, 64);

    {
        HgResourceID a = 0;
        HgResourceID b = 1;
        HgResourceID b_conf = 1 + rm.capacity;
        HgResourceID b_conf2 = 1 + rm.capacity * 2;
        HgResourceID c = 2;
        HgResourceID d = 3;
        HgResourceID e = 10;

        hg_test_assert(!rm.is_registered(a));
        hg_test_assert(!rm.is_registered(b));
        hg_test_assert(!rm.is_registered(b_conf));
        hg_test_assert(!rm.is_registered(b_conf2));
        hg_test_assert(!rm.is_registered(c));
        hg_test_assert(!rm.is_registered(d));
        hg_test_assert(!rm.is_registered(e));

        rm.register_resource(HgResource::binary, a);
        rm.register_resource(HgResource::texture, b);
        rm.register_resource(HgResource::binary, b_conf);
        rm.register_resource(HgResource::texture, b_conf2);
        rm.register_resource(HgResource::binary, c);
        rm.register_resource(HgResource::texture, d);
        rm.register_resource(HgResource::binary, e);

        hg_test_assert(rm.is_registered(a));
        hg_test_assert(rm.resources[rm.get_resource(a)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(b));
        hg_test_assert(rm.resources[rm.get_resource(b)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(b_conf));
        hg_test_assert(rm.resources[rm.get_resource(b_conf)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(b_conf2));
        hg_test_assert(rm.resources[rm.get_resource(b_conf2)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(c));
        hg_test_assert(rm.resources[rm.get_resource(c)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(d));
        hg_test_assert(rm.resources[rm.get_resource(d)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(e));
        hg_test_assert(rm.resources[rm.get_resource(e)].type == HgResource::binary);

        rm.unregister_resource(a);
        rm.unregister_resource(b);
        rm.unregister_resource(b_conf);
        rm.unregister_resource(b_conf2);
        rm.unregister_resource(c);
        rm.unregister_resource(d);
        rm.unregister_resource(e);

        hg_test_assert(!rm.is_registered(a));
        hg_test_assert(!rm.is_registered(b));
        hg_test_assert(!rm.is_registered(b_conf));
        hg_test_assert(!rm.is_registered(b_conf2));
        hg_test_assert(!rm.is_registered(c));
        hg_test_assert(!rm.is_registered(d));
        hg_test_assert(!rm.is_registered(e));

        rm.register_resource(HgResource::texture, a);
        rm.register_resource(HgResource::binary, b_conf2);
        rm.register_resource(HgResource::texture, d);
        rm.register_resource(HgResource::binary, b);

        hg_test_assert(rm.is_registered(a));
        hg_test_assert(rm.resources[rm.get_resource(a)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(b));
        hg_test_assert(rm.resources[rm.get_resource(b)].type == HgResource::binary);
        hg_test_assert(!rm.is_registered(b_conf));
        hg_test_assert(rm.is_registered(b_conf2));
        hg_test_assert(rm.resources[rm.get_resource(b_conf2)].type == HgResource::binary);
        hg_test_assert(!rm.is_registered(c));
        hg_test_assert(rm.is_registered(d));
        hg_test_assert(rm.resources[rm.get_resource(d)].type == HgResource::texture);
        hg_test_assert(!rm.is_registered(e));

        rm.register_resource(HgResource::texture, b_conf);
        rm.register_resource(HgResource::binary, e);
        rm.register_resource(HgResource::texture, c);

        hg_test_assert(rm.is_registered(a));
        hg_test_assert(rm.resources[rm.get_resource(a)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(b));
        hg_test_assert(rm.resources[rm.get_resource(b)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(b_conf));
        hg_test_assert(rm.resources[rm.get_resource(b_conf)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(b_conf2));
        hg_test_assert(rm.resources[rm.get_resource(b_conf2)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(c));
        hg_test_assert(rm.resources[rm.get_resource(c)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(d));
        hg_test_assert(rm.resources[rm.get_resource(d)].type == HgResource::texture);
        hg_test_assert(rm.is_registered(e));
        hg_test_assert(rm.resources[rm.get_resource(e)].type == HgResource::binary);

        rm.unregister_resource(e);
        rm.unregister_resource(b_conf);
        rm.unregister_resource(b);
        rm.unregister_resource(d);

        hg_test_assert(rm.is_registered(a));
        hg_test_assert(rm.resources[rm.get_resource(a)].type == HgResource::texture);
        hg_test_assert(!rm.is_registered(b));
        hg_test_assert(!rm.is_registered(b_conf));
        hg_test_assert(rm.is_registered(b_conf2));
        hg_test_assert(rm.resources[rm.get_resource(b_conf2)].type == HgResource::binary);
        hg_test_assert(rm.is_registered(c));
        hg_test_assert(rm.resources[rm.get_resource(c)].type == HgResource::texture);
        hg_test_assert(!rm.is_registered(d));
        hg_test_assert(!rm.is_registered(e));

        rm.unregister_resource(c);
        rm.unregister_resource(b_conf2);
        rm.unregister_resource(a);

        hg_test_assert(!rm.is_registered(a));
        hg_test_assert(!rm.is_registered(b));
        hg_test_assert(!rm.is_registered(b_conf));
        hg_test_assert(!rm.is_registered(b_conf2));
        hg_test_assert(!rm.is_registered(c));
        hg_test_assert(!rm.is_registered(d));
        hg_test_assert(!rm.is_registered(e));
    }

    HgFence fence;
    rm.destroy(&fence, 1);
    hg_test_assert(fence.wait(2.0));

    return true;
}

hg_test(HgECS) {
    hg_arena_scope(arena, hg_get_scratch());

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

