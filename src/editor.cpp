#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#define IM_ASSERT hg_assert
#include "imgui.h"
#include "imgui_impl_vulkan.h"

int main() {
    hg_defer(hg_debug("Exited successfully\n"));

    hg_init();
    hg_defer(hg_exit());

    hg_test();

    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgWindowConfig window_config{};
    window_config.title = "Hg Test";
    window_config.windowed = true;
    window_config.width = 1600;
    window_config.height = 900;
    window_config.preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;

    HgWindow* window = HgWindow::create(arena, window_config);
    hg_defer(window->destroy());

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

    hg_pipeline_2d_init(arena, 256, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hg_defer(hg_pipeline_2d_deinit());

    hg_pipeline_3d_init(arena, 256, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hg_defer(hg_pipeline_3d_deinit());

    hg_pipeline_2d_add_texture(texture_id);
    hg_defer(hg_pipeline_2d_remove_texture(texture_id));

    hg_pipeline_3d_add_model(0, 0, 0);
    hg_defer(hg_pipeline_3d_remove_model(0));

    HgTransform camera{};
    camera.position = {0, 0, -2};
    camera.scale = {1, 1, 1};
    camera.rotation = {1, 0, 0, 0};

    f32 aspect_ratio = 16.0f / 9.0f;
    u32 render_width = 0;
    u32 render_height = 0;

    HgECS ecs = ecs.create(4096);
    hg_defer(ecs.destroy());

    u32 scene_capacity = 8;
    HgEntity* scene = arena.alloc<HgEntity>(scene_capacity);
    u32 scene_size = 0;

    // u32 square = scene_size++;
    // scene[square] = ecs.spawn();
    // ecs.add<HgTransform>(scene[square]) = {};
    // ecs.get<HgTransform>(scene[square]).position.x = -1.5f;
    // ecs.add<HgSprite>(scene[square]) = {texture_id, {0.0f}, {1.0f}};

    // u32 dir_light = scene_size++;
    // scene[dir_light] = ecs.spawn();
    // ecs.add<HgDirLight3D>(scene[dir_light]) = {{1, 1, 1}, {1, 1, 1, 1}};

    u32 point_light = scene_size++;
    scene[point_light] = ecs.spawn();
    ecs.add<HgTransform>(scene[point_light]) = {};
    ecs.get<HgTransform>(scene[point_light]).position = {-1, -2, -1};
    ecs.add<HgPointLight3D>(scene[point_light]) = {{1, 1, 1, 2}};

    u32 cube = scene_size++;
    scene[cube] = ecs.spawn();
    ecs.add<HgTransform>(scene[cube]) = {};
    ecs.add<HgModel3D>(scene[cube]) = {};

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

    VkSampler render_sampler = nullptr;
    VkImage render_image = nullptr;
    VmaAllocation render_alloc = nullptr;
    VkImageView render_view = nullptr;
    VkDescriptorSet render_descriptor = nullptr;

    VkImage depth_image = nullptr;
    VkImageView depth_view = nullptr;
    VmaAllocation depth_alloc = nullptr;

    {
        VkSamplerCreateInfo render_sampler_info{};
        render_sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        vkCreateSampler(hg_vk_device, &render_sampler_info, nullptr, &render_sampler);
    }
    hg_defer(vkDestroySampler(hg_vk_device, render_sampler, nullptr));
    hg_defer(vmaDestroyImage(hg_vk_vma, render_image, render_alloc));
    hg_defer(vkDestroyImageView(hg_vk_device, render_view, nullptr));
    hg_defer(vmaDestroyImage(hg_vk_vma, depth_image, depth_alloc));
    hg_defer(vkDestroyImageView(hg_vk_device, depth_view, nullptr));

    bool show_render = true;
    bool show_editor = true;
    bool show_imgui_demo = false;
    bool move_3d = true;
    bool fixed_aspect = false;

    HgClock game_clock{};
    HgClock cpu_clock{};
    f64 cpu_delta = 0.0;

    for (;;) {
        f64 delta = game_clock.tick();

        HgQuat& cube_rot = ecs.get<HgTransform>(scene[cube]).rotation;
        cube_rot = hg_axis_angle({0, -1, 0}, (f32)delta) * cube_rot;

        HgArena& frame = hg_get_scratch(arena);
        HgArenaScope frame_scope{frame};

        hg_process_window_events(&window, 1);
        if (window->was_closed)
            goto quit;

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        ImGuiID dockspace_id = ImGui::GetID("dockspace");
        const ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(imgui_viewport->WorkPos);
        ImGui::SetNextWindowSize(imgui_viewport->WorkSize);
        ImGui::SetNextWindowViewport(imgui_viewport->ID);

        ImGuiWindowFlags dockspace_flags
            = ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Dockspace", nullptr, dockspace_flags);
        ImGui::PopStyleVar(3);

        ImGui::DockSpace(dockspace_id, {0.0f, 0.0f});

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {

                if (ImGui::MenuItem("Quit"))
                    goto quit;

                if (ImGui::MenuItem("Trigger Trap"))
                    abort();

                if (ImGui::MenuItem("Save Screenshot")) {
                    HgArena& scratch = hg_get_scratch();
                    HgArenaScope scratch_scope{scratch};

                    void* pixels = scratch.alloc(render_width * render_height * 4, 4);

                    HgVkImageStagingReadConfig config{};
                    config.dst = pixels;
                    config.src_image = render_image;
                    config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    config.subresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                    config.width = render_width;
                    config.height = render_height;
                    config.depth = 1;
                    config.format = VK_FORMAT_R8G8B8A8_SRGB;
                    hg_vk_image_staging_read(config);

                    stbi_write_png(
                        "screenshot.png",
                        (int)render_width,
                        (int)render_height,
                        4,
                        pixels,
                        (int)(render_width * sizeof(u32)));
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {

                ImGui::Checkbox("Render", &show_render);
                ImGui::Checkbox("Editor", &show_editor);
                ImGui::Checkbox("ImGui Demo", &show_imgui_demo);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (show_render) {
            if (ImGui::Begin("Render", &show_render)) {
                ImVec2 size = ImGui::GetContentRegionAvail();

                u32 view_height = fixed_aspect ? (u32)std::min(size.y, size.x / aspect_ratio) : (u32)size.y;
                u32 view_width = fixed_aspect ? (u32)((f32)view_height * aspect_ratio) : (u32)size.x;
                if (render_width != view_width || render_height != view_height) {
                    vkQueueWaitIdle(hg_vk_queue);

                    render_width = view_width;
                    render_height = view_height;

                    HgMat4 proj = hg_projection_perspective((f32)hg_pi * 0.5f, (f32)render_width / (f32)render_height, 0.1f, 1000.0f);
                    hg_pipeline_2d_update_projection(proj);
                    hg_pipeline_3d_update_projection(proj);

                    ImGui_ImplVulkan_RemoveTexture(render_descriptor);
                    vkDestroyImageView(hg_vk_device, render_view, nullptr);
                    vmaDestroyImage(hg_vk_vma, render_image, render_alloc);
                    vkDestroyImageView(hg_vk_device, depth_view, nullptr);
                    vmaDestroyImage(hg_vk_vma, depth_image, depth_alloc);

                    HgVkImageConfig render_image_info{};
                    render_image_info.width = render_width;
                    render_image_info.height = render_height;
                    render_image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
                    render_image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                            | VK_IMAGE_USAGE_SAMPLED_BIT
                                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

                    hg_vk_create_image(&render_image, &render_alloc, render_image_info);

                    HgVkImageViewConfig render_view_info{};
                    render_view_info.image = render_image;
                    render_view_info.format = render_image_info.format;
                    render_view_info.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                    render_view = hg_vk_create_image_view(render_view_info);

                    render_descriptor = ImGui_ImplVulkan_AddTexture(
                        render_sampler, render_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    HgVkImageConfig depth_image_info{};
                    depth_image_info.width = render_width;
                    depth_image_info.height = render_height;
                    depth_image_info.format = VK_FORMAT_D32_SFLOAT;
                    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

                    hg_vk_create_image(&depth_image, &depth_alloc, depth_image_info);

                    HgVkImageViewConfig depth_view_info{};
                    depth_view_info.image = depth_image;
                    depth_view_info.format = depth_image_info.format;
                    depth_view_info.subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                    depth_view = hg_vk_create_image_view(depth_view_info);
                }

                if (ImGui::IsWindowFocused()) {
                    if (move_3d && window->is_key_down[(u32)HgKey::lmouse]) {
                        f32 rot_speed = 2.0f;
                        HgQuat rot_x = hg_axis_angle({0, 1, 0}, (f32)window->mouse_delta_x * rot_speed);
                        HgQuat rot_y = hg_axis_angle({-1, 0, 0}, (f32)window->mouse_delta_y * rot_speed);
                        camera.rotation = rot_x * camera.rotation * rot_y;
                    }

                    HgVec3 movement = {0.0f};
                    if (move_3d) {
                        movement.y += window->is_key_down[(u32)HgKey::lshift] - window->is_key_down[(u32)HgKey::space];
                        movement.x += window->is_key_down[(u32)HgKey::d] - window->is_key_down[(u32)HgKey::a];
                        movement.z += window->is_key_down[(u32)HgKey::w] - window->is_key_down[(u32)HgKey::s];
                    } else {
                        movement.y += window->is_key_down[(u32)HgKey::s] - window->is_key_down[(u32)HgKey::w];
                        movement.x += window->is_key_down[(u32)HgKey::d] - window->is_key_down[(u32)HgKey::a];
                    }

                    if (movement != HgVec3{0.0f}) {
                        f32 move_speed = 1.5f;
                        HgVec3 rotated = hg_rotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                        camera.position += hg_norm(HgVec3{rotated.x, movement.y, rotated.z}) * move_speed * (f32)delta;
                    }
                }
                HgMat4 view = hg_view_matrix(camera.position, camera.scale, camera.rotation);
                hg_pipeline_2d_update_view(view);
                hg_pipeline_3d_update_view(view);

                ImGui::Image((ImTextureID)render_descriptor, {(f32)render_width, (f32)render_height});
            }
            ImGui::End();
        }

        if (show_editor) {
            if (ImGui::Begin("Editor", &show_editor)) {
                ImGuiTreeNodeFlags options_flags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Options", options_flags)) {
                    ImGui::SeparatorText("Time");
                    ImGui::Text("total: %.3fms", delta * 1.0e3);
                    ImGui::Text("cpu: %.3fms", cpu_delta * 1.0e3);

                    ImGui::SeparatorText("Camera");
                    if (ImGui::Button("Reset Camera"))
                        camera.position = {0, 0, -1}, camera.scale = {1, 1, 1}, camera.rotation = {1, 0, 0, 0};
                    ImGui::Checkbox("3D Movement", &move_3d);
                    ImGui::Checkbox("Fixed Aspect", &fixed_aspect);
                    if (fixed_aspect)
                        ImGui::DragFloat("Aspect", &aspect_ratio, 0.01f, 0.01f, 16.0f);
                }

                ImGuiTreeNodeFlags entity_flags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Entities", entity_flags)) {
                    if (ImGui::Button("Spawn Entity")) {
                        if (scene_size == scene_capacity) {
                            scene = arena.realloc(scene, scene_capacity, scene_capacity * 2);
                            scene_capacity *= 2;
                        }
                        scene[scene_size++] = ecs.spawn();
                    }

                    for (usize i = 0; i < scene_size; ++i) {
                        HgArena& scratch = hg_get_scratch();
                        HgArenaScope scratch_scope{scratch};
                        HgEntity e = scene[i];

                        char* name = HgString::create(scratch, "Entity ID: ")
                                .append(scratch, hg_int_to_str_base10(scratch, (i64)e.idx()))
                                .append(scratch, 0)
                                .chars;

                        if (ImGui::TreeNodeEx(name, entity_flags)) {
                            if (ImGui::Button("Despawn Entity")) {
                                ecs.despawn(e);
                                memmove(scene + i, scene + i + 1, sizeof(HgEntity) * (--scene_size - i));
                                --i;
                            } else {
                                ImGui::SameLine();
                                if (ImGui::Button("Add Component"))
                                    ImGui::OpenPopup("add_component");
                                if (ImGui::BeginPopup("add_component")) {
                                    ImGui::SeparatorText("Components");

                                    if (!ecs.has<HgTransform>(e) && ImGui::Selectable("Transform"))
                                        ecs.add<HgTransform>(e) = {};

                                    if (!ecs.has<HgSprite>(e) && ImGui::Selectable("Sprite")) {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgSprite>(e) = {texture_id, {0.0f, 0.0f}, {1.0f, 1.0f}};
                                    }

                                    if (!ecs.has<HgSprite>(e) && ImGui::Selectable("Model 3D")) {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgModel3D>(e) = {};
                                    }

                                    if (!ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.add<HgDirLight3D>(e) = {};

                                    if (!ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D")) {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgPointLight3D>(e) = {};
                                    }

                                    ImGui::EndPopup();
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Remove Component"))
                                    ImGui::OpenPopup("remove_component");
                                if (ImGui::BeginPopup("remove_component")) {
                                    ImGui::SeparatorText("Components");

                                    if (ecs.has<HgTransform>(e) && ImGui::Selectable("Transform")) {
                                        ecs.remove<HgTransform>(e);
                                        if (ecs.has<HgSprite>(e))
                                            ecs.remove<HgSprite>(e);
                                        if (ecs.has<HgModel3D>(e))
                                            ecs.remove<HgModel3D>(e);
                                        if (ecs.has<HgPointLight3D>(e))
                                            ecs.remove<HgPointLight3D>(e);
                                    }

                                    if (ecs.has<HgSprite>(e) && ImGui::Selectable("Sprite"))
                                        ecs.remove<HgSprite>(e);

                                    if (ecs.has<HgModel3D>(e) && ImGui::Selectable("Model 3D"))
                                        ecs.remove<HgModel3D>(e);

                                    if (ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.remove<HgDirLight3D>(e);

                                    if (ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D"))
                                        ecs.remove<HgPointLight3D>(e);

                                    ImGui::EndPopup();
                                }

                                ImGuiTreeNodeFlags component_flags = entity_flags;

                                if (ecs.has<HgTransform>(e) && ImGui::TreeNodeEx("Transform", component_flags)) {
                                    HgTransform& tf = ecs.get<HgTransform>(e);
                                    ImGui::DragFloat3("Position", &tf.position.x, 0.01f);
                                    ImGui::DragFloat3("Scale", &tf.scale.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgSprite>(e) && ImGui::TreeNodeEx("Sprite", component_flags)) {
                                    HgSprite& s = ecs.get<HgSprite>(e);
                                    ImGui::DragFloat2("UV Position", &s.uv_pos.x, 0.01f);
                                    ImGui::DragFloat2("UV Size", &s.uv_size.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgModel3D>(e) && ImGui::TreeNodeEx("Model 3D", component_flags)) {
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgDirLight3D>(e) && ImGui::TreeNodeEx("Directional Light 3D", component_flags)) {
                                    HgDirLight3D& l = ecs.get<HgDirLight3D>(e);
                                    ImGui::DragFloat3("Direction", &l.dir.x, 0.01f);
                                    ImGui::DragFloat4("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgPointLight3D>(e) && ImGui::TreeNodeEx("Point Light 3D", component_flags)) {
                                    HgPointLight3D& l = ecs.get<HgPointLight3D>(e);
                                    ImGui::DragFloat4("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                }
            }
            ImGui::End();
        }

        if (show_imgui_demo)
            ImGui::ShowDemoWindow(&show_imgui_demo);

        ImGui::Render();

        cpu_delta = cpu_clock.tick();
        VkCommandBuffer cmd = window->begin_recording();
        cpu_clock.tick();
        if (cmd != nullptr) {
            HgRenderer renderer = renderer.create(frame, 64, 64);

            HgImageRenderID render_image_id = renderer.add_image(
                render_image,
                render_view,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            HgImageRenderID depth_image_id = renderer.add_image(
                depth_image,
                depth_view,
                {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

            HgImageRenderID window_image_id = renderer.add_image(
                window->images[window->current_image],
                window->views[window->current_image],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            {
                HgRenderAttachment render_color_attachment{};
                render_color_attachment.image = render_image_id;
                render_color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
                render_color_attachment.clear_value.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                HgRenderPass render_pass{};
                render_pass.color_attachments = &render_color_attachment;
                render_pass.color_attachment_count = 1;
                render_pass.depth_attachment.image = depth_image_id;
                render_pass.depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
                render_pass.depth_attachment.clear_value.depthStencil = {1.0f, 0};

                renderer.begin_pass(cmd, render_width, render_height, render_pass);

                hg_draw_2d(ecs, cmd);
                hg_draw_3d(ecs, cmd);

                renderer.end_pass(cmd);
            }

            {
                HgRenderAttachment gui_color_attachment{};
                gui_color_attachment.image = window_image_id;
                gui_color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
                gui_color_attachment.clear_value.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                HgRenderPass gui_pass{};
                gui_pass.sampled_images = &render_image_id;
                gui_pass.sampled_image_count = 1;
                gui_pass.color_attachments = &gui_color_attachment;
                gui_pass.color_attachment_count = 1;

                renderer.begin_pass(cmd, window->width, window->height, gui_pass);

                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

                renderer.end_pass(cmd);
            }

            HgImageBarrier present_barrier{};
            present_barrier.image = window_image_id;
            present_barrier.next_usage = HgRenderUsage::present_src;

            renderer.barrier(cmd, nullptr, 0, &present_barrier, 1);

            window->end_and_present(cmd);

            // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            //     ImGui::UpdatePlatformWindows();
            //     ImGui::RenderPlatformWindowsDefault();
            // }
        }
    }
quit:
    vkDeviceWaitIdle(hg_vk_device);
}

