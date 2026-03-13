#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#include <thread>

#include <emmintrin.h>

#define IM_ASSERT hg_assert
#include "imgui.h"
#include "imgui_impl_vulkan.h"

void editor_example();
void minimal_example();

int main() {
    // minimal_example();
    editor_example();
}

void minimal_example() {
    hg_init();
    hg_defer(hg_exit());

    hg_tests_run();

    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgWindowConfig window_config{};
    window_config.title = "Hg Small Test";
    window_config.windowed = true;
    window_config.width = 1200;
    window_config.height = 800;

    HgWindow* window = HgWindow::create(arena, window_config);
    hg_defer(window->destroy());

    hg_pipeline_2d_init(arena, 256, window->format, VK_FORMAT_D32_SFLOAT);
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

    HgClock game_clock{};
    for (;;) {
        f64 delta = game_clock.tick();

        HgArena& frame = hg_get_scratch(arena);
        HgArenaScope frame_scope{frame};

        hg_process_window_events(&window, 1);
        if (window->was_closed)
            goto quit;

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

        VkCommandBuffer cmd = window->begin_recording();
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

            renderer.end_pass(cmd);

            HgImageBarrier present_barrier{};
            present_barrier.image = window_image;
            present_barrier.next_usage = HgRenderUsage::present_src;

            renderer.barrier(cmd, nullptr, 0, &present_barrier, 1);

            window->end_and_present(cmd);
        }
    }

quit:
    vkDeviceWaitIdle(hg_vk_device);
}

void editor_example() {
    hg_defer(hg_debug("Exited successfully\n"));

    hg_init();
    hg_defer(hg_exit());

    hg_tests_run();

    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgWindowConfig window_config{};
    window_config.title = "Hg Test";
    // window_config.windowed = true;
    // window_config.width = 1600;
    // window_config.height = 900;
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
    camera.position = {0, 0, -1};
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

    u32 square = scene_size++;
    scene[square] = ecs.spawn();
    ecs.add<HgTransform>(scene[square]) = {};
    ecs.get<HgTransform>(scene[square]).position.x = -1.5f;
    ecs.add<HgSprite>(scene[square]) = {texture_id, {0.0f}, {1.0f}};

    u32 dir_light = scene_size++;
    scene[dir_light] = ecs.spawn();
    ecs.add<HgTransform>(scene[dir_light]) = {};
    ecs.get<HgTransform>(scene[dir_light]).position.y = -2;
    ecs.add<HgPointLight3D>(scene[dir_light]) = {{1, 1, 1}};

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

                u32 view_height = fixed_aspect ? (u32)std::min(size.y, size.x / aspect_ratio) : size.y;
                u32 view_width = fixed_aspect ? view_height * aspect_ratio : size.x;
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
                                    ImGui::DragFloat3("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgPointLight3D>(e) && ImGui::TreeNodeEx("Point Light 3D", component_flags)) {
                                    HgPointLight3D& l = ecs.get<HgPointLight3D>(e);
                                    ImGui::DragFloat3("Color", &l.color.x, 0.01f);
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

    hg_test_assert(abs(rotated_vec.x - 1.0f) < FLT_EPSILON
                && abs(rotated_vec.y - 0.0f) < FLT_EPSILON
                && abs(rotated_vec.y - 0.0f) < FLT_EPSILON);

    hg_test_assert(abs(mat_rotated_vec.x - rotated_vec.x) < FLT_EPSILON
                && abs(mat_rotated_vec.y - rotated_vec.y) < FLT_EPSILON
                && abs(mat_rotated_vec.y - rotated_vec.z) < FLT_EPSILON);

    return true;
}

hg_test(HgArena) {
    void* block = malloc(1024);
    hg_defer(free(block));

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

        arena.head = 0;
    }

    return true;
}

hg_test(HgString) {
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf":
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next != nullptr);
        hg_test_assert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->msg == "on line 4, found unexpected token \"}\"\n");

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": true
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": false
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": asdf
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next != nullptr);
        hg_test_assert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->msg == "on line 3, found unexpected token \"asdf\"\n");

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0,
                "hjkl": 5678.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, 3, 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1 2 3 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors == nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, "3", 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_test_assert(json.errors != nullptr);
        hg_test_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_test_assert(error->next == nullptr);
        hg_test_assert(error->msg ==
            "on line 3, array has element which is not the same type as the first valid element\n");

        HgJson::Node* node = json.file;
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
        hg_test_assert(elem->next == nullptr);
        hg_test_assert(elem->value != nullptr);
        hg_test_assert(elem->value->type == HgJson::integer);
        hg_test_assert(elem->value->integer == 4);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        hg_test_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_test_assert(node->type == HgJson::jstruct);
        hg_test_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_test_assert(field->next == nullptr);
        hg_test_assert(field->name == "asdf");
        hg_test_assert(field->value != nullptr);
        hg_test_assert(field->value->type == HgJson::jstruct);
        hg_test_assert(field->value->array.elems != nullptr);

        HgJson::Field* sub_field = field->value->jstruct.fields;
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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        hg_test_assert(json.file != nullptr);

        HgJson::Node* main_struct = json.file;
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

hg_test(HgHashMap) {
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashMap<u32, u32> map = map.create(arena, 64);

        bool has_any = false;
        map.for_each([&](u32, u32) {
            has_any = true;
        });
        hg_test_assert(!has_any);

        map.insert(12, 24);
        map.insert(42, 84);
        map.insert(100, 200);

        bool has_12 = false;
        bool has_42 = false;
        bool has_100 = false;
        bool has_other = false;
        map.for_each([&](u32 k, u32 v) {
            if (k == 12 && v == 24)
                has_12 = true;
            else if (k == 42 && v == 84)
                has_42 = true;
            else if (k == 100 && v == 200)
                has_100 = true;
            else
                has_other = true;
        });
        hg_test_assert(has_12);
        hg_test_assert(has_42);
        hg_test_assert(has_100);
        hg_test_assert(!has_other);

        map.for_each([&](u32 k, u32) {
            map.remove(k);
        });

        has_any = false;
        map.for_each([&](u32, u32) {
            has_any = true;
        });
        hg_test_assert(!has_any);
    }

    return true;
}

hg_test(HgHashSet) {
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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

hg_test(hg_thread_pool) {
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgFence fence;
    {
        bool a = false;
        bool b = false;

        hg_call_par(&fence, 1, &a, [](void *pa) {
            *(bool*)pa = true;
        });
        hg_call_par(&fence, 1, &b, [](void *pb) {
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
            hg_call_par(&fence, 1, &val, [](void* data) {
                *(bool*)data = true;
            });
        }

        hg_test_assert(hg_thread_pool_help(fence, 2.0));

        for (bool& val : vals) {
            hg_test_assert(val == true);
        }
    }

    {
        bool vals[100] = {};

        hg_for_par(sizeof(vals) / sizeof(*vals), 16, [&](usize begin, usize end) {
            hg_assert(begin < end && end <= sizeof(vals) / sizeof(*vals));
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
                    hg_call_par(&fence, 1, vals + i, fn);
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }

            hg_test_assert(hg_thread_pool_help(fence, 2.0));
            for (auto val : vals) {
                hg_test_assert(val == true);
            }
        }
    }

    return true;
}

hg_test(hg_io_thread) {
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgFence fence;
    {
        bool vals[100] = {};

        hg_io_request(&fence, 1, vals, {}, [](void* pvals, HgStringView) {
            for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
                ((bool*)pvals)[i] = true;
            }
        });

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        bool vals[100] = {};

        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                *(bool*)pval = true;
            });
        }

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        bool vals[100] = {};

        vals[0] = true;

        for (usize i = 1; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                *(bool*)pval = *((bool*)pval - 1);
            });
        }

        hg_test_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_test_assert(vals[i] == true);
        }
    }

    {
        for (u32 n = 0; n < 3; ++n) {

            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                        *(bool*)pval = !*(bool*)pval;
                    });
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j) {
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
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    u32 save_data[] = {12, 42, 100, 128};

    const char* file_path = "hg_test_dir/file_bin_test.bin";
    HgBinary bin{};

    {
        bin.load(arena, "file_does_not_exist.bin");
        hg_test_assert(bin.data == nullptr);
        hg_test_assert(bin.size == 0);
    }

    {
        bin.data = save_data;
        bin.size = sizeof(save_data);

        bin.store("dir/does/not/exist.bin");

        FILE* file_handle = fopen("dir/does/not/exist.bin", "rb");
        hg_test_assert(file_handle == nullptr);
    }

    {
        bin.data = save_data;
        bin.size = sizeof(save_data);

        bin.store(file_path);
        HgBinary new_bin = bin.load(arena, file_path);

        hg_test_assert(new_bin.data != nullptr);
        hg_test_assert(new_bin.data != save_data);
        hg_test_assert(new_bin.size == sizeof(save_data));
        hg_test_assert(memcmp(save_data, new_bin.data, new_bin.size) == 0);
    }

    return true;
}

hg_test(HgTexture) {
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    struct color {
        u8 r, g, b, a;

        operator u32() { return *(u32*)this; }
    };

    u32 red =    color{0xff, 0x00, 0x00, 0xff};
    u32 green =  color{0x00, 0xff, 0x00, 0xff};
    u32 blue =   color{0x00, 0x00, 0xff, 0xff};
    u32 yellow = color{0xff, 0xff, 0x00, 0xff};

    constexpr VkFormat save_format = VK_FORMAT_R8G8B8A8_SRGB;
    constexpr u32 save_width = 2;
    constexpr u32 save_height = 2;
    constexpr u32 save_depth = 1;
    u32 save_data[save_width][save_height] = {
        {red, green},
        {blue, yellow},
    };

    HgBinary bin{};

    {
        HgTexture::Info info;
        memcpy(info.identifier, HgTexture::texture_identifier, sizeof(HgTexture::texture_identifier));
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.width = save_width;
        info.height = save_height;
        info.depth = save_depth;
        bin.grow(arena, sizeof(HgTexture::Info));
        bin.overwrite(0, info);

        usize pixel_idx = bin.size;
        bin.grow(arena, sizeof(save_data));
        bin.overwrite(pixel_idx, save_data, sizeof(save_data));
    }

    {
        HgTexture texture = bin;

        VkFormat format;
        u32 width, height, depth;
        hg_test_assert(texture.get_info(&format, &width, &height, &depth));
        hg_test_assert(format == save_format);
        hg_test_assert(width == save_width);
        hg_test_assert(height == save_height);
        hg_test_assert(depth == save_depth);
        hg_test_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

        void* pixels = texture.get_pixels();
        hg_test_assert(pixels != nullptr);
        hg_test_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
    }

    {
        HgStringView file_path = "hg_test_dir/file_image_test.hgtex";

        bin.store(file_path);
        HgTexture file_texture = HgBinary::load(arena, file_path);

        VkFormat format;
        u32 width, height, depth;
        hg_test_assert(file_texture.get_info(&format, &width, &height, &depth));
        hg_test_assert(format == save_format);
        hg_test_assert(width == save_width);
        hg_test_assert(height == save_height);
        hg_test_assert(depth == save_depth);
        hg_test_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

        void* pixels = file_texture.get_pixels();
        hg_test_assert(pixels != nullptr);
        hg_test_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
    }

    {
        HgStringView tex_path = "tex";
        HgStringView file_path = "hg_test_dir/file_image_test.png";
        HgResource tex_id = hg_resource_id(tex_path);
        HgResource file_id = hg_resource_id(file_path);
        hg_alloc_resource(tex_id);
        hg_alloc_resource(file_id);
        *hg_get_resource(tex_id) = bin;

        HgFence fence;
        hg_export_png(&fence, 1, tex_id, file_path);
        hg_import_png(&fence, 1, file_id, file_path);
        hg_test_assert(fence.wait(2.0));

        HgTexture file_texture = *hg_get_resource(file_id);

        VkFormat format;
        u32 width, height, depth;
        hg_test_assert(file_texture.get_info(&format, &width, &height, &depth));
        hg_test_assert(format == save_format);
        hg_test_assert(width == save_width);
        hg_test_assert(height == save_height);
        hg_test_assert(depth == save_depth);
        hg_test_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

        void* pixels = file_texture.get_pixels();
        hg_test_assert(pixels != nullptr);
        hg_test_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
    }

    hg_resources_reset();

    return true;
}

hg_test(hg_resource_management) {
    hg_warn("hg_resource_management test not implemented yet\n");
    return true;
}

hg_test(hg_ecs) {
    HgArena& arena = hg_get_scratch();
    HgArenaScope arena_scope{arena};

    HgECS ecs = ecs.create(1024);
    hg_defer(ecs.destroy());

    hg_defer(ecs.reset());

    HgEntity e1 = ecs.spawn();
    HgEntity e2 = ecs.spawn();
    HgEntity e3 = {};
    hg_test_assert(ecs.alive(e1));
    hg_test_assert(ecs.alive(e2));
    hg_test_assert(!ecs.alive(e3));

    ecs.despawn(e1);
    hg_test_assert(!ecs.alive(e1));
    e3 = ecs.spawn();
    hg_test_assert(ecs.alive(e3));
    hg_test_assert(e3.idx() == e1.idx() && e3 != e1);

    e1 = ecs.spawn();
    hg_test_assert(ecs.alive(e1));

    {
        hg_test_assert(!ecs.has<u32>(e1));
        hg_test_assert(!ecs.has<u32>(e2));
        hg_test_assert(!ecs.has<u32>(e3));

        ecs.add<u32>(e1) = 1;
        hg_test_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
        hg_test_assert(!ecs.has<u32>(e2));
        hg_test_assert(!ecs.has<u32>(e3));
        ecs.add<u32>(e2) = 2;
        hg_test_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
        hg_test_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
        hg_test_assert(!ecs.has<u32>(e3));
        ecs.add<u32>(e3) = 3;
        hg_test_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
        hg_test_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
        hg_test_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);

        ecs.remove<u32>(e1);
        hg_test_assert(!ecs.has<u32>(e1));
        hg_test_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
        hg_test_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
        ecs.remove<u32>(e2);
        hg_test_assert(!ecs.has<u32>(e1));
        hg_test_assert(!ecs.has<u32>(e2));
        hg_test_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
        ecs.remove<u32>(e3);
        hg_test_assert(!ecs.has<u32>(e1));
        hg_test_assert(!ecs.has<u32>(e2));
        hg_test_assert(!ecs.has<u32>(e3));
    }

    {
        bool has_unknown = false;
        ecs.for_each<u32>([&](HgEntity, u32&) {
            has_unknown = true;
        });
        hg_test_assert(!has_unknown);

        hg_test_assert(ecs.count<u32>() == 0);
        hg_test_assert(ecs.count<u64>() == 0);
    }

    {
        ecs.add<u32>(e1) = 12;
        ecs.add<u32>(e2) = 42;
        ecs.add<u32>(e3) = 100;
        hg_test_assert(ecs.count<u32>() == 3);
        hg_test_assert(ecs.count<u64>() == 0);

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
        hg_test_assert(has_12);
        hg_test_assert(has_42);
        hg_test_assert(has_100);
        hg_test_assert(!has_unknown);
    }

    {
        ecs.add<u64>(e2) = 2042;
        ecs.add<u64>(e3) = 2100;
        hg_test_assert(ecs.count<u32>() == 3);
        hg_test_assert(ecs.count<u64>() == 2);

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
        hg_test_assert(ecs.count<u32>() == 2);
        hg_test_assert(ecs.count<u64>() == 2);

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
        hg_test_assert(ecs.count<u32>() == 1);
        hg_test_assert(ecs.count<u64>() == 1);
    }

    ecs.reset();
    hg_test_assert(ecs.count<u32>() == 0);
    hg_test_assert(ecs.count<u64>() == 0);

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

        bool success;
        ecs.for_par<u32>(16, [&](HgEntity, u32& c) {
            c += 4;
        });
        success = true;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != 16)
                success = false;
        });
        hg_test_assert(success);

        ecs.for_par<u64>(16, [&](HgEntity, u64& c) {
            c += 3;
        });
        success = true;
        ecs.for_each<u64>([&](HgEntity, u64 c) {
            if (c != 45)
                success = false;
        });
        hg_test_assert(success);

        ecs.for_par<u32, u64>(16, [&](HgEntity, u32& c32, u64& c64) {
            c64 -= c32;
        });
        success = true;
        ecs.for_each<u64>([&](HgEntity e, u64 c) {
            if (ecs.has<u32>(e)) {
                if (c != 29)
                    success = false;
            } else {
                if (c != 45)
                    success = false;
            }
        });
        hg_test_assert(success);
    }

    ecs.reset();

    auto comparison = [](void*, HgECS& pecs, HgEntity lhs, HgEntity rhs) {
        return pecs.get<u32>(lhs) < pecs.get<u32>(rhs);
    };

    {
        ecs.add<u32>(ecs.spawn()) = 42;

        ecs.sort<u32>(nullptr, comparison);

        bool success = true;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != 42)
                success = false;
        });
        hg_test_assert(success);

        ecs.reset();
    }

    {
        u32 small_scramble_1[] = {1, 0};
        for (u32 i = 0; i < sizeof(small_scramble_1) / sizeof(*small_scramble_1); ++i) {
            ecs.add<u32>(ecs.spawn()) = small_scramble_1[i];
        }

        {
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hg_test_assert(success);
        }

        {
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hg_test_assert(success);
        }

        ecs.reset();
    }

    {
        u32 medium_scramble_1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
        for (u32 i = 0; i < sizeof(medium_scramble_1) / sizeof(*medium_scramble_1); ++i) {
            ecs.add<u32>(ecs.spawn()) = medium_scramble_1[i];
        }
        ecs.sort<u32>(nullptr, comparison);

        bool success = true;
        u32 elem = 0;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != elem)
                success = false;
            ++elem;
        });
        hg_test_assert(success);

        ecs.reset();
    }

    {
        u32 medium_scramble_2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
        for (u32 i = 0; i < sizeof(medium_scramble_2) / sizeof(*medium_scramble_2); ++i) {
            ecs.add<u32>(ecs.spawn()) = medium_scramble_2[i];
        }
        ecs.sort<u32>(nullptr, comparison);
        ecs.sort<u32>(nullptr, comparison);

        bool success = true;
        u32 elem = 0;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != elem)
                success = false;
            ++elem;
        });
        hg_test_assert(success);

        ecs.reset();
    }

    {
        for (u32 i = 127; i < 128; --i) {
            ecs.add<u32>(ecs.spawn()) = i;
        }
        ecs.sort<u32>(nullptr, comparison);

        bool success = true;
        u32 elem = 0;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != elem)
                success = false;
            ++elem;
        });
        hg_test_assert(success);

        ecs.reset();
    }

    {
        for (u32 i = 127; i < 128; --i) {
            ecs.add<u32>(ecs.spawn()) = i / 2;
        }
        ecs.sort<u32>(nullptr, comparison);
        ecs.sort<u32>(nullptr, comparison);

        bool success = true;
        u32 elem = 0;
        ecs.for_each<u32>([&](HgEntity, u32 c) {
            if (c != elem / 2)
                success = false;
            ++elem;
        });
        hg_test_assert(success);

        ecs.reset();
    }

    return true;
}

