#include "hurdygurdy.hpp"

#define IM_ASSERT hgAssert
#include "imgui.h"

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

    HgWindow* window = HgWindow::create(arena, windowConfig);
    hgDefer(window->destroy());

    hgInitPipeline3D(arena, 256, window->format, VK_FORMAT_D32_SFLOAT);
    hgDefer(hgDeinitPipeline3D());

    hgAddModel3D(0, 0, 0);
    hgDefer(hgRemoveModel3D(0));

    HgTransform camera{};
    camera.position.z = -1;

    HgECS ecs = ecs.create(4096);
    hgDefer(ecs.destroy());

    HgEntity cube = ecs.spawn();
    ecs.add<HgTransform>(cube) = {};
    ecs.add<HgModel3D>(cube) = {0};

    HgEntity light = ecs.spawn();
    ecs.add<HgTransform>(light);
    ecs.get<HgTransform>(light).position = HgVec3{-1, -2, -1};
    ecs.add<HgPointLight3D>(light) = {HgVec4{1, 1, 1, 3}};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplHurdyGurdy_Init(window, 1, &window->format, VK_FORMAT_D32_SFLOAT);
    hgDefer(ImGui_ImplHurdyGurdy_Shutdown());

    u32 depthWidth = window->width;
    u32 depthHeight = window->height;

    HgCreateVkImage depthImageConfig{};
    depthImageConfig.width = depthWidth;
    depthImageConfig.height = depthHeight;
    depthImageConfig.format = VK_FORMAT_D32_SFLOAT;
    depthImageConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImage depthImage;
    VmaAllocation depthAlloc;
    hgCreateVkImage(&depthImage, &depthAlloc, depthImageConfig);
    hgDefer(vmaDestroyImage(hgVkVma, depthImage, depthAlloc));

    HgCreateVkImageView depthViewConfig{};
    depthViewConfig.image = depthImage;
    depthViewConfig.format = depthImageConfig.format;
    depthViewConfig.subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageView depthView = hgCreateVkImageView(depthViewConfig);
    hgDefer(vkDestroyImageView(hgVkDevice, depthView, nullptr));

    HgClock gameClock{};

    f64 frameTime = 0.0;
    f64 recordTime = 0.0f;
    f64 acquireTime = 0.0;
    f64 presentTime = 0.0f;

    for (;;)
    {
        f64 delta = gameClock.tick();

        HgClock frameClock{};

        HgArena* frame = hgGetScratch(&arena, 1);
        HgArenaScope frameScope{frame};

        hgProcessWindowEvents(&window, 1);
        if (window->wasClosed)
            goto quit;

        if ((depthWidth != window->width || depthHeight != window->height) && window->width * window->height != 0)
        {
            vkDestroyImageView(hgVkDevice, depthView, nullptr);
            vmaDestroyImage(hgVkVma, depthImage, depthAlloc);

            depthImageConfig.width = depthWidth = window->width;
            depthImageConfig.height = depthHeight = window->height;
            hgCreateVkImage(&depthImage, &depthAlloc, depthImageConfig);

            depthViewConfig.image = depthImage;
            depthView = hgCreateVkImageView(depthViewConfig);
        }

        hgUpdateProjection3D(
            hgPerspectiveProjection((f32)hgPi * 0.5f, (f32)window->width / (f32)window->height, 0.1f, 1000.0f));

        HgQuat& cubeRot = ecs.get<HgTransform>(cube).rotation;
        cubeRot = hgAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * cubeRot;

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

        hgUpdateView3D(hgViewMatrix(camera.position, camera.scale, camera.rotation));

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Time"))
        {
            ImGui::Text("delta: %.3fms\n", delta * 1e3);

            ImGui::Text("cpu: %.3fms\n", (frameTime + recordTime) * 1e3);
            ImGui::Text("frame: %.3fms\n", frameTime * 1e3);
            ImGui::Text("record: %.3fms\n", recordTime * 1e3);

            ImGui::Text("gpu: %.3fms\n", (acquireTime + presentTime) * 1e3);
            ImGui::Text("acquire: %.3fms\n", acquireTime * 1e3);
            ImGui::Text("present: %.3fms\n", presentTime * 1e3);
        }
        ImGui::End();

        ImGui::Render();

        frameTime = frameClock.tick();

        HgClock acquireClock{};
        VkCommandBuffer cmd = window->beginRecording();
        acquireTime = acquireClock.tick();

        HgClock recordClock{};

        if (cmd != nullptr)
        {
            HgRenderer renderer = renderer.create(frame, 64, 64);

            HgImageRenderID windowImageID = renderer.addImage(
                window->images[window->currentImage],
                window->views[window->currentImage],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            HgImageRenderID depthImageID = renderer.addImage(
                depthImage,
                depthView,
                {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

            HgRenderAttachment colorAttachment{};
            colorAttachment.image = windowImageID;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

            HgRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;
            pass.depthAttachment.image = depthImageID;
            pass.depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            pass.depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            pass.depthAttachment.clearValue.depthStencil = {1.0f, 0};

            renderer.beginPass(cmd, window->width, window->height, pass);

            hgDraw3D(&ecs, cmd);

            ImGui_ImplHurdyGurdy_Draw(cmd);

            renderer.endPass(cmd);

            HgImageBarrier presentBarrier{};
            presentBarrier.image = windowImageID;
            presentBarrier.nextUsage = HgRenderUsage_presentSrc;

            renderer.barrier(cmd, nullptr, 0, &presentBarrier, 1);

            recordTime = recordClock.tick();

            HgClock presentClock{};
            window->endAndPresent(cmd);
            presentTime = presentClock.tick();
        }
    }

quit:
    vkDeviceWaitIdle(hgVkDevice);
}

