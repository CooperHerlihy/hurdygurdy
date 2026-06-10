#include "hurdygurdy.hpp"

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    HgWindow* window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    hgDefer(hgWindowDestroy(window));

    f32 musicData[2000];
    HgSoundAsset* music = hgAssetCreate<HgSound>();
    music->data.data = musicData;
    music->data.size = sizeof(musicData);
    music->data.frequency = 8000;
    music->data.channels = 1;

    for (u32 i = 0; i < hgArrayCount(musicData); ++i)
    {
        f32 t = (f32)i * (f32)hgPi * 2.0f / 8000.0f;
        musicData[i] = 0;
        for (u32 j = 1; j <= 64; ++j)
        {
            f32 x = (f32)j;
            musicData[i] += 1.0f / x * std::sin(100.f * t * x);
        }
    }

    f32 soundData[2000];
    HgSoundAsset* sound = hgAssetCreate<HgSound>();
    sound->data.data = soundData;
    sound->data.size = sizeof(soundData);
    sound->data.frequency = 8000;
    sound->data.channels = 1;

    for (u32 i = 0; i < hgArrayCount(soundData); ++i)
    {
        f32 t = (f32)i * (f32)hgPi * 2.0f / 8000.0f;
        soundData[i] = hgNoiseNorm(42.0f, t);
    }

    HgAudioPlayer audio = hgAudioPlayerCreate();
    hgAudioPlayerMusic(&audio, music);
    hgAudioPlayerSetMusicGain(&audio, music, 0.3f);
    hgAudioPlayerMusicPause(&audio, music);

    hgInit2D(hgWindowImageFormat(window));
    hgDefer(hgDeinit2D());

    u32 width = hgWindowWidth(window);
    u32 height = hgWindowHeight(window);

    HgCamera camera = hgCameraCreate();
    hgDefer(hgCameraDestroy(&camera));

    HgLayer2D background = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&background));

    hgLayerClear2D(&background);
    hgDrawRect2D(&background,
        HgVec4{.002f, 0, .012f, 1},
        {HgVec2{(f32)width / (f32)height - 0.5f, 0.5f} / 2.0f, HgVec2{0.5f}});

    HgLayer2D spriteLayer = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&spriteLayer));

    HgSprite2D sprite = {nullptr, {HgVec2{0}, HgVec2{1}}};
    HgVec2 spriteSize{0.1f, 0.1f};
    HgVec2 spritePos = (HgVec2{(f32)width / (f32)height, 1} - spriteSize) / 2.0f;

    HgClock gameClock;
    hgClockTick(&gameClock);
    for (;;)
    {
        f64 delta = hgClockTick(&gameClock);

        hgAudioPlayerUpdate(&audio);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        width = hgWindowWidth(window);
        height = hgWindowHeight(window);

        hgCameraSetOrthographic(&camera, (f32)width / (f32)height);

        if (hgIsButtonDown(window, HgButton_lmouse))
        {
            f32 moveSpeed = 1.0f;
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
            f32 moveSpeed = 0.4f;
            spritePos += hgVecNorm2(spriteMove) * moveSpeed * (f32)delta;
        }

        hgLayerClear2D(&spriteLayer);

        hgDrawSprite2D(&spriteLayer, &sprite, {spritePos, spriteSize});

        u32 eventCount;
        HgWindowEvent* event = hgWindowEvents(window, &eventCount);
        for (u32 i = 0; i < eventCount; ++i, ++event)
        {
            if (event->type == HgWindowEventType_buttonPress &&
                event->button.button == HgButton_space)
            {
                hgAudioPlayerSound(&audio, sound, 0.5f);
            }
        }

        if (hgIsButtonDown(window, HgButton_m))
        {
            hgAudioPlayerMusic(&audio, music);
        }
        else
        {
            hgAudioPlayerMusicPause(&audio, music);
        }

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

            HgLayer2D* layers[] = {&background, &spriteLayer};
            hgRender2D(cmd, &camera, layers, (u32)hgArrayCount(layers));

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

