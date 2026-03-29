#include "hurdygurdy.hpp"

#include <random>

#define IM_ASSERT hgAssert
#include "imgui.h"

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    HgCreateWindow windowConfig{};
    windowConfig.title = "Hg Small Test";
    windowConfig.width = 1200;
    windowConfig.height = 800;

    HgWindow* window = hgCreateWindow(&windowConfig);
    hgDefer(hgDestroyWindow(window));

    HgFormat windowFormat;
    hgGetWindowSize(window, nullptr, nullptr, &windowFormat);
    hgInitPipeline2D(windowFormat, HgFormat_undefined);
    hgDefer(hgDeinitPipeline2D());

    HgResource noiseShaderID = hgResourceID("build/noise.comp.spv");
    HgFence fence;
    hgLoadResource(&fence, 1, noiseShaderID, "build/noise.comp.spv");

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

    hgWaitForFenceIndefinite(&fence);
    HgBinary* noiseShaderCode = hgGetResource(noiseShaderID);
    HgGpuPipeline* noisePipeline = hgCreateGpuComputePipeline(sizeof(NoisePush), (u8*)noiseShaderCode->data, noiseShaderCode->size);
    hgDefer(hgDestroyGpuPipeline(noisePipeline));

    hgUnloadResource(nullptr, 0, noiseShaderID);

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    HgResource noiseTexID = hgResourceID("noiseTex");
    hgLoadEmptyTexture(noiseTexID);
    hgDefer(hgUnloadTexture(noiseTexID));

    HgTextureResource* noiseTex = hgGetTexture(noiseTexID);

    noiseTex->image = hgCreateGpuImage(
        noiseWidth,
        noiseHeight,
        HgFormat_r8g8b8a8_unorm,
        HgGpuImageUsage_storage | HgGpuImageUsage_sampled);
    noiseTex->view = hgCreateGpuView(noiseTex->image, {HgGpuAspect_color, 0, 1, 0, 1});

    HgGpuDescriptor noiseStorageDesc = hgCreateGpuDescriptor(HgGpuDescriptorType_storageImage);
    hgDefer(hgDestroyGpuDescriptor(noiseStorageDesc));

    HgGpuImageDescriptorInfo noiseStorageInfo{nullptr, noiseTex->view, HgGpuLayout_general};
    hgUpdateGpuDescriptor(noiseStorageDesc, nullptr, &noiseStorageInfo);

    noiseTex->sampler = hgCreateGpuSampler(HgGpuFilter_linear);
    hgDefer(hgDestroyGpuSampler(noiseTex->sampler));

    noiseTex->descriptor = hgCreateGpuDescriptor(HgGpuDescriptorType_combinedImageSampler);

    HgGpuImageDescriptorInfo noiseSamplerInfo{noiseTex->sampler, noiseTex->view, HgGpuLayout_shaderReadOnly};
    hgUpdateGpuDescriptor(noiseTex->descriptor, nullptr, &noiseSamplerInfo);

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

    ImGui_ImplHurdyGurdy_Init(window, 1, &windowFormat);
    hgDefer(ImGui_ImplHurdyGurdy_Shutdown());

    HgClock gameClock{};
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        HgArena* frame = hgGetScratch();
        HgArenaScope frameScope{frame};

        hgProcessEvents();
        if (hgWasQuit())
            goto quit;

        u32 windowWidth, windowHeight;
        hgGetWindowSize(window, &windowWidth, &windowHeight, nullptr);

        HgMat4 proj = hgPerspective((f32)hgPi * 0.5f, (f32)windowWidth / (f32)windowHeight, 0.1f, 1000.0f);
        hgUpdateProjection2D(&proj);

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (hgIsKeyDown(HgKey_lmouse))
            {
                f32 dx, dy;
                hgGetMouseDelta(&dx, &dy);

                f32 rotSpeed = 2.0f;
                HgQuat rotX = hgAxisAngle(HgVec3{0, 1, 0}, dx * rotSpeed / (f32)windowHeight);
                HgQuat rotY = hgAxisAngle(HgVec3{-1, 0, 0}, dy * rotSpeed / (f32)windowHeight);

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
                HgVec3 rotated = hgRotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                camera.position += hgNorm(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
            }

            HgMat4 view = hgViewMatrix(camera.position, camera.scale, camera.rotation);
            hgUpdateView2D(&view);
        }

        ImGui_ImplHurdyGurdy_NewFrame();
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

        HgGpuCommands* cmd = hgWindowBeginRecording(window);
        if (cmd != nullptr)
        {
            HgRenderer renderer = renderer.create(frame, 32, 32);

            HgRenderPass computePass{};
            computePass.storageImages = &noiseTex->view;
            computePass.storageImageCount = 1;

            renderer.prepareResources(cmd, &computePass);

            hgBindGpuPipeline(cmd, noisePipeline);

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

            HgRenderAttachment colorAttachment{};
            colorAttachment.image = hgGetCurrentWindowImage(window);
            colorAttachment.loadOp = HgAttachmentLoadOp_CLEAR;

            HgRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.sampledImages = &noiseTex->view;
            pass.sampledImageCount = 1;

            renderer.beginPass(cmd, windowWidth, windowHeight, &pass);

            hgDraw2D(&ecs, cmd);

            ImGui_ImplHurdyGurdy_Draw(cmd);

            renderer.endPass(cmd);

            HgImageBarrier presentBarrier{};
            presentBarrier.image = hgGetCurrentWindowImage(window);
            presentBarrier.nextLayout = HgGpuLayout_presentSrc;

            renderer.barrier(cmd, nullptr, 0, &presentBarrier, 1);

            hgWindowEndAndPresent(window, cmd);
        }
    }

quit:
    hgGraphicsWaitIdle();
}

