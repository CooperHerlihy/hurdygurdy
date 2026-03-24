#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#define IM_ASSERT hgAssert
#include "imgui.h"
#include "imgui_impl_vulkan.h"

int main()
{
    hgDefer(hgDebug("Exited successfully\n"));

    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    HgWindowConfig windowConfig{};
    windowConfig.title = "Hg Test";
    windowConfig.windowed = true;
    windowConfig.width = 1600;
    windowConfig.height = 900;
    windowConfig.preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    HgWindow* window = HgWindow::create(arena, &windowConfig);
    hgDefer(window->destroy());

    hgInitPipeline2D(VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hgDefer(hgDeinitPipeline2D());

    hgInitPipeline3D(VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hgDefer(hgDeinitPipeline3D());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplHurdyGurdy_Init(window, 1, &window->format);
    hgDefer(ImGui_ImplHurdyGurdy_Shutdown());

    HgTransform camera{};
    camera.position = HgVec3{0, 0, -1};

    f32 aspectRatio = 16.0f / 9.0f;
    u32 renderWidth = window->width;
    u32 renderHeight = window->height;

    HgECS ecs = ecs.create(4096);
    hgDefer(ecs.destroy());

    u32 sceneCapacity = 8;
    HgEntity* scene = hgAlloc<HgEntity>(arena, sceneCapacity);
    u32 sceneSize = 0;

    HgEntity pointLight = ecs.spawn();
    scene[sceneSize++] = pointLight;
    ecs.add<HgTransform>(pointLight) = {};
    ecs.get<HgTransform>(pointLight).position = HgVec3{0, -2, 0};
    ecs.add<HgPointLight3D>(pointLight) = {HgVec4{1, 1, 1, 4}};

    HgEntity square = ecs.spawn();
    scene[sceneSize++] = square;
    ecs.add<HgTransform>(square) = {};
    ecs.get<HgTransform>(square).position = HgVec3{-1, 0, 1};
    ecs.add<HgSprite2D>(square) = {0, HgVec2{0.0f}, HgVec2{1.0f}};

    HgEntity cube = ecs.spawn();
    ecs.add<HgTransform>(cube) = {};
    ecs.get<HgTransform>(cube).position = HgVec3{1, 0, 1};
    ecs.add<HgModel3D>(cube) = {};
    u32 cubeIdx = sceneSize++;
    scene[cubeIdx] = cube;

    HgImage* renderImage = nullptr;
    hgDefer(hgDestroyImage(renderImage));

    HgImageView* renderView = nullptr;
    hgDefer(hgDestroyImageView(renderView));

    VkSampler renderSampler = hgCreateSampler(VK_FILTER_NEAREST);
    hgDefer(vkDestroySampler(hgVkDevice, renderSampler, nullptr));

    VkDescriptorSet renderDescriptor = nullptr;
    hgDefer(ImGui_ImplVulkan_RemoveTexture(renderDescriptor));

    HgImage* depthImage = nullptr;
    hgDefer(hgDestroyImage(depthImage));

    HgImageView* depthView = nullptr;
    hgDefer(hgDestroyImageView(depthView));

    bool showRender = true;
    bool showEditor = true;
    bool showImguiDemo = false;
    bool move3D = true;
    bool fixedAspect = false;
    f64 cpuDelta = 0.0;

    HgClock gameClock{};
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);
        HgClock cpuClock{};

        HgArena* frame = hgGetScratch();
        HgArenaScope frameScope{frame};

        if (ecs.alive(square))
        {
            HgQuat& squareRot = ecs.get<HgTransform>(square).rotation;
            squareRot = hgAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * squareRot;
        }
        if (ecs.alive(cube))
        {
            HgQuat& cubeRot = ecs.get<HgTransform>(cube).rotation;
            cubeRot = hgAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * cubeRot;
        }

        hgProcessWindowEvents(&window, 1);
        if (window->wasClosed)
            goto quit;

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        ImGuiID dockspaceID = ImGui::GetID("dockspace");
        const ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(imguiViewport->WorkPos);
        ImGui::SetNextWindowSize(imguiViewport->WorkSize);
        ImGui::SetNextWindowViewport(imguiViewport->ID);

        ImGuiWindowFlags dockspaceFlags
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
        ImGui::Begin("Dockspace", nullptr, dockspaceFlags);
        ImGui::PopStyleVar(3);

        ImGui::DockSpace(dockspaceID, {0.0f, 0.0f});

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {

                if (ImGui::MenuItem("Quit"))
                    goto quit;

                if (ImGui::MenuItem("Trigger Trap"))
                    abort();

                if (ImGui::MenuItem("Save Screenshot"))
                {
                    HgArena* scratch = hgGetScratch();
                    HgArenaScope scratchScope{scratch};

                    void* pixels = hgAlloc(scratch, renderWidth * renderHeight * 4, 4);

                    hgReadImage(
                        pixels,
                        renderImage,
                        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    stbi_write_png(
                        "screenshot.png",
                        (int)renderWidth,
                        (int)renderHeight,
                        4,
                        pixels,
                        (int)(renderWidth * sizeof(u32)));
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {

                ImGui::Checkbox("Render", &showRender);
                ImGui::Checkbox("Editor", &showEditor);
                ImGui::Checkbox("ImGui Demo", &showImguiDemo);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (showRender)
        {
            if (ImGui::Begin("Render", &showRender))
            {
                ImVec2 size = ImGui::GetContentRegionAvail();

                u32 viewHeight = fixedAspect ? (u32)std::min(size.y, size.x / aspectRatio) : (u32)size.y;
                u32 viewWidth = fixedAspect ? (u32)((f32)viewHeight * aspectRatio) : (u32)size.x;
                if (renderWidth != viewWidth || renderHeight != viewHeight)
                {
                    vkQueueWaitIdle(hgVkQueue);

                    renderWidth = viewWidth;
                    renderHeight = viewHeight;

                    HgMat4 proj = hgPerspective((f32)hgPi * 0.5f, (f32)renderWidth / (f32)renderHeight, 0.1f, 1000.0f);
                    hgUpdateProjection2D(&proj);
                    hgUpdateProjection3D(&proj);

                    ImGui_ImplVulkan_RemoveTexture(renderDescriptor);
                    hgDestroyImageView(depthView);
                    hgDestroyImage(depthImage);
                    hgDestroyImageView(renderView);
                    hgDestroyImage(renderImage);

                    renderImage = hgCreateImage(
                            renderWidth,
                            renderHeight,
                            VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                    renderView = hgCreateImageView(renderImage, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

                    renderDescriptor = ImGui_ImplVulkan_AddTexture(
                        renderSampler, renderView->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    depthImage = hgCreateImage(
                        renderWidth,
                        renderHeight,
                        VK_FORMAT_D32_SFLOAT,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

                    depthView = hgCreateImageView(depthImage, {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});
                }

                if (ImGui::IsWindowFocused())
                {
                    if (move3D && window->isKeyDown[HgKey_lmouse])
                    {
                        f32 rotSpeed = 2.0f;
                        HgQuat rotX = hgAxisAngle(HgVec3{0, 1, 0}, (f32)window->mouseDeltaX * rotSpeed);
                        HgQuat rotY = hgAxisAngle(HgVec3{-1, 0, 0}, (f32)window->mouseDeltaY * rotSpeed);
                        camera.rotation = rotX * camera.rotation * rotY;
                    }

                    HgVec3 movement = HgVec3{0.0f};
                    if (move3D)
                    {
                        movement.y += window->isKeyDown[HgKey_lshift] - window->isKeyDown[HgKey_space];
                        movement.x += window->isKeyDown[HgKey_d] - window->isKeyDown[HgKey_a];
                        movement.z += window->isKeyDown[HgKey_w] - window->isKeyDown[HgKey_s];
                    } else {
                        movement.y += window->isKeyDown[HgKey_s] - window->isKeyDown[HgKey_w];
                        movement.x += window->isKeyDown[HgKey_d] - window->isKeyDown[HgKey_a];
                    }

                    if (movement != HgVec3{0.0f})
                    {
                        f32 moveSpeed = 1.5f;
                        HgVec3 rotated = hgRotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                        camera.position += hgNorm(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
                    }
                }
                HgMat4 view = hgViewMatrix(camera.position, camera.scale, camera.rotation);
                hgUpdateView2D(&view);
                hgUpdateView3D(&view);

                ImGui::Image((ImTextureID)renderDescriptor, {(f32)renderWidth, (f32)renderHeight});
            }
            ImGui::End();
        }

        if (showEditor)
        {
            if (ImGui::Begin("Editor", &showEditor))
            {
                ImGuiTreeNodeFlags optionsFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Options", optionsFlags))
                {
                    ImGui::SeparatorText("Time");
                    ImGui::Text("total: %.3fms", delta * 1.0e3);
                    ImGui::Text("cpu: %.3fms", cpuDelta * 1.0e3);

                    ImGui::SeparatorText("Camera");
                    if (ImGui::Button("Reset Camera"))
                    {
                        camera.position = HgVec3{0, 0, -1};
                        camera.scale = HgVec3{1, 1, 1};
                        camera.rotation = HgQuat{1, 0, 0, 0};
                    }
                    ImGui::Checkbox("3D Movement", &move3D);
                    ImGui::Checkbox("Fixed Aspect", &fixedAspect);
                    if (fixedAspect)
                        ImGui::DragFloat("Aspect", &aspectRatio, 0.01f, 0.01f, 16.0f);
                }

                ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Entities", entityFlags))
                {
                    if (ImGui::Button("Spawn Entity"))
                    {
                        if (sceneSize == sceneCapacity)
                        {
                            scene = hgRealloc(arena, scene, sceneCapacity, sceneCapacity * 2);
                            sceneCapacity *= 2;
                        }
                        scene[sceneSize++] = ecs.spawn();
                    }

                    for (u32 i = 0; i < sceneSize; ++i)
                    {
                        HgArena* scratch = hgGetScratch();
                        HgArenaScope scratchScope{scratch};
                        HgEntity e = scene[i];

                        HgString nameStr = hgCopyString(scratch, "Entitiy ID: ");
                        hgAppendString(scratch, &nameStr, hgIntToStr(scratch, (i64)e.idx()));
                        char* name = hgCString(scratch, nameStr);

                        if (ImGui::TreeNodeEx(name, entityFlags))
                        {
                            if (ImGui::Button("Despawn Entity"))
                            {
                                ecs.despawn(e);
                                memmove(scene + i, scene + i + 1, sizeof(HgEntity) * (--sceneSize - i));
                                --i;
                            } else {
                                ImGui::SameLine();
                                if (ImGui::Button("Add Component"))
                                    ImGui::OpenPopup("addComponent");
                                if (ImGui::BeginPopup("addComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (!ecs.has<HgTransform>(e) && ImGui::Selectable("Transform"))
                                        ecs.add<HgTransform>(e) = {};

                                    if (!ecs.has<HgSprite2D>(e) && ImGui::Selectable("Sprite 2D"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgSprite2D>(e) = {0, HgVec2{0.0f, 0.0f}, HgVec2{1.0f, 1.0f}};
                                    }

                                    if (!ecs.has<HgModel3D>(e) && ImGui::Selectable("Model 3D"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgModel3D>(e) = {};
                                    }

                                    if (!ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.add<HgDirLight3D>(e) = {};

                                    if (!ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgPointLight3D>(e) = {};
                                    }

                                    ImGui::EndPopup();
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Remove Component"))
                                    ImGui::OpenPopup("removeComponent");
                                if (ImGui::BeginPopup("removeComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (ecs.has<HgTransform>(e) && ImGui::Selectable("Transform"))
                                    {
                                        ecs.remove<HgTransform>(e);
                                        if (ecs.has<HgSprite2D>(e))
                                            ecs.remove<HgSprite2D>(e);
                                        if (ecs.has<HgModel3D>(e))
                                            ecs.remove<HgModel3D>(e);
                                        if (ecs.has<HgPointLight3D>(e))
                                            ecs.remove<HgPointLight3D>(e);
                                    }

                                    if (ecs.has<HgSprite2D>(e) && ImGui::Selectable("Sprite"))
                                        ecs.remove<HgSprite2D>(e);

                                    if (ecs.has<HgModel3D>(e) && ImGui::Selectable("Model 3D"))
                                        ecs.remove<HgModel3D>(e);

                                    if (ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.remove<HgDirLight3D>(e);

                                    if (ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D"))
                                        ecs.remove<HgPointLight3D>(e);

                                    ImGui::EndPopup();
                                }

                                ImGuiTreeNodeFlags componentFlags = entityFlags;

                                if (ecs.has<HgTransform>(e) && ImGui::TreeNodeEx("Transform", componentFlags))
                                {
                                    HgTransform& tf = ecs.get<HgTransform>(e);
                                    ImGui::DragFloat3("Position", &tf.position.x, 0.01f);
                                    ImGui::DragFloat3("Scale", &tf.scale.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgSprite2D>(e) && ImGui::TreeNodeEx("Sprite", componentFlags))
                                {
                                    HgSprite2D& s = ecs.get<HgSprite2D>(e);
                                    ImGui::DragFloat2("UV Position", &s.uvPos.x, 0.01f);
                                    ImGui::DragFloat2("UV Size", &s.uvSize.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgModel3D>(e) && ImGui::TreeNodeEx("Model 3D", componentFlags))
                                {
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgDirLight3D>(e) && ImGui::TreeNodeEx("Directional Light 3D", componentFlags))
                                {
                                    HgDirLight3D& l = ecs.get<HgDirLight3D>(e);
                                    ImGui::DragFloat3("Direction", &l.dir.x, 0.01f);
                                    ImGui::DragFloat4("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgPointLight3D>(e) && ImGui::TreeNodeEx("Point Light 3D", componentFlags))
                                {
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

        if (showImguiDemo)
            ImGui::ShowDemoWindow(&showImguiDemo);

        ImGui::Render();

        cpuDelta = hgClockTick(&cpuClock);
        VkCommandBuffer cmd = window->beginRecording();
        if (cmd != nullptr)
        {
            hgClockTick(&cpuClock);

            HgRenderer renderer = renderer.create(frame, 32, 32);

            HgRenderAttachment renderColorAttachment{};
            renderColorAttachment.image = renderView;
            renderColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            renderColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            HgRenderAttachment renderDepthAttachment{};
            renderDepthAttachment.image = depthView;
            renderDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            renderDepthAttachment.clearValue.depthStencil = {1.0f, 0};

            HgRenderPass renderPass{};
            renderPass.colorAttachments = &renderColorAttachment;
            renderPass.colorAttachmentCount = 1;
            renderPass.depthAttachment = &renderDepthAttachment;

            renderer.beginPass(cmd, renderWidth, renderHeight, &renderPass);

            hgDraw3D(&ecs, cmd);
            hgDraw2D(&ecs, cmd);

            renderer.endPass(cmd);

            HgRenderAttachment guiColorAttachment{};
            guiColorAttachment.image = &window->views[window->currentImage];
            guiColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            guiColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            HgRenderPass guiPass{};
            guiPass.sampledImages = &renderView;
            guiPass.sampledImageCount = 1;
            guiPass.colorAttachments = &guiColorAttachment;
            guiPass.colorAttachmentCount = 1;

            renderer.beginPass(cmd, window->width, window->height, &guiPass);

            ImGui_ImplHurdyGurdy_Draw(cmd);

            renderer.endPass(cmd);

            HgImageBarrier presentBarrier{};
            presentBarrier.image = &window->views[window->currentImage];
            presentBarrier.nextLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            renderer.barrier(cmd, nullptr, 0, &presentBarrier, 1);

            cpuDelta += hgClockTick(&cpuClock);
            window->endAndPresent(cmd);
        }
    }
quit:
    vkDeviceWaitIdle(hgVkDevice);
}

