#include "hurdygurdy.hpp"

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgArena* arena = hgScratch();
    hgArenaScope(arena);

    HgWindow* window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    u32 width = 0;
    u32 height = 0;

    HgGpuImage* depthImage = nullptr;
    HgGpuView* depthView = nullptr;
    hgDefer(hgGpuImageDestroy(depthImage));
    hgDefer(hgGpuViewDestroy(depthView));

    HgBinaryAsset* spriteVertSpv = hgAssetLoad<HgBinary>("build/sprite.vert.spv");
    HgBinaryAsset* spriteFragSpv = hgAssetLoad<HgBinary>("build/sprite.frag.spv");
    hgDefer(hgAssetUnload(spriteVertSpv));
    hgDefer(hgAssetUnload(spriteFragSpv));

    struct VPUniform {
        HgMat4 proj;
        HgMat4 view;
    };

    struct SpritePipelinePush {
        HgMat4 model;
        HgVec2 uvPos;
        HgVec2 uvSize;
        u32 viewProj;
        u32 texture;
    };

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = spriteVertSpv->data.data;
    pipelineConfig.vertexShaderSize = spriteVertSpv->data.size;
    pipelineConfig.fragmentShader = spriteFragSpv->data.data;
    pipelineConfig.fragmentShaderSize = spriteFragSpv->data.size;
    pipelineConfig.pushConstantSize = sizeof(SpritePipelinePush);
    HgFormat windowFormat = hgWindowImageFormat(window);
    pipelineConfig.colorAttachmentFormats = &windowFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = HgFormat_d32_sfloat;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    HgGpuPipeline* spritePipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);
    hgDefer(hgGpuPipelineDestroy(spritePipeline));

    HgGpuTexture tex;

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    tex.image = hgGpuImageCreate(2, 2, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);
    hgDefer(hgGpuImageDestroy(tex.image));

    tex.view = hgGpuViewCreate(
        tex.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(tex.view, defaultColors);
    hgDefer(hgGpuViewDestroy(tex.view));

    HgCamera cameraC = hgCameraCreate();
    hgDefer(hgCameraDestroy(&cameraC));

    cameraC.type = HgCameraType_perspective;
    cameraC.perspective.fov = (f32)hgPi * 0.5f;
    cameraC.perspective.near = 0.1f;
    cameraC.perspective.far = 1000.0f;

    HgTransform cameraTf = {};
    cameraTf.position = HgVec3{0, 0, -1};

    HgTransform sprite = {};
    sprite.mat = hgMatModel3D(sprite.position, sprite.scale, sprite.rotation);

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

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

        if (width != hgWindowWidth(window) || height != hgWindowHeight(window))
        {
            width = hgWindowWidth(window);
            height = hgWindowHeight(window);

            hgGpuWaitIdle();
            hgGpuViewDestroy(depthView);
            hgGpuImageDestroy(depthImage);

            depthImage = hgGpuImageCreate(width, height, HgFormat_d32_sfloat, HgGpuImageUsage_depthStencilAttachment);
            depthView = hgGpuViewCreate(depthImage, HgGpuAspect_depth);

            cameraC.perspective.aspect = (f32)width / (f32)height;
        }

        cameraTf.mat = hgMatModel3D(cameraTf.position, cameraTf.scale, cameraTf.rotation);

        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
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

            hgGpuRenderPassBegin(cmd, &pass);

            hgGpuSetViewport(cmd, 0, 0, (f32)hgWindowWidth(window), (f32)hgWindowHeight(window));
            hgGpuSetScissor(cmd, 0, 0, hgWindowWidth(window), hgWindowHeight(window));

            VPUniform vp{};
            vp.view = hgMatModelToView(cameraTf.mat);
            vp.proj = hgMatPerspective(
                cameraC.perspective.fov,
                cameraC.perspective.aspect,
                cameraC.perspective.near,
                cameraC.perspective.far);

            hgGpuBufferWrite(cameraC.vpBuffer, 0, &vp, sizeof(vp));

            hgGpuBindPipeline(cmd, spritePipeline);

            SpritePipelinePush push{};
            push.model = sprite.mat;
            push.uvPos = HgVec2{0.0f};
            push.uvSize = HgVec2{1.0f};
            push.viewProj = hgGpuBufferUniformDescriptor(cameraC.vpBuffer);
            push.texture = hgGpuImageSamplerDescriptor(tex.view);

            hgGpuPushConstants(cmd, spritePipeline, 0, &push, sizeof(push));

            hgGpuDraw(cmd, 0, 6, 0, 1);

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

