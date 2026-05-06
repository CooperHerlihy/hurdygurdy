#include "hurdygurdy.hpp"

#include <random>

#define IM_ASSERT hgAssert
#include "imgui.h"

int main()
{
    hgInit(nullptr);
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    HgArenaScope arenaScope{arena};

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

    HgBinaryHandle noiseShaderHandle = hgAssetLoad<HgBinary>("build/noise.comp.spv");

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    HgGpuTextureHandle noiseTexHandle = hgAssetCreate<HgGpuTexture>(HgStringView{});
    HgGpuTexture* noiseTex = hgAssetGet(noiseTexHandle);

    noiseTex->image = hgGpuImageCreate(
        noiseWidth,
        noiseHeight,
        HgFormat_r8g8b8a8_unorm,
        HgGpuImageUsage_storage | HgGpuImageUsage_sampled);
    hgDefer(hgGpuImageDestroy(noiseTex->image));

    noiseTex->view = hgGpuViewCreate(noiseTex->image, 0, 1, 0, 1, HgGpuAspect_color);
    hgDefer(hgGpuViewDestroy(noiseTex->view));

    HgGpuDescriptor noiseStorageDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_storageImage);
    hgDefer(hgGpuDescriptorDestroy(noiseStorageDesc));

    HgGpuImageDescriptorInfo noiseStorageInfo{nullptr, noiseTex->view, HgGpuLayout_general};
    hgGpuDescriptorUpdate(noiseStorageDesc, nullptr, &noiseStorageInfo);

    noiseTex->sampler = hgGpuSamplerCreate(HgGpuFilter_linear);
    hgDefer(hgGpuSamplerDestroy(noiseTex->sampler));

    noiseTex->descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);
    hgDefer(hgGpuDescriptorDestroy(noiseTex->descriptor));

    HgGpuImageDescriptorInfo noiseSamplerInfo{noiseTex->sampler, noiseTex->view, HgGpuLayout_shaderReadOnly};
    hgGpuDescriptorUpdate(noiseTex->descriptor, nullptr, &noiseSamplerInfo);

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

    HgBinary* noiseShaderCode = hgAssetGet(noiseShaderHandle);
    HgGpuPipeline* noisePipeline = hgGpuPipelineCreateCompute(sizeof(NoisePush), (u8*)noiseShaderCode->data, noiseShaderCode->size);
    hgDefer(hgGpuPipelineDestroy(noisePipeline));

    hgAssetUnload(noiseShaderHandle);

    HgEcs ecs = hgEcsCreate(arena, 128, 128);
    hgEcsRegisterType(&ecs, arena, HgTransform, 128);
    hgEcsRegisterType(&ecs, arena, HgCamera, 8);
    hgEcsRegisterType(&ecs, arena, HgSprite, 128);

    HgEntity camera = hgEcsCreate(&ecs);

    HgTransform* cameraTf = hgEcsAdd<HgTransform>(&ecs, camera);
    *cameraTf = {};
    cameraTf->position = HgVec3{0, 0, -1};

    HgCamera* cameraC = hgEcsAdd<HgCamera>(&ecs, camera);
    *cameraC = {};
    cameraC->type = HgCameraType_perspective;
    cameraC->perspective.fov = (f32)hgPi * 0.5f;
    cameraC->perspective.near = 0.1f;
    cameraC->perspective.far = 1000.0f;
    hgCameraCreate(cameraC);
    hgDefer(hgCameraDestroy(cameraC));

    HgEntity noiseSquare = hgEcsCreate(&ecs);
    *hgEcsAdd<HgTransform>(&ecs, noiseSquare) = {};
    *hgEcsAdd<HgSprite>(&ecs, noiseSquare) = {noiseTexHandle, HgVec2{0}, HgVec2{1}};

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
            depthView = hgGpuViewCreate(depthImage, 0, 1, 0, 1, HgGpuAspect_depth);

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
        }

        hgImGuiNewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Options"))
        {
            if (ImGui::Button("New Seed"))
            {
                noiseSeed = std::random_device{}();
            }
            ImGui::DragInt("ScaleBegin", (int*)&noiseScaleBegin, 1, 1, noiseScaleEnd - 1);
            ImGui::DragInt("ScaleEnd", (int*)&noiseScaleEnd, 1, noiseScaleBegin + 1, noiseWidth);
            ImGui::DragInt("Tiling", (int*)&noiseTiling, 1, 0, noiseWidth);
        }
        ImGui::End();

        ImGui::Render();

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);

        HgGpuComputePass computePass{};
        computePass.storageImages = &noiseTex->view;
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
        noisePush.outImageIdx = hgGpuDescriptorIdx(noiseStorageDesc);

        hgGpuPushConstants(cmd, noisePipeline, 0, &noisePush, sizeof(noisePush));

        hgGpuCompute(cmd, noiseWidth / 16, noiseHeight / 16, 1);

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
            pass.sampledImages = &noiseTex->view;
            pass.sampledImageCount = 1;

            hgGpuRenderPassBegin(cmd, hgWindowWidth(window), hgWindowHeight(window), &pass);

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

