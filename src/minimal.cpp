#include "hurdygurdy.hpp"

#define IM_ASSERT hgAssert
#include "imgui.h"

#include <emmintrin.h>

static volatile bool quit = false;

static HgClock cpuClock{};
static f64 cpuTime = 0.0f;

static bool renderDebug = false;

int main()
{
    if (!hgInit())
        hgPanic("Could not initialize Hurdy Gurdy\n");
    hgDefer(hgDeinit());

    hgTest();

    HgWindow* window = hgWindowCreate("Hg Minimal Example", 1200, 800, nullptr);
    if (window == nullptr)
        hgPanic("Could not create window\n");
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
        soundData[i] = hgNoiseNorm(42.0f, t) / (t + 0.1f);
    }

    HgAudioPlayer audio = hgAudioPlayerCreate();
    hgAudioPlayerMusic(&audio, music);
    hgAudioPlayerSetMusicGain(&audio, music, 0.3f);
    hgAudioPlayerMusicPause(&audio, music);

    hgRendererInit2D(hgWindowImageFormat(window));
    hgDefer(hgRendererDeinit2D());

    u32 width = hgWindowWidth(window);
    u32 height = hgWindowHeight(window);

    HgCamera camera = hgCameraCreate();
    hgDefer(hgCameraDestroy(&camera));

    HgLayer2D backgroundLayer = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&backgroundLayer));

    hgLayerClear2D(&backgroundLayer);
    hgDrawRect2D(&backgroundLayer,
        HgVec4{.002f, 0, .012f, 1},
        HgRect2D{
            HgVec2{
                (f32)width / (f32)height - 0.5f,
                0.5f,
            } / 2.0f,
            HgVec2{
                0.5f,
                0.5f,
            }
        });

    HgLayer2D spriteLayer = hgLayerCreate2D();
    hgDefer(hgLayerDestroy2D(&spriteLayer));

    HgSprite2D sprite = {nullptr, {HgVec2{0}, HgVec2{1}}};
    HgVec2 spriteSize{0.1f, 0.1f};
    HgVec2 spritePos = (HgVec2{(f32)width / (f32)height, 1} - spriteSize) / 2.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    hgImGuiInit(window, hgWindowImageFormat(window));
    hgDefer(hgImGuiDeinit());

    // temporary, trick the OS into thinking we're important
    hgThreadsCall(nullptr, nullptr, [](void*)
    {
        while(!quit)
        {
            _mm_pause();
        }
    });

    HgClock gameClock;
    hgClockTick(&gameClock);
    hgClockTick(&cpuClock);
    for (;;)
    {
        hgClockTick(&cpuClock);
        f64 delta = hgClockTick(&gameClock);

        hgProcessEvents();
        if (hgWasQuit() || hgWindowWasClosed(window))
            goto quit;

        hgAudioPlayerUpdate(&audio);

        hgImGuiNewFrame();
        ImGui::NewFrame();

        width = hgWindowWidth(window);
        height = hgWindowHeight(window);

        hgCameraSetOrthographic(&camera, (f32)width / (f32)height, 1.0f);

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
                hgAudioPlayerSound(&audio, sound, 0.5f);
        }

        if (hgIsButtonDown(window, HgButton_m))
            hgAudioPlayerMusic(&audio, music);
        else
            hgAudioPlayerMusicPause(&audio, music);

        if (ImGui::Begin("Info"))
        {
            ImGui::Text("Total: %.3fms", delta * 1.0e3);

            cpuTime += hgClockTick(&cpuClock);
            ImGui::Text("Cpu: %.3fms", cpuTime * 1.0e3);
            cpuTime = 0.0f;

            ImGui::Checkbox("Render Debug", &renderDebug);
        }
        ImGui::End();

        ImGui::Render();

        cpuTime += hgClockTick(&cpuClock);
        HgGpuCmd* cmd = hgGpuFrameBegin(&window, 1);
        hgClockTick(&cpuClock);
        if (hgWindowImageView(window) != nullptr)
        {
            HgGpuRenderAttachment colorAttachment{};
            colorAttachment.image = hgWindowImageView(window);

            HgGpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

            hgGpuRenderPassBegin(cmd, &pass);

            hgRenderLayer2D(cmd, &camera, &backgroundLayer);
            hgRenderLayer2D(cmd, &camera, &spriteLayer);

            if (renderDebug)
            {
                hgRenderDebug2D(cmd, &camera, &backgroundLayer);
                hgRenderDebug2D(cmd, &camera, &spriteLayer);
            }

            hgImGuiDraw(cmd);

            hgGpuRenderPassEnd(cmd);
        }
        cpuTime += hgClockTick(&cpuClock);

        hgGpuFrameEnd(cmd);
    }

quit:
    quit = true;
    hgGpuWaitIdle();
}

