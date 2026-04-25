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

    HgWindow* window = hgWindowCreate("Hg Small Test", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    HgWindow* window2 = hgWindowCreate("Hg Small Test", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    hgInitPipeline2D(hgWindowFormat(window), HgFormat_undefined);
    hgDefer(hgDeinitPipeline2D());

    HgResource noiseShaderID = hgResourceID("build/noise.comp.spv");
    HgFence fence;
    hgLoadResource(&fence, noiseShaderID, "build/noise.comp.spv");

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

    hgFenceWaitIndefinite(&fence);
    HgBinary* noiseShaderCode = hgGetResource(noiseShaderID);
    HgGpuPipeline* noisePipeline = hgGpuPipelineCreateCompute(sizeof(NoisePush), (u8*)noiseShaderCode->data, noiseShaderCode->size);
    hgDefer(hgGpuPipelineDestroy(noisePipeline));

    hgUnloadResource(nullptr, noiseShaderID);

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    HgResource noiseTexID = hgResourceID("noiseTex");
    hgLoadEmptyTexture(noiseTexID);
    hgDefer(hgUnloadTexture(noiseTexID));

    HgTextureResource* noiseTex = hgGetTexture(noiseTexID);

    noiseTex->image = hgGpuImageCreate(
        noiseWidth,
        noiseHeight,
        HgFormat_r8g8b8a8_unorm,
        HgGpuImageUsage_storage | HgGpuImageUsage_sampled);
    noiseTex->view = hgGpuViewCreate(noiseTex->image, 0, 1, 0, 1, HgGpuAspect_color);

    HgGpuDescriptor noiseStorageDesc = hgGpuDescriptorCreate(HgGpuDescriptorType_storageImage);
    hgDefer(hgGpuDescriptorDestroy(noiseStorageDesc));

    HgGpuImageDescriptorInfo noiseStorageInfo{nullptr, noiseTex->view, HgGpuLayout_general};
    hgGpuDescriptorUpdate(noiseStorageDesc, nullptr, &noiseStorageInfo);

    noiseTex->sampler = hgGpuSamplerCreate(HgGpuFilter_linear);
    hgDefer(hgGpuSamplerDestroy(noiseTex->sampler));

    noiseTex->descriptor = hgGpuDescriptorCreate(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo noiseSamplerInfo{noiseTex->sampler, noiseTex->view, HgGpuLayout_shaderReadOnly};
    hgGpuDescriptorUpdate(noiseTex->descriptor, nullptr, &noiseSamplerInfo);

    HgTransform camera{};
    camera.position.z = -1;

    HgECS ecs = ecs.create(arena, 128, 128);
    ecs.createComponent<HgTransform>(arena, 128);
    ecs.createComponent<HgSprite2D>(arena, 128);

    HgEntity noiseSquare = ecs.spawn();
    ecs.add<HgTransform>(noiseSquare) = {};
    ecs.add<HgSprite2D>(noiseSquare) = {noiseTexID, HgVec2{0}, HgVec2{1}};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    HgFormat windowFormat = hgWindowFormat(window);
    hgImGuiInit(window, &windowFormat, 1);
    hgDefer(hgImGuiDeinit());

    HgClock gameClock{};
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        HgArena* frame = hgScratch();
        HgArenaScope frameScope{frame};

        hgProcessEvents();
        if (hgWasQuit())
            goto quit;

        HgMat4 proj = hgMatPerspective(
            (f32)hgPi * 0.5f,
            (f32)hgWindowWidth(window) /
            (f32)hgWindowHeight(window),
            0.1f,
            1000.0f);
        hgUpdateProjection2D(&proj);

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (hgIsKeyDown(HgKey_lmouse))
            {
                f32 rotSpeed = 2.0f;
                f32 dx = hgGetMouseDeltaX();
                f32 dy = hgGetMouseDeltaY();
                HgQuat rotX = hgQuatAxisAngle(HgVec3{0, 1, 0}, dx * rotSpeed / (f32)hgWindowHeight(window));
                HgQuat rotY = hgQuatAxisAngle(HgVec3{-1, 0, 0}, dy * rotSpeed / (f32)hgWindowHeight(window));
                camera.rotation = rotX * camera.rotation * rotY;
            }

            HgVec3 movement = HgVec3{
                (f32)(hgIsKeyDown(HgKey_d) - hgIsKeyDown(HgKey_a)),
                (f32)(hgIsKeyDown(HgKey_lshift) - hgIsKeyDown(HgKey_space)),
                (f32)(hgIsKeyDown(HgKey_w) - hgIsKeyDown(HgKey_s)),
            };
            if (movement != HgVec3{0.0f})
            {
                f32 moveSpeed = 1.5f;
                HgVec3 rotated = hgVecRotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                camera.position += hgVecNorm3(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
            }

            HgMat4 view = hgMatView(camera.position, camera.scale, camera.rotation);
            hgUpdateView2D(&view);
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

        HgWindow* windows[] = {window, window2};
        HgGpuCmd* cmd = hgGpuFrameBegin(windows, 2);

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

        if (hgWindowCurrentImage(window) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowCurrentImage(window);
            colorAttachment.loadOp = HgGpuLoadOp_clear;

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.sampledImages = &noiseTex->view;
            pass.sampledImageCount = 1;

            hgGpuRenderPassBegin(cmd, hgWindowWidth(window), hgWindowHeight(window), &pass);

            hgDraw2D(&ecs, cmd);

            hgImGuiDraw(cmd);

            hgGpuRenderPassEnd(cmd);

            HgGpuImageBarrier presentBarrier{};
            presentBarrier.image = hgWindowCurrentImage(window);
            presentBarrier.nextLayout = HgGpuLayout_presentSrc;

            hgGpuMemoryBarrier(cmd, nullptr, 0, &presentBarrier, 1);
        }

        if (hgWindowCurrentImage(window2) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowCurrentImage(window2);
            colorAttachment.loadOp = HgGpuLoadOp_clear;

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.sampledImages = &noiseTex->view;
            pass.sampledImageCount = 1;

            hgGpuRenderPassBegin(cmd, hgWindowWidth(window2), hgWindowHeight(window2), &pass);

            hgDraw2D(&ecs, cmd);

            hgImGuiDraw(cmd);

            hgGpuRenderPassEnd(cmd);

            HgGpuImageBarrier presentBarrier{};
            presentBarrier.image = hgWindowCurrentImage(window2);
            presentBarrier.nextLayout = HgGpuLayout_presentSrc;

            hgGpuMemoryBarrier(cmd, nullptr, 0, &presentBarrier, 1);
        }

        hgGpuFrameEnd(cmd);
    }

quit:
    hgGpuWaitIdle();
}

