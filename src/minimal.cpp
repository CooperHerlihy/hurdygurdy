#include "hurdygurdy.hpp"

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgWindow* window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    hgInit2D(hgWindowImageFormat(window));
    hgDefer(hgDeinit2D());

    HgCamera camera = hgCameraCreate();
    hgDefer(hgCameraDestroy(&camera));

    HgLayer2D background = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&background));

    hgLayerClear2D(&background);
    hgRect2D(&background, HgVec2{-.5f, -.5f}, HgVec2{1, 1}, HgVec4{.002f, 0, .012f, 1});

    HgLayer2D spriteLayer = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&spriteLayer));

    HgSprite2D sprite = {nullptr, 0, 0, 1, 1};
    HgVec2 spritePos{-0.1f, -0.1f};

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        u32 width = hgWindowWidth(window);
        u32 height = hgWindowHeight(window);

        hgCameraSetOrthographic(&camera, (f32)width / (f32)height);

        if (hgIsButtonDown(window, HgButton_lmouse))
        {
            f32 moveSpeed = 1.5f;
            camera.position.x -= hgMouseDeltaX(window) * moveSpeed;
            camera.position.y -= hgMouseDeltaY(window) * moveSpeed;
        }

        hgCameraUpdate(&camera);

        HgVec2 spriteMove = HgVec2{
            (f32)(hgIsButtonDown(window, HgButton_d) - hgIsButtonDown(window, HgButton_a)),
            (f32)(hgIsButtonDown(window, HgButton_s) - hgIsButtonDown(window, HgButton_w)),
        };
        if (spriteMove != HgVec2{0.0f})
        {
            f32 moveSpeed = 0.8f;
            spritePos += hgVecNorm2(spriteMove) * moveSpeed * (f32)delta;
        }

        hgLayerClear2D(&spriteLayer);

        hgSprite2D(&spriteLayer, spritePos, HgVec2{.2f}, &sprite);

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
        if (hgWindowImageView(window) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowImageView(window);

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

            hgGpuRenderPassBegin(cmd, &pass);

            hgGpuSetViewport(cmd, 0, 0, (f32)width, (f32)height);
            hgGpuSetScissor(cmd, 0, 0, width, height);

            hgDraw2D(cmd, &camera, &background);
            hgDraw2D(cmd, &camera, &spriteLayer);

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

