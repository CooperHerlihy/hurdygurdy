#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#define IM_ASSERT hgAssert
#include "imgui.h"

static HgWindow* window = nullptr;

static u32 width = 0;
static u32 height = 0;
static f32 aspectRatio = 16.0f / 9.0f;

static HgGpuImage* renderImage = nullptr;
static HgGpuView* renderView = nullptr;
static HgGpuSampler* renderSampler = nullptr;
static void* renderImGuiTex = nullptr;

static HgGpuImage* depthImage = nullptr;
static HgGpuView* depthView = nullptr;

static HgEcs ecs = {};
static HgEntity root;

static HgEntity camera;
static HgTransform* cameraTf;
static HgCamera* cameraC;

static bool showEditor = true;
static bool showRender = true;
static bool showImguiDemo = false;
static bool fixedAspect = false;
static bool move3D = true;

static f64 delta = 0.0;
static f64 cpuDelta = 0.0;

static bool quit = false;
static bool renderHovered = false;

struct Name {
    HgStringView name;
};

struct Spin {
    f32 speed;
};

void init(HgArena* arena)
{
    HgWindowConfig windowConfig{};
    windowConfig.fullscreen = true;
    windowConfig.preferredPresentMode = HgGpuPresentMode_mailbox;

    window = hgWindowCreate("Hg Editor Example", 1600, 900, &windowConfig);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    hgImGuiInit(window, hgWindowImageFormat(window));

    renderSampler = hgGpuSamplerCreate(HgGpuFilter_nearest);

    hgSpritesInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgModelsInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);
    hgSkyboxInit(HgFormat_r8g8b8a8_srgb, HgFormat_d32_sfloat);

    ecs = hgEcsCreate(arena, 1024, 64);

    hgEcsRegisterType(&ecs, arena, HgNode, 1024);
    hgEcsRegisterType(&ecs, arena, HgTransform, 1024);
    hgEcsRegisterType(&ecs, arena, HgCamera, 8);
    hgEcsRegisterType(&ecs, arena, HgSkybox, 8);
    hgEcsRegisterType(&ecs, arena, HgSprite, 256);
    hgEcsRegisterType(&ecs, arena, HgDirLight, 64);
    hgEcsRegisterType(&ecs, arena, HgPointLight, 64);
    hgEcsRegisterType(&ecs, arena, HgModel, 256);

    hgEcsRegisterType(&ecs, arena, Name, 1024);
    hgEcsRegisterType(&ecs, arena, Spin, 256);

    root = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, root) = {"root"};
    hgEcsAdd<HgNode>(&ecs, root);

    camera = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, camera) = {"camera"};
    hgEcsAdd<HgNode>(&ecs, camera);
    cameraTf = hgEcsAdd<HgTransform>(&ecs, camera);
    cameraC = hgEcsAdd<HgCamera>(&ecs, camera);

    cameraTf->position = HgVec3{0, 0, -1};

    cameraC->type = HgCameraType_perspective;
    cameraC->perspective.fov = (f32)hgPi * 0.5f;
    cameraC->perspective.near = 0.01f;
    cameraC->perspective.far = 1000.0f;

    HgEntity skybox = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, skybox) = {"skybox"};
    hgEcsAdd<HgNode>(&ecs, skybox);
    hgEcsAdd<HgSkybox>(&ecs, skybox);

    HgEntity pointLight = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, pointLight) = {"pointLight"};
    hgEcsAdd<HgNode>(&ecs, pointLight);
    hgEcsAdd<HgTransform>(&ecs, pointLight);
    hgEcsGet<HgTransform>(&ecs, pointLight)->position = HgVec3{0, -2, 0};
    *hgEcsAdd<HgPointLight>(&ecs, pointLight) = {HgVec4{1, 1, 1, 4}};

    // HgEntity square = hgEcsSpawn(&ecs);
    // *hgEcsAdd<Name>(&ecs, square) = {"square"};
    // hgEcsAdd<HgNode>(&ecs, square);
    // hgEcsAdd<HgTransform>(&ecs, square);
    // hgEcsGet<HgTransform>(&ecs, square)->position = HgVec3{-1, 0, 1};
    // *hgEcsAdd<HgSprite>(&ecs, square) = {HgGpuTextureHandle{}, HgVec2{0.0f}, HgVec2{1.0f}};
    // *hgEcsAdd<Spin>(&ecs, square) = {1.0f};
    //
    // HgEntity cube = hgEcsSpawn(&ecs);
    // *hgEcsAdd<Name>(&ecs, cube) = {"cube"};
    // hgEcsAdd<HgNode>(&ecs, cube);
    // hgEcsAdd<HgTransform>(&ecs, cube);
    // hgEcsGet<HgTransform>(&ecs, cube)->position = HgVec3{1, 0, 1};
    // *hgEcsAdd<HgModel>(&ecs, cube) = {HgGpuMeshHandle{}, HgGpuTextureHandle{}, HgGpuTextureHandle{}};
    // *hgEcsAdd<Spin>(&ecs, cube) = {1.0f};

    HgEntity cubeA = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, cubeA) = {"cubeA"};
    hgEcsAdd<HgNode>(&ecs, cubeA);
    hgEcsAdd<HgTransform>(&ecs, cubeA);
    hgEcsGet<HgTransform>(&ecs, cubeA)->scale = HgVec3{.3, .3, .3};
    hgEcsGet<HgTransform>(&ecs, cubeA)->position = HgVec3{0, 0, 0};
    *hgEcsAdd<HgModel>(&ecs, cubeA) = {HgGpuMeshHandle{}, HgGpuTextureHandle{}, HgGpuTextureHandle{}};
    *hgEcsAdd<Spin>(&ecs, cubeA) = {1.0f};

    HgEntity cubeAA = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, cubeAA) = {"cubeAA"};
    hgEcsAdd<HgNode>(&ecs, cubeAA);
    hgEcsAdd<HgTransform>(&ecs, cubeAA);
    hgEcsGet<HgTransform>(&ecs, cubeAA)->scale = HgVec3{.3, .3, .3};
    hgEcsGet<HgTransform>(&ecs, cubeAA)->position = HgVec3{.5, 0, 0};
    *hgEcsAdd<HgModel>(&ecs, cubeAA) = {HgGpuMeshHandle{}, HgGpuTextureHandle{}, HgGpuTextureHandle{}};
    // *hgEcsAdd<Spin>(&ecs, cubeAA) = {1.0f};

    HgEntity cubeAB = hgEcsSpawn(&ecs);
    *hgEcsAdd<Name>(&ecs, cubeAB) = {"cubeAB"};
    hgEcsAdd<HgNode>(&ecs, cubeAB);
    hgEcsAdd<HgTransform>(&ecs, cubeAB);
    hgEcsGet<HgTransform>(&ecs, cubeAB)->scale = HgVec3{.3, .3, .3};
    hgEcsGet<HgTransform>(&ecs, cubeAB)->position = HgVec3{-.5, 0, 0};
    *hgEcsAdd<HgModel>(&ecs, cubeAB) = {HgGpuMeshHandle{}, HgGpuTextureHandle{}, HgGpuTextureHandle{}};
    // *hgEcsAdd<Spin>(&ecs, cubeAB) = {1.0f};

    hgNodeAddChild(&ecs, cubeA, cubeAA);
    hgNodeAddChild(&ecs, cubeA, cubeAB);

    hgNodeAddChild(&ecs, root, cubeA);
    // hgNodeAddChild(&ecs, root, cube);
    // hgNodeAddChild(&ecs, root, square);
    hgNodeAddChild(&ecs, root, pointLight);
    hgNodeAddChild(&ecs, root, skybox);
    hgNodeAddChild(&ecs, root, camera);
}

void deinit()
{
    hgEcsReset(&ecs);

    hgModelsDeinit();
    hgSpritesDeinit();
    hgSkyboxDeinit();

    hgGpuImageDestroy(depthImage);
    hgGpuViewDestroy(depthView);

    hgGpuImageDestroy(renderImage);
    hgGpuViewDestroy(renderView);
    hgGpuSamplerDestroy(renderSampler);
    hgImGuiTextureDestroy(renderImGuiTex);

    hgImGuiDeinit();
    ImGui::DestroyContext();

    hgWindowDestroy(window);
}

void setupDockspace()
{
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
}

void drawMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {

            if (ImGui::MenuItem("Quit"))
                quit = true;

            if (ImGui::MenuItem("Trigger Trap"))
                abort();

            if (ImGui::MenuItem("Save Screenshot"))
            {
                void* pixels = hgAlloc(hgScratch(), width * height * 4, 4);

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
}

template<typename C, typename... Reqs>
void addComponent(HgEntity e, const char* name)
{
    if (!hgEcsHas<C>(&ecs, e) && ImGui::MenuItem(name))
    {
        ([&] {
            if (!hgEcsHas<Reqs>(&ecs, e))
                hgEcsAdd<Reqs>(&ecs, e);
        }(), ...);
        hgEcsAdd<C>(&ecs, e);
    }
}

template<typename C, typename... Deps>
void removeComponent(HgEntity e, const char* name)
{
    if (hgEcsHas<C>(&ecs, e) && ImGui::MenuItem(name))
    {
        hgEcsRemove<C>(&ecs, e);
        ([&] {
            if (hgEcsHas<Deps>(&ecs, e))
                hgEcsRemove<Deps>(&ecs, e);
        }(), ...);
    }
}

void drawEditorEntity(HgArena* frame, HgEntity e)
{
    char* name = nullptr;
    if (hgEcsHas<Name>(&ecs, e))
    {
        name = hgCString(frame, hgEcsGet<Name>(&ecs, e)->name);
    }
    else
    {
        HgString nameStr = hgStringCopy(frame, "ID ");
        hgStringAppend(frame, &nameStr, hgIntegerToString(frame, (i64)hgHandleIdx(e.handle)));
        name = hgCString(frame, nameStr);
    }

    HgNode* node = hgEcsGet<HgNode>(&ecs, e);

    if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen))
    {
        hgDefer(ImGui::TreePop());

        if (ImGui::BeginPopupContextItem())
        {
            hgDefer(ImGui::EndPopup());

            if (ImGui::MenuItem("Destroy"))
            {
                // hgEcsDespawn(&ecs, e);
                hgNodeDestroy(&ecs, e);
                return;
            }

            if (ImGui::MenuItem("Add Child"))
            {
                HgEntity child = hgEcsSpawn(&ecs);
                *hgEcsAdd<HgNode>(&ecs, child) = {};
                hgNodeAddChild(&ecs, e, child);
            }

            if (ImGui::BeginMenu("Add Component"))
            {
                addComponent<HgTransform>(e, "Transform");
                addComponent<HgSprite, HgTransform>(e, "Sprite 2D");
                addComponent<HgModel, HgTransform>(e, "Model 3D");
                addComponent<HgDirLight>(e, "Directional Light");
                addComponent<HgPointLight, HgTransform>(e, "Point Light");
                addComponent<Spin>(e, "Spin");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Remove Component"))
            {
                ImGui::SeparatorText("Components");

                removeComponent<HgTransform, HgSprite, HgModel, HgPointLight>(e, "Transform");
                removeComponent<HgSprite>(e, "Sprite 2D");
                removeComponent<HgModel>(e, "Model 3D");
                removeComponent<HgDirLight>(e, "Direction Light 3D");
                removeComponent<HgPointLight>(e, "Point Light 3D");
                removeComponent<Spin>(e, "Spin");

                ImGui::EndPopup();
            }
        }

        ImGuiTreeNodeFlags componentFlags = ImGuiTreeNodeFlags_DefaultOpen;

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

        if (hgEcsHas<Spin>(&ecs, e) && ImGui::TreeNodeEx("Spin", componentFlags))
        {
            ImGui::DragFloat("Speed", &hgEcsGet<Spin>(&ecs, e)->speed, 0.1f);
            ImGui::TreePop();
        }

        HgEntity child = node->firstChild;
        while (!hgHandleIsNull(child.handle))
        {
            HgEntity next = hgEcsGet<HgNode>(&ecs, child)->nextSibling;
            drawEditorEntity(frame, child);
            child = next;
        }
    }
}

void drawEditor(HgArena* frame)
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

        if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
        {
            drawEditorEntity(frame, root);
        }
    }
    ImGui::End();
}

void drawRender()
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
            hgGpuViewDestroy(renderView);

            hgGpuImageDestroy(depthImage);
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

        ImGui::Image((ImTextureID)renderImGuiTex, {(f32)width, (f32)height});

        renderHovered = ImGui::IsWindowFocused();
    }
    ImGui::End();
}

void render()
{
    HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
    if (hgWindowImageView(window) != nullptr)
    {
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
    hgGpuFrameEnd(cmd);
}

void drawUI(HgArena* frame)
{
    hgImGuiNewFrame();
    ImGui::NewFrame();

    setupDockspace();

    drawMenu();

    if (showEditor)
        drawEditor(frame);

    if (showRender)
        drawRender();
    else
        renderHovered = false;

    if (showImguiDemo)
        ImGui::ShowDemoWindow(&showImguiDemo);

    ImGui::Render();
}


int main()
{
    hgDefer(hgDebug("Exited successfully\n"));

    hgInit(nullptr);
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    HgArenaScope arenaScope{arena};

    init(arena);
    hgDefer(deinit());

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        delta = hgClockTick(&gameClock);
        HgClock cpuClock;
        hgClockTick(&cpuClock);

        HgArena* frame = hgScratch(&arena, 1);
        HgArenaScope frameScope{frame};

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        hgEcsForPar<Spin, HgTransform>(&ecs, [&](HgEntity e, Spin* spin, HgTransform*)
        {
            // tf->rotation = hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta * spin->speed) * tf->rotation;
            // hgTransformMove(&ecs, e, {}, {}, hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta * spin->speed));

            // hgTransformMove(&ecs, e, HgVec3{0.05f * (f32)delta, 0, 0});

            // hgTransformMove(&ecs, e, HgVec3{}, HgVec3{1.0f + 0.1f * (f32)delta, 1, 1});

            hgTransformMove(
                &ecs,
                e,
                HgVec3{},
                HgVec3{1},
                hgQuatAxisAngle(HgVec3{0, -1, 0}, (f32)delta * spin->speed));
        });

        if (renderHovered)
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

        drawUI(frame);

        cpuDelta = hgClockTick(&cpuClock);
        render();
    }
quit:
    hgGpuWaitIdle();
}

