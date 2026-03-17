#include "hurdygurdy.hpp"

#define IM_ASSERT hg_assert
#include "imgui.h"

int main() {
    hg_init();
    hg_defer(hg_exit());

    hg_test();

    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgWindowConfig window_config{};
    window_config.title = "Hg Small Test";
    window_config.windowed = true;
    window_config.width = 1200;
    window_config.height = 800;

    HgWindow* window = HgWindow::create(arena, window_config);
    hg_defer(window->destroy());

    hg_pipeline_3d_init(arena, 256, window->format, VK_FORMAT_D32_SFLOAT);
    hg_defer(hg_pipeline_3d_deinit());

    hg_pipeline_3d_add_model(0, 0, 0);
    hg_defer(hg_pipeline_3d_remove_model(0));

    HgTransform camera{};
    camera.position.z = -1;

    HgECS ecs = ecs.create(4096);
    hg_defer(ecs.destroy());

    HgEntity cube = ecs.spawn();
    ecs.add<HgTransform>(cube) = {};
    ecs.add<HgModel3D>(cube) = {0};

    HgEntity light = ecs.spawn();
    ecs.add<HgTransform>(light);
    ecs.get<HgTransform>(light).position = {-1, -2, -1};
    ecs.add<HgPointLight3D>(light) = {{1, 1, 1, 3}};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hg_defer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplHurdyGurdy_Init(window, 1, &window->format, VK_FORMAT_D32_SFLOAT);
    hg_defer(ImGui_ImplHurdyGurdy_Shutdown());

    u32 depth_width = window->width;
    u32 depth_height = window->height;

    HgVkImageConfig depth_image_config{};
    depth_image_config.width = depth_width;
    depth_image_config.height = depth_height;
    depth_image_config.format = VK_FORMAT_D32_SFLOAT;
    depth_image_config.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImage depth_image;
    VmaAllocation depth_alloc;
    hg_vk_create_image(&depth_image, &depth_alloc, depth_image_config);
    hg_defer(vmaDestroyImage(hg_vk_vma, depth_image, depth_alloc));

    HgVkImageViewConfig depth_view_config{};
    depth_view_config.image = depth_image;
    depth_view_config.format = depth_image_config.format;
    depth_view_config.subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageView depth_view = hg_vk_create_image_view(depth_view_config);
    hg_defer(vkDestroyImageView(hg_vk_device, depth_view, nullptr));

    HgClock game_clock{};

    f64 frame_time = 0.0;
    f64 record_time = 0.0f;
    f64 acquire_time = 0.0;
    f64 present_time = 0.0f;

    for (;;) {
        f64 delta = game_clock.tick();

        HgClock frame_clock{};

        HgArena& frame = hg_get_scratch(arena);
        HgArenaScope frame_scope{frame};

        hg_process_window_events(&window, 1);
        if (window->was_closed)
            goto quit;

        if ((depth_width != window->width || depth_height != window->height) && window->width * window->height != 0) {
            vkDestroyImageView(hg_vk_device, depth_view, nullptr);
            vmaDestroyImage(hg_vk_vma, depth_image, depth_alloc);

            depth_image_config.width = depth_width = window->width;
            depth_image_config.height = depth_height = window->height;
            hg_vk_create_image(&depth_image, &depth_alloc, depth_image_config);

            depth_view_config.image = depth_image;
            depth_view = hg_vk_create_image_view(depth_view_config);
        }

        hg_pipeline_3d_update_projection(
            hg_projection_perspective((f32)hg_pi * 0.5f, (f32)window->width / (f32)window->height, 0.1f, 1000.0f));

        HgQuat& cube_rot = ecs.get<HgTransform>(cube).rotation;
        cube_rot = hg_axis_angle({0, -1, 0}, (f32)delta) * cube_rot;

        if (window->is_key_down[(u32)HgKey::lmouse]) {
            f32 rot_speed = 2.0f;
            HgQuat rot_x = hg_axis_angle({0, 1, 0}, (f32)window->mouse_delta_x * rot_speed);
            HgQuat rot_y = hg_axis_angle({-1, 0, 0}, (f32)window->mouse_delta_y * rot_speed);
            camera.rotation = rot_x * camera.rotation * rot_y;
        }

        HgVec3 movement = {
            (f32)(window->is_key_down[(u32)HgKey::d] - window->is_key_down[(u32)HgKey::a]),
            (f32)(window->is_key_down[(u32)HgKey::lshift] - window->is_key_down[(u32)HgKey::space]),
            (f32)(window->is_key_down[(u32)HgKey::w] - window->is_key_down[(u32)HgKey::s]),
        };
        if (movement != HgVec3{0.0f}) {
            f32 move_speed = 1.5f;
            HgVec3 rotated = hg_rotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
            camera.position += hg_norm(HgVec3{rotated.x, movement.y, rotated.z}) * move_speed * (f32)delta;
        }

        hg_pipeline_3d_update_view(hg_view_matrix(camera.position, camera.scale, camera.rotation));

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Time")) {
            ImGui::Text("delta: %.3fms\n", delta * 1e3);

            ImGui::Text("cpu: %.3fms\n", (frame_time + record_time) * 1e3);
            ImGui::Text("frame: %.3fms\n", frame_time * 1e3);
            ImGui::Text("record: %.3fms\n", record_time * 1e3);

            ImGui::Text("gpu: %.3fms\n", (acquire_time + present_time) * 1e3);
            ImGui::Text("acquire: %.3fms\n", acquire_time * 1e3);
            ImGui::Text("present: %.3fms\n", present_time * 1e3);
        }
        ImGui::End();

        ImGui::Render();

        frame_time = frame_clock.tick();

        HgClock acquire_clock{};
        VkCommandBuffer cmd = window->begin_recording();
        acquire_time = acquire_clock.tick();

        HgClock record_clock{};

        if (cmd != nullptr) {
            HgRenderer renderer = renderer.create(frame, 64, 64);

            HgImageRenderID window_image_id = renderer.add_image(
                window->images[window->current_image],
                window->views[window->current_image],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            HgImageRenderID depth_image_id = renderer.add_image(
                depth_image,
                depth_view,
                {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

            HgRenderAttachment color_attachment{};
            color_attachment.image = window_image_id;
            color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

            HgRenderPass pass{};
            pass.color_attachments = &color_attachment;
            pass.color_attachment_count = 1;
            pass.depth_attachment.image = depth_image_id;
            pass.depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
            pass.depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            pass.depth_attachment.clear_value.depthStencil = {1.0f, 0};

            renderer.begin_pass(cmd, window->width, window->height, pass);

            hg_draw_3d(ecs, cmd);

            ImGui_ImplHurdyGurdy_Draw(cmd);

            renderer.end_pass(cmd);

            HgImageBarrier present_barrier{};
            present_barrier.image = window_image_id;
            present_barrier.next_usage = HgRenderUsage::present_src;

            renderer.barrier(cmd, nullptr, 0, &present_barrier, 1);

            record_time = record_clock.tick();

            HgClock present_clock{};
            window->end_and_present(cmd);
            present_time = present_clock.tick();
        }
    }

quit:
    vkDeviceWaitIdle(hg_vk_device);
}

