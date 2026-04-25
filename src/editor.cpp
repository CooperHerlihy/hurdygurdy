#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#define IM_ASSERT hgAssert
#include "imgui.h"

int main()
{
    hgDefer(hgDebug("Exited successfully\n"));

    hgInit(nullptr);
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    HgArenaScope arenaScope{arena};

    HgWindowConfig windowConfig{};
    windowConfig.preferredPresentMode = HgGpuPresentMode_mailbox;

    HgWindow* window = hgWindowCreate("Hg Test", 1600, 900, &windowConfig);
    hgDefer(hgWindowDestroy(window));

    hgInitPipeline2D(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgDefer(hgDeinitPipeline2D());

    hgInitPipeline3D(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgDefer(hgDeinitPipeline3D());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    HgFormat windowFormat = hgWindowImageFormat(window);
    hgImGuiInit(window, &windowFormat, 1);
    hgDefer(hgImGuiDeinit());

    HgTransform camera{};
    camera.position = HgVec3{0, 0, -1};

    f32 aspectRatio = 16.0f / 9.0f;
    u32 renderWidth = 0;
    u32 renderHeight = 0;

    HgECS ecs = ecs.create(arena, 1024, 128);
    ecs.createComponent<HgTransform>(arena, 1024);
    ecs.createComponent<HgSprite2D>(arena, 1024);
    ecs.createComponent<HgModel3D>(arena, 1024);
    ecs.createComponent<HgDirLight3D>(arena, 1024);
    ecs.createComponent<HgPointLight3D>(arena, 1024);

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

    HgGpuImage* renderImage = nullptr;
    HgGpuView* renderView = nullptr;
    HgGpuSampler* renderSampler = hgGpuSamplerCreate(HgGpuFilter_nearest);
    void* renderImGuiTex = nullptr;
    hgDefer(hgGpuImageDestroy(renderImage));
    hgDefer(hgGpuViewDestroy(renderView));
    hgDefer(hgGpuSamplerDestroy(renderSampler));
    hgDefer(hgImGuiTextureDestroy(renderImGuiTex));

    HgGpuImage* depthImage = nullptr;
    HgGpuView* depthView = nullptr;
    hgDefer(hgGpuImageDestroy(depthImage));
    hgDefer(hgGpuViewDestroy(depthView));

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

        HgArena* frame = hgScratch(&arena, 1);
        HgArenaScope frameScope{frame};

        if (ecs.alive(square))
        {
            HgQuat& squareRot = ecs.get<HgTransform>(square).rotation;
            squareRot = hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * squareRot;
        }
        if (ecs.alive(cube))
        {
            HgQuat& cubeRot = ecs.get<HgTransform>(cube).rotation;
            cubeRot = hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * cubeRot;
        }

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        hgImGuiNewFrame();
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
                    void* pixels = hgAlloc(frame, renderWidth * renderHeight * 4, 4);

                    hgGpuImageRead(pixels, renderView);
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

                u32 viewHeight = std::max((u32)1, fixedAspect ? (u32)std::min(size.y, size.x / aspectRatio) : (u32)size.y);
                u32 viewWidth = std::max((u32)1, fixedAspect ? (u32)((f32)viewHeight * aspectRatio) : (u32)size.x);
                if (renderWidth != viewWidth || renderHeight != viewHeight)
                {
                    hgGpuWaitIdle();

                    renderWidth = viewWidth;
                    renderHeight = viewHeight;

                    HgMat4 proj = hgMatPerspective((f32)hgPi * 0.5f, (f32)renderWidth / (f32)renderHeight, 0.1f, 1000.0f);
                    hgUpdateProjection2D(&proj);
                    hgUpdateProjection3D(&proj);

                    hgImGuiTextureDestroy(renderImGuiTex);
                    hgGpuViewDestroy(depthView);
                    hgGpuImageDestroy(depthImage);
                    hgGpuViewDestroy(renderView);
                    hgGpuImageDestroy(renderImage);

                    renderImage = hgGpuImageCreate(
                        renderWidth,
                        renderHeight,
                        HgFormat_r8g8b8a8_srgb,
                        HgGpuImageUsage_colorAttachment |
                        HgGpuImageUsage_sampled |
                        HgGpuImageUsage_transferSrc);

                    depthImage = hgGpuImageCreate(
                        renderWidth,
                        renderHeight,
                        HgFormat_d32_sfloat,
                        HgGpuImageUsage_depthStencilAttachment);

                    renderView = hgGpuViewCreate(renderImage, 0, 1, 0, 1, HgGpuAspect_color);
                    depthView = hgGpuViewCreate(depthImage, 0, 1, 0, 1, HgGpuAspect_depth);

                    renderImGuiTex = hgImGuiTextureCreate(renderView, renderSampler, HgGpuLayout_shaderReadOnly);
                }

                if (ImGui::IsWindowFocused())
                {
                    if (move3D && hgIsButtonDown(window, HgButton_lmouse))
                    {
                        f32 rotSpeed = 2.0f;
                        HgQuat rotX = hgQuatAxisAngle(HgVec3{ 0, 1, 0}, hgMouseDeltaX(window) * rotSpeed);
                        HgQuat rotY = hgQuatAxisAngle(HgVec3{-1, 0, 0}, hgMouseDeltaY(window) * rotSpeed);
                        camera.rotation = rotX * camera.rotation * rotY;
                    }

                    HgVec3 movement = HgVec3{0.0f};
                    if (move3D)
                    {
                        movement.y += hgIsButtonDown(window, HgButton_lshift) - hgIsButtonDown(window, HgButton_space);
                        movement.x += hgIsButtonDown(window, HgButton_d) - hgIsButtonDown(window, HgButton_a);
                        movement.z += hgIsButtonDown(window, HgButton_w) - hgIsButtonDown(window, HgButton_s);
                    } else {
                        movement.y += hgIsButtonDown(window, HgButton_s) - hgIsButtonDown(window, HgButton_w);
                        movement.x += hgIsButtonDown(window, HgButton_d) - hgIsButtonDown(window, HgButton_a);
                    }

                    if (movement != HgVec3{0.0f})
                    {
                        f32 moveSpeed = 1.5f * (f32)delta;
                        HgVec3 rotated = hgVecRotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                        camera.position += hgVecNorm3(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed;
                    }
                }
                HgMat4 view = hgMatView(camera.position, camera.scale, camera.rotation);
                hgUpdateView2D(&view);
                hgUpdateView3D(&view);

                ImGui::Image((ImTextureID)renderImGuiTex, {(f32)renderWidth, (f32)renderHeight});
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
                        HgEntity e = scene[i];

                        HgString nameStr = hgStringCopy(frame, "Entitiy ID: ");
                        hgStringAppend(frame, &nameStr, hgIntegerToString(frame, (i64)e.idx()));
                        char* name = hgCString(frame, nameStr);

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
        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
        if (hgWindowImageView(window) != nullptr)
        {
            hgClockTick(&cpuClock);

            HgGpuRenderAttachment renderColorAttachment{};
            renderColorAttachment.image = renderView;
            renderColorAttachment.loadOp = HgGpuLoadOp_clear;
            renderColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            HgGpuRenderAttachment renderDepthAttachment{};
            renderDepthAttachment.image = depthView;
            renderDepthAttachment.loadOp = HgGpuLoadOp_clear;
            renderDepthAttachment.clearValue.depthStencil = {1.0f, 0};

            HgGpuRenderPass renderPass{};
            renderPass.colorAttachments = &renderColorAttachment;
            renderPass.colorAttachmentCount = 1;
            renderPass.depthAttachment = &renderDepthAttachment;

            hgGpuRenderPassBegin(cmd, renderWidth, renderHeight, &renderPass);

            hgDraw3D(&ecs, cmd);
            hgDraw2D(&ecs, cmd);

            hgGpuRenderPassEnd(cmd);

            HgGpuRenderAttachment guiColorAttachment{};
            guiColorAttachment.image = hgWindowImageView(window);
            guiColorAttachment.loadOp = HgGpuLoadOp_clear;
            guiColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            HgGpuRenderPass guiPass{};
            guiPass.sampledImages = &renderView;
            guiPass.sampledImageCount = 1;
            guiPass.colorAttachments = &guiColorAttachment;
            guiPass.colorAttachmentCount = 1;

            hgGpuRenderPassBegin(cmd, hgWindowWidth(window), hgWindowHeight(window), &guiPass);

            hgImGuiDraw(cmd);

            hgGpuRenderPassEnd(cmd);

            HgGpuImageBarrier presentBarrier{};
            presentBarrier.image = hgWindowImageView(window);
            presentBarrier.nextLayout = HgGpuLayout_presentSrc;

            hgGpuMemoryBarrier(cmd, nullptr, 0, &presentBarrier, 1);
        }
        cpuDelta += hgClockTick(&cpuClock);
        hgGpuFrameEnd(cmd);
    }
quit:
    hgGpuWaitIdle();
}

