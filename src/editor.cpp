#include "hurdygurdy.hpp"

using namespace hg;

#include "stb_image_write.h"

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

#include <cstdio>
#include <emmintrin.h>

static Window* window = nullptr;

static u32 width = 0;
static u32 height = 0;
static f32 aspectRatio = 16.0f / 9.0f;

static GpuImage* renderImage;
static GpuView* renderView;
static void* renderImGuiTex;

static GpuImage* depthImage;
static GpuView* depthView;

static Ecs* ecs = nullptr;
static Entity root;

static Entity player;
static Transform* transform;
static Camera* camera;

static f32 audioData[4000];
static SoundAsset* audio;

static bool showEditor = true;
static bool showRender = true;
static bool showImguiDemo = false;
static bool fixedAspect = false;
static bool move3D = true;

static f64 delta = 0.0;

Clock cpuClock;
static f64 cpuDelta = 0.0;

volatile static bool quit = false;
static bool renderHovered = false;

struct Name {
    String name;
};

struct Spin {
    f32 speed;
};

void init(Arena* arena)
{
    WindowConfig windowConfig{};
    windowConfig.fullscreen = true;
    // windowConfig.preferredPresentMode = GpuPresentMode_mailbox;

    window = windowCreate("Hg Editor Example", 1600, 900, &windowConfig);
    if (window == nullptr)
        HG_PANIC("Could not create window\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    imGuiInit(window, windowImageFormat(window));

    audio = assetCreate<Sound>();
    audio->asset.data = audioData;
    audio->asset.size = sizeof(audioData);
    audio->asset.frequency = 8000;
    audio->asset.channels = 1;

    spritesInit(Format_r8g8b8a8_srgb, Format_d32_sfloat);
    modelsInit(Format_r8g8b8a8_srgb, Format_d32_sfloat);
    skyboxInit(Format_r8g8b8a8_srgb, Format_d32_sfloat);

    ecs = arenaAlloc<Ecs>(arena, 1);
    *ecs = ecsCreate();

    HG_ECS_REGISTER_TYPE(ecs, Node);
    HG_ECS_REGISTER_TYPE(ecs, Transform);
    HG_ECS_REGISTER_TYPE(ecs, Camera);
    HG_ECS_REGISTER_TYPE(ecs, Skybox);
    HG_ECS_REGISTER_TYPE(ecs, Sprite);
    HG_ECS_REGISTER_TYPE(ecs, DirLight);
    HG_ECS_REGISTER_TYPE(ecs, PointLight);
    HG_ECS_REGISTER_TYPE(ecs, Model);
    HG_ECS_REGISTER_TYPE(ecs, AudioSource);

    HG_ECS_REGISTER_TYPE(ecs, Name);
    HG_ECS_REGISTER_TYPE(ecs, Spin);

    root = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, root) = {"root"};
    nodeAdd(ecs, root);

    player = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, player) = {"player"};
    nodeAdd(ecs, player);
    transform = transformAdd(ecs, player, {0, 0, -1});
    camera = cameraAdd(ecs, player);

    camera->type = CameraType_perspective;
    camera->perspective.fov = (f32)HG_PI * 0.5f;
    camera->perspective.near = 0.01f;
    camera->perspective.far = 1000.0f;

    Entity skybox = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, skybox) = {"skybox"};
    nodeAdd(ecs, skybox);
    skyboxAdd(ecs, skybox, {});

    Entity pointLight = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, pointLight) = {"pointLight"};
    nodeAdd(ecs, pointLight);
    transformAdd(ecs, pointLight, {0, -2, 0});
    pointLightAdd(ecs, pointLight, {1, 1, 1, 4});

    Entity square = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, square) = {"square"};
    nodeAdd(ecs, square);
    transformAdd(ecs, square, {-1, 0, 1});
    spriteAdd(ecs, square, {});
    *ecsAdd<Spin>(ecs, square) = {1.0f};

    Entity cube = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, cube) = {"cube"};
    nodeAdd(ecs, cube);
    transformAdd(ecs, cube, {1, 0, 1});
    modelAdd(ecs, cube, {}, {}, {});
    *ecsAdd<Spin>(ecs, cube) = {1.0f};

    Entity sound = ecsSpawn(ecs);
    *ecsAdd<Name>(ecs, sound) = {"sound"};
    nodeAdd(ecs, sound);
    transformAdd(ecs, sound);
    ecsGet<Transform>(ecs, sound)->position = {0, 0, 1};
    transformUpdate(ecs, sound);
    audioSourceAdd(ecs, sound, assetCopy(audio), true);

    nodeAddChild(ecs, root, sound);
    nodeAddChild(ecs, root, cube);
    nodeAddChild(ecs, root, square);
    nodeAddChild(ecs, root, pointLight);
    nodeAddChild(ecs, root, skybox);
    nodeAddChild(ecs, root, player);
}

void deinit()
{
    ecsDestroy(ecs);

    modelsDeinit();
    spritesDeinit();
    skyboxDeinit();

    gpuImageDestroy(depthImage);
    gpuViewDestroy(depthView);

    gpuImageDestroy(renderImage);
    gpuViewDestroy(renderView);
    imGuiTextureDestroy(renderImGuiTex);

    imGuiDeinit();
    ImGui::DestroyContext();

    windowDestroy(window);
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
                void* pixels = arenaAlloc(getScratch(), width * height * 4, 4);

                gpuImageRead(pixels, renderView);
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
void addComponent(Entity e, const char* name)
{
    if (!ecsHas<C>(ecs, e) && ImGui::MenuItem(name))
    {
        ([&] {
            if (!ecsHas<Reqs>(ecs, e))
                ecsAdd<Reqs>(ecs, e);
        }(), ...);
        ecsAdd<C>(ecs, e);
    }
}

template<typename C, typename... Deps>
void removeComponent(Entity e, const char* name)
{
    if (ecsHas<C>(ecs, e) && ImGui::MenuItem(name))
    {
        ecsRemove<C>(ecs, e);
        ([&] {
            if (ecsHas<Deps>(ecs, e))
                ecsRemove<Deps>(ecs, e);
        }(), ...);
    }
}

void drawEditorEntity(Arena* frame, Entity e)
{
    char* name = nullptr;
    if (ecsHas<Name>(ecs, e))
    {
        name = cString(frame, ecsGet<Name>(ecs, e)->name);
    }
    else
    {
        StringBuilder nameStr = stringCopy(frame, "ID ");
        stringAppend(frame, &nameStr, integerToString(frame, (i64)handleIdx(e.handle)));
        name = cString(frame, nameStr);
    }

    Node* node = ecsGet<Node>(ecs, e);

    if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen))
    {
        HG_DEFER(ImGui::TreePop());

        if (ImGui::BeginPopupContextItem())
        {
            HG_DEFER(ImGui::EndPopup());

            if (ImGui::MenuItem("Destroy"))
            {
                nodeDestroy(ecs, e);
                return;
            }

            if (ImGui::MenuItem("Add Child"))
            {
                Entity child = ecsSpawn(ecs);
                *nodeAdd(ecs, child) = {};
                nodeAddChild(ecs, e, child);
            }

            if (ImGui::BeginMenu("Add Component"))
            {
                addComponent<Transform>(e, "Transform");
                addComponent<Sprite, Transform>(e, "Sprite 2D");
                addComponent<Model, Transform>(e, "Model 3D");
                addComponent<DirLight, Transform>(e, "Directional Light");
                addComponent<PointLight, Transform>(e, "Point Light");
                addComponent<Spin, Transform>(e, "Spin");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Remove Component"))
            {
                ImGui::SeparatorText("Components");

                removeComponent<Transform, Sprite, Model, PointLight>(e, "Transform");
                removeComponent<Sprite>(e, "Sprite 2D");
                removeComponent<Model>(e, "Model 3D");
                removeComponent<DirLight>(e, "Direction Light 3D");
                removeComponent<PointLight>(e, "Point Light 3D");
                removeComponent<Spin>(e, "Spin");

                ImGui::EndPopup();
            }
        }

        ImGuiTreeNodeFlags componentFlags = ImGuiTreeNodeFlags_DefaultOpen;

        if (ecsHas<Transform>(ecs, e) && ImGui::TreeNodeEx("Transform", componentFlags))
        {
            Transform* tf = ecsGet<Transform>(ecs, e);
            if (ImGui::DragFloat3("Position", &tf->position.x, 0.01f) +
                ImGui::DragFloat3("Scale", &tf->scale.x, 0.01f) +
                ImGui::DragFloat4("Rotation", &tf->rotation.r, 0.01f))
                transformUpdate(ecs, e);
            ImGui::TreePop();
        }

        if (ecsHas<Camera>(ecs, e) && ImGui::TreeNodeEx("Camera", componentFlags))
        {
            Camera* c = ecsGet<Camera>(ecs, e);
            if (c->type == CameraType_perspective)
            {
                if (ImGui::Button("Perspective"))
                {
                    c->type = CameraType_orthographic;
                    cameraSetOrthographic(c, (f32)width / (f32)height, 1.0f);
                }
                ImGui::DragFloat("FoV", &c->perspective.fov, 0.01f);
                ImGui::DragFloat("Aspect", &c->perspective.aspect, 0.01f);
                ImGui::DragFloat("Near", &c->perspective.near, 0.01f, 0.01f);
                ImGui::DragFloat("Far", &c->perspective.far, 0.01f);
            }
            else if (c->type == CameraType_orthographic)
            {
                if (ImGui::Button("Orthographic"))
                {
                    c->type = CameraType_perspective;
                    cameraSetPerspective(c, (f32)width / (f32)height);
                }
                ImGui::DragFloat2("Left, Right", &c->orthographic.left);
                ImGui::DragFloat2("Top, Bottom", &c->orthographic.top);
                ImGui::DragFloat2("Near, Far", &c->orthographic.near);
            }
            else
            {
                HG_ASSERT(false);
            }
            ImGui::TreePop();
        }

        if (ecsHas<Sprite>(ecs, e) && ImGui::TreeNodeEx("Sprite", componentFlags))
        {
            Sprite* s = ecsGet<Sprite>(ecs, e);
            ImGui::DragFloat2("UV Position", &s->uvPos.x, 0.01f);
            ImGui::DragFloat2("UV Size", &s->uvSize.x, 0.01f);
            ImGui::TreePop();
        }

        if (ecsHas<Model>(ecs, e) && ImGui::TreeNodeEx("Model 3D", componentFlags))
        {
            ImGui::TreePop();
        }

        if (ecsHas<DirLight>(ecs, e) && ImGui::TreeNodeEx("Directional Light 3D", componentFlags))
        {
            DirLight* l = ecsGet<DirLight>(ecs, e);
            ImGui::DragFloat3("Direction", &l->dir.x, 0.01f);
            ImGui::DragFloat4("Color", &l->color.x, 0.01f);
            ImGui::TreePop();
        }

        if (ecsHas<PointLight>(ecs, e) && ImGui::TreeNodeEx("Point Light 3D", componentFlags))
        {
            PointLight* l = ecsGet<PointLight>(ecs, e);
            ImGui::DragFloat4("Color", &l->color.x, 0.01f);
            ImGui::TreePop();
        }

        if (ecsHas<Spin>(ecs, e) && ImGui::TreeNodeEx("Spin", componentFlags))
        {
            ImGui::DragFloat("Speed", &ecsGet<Spin>(ecs, e)->speed, 0.1f);
            ImGui::TreePop();
        }

        Entity child = node->firstChild;
        while (child.handle != handleNull)
        {
            Entity next = ecsGet<Node>(ecs, child)->nextSibling;
            drawEditorEntity(frame, child);
            child = next;
        }
    }
}

void drawEditor(Arena* frame)
{
    if (ImGui::Begin("Editor", &showEditor))
    {
        ImGuiTreeNodeFlags optionsFlags = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::CollapsingHeader("Options", optionsFlags))
        {
            ImGui::SeparatorText("Time");
            ImGui::Text("total: %.3fms", delta * 1.0e3);
            ImGui::Text("cpu: %.3fms", cpuDelta * 1.0e3);
            cpuDelta = 0.0;

            ImGui::SeparatorText("Camera");
            if (ImGui::Button("Reset Camera"))
            {
                transform->position = {0, 0, -1};
                transform->scale = {1, 1, 1};
                transform->rotation = Quat{1, 0, 0, 0};
                transformUpdate(ecs, player);
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

        u32 viewHeight = max((u32)1, fixedAspect ? (u32)min(size.y, size.x / aspectRatio) : (u32)size.y);
        u32 viewWidth = max((u32)1, fixedAspect ? (u32)((f32)viewHeight * aspectRatio) : (u32)size.x);
        if (width != viewWidth || height != viewHeight)
        {
            width = viewWidth;
            height = viewHeight;

            gpuWaitIdle();

            imGuiTextureDestroy(renderImGuiTex);

            gpuViewDestroy(depthView);
            gpuViewDestroy(renderView);

            gpuImageDestroy(depthImage);
            gpuImageDestroy(renderImage);

            renderImage = gpuImageCreate(
                width,
                height,
                Format_r8g8b8a8_srgb,
                GpuImageUsage_colorAttachment |
                GpuImageUsage_sampled |
                GpuImageUsage_transferSrc);

            depthImage = gpuImageCreate(
                width,
                height,
                Format_d32_sfloat,
                GpuImageUsage_depthStencilAttachment);

            renderView = gpuViewCreate(renderImage, GpuAspect_color, GpuFilter_nearest);
            depthView = gpuViewCreate(depthImage, GpuAspect_depth, GpuFilter_nearest);

            renderImGuiTex = imGuiTextureCreate(renderView, GpuLayout_shaderReadOnly);

            camera->perspective.aspect = (f32)width / (f32)height;
        }

        ImGui::Image((ImTextureID)renderImGuiTex, {(f32)width, (f32)height});

        renderHovered = ImGui::IsWindowFocused();
    }
    ImGui::End();
}

void render()
{
    GpuCmd* cmd = gpuFrameBegin(&window, 1);
    clockTick(&cpuClock);
    if (windowImageView(window) != nullptr)
    {
        GpuRenderAttachment renderColorAttachment{};
        renderColorAttachment.image = renderView;
        renderColorAttachment.loadOp = GpuLoadOp_clear;
        renderColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        GpuRenderAttachment renderDepthAttachment{};
        renderDepthAttachment.image = depthView;
        renderDepthAttachment.loadOp = GpuLoadOp_clear;
        renderDepthAttachment.clearValue.depthStencil = {1.0f, 0};

        GpuRenderPass renderPass{};
        renderPass.colorAttachments = &renderColorAttachment;
        renderPass.colorAttachmentCount = 1;
        renderPass.depthAttachment = &renderDepthAttachment;

        gpuRenderPassBegin(cmd, &renderPass);

        cameraUpdateEcs(ecs, player);
        skyboxDraw(ecs, player, cmd);
        spritesDraw(ecs, player, cmd);
        modelsDraw(ecs, player, cmd);

        gpuRenderPassEnd(cmd);

        GpuRenderAttachment guiColorAttachment{};
        guiColorAttachment.image = windowImageView(window);
        guiColorAttachment.loadOp = GpuLoadOp_clear;
        guiColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        GpuRenderPass guiPass{};
        guiPass.sampledImages = renderView;
        guiPass.sampledImageCount = 1;
        guiPass.colorAttachments = &guiColorAttachment;
        guiPass.colorAttachmentCount = 1;

        gpuRenderPassBegin(cmd, &guiPass);

        imGuiDraw(cmd);

        gpuRenderPassEnd(cmd);
    }
    cpuDelta += clockTick(&cpuClock);
    gpuFrameEnd(cmd);
    clockTick(&cpuClock);
}

void drawUI(Arena* frame)
{
    imGuiNewFrame();
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
    HG_DEFER(printf("Exited successfully\n"));

    if (!hg::init())
        HG_PANIC("Could not initialize Hurdy Gurdy\n");
    HG_DEFER(hg::deinit());

    test();

    Arena* arena = getScratch();
    HG_ARENA_SCOPE(arena);

    for (u32 i = 0; i < arrayCount(audioData); ++i)
    {

        // saw harmonics
        f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
        audioData[i] = 0;
        for (u32 j = 1; j <= 64; ++j)
        {
            f32 x = (f32)j;
            audioData[i] += 1.0f / x * std::sin(100.f * t * x);
        }

        // // square harmonics
        // f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
        // audioData[i] = 0;
        // for (u32 j = 1; j <= 64; ++j)
        // {
        //     f32 x = (f32)j * 2.0f - 1.0f;
        //     audioData[i] += 1.0f / x * std::sin(100.f * t * x);
        // }

        // // square exact
        // if (i % 80 < 40)
        //     audioData[i] = 1.0f;
        // else
        //     audioData[i] = -1.0f;

    }

    ::init(arena);
    HG_DEFER(::deinit());

    // AudioPlayer audioPlayer = audioPlayerCreate(AudioFormat_f32, 8000, 1);
    // HG_DEFER(audioPlayerDestroy(audioPlayer));

    // f32 audioBase[8000];
    // for (u32 i = 0; i < 8000; ++i)
    // {
    //
    //     // // saw harmonics
    //     // f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
    //     // audioBase[i] = 0;
    //     // for (u32 j = 1; j <= 64; ++j)
    //     // {
    //     //     f32 x = (f32)j;
    //     //     audioBase[i] += 1.0f / x * std::sin(100.f * t * x);
    //     // }
    //
    //     // // square harmonics
    //     // f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
    //     // audioBase[i] = 0;
    //     // for (u32 j = 1; j <= 64; ++j)
    //     // {
    //     //     f32 x = (f32)j * 2.0f - 1.0f;
    //     //     audioBase[i] += 1.0f / x * std::sin(100.f * t * x);
    //     // }
    //
    //     // square exact
    //     if (i % 80 < 40)
    //         audioBase[i] = 1.0f;
    //     else
    //         audioBase[i] = -1.0f;
    //
    // }

    // temporary, trick the OS into thinking we're important
    callPar(nullptr, nullptr, [](void*)
    {
        while(!quit)
        {
            _mm_pause();
        }
    });

    Clock gameClock;
    clockTick(&gameClock);
    while (!quit)
    {
        delta = clockTick(&gameClock);

        Arena* frame = getScratch(&arena, 1);
        HG_ARENA_SCOPE(frame);

        processEvents();
        if (wasQuit() || windowWasClosed(window))
            quit = true;

        audioUpdate(ecs, player);

        ecsForEach<Spin, Transform>(ecs, [&](Entity e, Spin* spin, Transform* tf)
        {
            tf->rotation = quatAxisAngle({0, -1, 0}, (f32)delta * spin->speed) * tf->rotation;
            transformUpdate(ecs, e);
        });

        if (renderHovered)
        {
            if (move3D && isButtonDown(window, Button_lmouse))
            {
                f32 rotSpeed = 2.0f;
                Quat rotX = quatAxisAngle({ 0, 1, 0}, mouseDeltaX(window) * rotSpeed);
                Quat rotY = quatAxisAngle({-1, 0, 0}, mouseDeltaY(window) * rotSpeed);
                transform->rotation = rotX * transform->rotation * rotY;
            }

            Vec3 movement = Vec3{0.0f};
            if (move3D)
            {
                movement.y += (f32)(isButtonDown(window, Button_lshift) - isButtonDown(window, Button_space));
                movement.x += (f32)(isButtonDown(window, Button_d) - isButtonDown(window, Button_a));
                movement.z += (f32)(isButtonDown(window, Button_w) - isButtonDown(window, Button_s));
            } else {
                movement.y += (f32)(isButtonDown(window, Button_s) - isButtonDown(window, Button_w));
                movement.x += (f32)(isButtonDown(window, Button_d) - isButtonDown(window, Button_a));
            }

            if (movement != Vec3{0.0f})
            {
                f32 moveSpeed = 1.5f * (f32)delta;
                Vec3 rotated = vecRot3(transform->rotation, {movement.x, 0.0f, movement.z});
                transform->position += vecNorm3({rotated.x, movement.y, rotated.z}) * moveSpeed;
            }

            transformUpdate(ecs, player);
        }

        drawUI(frame);

        cpuDelta += clockTick(&cpuClock);
        render();
    }
    gpuWaitIdle();
}

