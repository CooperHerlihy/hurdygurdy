#include "hurdygurdy.hpp"

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

    hgInitPipeline2D(arena, 256, window->format, VK_FORMAT_UNDEFINED);
    hgDefer(hgDeinitPipeline2D());

    HgTransform camera{};
    camera.position.z = -1;

    HgECS ecs = ecs.create(4096);
    hgDefer(ecs.destroy());

    HgEntity square = ecs.spawn();
    ecs.add<HgTransform>(square) = {};
    ecs.add<HgSprite2D>(square) = {0, HgVec2{0}, HgVec2{1}};

    HgClock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();

        HgArena* frame = hgGetScratch();
        HgArenaScope frameScope{frame};

        hgProcessWindowEvents(&window, 1);
        if (window->wasClosed)
            goto quit;

        hgUpdateProjection2D(hgPerspective((f32)hgPi * 0.5f, (f32)window->width / (f32)window->height, 0.1f, 1000.0f));

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

        hgUpdateView2D(hgViewMatrix(camera.position, camera.scale, camera.rotation));

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

            renderer.beginPass(cmd, window->width, window->height, pass);

            hgDraw2D(&ecs, cmd);

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

