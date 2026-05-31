#include "hurdygurdy.hpp"

int main()
{
    hgInit(nullptr);
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    hgArenaScope(arena);

    HgWindow window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    u32 width = 0;
    u32 height = 0;

    HgGpuImage depthImage;
    HgGpuView depthView;
    hgDefer(hgGpuImageDestroy(depthImage));
    hgDefer(hgGpuViewDestroy(depthView));

    hgSpritesInit(hgWindowImageFormat(window), HgFormat_d32_sfloat);
    hgDefer(hgSpritesDeinit());

    HgEcs ecs = hgEcsCreate(arena, 128, 16);
    hgDefer(hgEcsReset(&ecs));

    hgEcsRegisterType(&ecs, arena, HgCamera, 8);
    hgEcsRegisterType(&ecs, arena, HgTransform, 128);
    hgEcsRegisterType(&ecs, arena, HgSprite, 128);

    HgEntity camera = hgEcsSpawn(&ecs);
    HgCamera* cameraC = hgEcsAdd<HgCamera>(&ecs, camera);
    HgTransform* cameraTf = hgEcsAdd<HgTransform>(&ecs, camera);
    cameraC->type = HgCameraType_perspective;
    cameraC->perspective.fov = (f32)hgPi * 0.5f;
    cameraC->perspective.near = 0.1f;
    cameraC->perspective.far = 1000.0f;
    cameraTf->position = HgVec3{0, 0, -2};

    HgEntity square = hgEcsSpawn(&ecs);
    hgEcsAdd<HgTransform>(&ecs, square);
    hgEcsAdd<HgSprite>(&ecs, square);

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

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
        if (!hgNullHandle(hgWindowImageView(window).handle))
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

            hgGpuRenderPassBegin(cmd, hgWindowWidth(window), hgWindowHeight(window), &pass);

            hgCameraUpdate(&ecs, camera);
            hgSpritesDraw(&ecs, camera, cmd);

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

