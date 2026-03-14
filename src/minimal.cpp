#include "hurdygurdy.hpp"

#define IM_ASSERT hg_assert
#include "imgui.h"
#include "imgui_impl_vulkan.h"

int main() {
    hg_init();
    hg_defer(hg_exit());

    hg_test();

    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgWindowConfig window_config{};
    window_config.title = "Hg Small Test";
    // window_config.windowed = true;
    window_config.width = 1200;
    window_config.height = 800;
    window_config.preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // window_config.preferred_present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    // window_config.preferred_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    // window_config.preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;

    HgWindow* window = HgWindow::create(arena, window_config);
    hg_defer(window->destroy());

    hg_pipeline_2d_init(arena, 256, window->format, VK_FORMAT_UNDEFINED);
    hg_defer(hg_pipeline_2d_deinit());

    HgStringView texture_path = "hg_test_dir/file_image_test.hgtex";
    HgResource texture_id = hg_resource_id(texture_path);
    {
        HgFence fence;
        hg_load_resource(&fence, 1, texture_id, texture_path);
        fence.wait(INFINITY);
        hg_load_gpu_texture(texture_id, VK_FILTER_NEAREST);
        hg_unload_resource(nullptr, 0, texture_id);
    }
    hg_defer(hg_unload_gpu_resource(texture_id));

    hg_pipeline_2d_add_texture(texture_id);
    hg_defer(hg_pipeline_2d_remove_texture(texture_id));

    HgTransform camera{};
    camera.position = {0, 0, -1};
    camera.scale = {1, 1, 1};
    camera.rotation = {1, 0, 0, 0};

    HgECS ecs = ecs.create(4096);
    hg_defer(ecs.destroy());

    HgEntity square = ecs.spawn();
    ecs.add<HgTransform>(square) = {};
    ecs.add<HgSprite>(square) = {texture_id, {0.0f}, {1.0f}};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hg_defer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplHurdyGurdy_Init(window);
    hg_defer(ImGui_ImplHurdyGurdy_Shutdown());

    ImGui_ImplVulkan_InitInfo imgui_info{};
    imgui_info.Instance = hg_vk_instance;
    imgui_info.PhysicalDevice = hg_vk_physical_device;
    imgui_info.Device = hg_vk_device;
    imgui_info.QueueFamily = hg_vk_queue_family;
    imgui_info.Queue = hg_vk_queue;
    imgui_info.DescriptorPoolSize = 1000;
    imgui_info.MinImageCount = window->image_count;
    imgui_info.ImageCount = window->image_count;
    imgui_info.MinAllocationSize = 1024 * 1024;
    imgui_info.UseDynamicRendering = true;
    imgui_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imgui_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imgui_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &window->format;
#ifdef HG_DEBUG_MODE
    imgui_info.CheckVkResultFn = [](VkResult err) {
        if (err != VK_SUCCESS)
            hg_warn("Vulkan error from ImGui: %s\n", hg_vk_result_to_string(err));
    };
#endif

    ImGui_ImplVulkan_Init(&imgui_info);
    hg_defer(ImGui_ImplVulkan_Shutdown());

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

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        hg_pipeline_2d_update_projection(
            hg_projection_perspective((f32)hg_pi * 0.5f, (f32)window->width / (f32)window->height, 0.1f, 1000.0f));

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

        hg_pipeline_2d_update_view(hg_view_matrix(camera.position, camera.scale, camera.rotation));

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

            HgImageRenderID window_image = renderer.add_image(
                window->images[window->current_image],
                window->views[window->current_image],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            HgRenderAttachment color_attachment{};
            color_attachment.image = window_image;
            color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

            HgRenderPass pass{};
            pass.color_attachments = &color_attachment;
            pass.color_attachment_count = 1;

            renderer.begin_pass(cmd, window->width, window->height, pass);

            hg_draw_2d(ecs, cmd);

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

            renderer.end_pass(cmd);

            HgImageBarrier present_barrier{};
            present_barrier.image = window_image;
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

