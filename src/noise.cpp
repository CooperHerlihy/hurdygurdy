#include "hurdygurdy.hpp"

using namespace hg;

#include <random>

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

#include "noise.comp.spv.h"

int main()
{
    if (!init())
        HG_PANIC("Could not initialize Hurdy Gurdy\n");
    HG_DEFER(deinit());

    test();

    Arena* arena = getScratch();
    HG_ARENA_SCOPE(arena);

    Window* window = windowCreate("Hg Noise Test", 1200, 800, nullptr);
    if (window == nullptr)
        HG_PANIC("Could not create window\n");
    HG_DEFER(windowDestroy(window));

    u32 width = 0;
    u32 height = 0;

    GpuImage* depthImage = nullptr;
    GpuView* depthView = nullptr;
    HG_DEFER(gpuImageDestroy(depthImage));
    HG_DEFER(gpuViewDestroy(depthView));

    spritesInit(windowImageFormat(window), Format_d32_sfloat);
    HG_DEFER(spritesDeinit());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    HG_DEFER(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    imGuiInit(window, windowImageFormat(window), Format_d32_sfloat);
    HG_DEFER(imGuiDeinit());

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    TextureAsset* noiseTex = assetCreate<Texture>();
    HG_DEFER(assetUnload(noiseTex));

    noiseTex->asset.image = gpuImageCreate(
        noiseWidth,
        noiseHeight,
        Format_r8g8b8a8_unorm,
        GpuImageUsage_storage | GpuImageUsage_sampled);

    noiseTex->asset.view = gpuViewCreate(noiseTex->asset.image, GpuAspect_color, GpuFilter_nearest);

    u32 noiseSeed = std::random_device{}();
    u32 noiseScaleBegin = 4;
    u32 noiseScaleEnd = 64;
    u32 noiseTiling = 0;

    struct NoisePush {
        u32 width;
        u32 height;
        u32 scaleBegin;
        u32 scaleEnd;
        u32 tiling;
        u32 seed;
        u32 outImageIdx;
    };

    GpuPipeline* noisePipeline =
        gpuPipelineCreateCompute(sizeof(NoisePush), noise_comp_spv, sizeof(noise_comp_spv));
    HG_DEFER(gpuPipelineDestroy(noisePipeline));

    Ecs ecs = ecsCreate();
    HG_DEFER(ecsDestroy(&ecs));

    HG_ECS_REGISTER_TYPE(&ecs, Camera);
    HG_ECS_REGISTER_TYPE(&ecs, Transform);
    HG_ECS_REGISTER_TYPE(&ecs, Sprite);

    Entity camera = ecsSpawn(&ecs);
    Camera* cameraC = cameraAdd(&ecs, camera);
    Transform* cameraTf = transformAdd(&ecs, camera, {0, 0, -1});
    cameraC->type = CameraType_perspective;
    cameraC->perspective.fov = (f32)HG_PI * 0.5f;
    cameraC->perspective.near = 0.1f;
    cameraC->perspective.far = 1000.0f;

    Entity noiseSquare = ecsSpawn(&ecs);
    transformAdd(&ecs, noiseSquare);
    spriteAdd(&ecs, noiseSquare, assetCopy(noiseTex));

    Clock gameClock;
    clockTick(&gameClock);
    for (;;)
    {
        f64 delta = clockTick(&gameClock);

        processEvents();
        if (wasQuit() || windowWasClosed(window))
            goto quit;

        if (width != windowWidth(window) || height != windowHeight(window))
        {
            width = windowWidth(window);
            height = windowHeight(window);

            gpuWaitIdle();
            gpuViewDestroy(depthView);
            gpuImageDestroy(depthImage);

            depthImage = gpuImageCreate(width, height, Format_d32_sfloat, GpuImageUsage_depthStencilAttachment);
            depthView = gpuViewCreate(depthImage, GpuAspect_depth);

            cameraC->perspective.aspect = (f32)width / (f32)height;
        }

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (isButtonDown(window, Button_lmouse))
            {
                f32 rotSpeed = 2.0f;
                Quat rotX = quatAxisAngle({ 0, 1, 0}, mouseDeltaX(window) * rotSpeed);
                Quat rotY = quatAxisAngle({-1, 0, 0}, mouseDeltaY(window) * rotSpeed);
                cameraTf->rotation = rotX * cameraTf->rotation * rotY;
            }

            Vec3 movement = {
                (f32)(isButtonDown(window, Button_d) - isButtonDown(window, Button_a)),
                (f32)(isButtonDown(window, Button_lshift) - isButtonDown(window, Button_space)),
                (f32)(isButtonDown(window, Button_w) - isButtonDown(window, Button_s)),
            };
            if (movement != Vec3{0.0f})
            {
                f32 moveSpeed = 1.5f;
                Vec3 rotated = vecRot3(cameraTf->rotation, {movement.x, 0.0f, movement.z});
                cameraTf->position += vecNorm3({rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
            }

            transformUpdate(&ecs, camera);
        }

        imGuiNewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Options"))
        {
            if (ImGui::Button("New Seed"))
            {
                noiseSeed = std::random_device{}();
            }
            ImGui::DragInt("ScaleBegin", (int*)&noiseScaleBegin, 1, 1, (int)noiseScaleEnd - 1);
            ImGui::DragInt("ScaleEnd", (int*)&noiseScaleEnd, 1, (int)noiseScaleBegin + 1, (int)noiseWidth);
            ImGui::DragInt("Tiling", (int*)&noiseTiling, 1, 0, (int)noiseWidth);
        }
        ImGui::End();

        ImGui::Render();

        GpuCmd* cmd = gpuFrameBegin(&window, 1);

        GpuComputePass computePass{};
        computePass.storageImages = noiseTex->asset.view;
        computePass.storageImageCount = 1;

        gpuComputePass(cmd, &computePass);

        gpuBindPipeline(cmd, noisePipeline);

        NoisePush noisePush{};
        noisePush.width = noiseWidth;
        noisePush.height = noiseHeight;
        noisePush.scaleBegin = noiseScaleBegin;
        noisePush.scaleEnd = noiseScaleEnd;
        noisePush.tiling = noiseTiling;
        noisePush.seed = noiseSeed;
        noisePush.outImageIdx = gpuImageStorageDescriptor(noiseTex->asset.view);

        gpuPushConstants(cmd, noisePipeline, &noisePush, sizeof(noisePush));

        gpuDispatch(cmd, noiseWidth / 16, noiseHeight / 16, 1);

        if (windowImageView(window) != nullptr)
        {
            GpuRenderAttachment colorAttachment{};
            colorAttachment.image = windowImageView(window);

            GpuRenderAttachment depthAttachment{};
            depthAttachment.image = depthView;
            depthAttachment.clearValue.depthStencil.depth = 1.0f;

            GpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.depthAttachment = &depthAttachment;
            pass.sampledImages = noiseTex->asset.view;
            pass.sampledImageCount = 1;

            gpuRenderPassBegin(cmd, &pass);

            cameraUpdateEcs(&ecs, camera);
            spritesDraw(&ecs, camera, cmd);

            imGuiDraw(cmd);

            gpuRenderPassEnd(cmd);
        }

        gpuFrameEnd(cmd);
    }

quit:
    gpuWaitIdle();
}

