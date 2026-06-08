#include "hurdygurdy.hpp"

#include <random>

#define IM_ASSERT hgAssert
#include "imgui.h"

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    hgArenaScope(arena);

    HgWindow* window = hgWindowCreate("Hg Noise Test", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    u32 width = 0;
    u32 height = 0;

    HgGpuImage* depthImage = nullptr;
    HgGpuView* depthView = nullptr;
    hgDefer(hgGpuImageDestroy(depthImage));
    hgDefer(hgGpuViewDestroy(depthView));

    hgSpritesInit(hgWindowImageFormat(window), HgFormat_d32_sfloat);
    hgDefer(hgSpritesDeinit());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    hgImGuiInit(window, hgWindowImageFormat(window), HgFormat_d32_sfloat);
    hgDefer(hgImGuiDeinit());

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    HgGpuTextureAsset* noiseTex = hgAssetCreate<HgGpuTexture>();
    hgDefer(hgAssetUnload(noiseTex));

    noiseTex->data.image = hgGpuImageCreate(
        noiseWidth,
        noiseHeight,
        HgFormat_r8g8b8a8_unorm,
        HgGpuImageUsage_storage | HgGpuImageUsage_sampled);

    noiseTex->data.view = hgGpuViewCreate(noiseTex->data.image, HgGpuAspect_color, HgGpuFilter_nearest);

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

    HgBinaryAsset* noiseShaderCode = hgAssetLoad<HgBinary>("build/noise.comp.spv");
    HgGpuPipeline* noisePipeline = hgGpuPipelineCreateCompute(sizeof(NoisePush), (u8*)noiseShaderCode->data.data, noiseShaderCode->data.size);
    hgDefer(hgGpuPipelineDestroy(noisePipeline));

    hgAssetUnload(noiseShaderCode);

    HgEcs ecs = hgEcsCreate();
    hgDefer(hgEcsDestroy(&ecs));

    hgEcsRegisterType(&ecs, HgCamera);
    hgEcsRegisterType(&ecs, HgTransform);
    hgEcsRegisterType(&ecs, HgSprite);

    HgEntity camera = hgEcsSpawn(&ecs);
    HgCamera* cameraC = hgCameraAdd(&ecs, camera);
    HgTransform* cameraTf = hgTransformAdd(&ecs, camera, HgVec3{0, 0, -1});
    cameraC->type = HgCameraType_perspective;
    cameraC->perspective.fov = (f32)hgPi * 0.5f;
    cameraC->perspective.near = 0.1f;
    cameraC->perspective.far = 1000.0f;

    HgEntity noiseSquare = hgEcsSpawn(&ecs);
    hgTransformAdd(&ecs, noiseSquare);
    hgSpriteAdd(&ecs, noiseSquare, hgAssetCopy(noiseTex));

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        if (width != hgWindowWidth(window) || height != hgWindowHeight(window))
        {
            width = hgWindowWidth(window);
            height = hgWindowHeight(window);

            hgGpuWaitIdle();
            hgGpuViewDestroy(depthView);
            hgGpuImageDestroy(depthImage);

            depthImage = hgGpuImageCreate(width, height, HgFormat_d32_sfloat, HgGpuImageUsage_depthStencilAttachment);
            depthView = hgGpuViewCreate(depthImage, HgGpuAspect_depth);

            cameraC->perspective.aspect = (f32)width / (f32)height;
        }

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (hgIsButtonDown(window, HgButton_lmouse))
            {
                f32 rotSpeed = 2.0f;
                HgQuat rotX = hgQuatAxisAngle(HgVec3{ 0, 1, 0}, hgMouseDeltaX(window) * rotSpeed);
                HgQuat rotY = hgQuatAxisAngle(HgVec3{-1, 0, 0}, hgMouseDeltaY(window) * rotSpeed);
                cameraTf->rotation = rotX * cameraTf->rotation * rotY;
            }

            HgVec3 movement = HgVec3{
                (f32)(hgIsButtonDown(window, HgButton_d) - hgIsButtonDown(window, HgButton_a)),
                (f32)(hgIsButtonDown(window, HgButton_lshift) - hgIsButtonDown(window, HgButton_space)),
                (f32)(hgIsButtonDown(window, HgButton_w) - hgIsButtonDown(window, HgButton_s)),
            };
            if (movement != HgVec3{0.0f})
            {
                f32 moveSpeed = 1.5f;
                HgVec3 rotated = hgVecRotate(cameraTf->rotation, HgVec3{movement.x, 0.0f, movement.z});
                cameraTf->position += hgVecNorm3(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
            }

            hgTransformUpdate(&ecs, camera);
        }

        hgImGuiNewFrame();
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

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);

        HgGpuComputePass computePass{};
        computePass.storageImages = noiseTex->data.view;
        computePass.storageImageCount = 1;

        hgGpuComputePass(cmd, &computePass);

        hgGpuBindPipeline(cmd, noisePipeline);

        NoisePush noisePush{};
        noisePush.width = noiseWidth;
        noisePush.height = noiseHeight;
        noisePush.scaleBegin = noiseScaleBegin;
        noisePush.scaleEnd = noiseScaleEnd;
        noisePush.tiling = noiseTiling;
        noisePush.seed = noiseSeed;
        noisePush.outImageIdx = hgGpuImageStorageDescriptor(noiseTex->data.view);

        hgGpuPushConstants(cmd, noisePipeline, 0, &noisePush, sizeof(noisePush));

        hgGpuDispatch(cmd, noiseWidth / 16, noiseHeight / 16, 1);

        if (hgWindowImageView(window) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowImageView(window);

            HgGpuRenderAttachment depthAttachment{};
            depthAttachment.image = depthView;
            depthAttachment.clearValue.depthStencil.depth = 1.0f;

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.depthAttachment = &depthAttachment;
            pass.sampledImages = noiseTex->data.view;
            pass.sampledImageCount = 1;

            hgGpuRenderPassBegin(cmd, &pass);

            hgGpuSetViewport(cmd, 0, 0, (f32)hgWindowWidth(window), (f32)hgWindowHeight(window));
            hgGpuSetScissor(cmd, 0, 0, hgWindowWidth(window), hgWindowHeight(window));

            hgCameraUpdate(&ecs, camera);
            hgSpritesDraw(&ecs, camera, cmd);

            hgImGuiDraw(cmd);

            hgGpuRenderPassEnd(cmd);

            HgGpuImageBarrier presentBarrier{};
            presentBarrier.image = hgWindowImageView(window);
            presentBarrier.nextLayout = HgGpuLayout_presentSrc;

            hgGpuMemoryBarrier(cmd, nullptr, 0, &presentBarrier, 1);
        }

        hgGpuFrameEnd(cmd);
    }

quit:
    hgGpuWaitIdle();
}

