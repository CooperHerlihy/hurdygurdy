#include "hurdygurdy.hpp"

#include <random>

#define IM_ASSERT hgAssert
#include "imgui.h"

VkSampler noiseSampler = nullptr;
HgResource noiseTexID = hgResourceID("noiseTexID");

static u32 noiseWidth = 512;
static u32 noiseHeight = 512;
static u32 noiseTile = 0;

void createNoiseTex()
{
    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    HgFence fence;
    HgResource shaderID = hgResourceID("build/noise.comp.spv");
    hgLoadResource(&fence, 1, shaderID, "build/noise.comp.spv");
    hgDefer(hgUnloadResource(nullptr, 0, shaderID));

    HgImage* image = hgCreateImage(
        noiseWidth,
        noiseHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    HgImageView* view = hgCreateImageView(
        image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
    hgDefer(hgDestroyDescriptor(desc));

    VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
    hgUpdateDescriptor(desc, nullptr, &imageInfo);

    struct ComputePush
    {
        u32 width;
        u32 height;
        u32 tile;
        u32 seed;
        u32 outImageIdx;
    };
    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, sizeof(ComputePush)};
    VkPipelineLayout layout = hgCreatePipelineLayout(&pushRange);
    hgDefer(vkDestroyPipelineLayout(hgVkDevice, layout, nullptr));

    hgWaitForFenceIndefinite(&fence);
    HgBinary* shaderCode = hgGetResource(shaderID);
    VkPipeline pipeline = hgCreateComputePipeline(
        layout, (u8*)shaderCode->data, shaderCode->size);
    hgDefer(vkDestroyPipeline(hgVkDevice, pipeline, nullptr));

    VkCommandBuffer cmd = hgBeginVkCmd();

    HgRenderer renderer = renderer.create(arena, 4, 4);

    HgRenderPass computePass{};
    computePass.storageImages = &view;
    computePass.storageImageCount = 1;
    renderer.prepareResources(cmd, &computePass);

    hgBindComputePipeline(cmd, pipeline, layout);

    ComputePush push{};
    push.width = noiseWidth;
    push.height = noiseHeight;
    push.tile = noiseTile;
    push.seed = std::random_device{}();
    push.outImageIdx = hgDescriptorIdx(desc);
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

    vkCmdDispatch(cmd, noiseWidth / 16, noiseHeight / 16, 1);

    HgRenderPass sampledPass{};
    sampledPass.sampledImages = &view;
    sampledPass.sampledImageCount = 1;
    renderer.prepareResources(cmd, &sampledPass);

    hgEndVkCmd(cmd);

    hgLoadEmptyTexture(noiseTexID);
    HgTextureResource* noiseTex = hgGetTexture(noiseTexID);;
    noiseTex->image = image;
    noiseTex->view = view;
    noiseTex->sampler = noiseSampler;
    noiseTex->descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo noiseTexInfo{noiseTex->sampler, noiseTex->view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    hgUpdateDescriptor(noiseTex->descriptor, nullptr, &noiseTexInfo);
}

int main()
{
    hgInit();
    hgDefer(hgExit());

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

    noiseSampler = hgCreateSampler(VK_FILTER_LINEAR);
    hgDefer(vkDestroySampler(hgVkDevice, noiseSampler, nullptr));

    createNoiseTex();
    hgDefer(hgUnloadTexture(noiseTexID));

    HgTransform camera{};
    camera.position.z = -1;

    HgECS ecs = ecs.create(4096);
    hgDefer(ecs.destroy());

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

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Options");

        if (ImGui::Button("Redraw"))
        {
            vkQueueWaitIdle(hgVkQueue);

            hgUnloadTexture(noiseTexID);
            createNoiseTex();
        }

        ImGui::DragInt("Width", (int*)&noiseWidth);
        ImGui::DragInt("Height", (int*)&noiseHeight);
        ImGui::DragInt("Tile", (int*)&noiseTile);

        ImGui::End();

        ImGui::Render();

        VkCommandBuffer cmd = window->beginRecording();
        if (cmd != nullptr)
        {
            HgRenderer renderer = renderer.create(frame, 32, 32);

            HgRenderAttachment colorAttachment{};
            colorAttachment.image = &window->views[window->currentImage];
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

            HgRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

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

