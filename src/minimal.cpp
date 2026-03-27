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

    HgWindowConfig windowConfig{};
    windowConfig.title = "Hg Small Test";
    windowConfig.windowed = true;
    windowConfig.width = 1200;
    windowConfig.height = 800;

    HgWindow* window = HgWindow::create(arena, &windowConfig);
    hgDefer(window->destroy());

    hgInitPipeline2D(window->format, VK_FORMAT_UNDEFINED);
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
    VkPushConstantRange noisePushRange{VK_SHADER_STAGE_ALL, 0, sizeof(NoisePush)};
    VkPipelineLayout noiseLayout = hgCreatePipelineLayout(&noisePushRange);
    hgDefer(vkDestroyPipelineLayout(hgVkDevice, noiseLayout, nullptr));

    hgWaitForFenceIndefinite(&fence);
    HgBinary* noiseShaderCode = hgGetResource(noiseShaderID);
    VkPipeline noisePipeline = hgCreateComputePipeline(noiseLayout, (u8*)noiseShaderCode->data, noiseShaderCode->size);
    hgDefer(vkDestroyPipeline(hgVkDevice, noisePipeline, nullptr));

    hgUnloadResource(nullptr, 0, noiseShaderID);

    u32 noiseWidth = 256;
    u32 noiseHeight = 256;

    HgResource noiseTexID = hgResourceID("noiseTex");
    hgLoadEmptyTexture(noiseTexID);
    hgDefer(hgUnloadTexture(noiseTexID));

    HgTextureResource* noiseTex = hgGetTexture(noiseTexID);

    noiseTex->image = hgCreateImage(
        noiseWidth,
        noiseHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    noiseTex->view = hgCreateImageView(noiseTex->image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    HgDescriptor noiseStorageDesc = hgCreateDescriptor(HgDescriptorType_storageImage);
    hgDefer(hgDestroyDescriptor(noiseStorageDesc));

    VkDescriptorImageInfo noiseStorageInfo{nullptr, noiseTex->view->view, VK_IMAGE_LAYOUT_GENERAL};
    hgUpdateDescriptor(noiseStorageDesc, nullptr, &noiseStorageInfo);

    noiseTex->sampler = hgCreateSampler(VK_FILTER_LINEAR);
    hgDefer(vkDestroySampler(hgVkDevice, noiseTex->sampler, nullptr));

    noiseTex->descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo noiseSamplerInfo{noiseTex->sampler, noiseTex->view->view, VK_IMAGE_LAYOUT_GENERAL};
    hgUpdateDescriptor(noiseTex->descriptor, nullptr, &noiseSamplerInfo);

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

    ImGui_ImplHurdyGurdy_Init(window, 1, &window->format);
    hgDefer(ImGui_ImplHurdyGurdy_Shutdown());

    HgClock gameClock{};
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        HgArena* frame = hgGetScratch();
        HgArenaScope frameScope{frame};

        hgProcessWindowEvents(&window, 1);
        if (window->wasClosed)
            goto quit;

        HgMat4 proj = hgPerspective((f32)hgPi * 0.5f, (f32)window->width / (f32)window->height, 0.1f, 1000.0f);
        hgUpdateProjection2D(&proj);

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (window->isKeyDown[HgKey_lmouse])
            {
                f32 rotSpeed = 2.0f;
                HgQuat rotX = hgAxisAngle(HgVec3{0, 1, 0}, (f32)window->mouseDeltaX * rotSpeed);
                HgQuat rotY = hgAxisAngle(HgVec3{-1, 0, 0}, (f32)window->mouseDeltaY * rotSpeed);
                camera.rotation = rotX * camera.rotation * rotY;
            }

            HgVec3 movement = HgVec3{
                (f32)(window->isKeyDown[HgKey_d] - window->isKeyDown[HgKey_a]),
                (f32)(window->isKeyDown[HgKey_lshift] - window->isKeyDown[HgKey_space]),
                (f32)(window->isKeyDown[HgKey_w] - window->isKeyDown[HgKey_s]),
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

        VkCommandBuffer cmd = window->beginRecording();
        if (cmd != nullptr)
        {
            HgRenderer renderer = renderer.create(frame, 32, 32);

            HgRenderPass computePass{};
            computePass.storageImages = &noiseTex->view;
            computePass.storageImageCount = 1;

            renderer.prepareResources(cmd, &computePass);

            hgBindComputePipeline(cmd, noisePipeline, noiseLayout);

            NoisePush noisePush{};
            noisePush.width = noiseWidth;
            noisePush.height = noiseHeight;
            noisePush.scaleBegin = noiseScaleBegin;
            noisePush.scaleEnd = noiseScaleEnd;
            noisePush.tiling = noiseTiling;
            noisePush.seed = noiseSeed;
            noisePush.outImageIdx = hgDescriptorIdx(noiseStorageDesc);

            vkCmdPushConstants(cmd, noiseLayout, VK_SHADER_STAGE_ALL, 0, sizeof(noisePush), &noisePush);

            vkCmdDispatch(cmd, noiseWidth / 16, noiseHeight / 16, 1);

            HgRenderAttachment colorAttachment{};
            colorAttachment.image = &window->views[window->currentImage];
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

            HgRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.sampledImages = &noiseTex->view;
            pass.sampledImageCount = 1;

            renderer.beginPass(cmd, window->width, window->height, &pass);

            hgDraw2D(&ecs, cmd);

            ImGui_ImplHurdyGurdy_Draw(cmd);

            renderer.endPass(cmd);

            HgImageBarrier presentBarrier{};
            presentBarrier.image = &window->views[window->currentImage];
            presentBarrier.nextLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            renderer.barrier(cmd, nullptr, 0, &presentBarrier, 1);

            window->endAndPresent(cmd);
        }
    }

quit:
    vkDeviceWaitIdle(hgVkDevice);
}

