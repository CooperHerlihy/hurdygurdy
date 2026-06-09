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

    HgArena* arena = hgScratch();
    hgArenaScope(arena);

    HgWindow* window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    HgCamera cameraC = hgCameraCreate();
    hgDefer(hgCameraDestroy(&cameraC));

    // cameraC.type = HgCameraType_perspective;
    // cameraC.type = HgCameraType_orthographic;

    cameraC.perspective.fov = (f32)hgPi * 0.5f;
    cameraC.perspective.near = 0.1f;
    cameraC.perspective.far = 1000.0f;

    cameraC.orthographic.left = -1;
    cameraC.orthographic.right = 1;
    cameraC.orthographic.top = -1;
    cameraC.orthographic.bottom = 1;
    cameraC.orthographic.near = 0;
    cameraC.orthographic.far = 10;

    HgTransform cameraTf = {};
    cameraTf.position = HgVec3{0, 0, -1};

    hg2dInit(hgWindowImageFormat(window));
    hgDefer(hg2dDeinit());

    Hg2dLayer layer = hg2dLayerCreate();
    hgDefer(hg2dLayerDestroy(&layer));

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        cameraC.perspective.aspect = (f32)hgWindowWidth(window) / (f32)hgWindowHeight(window);

        if (hgIsButtonDown(window, HgButton_lmouse))
        {
            f32 rotSpeed = 2.0f;
            HgQuat rotX = hgQuatAxisAngle(HgVec3{ 0, 1, 0}, hgMouseDeltaX(window) * rotSpeed);
            HgQuat rotY = hgQuatAxisAngle(HgVec3{-1, 0, 0}, hgMouseDeltaY(window) * rotSpeed);
            cameraTf.rotation = rotX * cameraTf.rotation * rotY;
        }

        HgVec3 movement = HgVec3{
            (f32)(hgIsButtonDown(window, HgButton_d) - hgIsButtonDown(window, HgButton_a)),
            (f32)(hgIsButtonDown(window, HgButton_lshift) - hgIsButtonDown(window, HgButton_space)),
            (f32)(hgIsButtonDown(window, HgButton_w) - hgIsButtonDown(window, HgButton_s)),
        };
        if (movement != HgVec3{0.0f})
        {
            f32 moveSpeed = 1.5f;
            HgVec3 rotated = hgVecRotate(cameraTf.rotation, HgVec3{movement.x, 0.0f, movement.z});
            cameraTf.position += hgVecNorm3(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
        }

        cameraTf.mat = hgMatModel3D(cameraTf.position, cameraTf.scale, cameraTf.rotation);

        hg2dLayerClear(&layer);

        Hg2dSprite sprite = {nullptr, HgVec2{0}, HgVec2{1}};
        hg2dSpriteDraw(&layer, HgVec2{0, 0}, HgVec2{1, 1}, &sprite);

        hg2dRectDraw(&layer, HgVec2{-1, -1}, HgVec2{1, 1}, HgVec4{1, 0, 0, 1});

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
        if (hgWindowImageView(window) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowImageView(window);

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

            hgGpuRenderPassBegin(cmd, &pass);

            hgGpuSetViewport(cmd, 0, 0, (f32)hgWindowWidth(window), (f32)hgWindowHeight(window));
            hgGpuSetScissor(cmd, 0, 0, hgWindowWidth(window), hgWindowHeight(window));

            VPUniform vp{};
            vp.view = hgMatModelToView(cameraTf.mat);

            // vp.proj = hgMatOrthographic(
            //     cameraC.orthographic.left,
            //     cameraC.orthographic.right,
            //     cameraC.orthographic.top,
            //     cameraC.orthographic.bottom,
            //     cameraC.orthographic.near,
            //     cameraC.orthographic.far);

            vp.proj = hgMatPerspective(
                cameraC.perspective.fov,
                cameraC.perspective.aspect,
                cameraC.perspective.near,
                cameraC.perspective.far);

            // VPUniform vp{};
            // vp.view = HgMat4{1};
            // vp.proj = HgMat4{1};
            // hgGpuBufferWrite(cameraC.vpBuffer, 0, &vp, sizeof(vp));

            hgGpuBufferWrite(cameraC.vpBuffer, 0, &vp, sizeof(vp));

            hg2dDraw(cmd, &cameraC, &layer);

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

