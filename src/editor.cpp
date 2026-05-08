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
    windowConfig.fullscreen = true;
    // windowConfig.preferredPresentMode = HgGpuPresentMode_mailbox;

    HgWindow* window = hgWindowCreate("Hg Editor Example", 1600, 900, &windowConfig);
    hgDefer(hgWindowDestroy(window));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    hgImGuiInit(window, hgWindowImageFormat(window));
    hgDefer(hgImGuiDeinit());

    u32 width = 0;
    u32 height = 0;
    f32 aspectRatio = 16.0f / 9.0f;

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

    hgSpritesInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgModelsInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgSkyboxInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgDefer(hgSkyboxDeinit());
    hgDefer(hgSpritesDeinit());
    hgDefer(hgModelsDeinit());

    HgEcs ecs = hgEcsCreate(arena, 4096, 64);
    hgDefer(hgEcsReset(&ecs));

    // hgEcsRegisterType(&ecs, arena, HgNode, 1024);
    hgEcsRegisterType(&ecs, arena, HgTransform, 1024);
    hgEcsRegisterType(&ecs, arena, HgCamera, 8);
    hgEcsRegisterType(&ecs, arena, HgSkybox, 8);
    hgEcsRegisterType(&ecs, arena, HgSprite, 256);
    hgEcsRegisterType(&ecs, arena, HgDirLight, 64);
    hgEcsRegisterType(&ecs, arena, HgPointLight, 64);
    hgEcsRegisterType(&ecs, arena, HgModel, 256);

    u32 sceneCapacity = 32;
    HgEntity* scene = hgAlloc<HgEntity>(arena, sceneCapacity);
    u32 sceneSize = 0;

    HgEntity camera = hgEcsCreate(&ecs);
    scene[sceneSize++] = camera;

    HgTransform* cameraTf = hgEcsAdd<HgTransform>(&ecs, camera);
    *cameraTf = {};
    cameraTf->position = HgVec3{0, 0, -1};

    HgCamera* cameraC = hgEcsAdd<HgCamera>(&ecs, camera);
    cameraC->type = HgCameraType_perspective;
    cameraC->perspective.fov = (f32)hgPi * 0.5f;
    cameraC->perspective.near = 0.1f;
    cameraC->perspective.far = 1000.0f;

    HgEntity skybox = hgEcsCreate(&ecs);
    scene[sceneSize++] = skybox;
    *hgEcsAdd<HgSkybox>(&ecs, skybox) = {};

    HgEntity pointLight = hgEcsCreate(&ecs);
    scene[sceneSize++] = pointLight;
    *hgEcsAdd<HgTransform>(&ecs, pointLight) = {};
    hgEcsGet<HgTransform>(&ecs, pointLight)->position = HgVec3{0, -2, 0};
    *hgEcsAdd<HgPointLight>(&ecs, pointLight) = {HgVec4{1, 1, 1, 4}};

    HgEntity square = hgEcsCreate(&ecs);
    scene[sceneSize++] = square;
    *hgEcsAdd<HgTransform>(&ecs, square) = {};
    hgEcsGet<HgTransform>(&ecs, square)->position = HgVec3{-1, 0, 1};
    *hgEcsAdd<HgSprite>(&ecs, square) = {HgGpuTextureHandle{}, HgVec2{0.0f}, HgVec2{1.0f}};

    HgEntity cube = hgEcsCreate(&ecs);
    scene[sceneSize++] = cube;
    *hgEcsAdd<HgTransform>(&ecs, cube) = {};
    hgEcsGet<HgTransform>(&ecs, cube)->position = HgVec3{1, 0, 1};
    *hgEcsAdd<HgModel>(&ecs, cube) = {HgGpuMeshHandle{}, HgGpuTextureHandle{}, HgGpuTextureHandle{}};

    bool showRender = true;
    bool showEditor = true;
    bool showImguiDemo = false;
    bool move3D = true;
    bool fixedAspect = false;
    f64 cpuDelta = 0.0;

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);
        HgClock cpuClock;
        hgClockTick(&cpuClock);

        HgArena* frame = hgScratch(&arena, 1);
        HgArenaScope frameScope{frame};

        if (hgEcsAlive(&ecs, square))
        {
            HgQuat& squareRot = hgEcsGet<HgTransform>(&ecs, square)->rotation;
            squareRot = hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * squareRot;
        }
        if (hgEcsAlive(&ecs, cube))
        {
            HgQuat& cubeRot = hgEcsGet<HgTransform>(&ecs, cube)->rotation;
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
                    void* pixels = hgAlloc(frame, width * height * 4, 4);

                    hgGpuImageRead(pixels, renderView);
                    stbi_write_png(
                        "screenshot.png",
                        (int)width,
                        (int)height,
                        4,
                        pixels,
                        (int)(width * sizeof(u32)));
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
                if (width != viewWidth || height != viewHeight)
                {
                    width = viewWidth;
                    height = viewHeight;

                    hgGpuWaitIdle();
                    hgImGuiTextureDestroy(renderImGuiTex);
                    hgGpuViewDestroy(depthView);
                    hgGpuImageDestroy(depthImage);
                    hgGpuViewDestroy(renderView);
                    hgGpuImageDestroy(renderImage);

                    renderImage = hgGpuImageCreate(
                        width,
                        height,
                        HgFormat_r8g8b8a8_srgb,
                        HgGpuImageUsage_colorAttachment |
                        HgGpuImageUsage_sampled |
                        HgGpuImageUsage_transferSrc);

                    depthImage = hgGpuImageCreate(
                        width,
                        height,
                        HgFormat_d32_sfloat,
                        HgGpuImageUsage_depthStencilAttachment);

                    renderView = hgGpuViewCreate(renderImage, 0, 1, 0, 1, HgGpuAspect_color);
                    depthView = hgGpuViewCreate(depthImage, 0, 1, 0, 1, HgGpuAspect_depth);

                    renderImGuiTex = hgImGuiTextureCreate(renderView, renderSampler, HgGpuLayout_shaderReadOnly);

                    cameraC->perspective.aspect = (f32)width / (f32)height;
                }

                if (ImGui::IsWindowFocused())
                {
                    if (move3D && hgIsButtonDown(window, HgButton_lmouse))
                    {
                        f32 rotSpeed = 2.0f;
                        HgQuat rotX = hgQuatAxisAngle(HgVec3{ 0, 1, 0}, hgMouseDeltaX(window) * rotSpeed);
                        HgQuat rotY = hgQuatAxisAngle(HgVec3{-1, 0, 0}, hgMouseDeltaY(window) * rotSpeed);
                        cameraTf->rotation = rotX * cameraTf->rotation * rotY;
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
                        HgVec3 rotated = hgVecRotate(cameraTf->rotation, HgVec3{movement.x, 0.0f, movement.z});
                        cameraTf->position += hgVecNorm3(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed;
                    }
                }

                ImGui::Image((ImTextureID)renderImGuiTex, {(f32)width, (f32)height});
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
                        cameraTf->position = HgVec3{0, 0, -1};
                        cameraTf->scale = HgVec3{1, 1, 1};
                        cameraTf->rotation = HgQuat{1, 0, 0, 0};
                    }
                    ImGui::Checkbox("3D Movement", &move3D);
                    ImGui::Checkbox("Fixed Aspect", &fixedAspect);
                    if (fixedAspect)
                        ImGui::DragFloat("Aspect", &aspectRatio, 0.01f, 0.01f, 16.0f);
                }

                ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Entities", entityFlags))
                {
                    if (ImGui::Button("Create Entity"))
                    {
                        if (sceneSize == sceneCapacity)
                        {
                            scene = hgRealloc(arena, scene, sceneCapacity, sceneCapacity * 2);
                            sceneCapacity *= 2;
                        }
                        scene[sceneSize++] = hgEcsCreate(&ecs);
                    }

                    for (u32 i = 0; i < sceneSize; ++i)
                    {
                        HgEntity e = scene[i];

                        HgString nameStr = hgStringCopy(frame, "Entitiy ID: ");
                        hgStringAppend(frame, &nameStr, hgIntegerToString(frame, (i64)hgHandleIdx(e.handle)));
                        char* name = hgCString(frame, nameStr);

                        if (ImGui::TreeNodeEx(name, entityFlags))
                        {
                            if (ImGui::Button("Destroy Entity"))
                            {
                                hgEcsDestroy(&ecs, e);
                                memmove(scene + i, scene + i + 1, sizeof(HgEntity) * (--sceneSize - i));
                                --i;
                            } else {
                                ImGui::SameLine();
                                if (ImGui::Button("Add Component"))
                                    ImGui::OpenPopup("addComponent");
                                if (ImGui::BeginPopup("addComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (!hgEcsHas<HgTransform>(&ecs, e) && ImGui::Selectable("Transform"))
                                        *hgEcsAdd<HgTransform>(&ecs, e) = {};

                                    if (!hgEcsHas<HgSprite>(&ecs, e) && ImGui::Selectable("Sprite 2D"))
                                    {
                                        if (!hgEcsHas<HgTransform>(&ecs, e))
                                            *hgEcsAdd<HgTransform>(&ecs, e) = {};
                                        *hgEcsAdd<HgSprite>(&ecs, e) = {HgGpuTextureHandle{}, HgVec2{0.0f, 0.0f}, HgVec2{1.0f, 1.0f}};
                                    }

                                    if (!hgEcsHas<HgModel>(&ecs, e) && ImGui::Selectable("Model 3D"))
                                    {
                                        if (!hgEcsHas<HgTransform>(&ecs, e))
                                            *hgEcsAdd<HgTransform>(&ecs, e) = {};
                                        *hgEcsAdd<HgModel>(&ecs, e) = {};
                                    }

                                    if (!hgEcsHas<HgDirLight>(&ecs, e) && ImGui::Selectable("Direction Light 3D"))
                                        *hgEcsAdd<HgDirLight>(&ecs, e) = {};

                                    if (!hgEcsHas<HgPointLight>(&ecs, e) && ImGui::Selectable("Point Light 3D"))
                                    {
                                        if (!hgEcsHas<HgTransform>(&ecs, e))
                                            *hgEcsAdd<HgTransform>(&ecs, e) = {};
                                        *hgEcsAdd<HgPointLight>(&ecs, e) = {};
                                    }

                                    ImGui::EndPopup();
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Remove Component"))
                                    ImGui::OpenPopup("removeComponent");
                                if (ImGui::BeginPopup("removeComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (hgEcsHas<HgTransform>(&ecs, e) && ImGui::Selectable("Transform"))
                                    {
                                        hgEcsRemove<HgTransform>(&ecs, e);
                                        if (hgEcsHas<HgSprite>(&ecs, e))
                                            hgEcsRemove<HgSprite>(&ecs, e);
                                        if (hgEcsHas<HgModel>(&ecs, e))
                                            hgEcsRemove<HgModel>(&ecs, e);
                                        if (hgEcsHas<HgPointLight>(&ecs, e))
                                            hgEcsRemove<HgPointLight>(&ecs, e);
                                    }

                                    if (hgEcsHas<HgSprite>(&ecs, e) && ImGui::Selectable("Sprite"))
                                        hgEcsRemove<HgSprite>(&ecs, e);

                                    if (hgEcsHas<HgModel>(&ecs, e) && ImGui::Selectable("Model 3D"))
                                        hgEcsRemove<HgModel>(&ecs, e);

                                    if (hgEcsHas<HgDirLight>(&ecs, e) && ImGui::Selectable("Direction Light 3D"))
                                        hgEcsRemove<HgDirLight>(&ecs, e);

                                    if (hgEcsHas<HgPointLight>(&ecs, e) && ImGui::Selectable("Point Light 3D"))
                                        hgEcsRemove<HgPointLight>(&ecs, e);

                                    ImGui::EndPopup();
                                }

                                ImGuiTreeNodeFlags componentFlags = entityFlags;

                                if (hgEcsHas<HgTransform>(&ecs, e) && ImGui::TreeNodeEx("Transform", componentFlags))
                                {
                                    HgTransform* tf = hgEcsGet<HgTransform>(&ecs, e);
                                    ImGui::DragFloat3("Position", &tf->position.x, 0.01f);
                                    ImGui::DragFloat3("Scale", &tf->scale.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (hgEcsHas<HgSprite>(&ecs, e) && ImGui::TreeNodeEx("Sprite", componentFlags))
                                {
                                    HgSprite* s = hgEcsGet<HgSprite>(&ecs, e);
                                    ImGui::DragFloat2("UV Position", &s->uvPos.x, 0.01f);
                                    ImGui::DragFloat2("UV Size", &s->uvSize.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (hgEcsHas<HgModel>(&ecs, e) && ImGui::TreeNodeEx("Model 3D", componentFlags))
                                {
                                    ImGui::TreePop();
                                }

                                if (hgEcsHas<HgDirLight>(&ecs, e) && ImGui::TreeNodeEx("Directional Light 3D", componentFlags))
                                {
                                    HgDirLight* l = hgEcsGet<HgDirLight>(&ecs, e);
                                    ImGui::DragFloat3("Direction", &l->dir.x, 0.01f);
                                    ImGui::DragFloat4("Color", &l->color.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (hgEcsHas<HgPointLight>(&ecs, e) && ImGui::TreeNodeEx("Point Light 3D", componentFlags))
                                {
                                    HgPointLight* l = hgEcsGet<HgPointLight>(&ecs, e);
                                    ImGui::DragFloat4("Color", &l->color.x, 0.01f);
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

            hgGpuRenderPassBegin(cmd, width, height, &renderPass);

            hgCameraUpdate(&ecs, camera);
            hgSkyboxDraw(&ecs, camera, cmd);
            hgSpritesDraw(&ecs, camera, cmd);
            hgModelsDraw(&ecs, camera, cmd);

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

