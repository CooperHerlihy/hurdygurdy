#include "hurdygurdy.hpp"

using namespace hg;

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

#include <emmintrin.h>

static volatile bool quit = false;

static Clock cpuClock{};
static f64 cpuTime = 0.0f;

static bool renderDebug = false;

int main()
{
    if (!init())
        HG_PANIC("Could not initialize Hurdy Gurdy\n");
    HG_DEFER(deinit());

    Window* window = windowCreate("Hg Minimal Example", 1200, 800, nullptr);
    if (window == nullptr)
        HG_PANIC("Could not create window\n");
    HG_DEFER(windowDestroy(window));

    f32 musicData[2000];
    SoundAsset* music = assetCreate<Sound>();
    music->asset.data = musicData;
    music->asset.size = sizeof(musicData);
    music->asset.frequency = 8000;
    music->asset.channels = 1;

    for (u32 i = 0; i < arrayCount(musicData); ++i)
    {
        f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
        musicData[i] = 0;
        for (u32 j = 1; j <= 64; ++j)
        {
            f32 x = (f32)j;
            musicData[i] += 1.0f / x * std::sin(100.f * t * x);
        }
    }

    f32 soundData[2000];
    SoundAsset* sound = assetCreate<Sound>();
    sound->asset.data = soundData;
    sound->asset.size = sizeof(soundData);
    sound->asset.frequency = 8000;
    sound->asset.channels = 1;

    for (u32 i = 0; i < arrayCount(soundData); ++i)
    {
        f32 t = (f32)i * (f32)HG_PI * 2.0f / 8000.0f;
        soundData[i] = noiseNorm(42.0f, t) / (t + 0.1f);
    }

    AudioPlayer audio = audioPlayerCreate();
    audioPlayerMusic(&audio, music);
    audioPlayerSetMusicGain(&audio, music, 0.3f);
    audioPlayerMusicPause(&audio, music);

    rendererInit2D(windowImageFormat(window));
    HG_DEFER(rendererDeinit2D());

    u32 width = windowWidth(window);
    u32 height = windowHeight(window);

    Camera camera = cameraCreate();
    HG_DEFER(cameraDestroy(&camera));

    Layer2D backgroundLayer = layerCreate2D();
    HG_DEFER(layerDestroy2D(&backgroundLayer));

    layerClear2D(&backgroundLayer);
    drawRect2D(&backgroundLayer,
        {.002f, 0, .012f, 1},
        {
            Vec2{
                (f32)width / (f32)height - 0.5f,
                0.5f,
            } / 2.0f,
            Vec2{
                0.5f,
                0.5f,
            }
        });

    Layer2D spriteLayer = layerCreate2D();
    HG_DEFER(layerDestroy2D(&spriteLayer));

    Sprite2D sprite = {nullptr, {Vec2{0}, Vec2{1}}};
    Vec2 spriteSize{0.1f, 0.1f};
    Vec2 spritePos = (Vec2{(f32)width / (f32)height, 1} - spriteSize) / 2.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    HG_DEFER(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    initImGui(window, windowImageFormat(window));
    HG_DEFER(deinitImGui());

    // temporary, trick the OS into thinking we're important
    callPar(nullptr, nullptr, [](void*)
    {
        while(!quit)
        {
            _mm_pause();
        }
    });

    Clock gameClock;
    clockTick(&gameClock);
    clockTick(&cpuClock);
    for (;;)
    {
        clockTick(&cpuClock);
        f64 delta = clockTick(&gameClock);

        processEvents();
        if (wasQuit() || windowWasClosed(window))
            goto quit;

        audioPlayerUpdate(&audio);

        beginImGuiFrame();
        ImGui::NewFrame();

        width = windowWidth(window);
        height = windowHeight(window);

        cameraSetOrthographic(&camera, (f32)width / (f32)height, 1.0f);

        if (isButtonDown(window, Button_lmouse))
        {
            f32 moveSpeed = 1.0f;
            camera.position.x -= mouseDeltaX(window) * moveSpeed;
            camera.position.y -= mouseDeltaY(window) * moveSpeed;
        }

        cameraUpdate(&camera);

        Vec2 spriteMove = {
            (f32)(isButtonDown(window, Button_d) - isButtonDown(window, Button_a)),
            (f32)(isButtonDown(window, Button_s) - isButtonDown(window, Button_w)),
        };
        if (spriteMove != Vec2{0.0f})
        {
            f32 moveSpeed = 0.4f;
            spritePos += vecNorm2(spriteMove) * moveSpeed * (f32)delta;
        }

        layerClear2D(&spriteLayer);

        drawSprite2D(&spriteLayer, &sprite, {spritePos, spriteSize});

        u32 eventCount;
        WindowEvent* event = windowEvents(window, &eventCount);
        for (u32 i = 0; i < eventCount; ++i, ++event)
        {
            if (event->type == WindowEventType_buttonPress &&
                event->button.button == Button_space)
                audioPlayerSound(&audio, sound, 0.5f);
        }

        if (isButtonDown(window, Button_m))
            audioPlayerMusic(&audio, music);
        else
            audioPlayerMusicPause(&audio, music);

        if (ImGui::Begin("Info"))
        {
            ImGui::Text("Total: %.3fms", delta * 1.0e3);

            cpuTime += clockTick(&cpuClock);
            ImGui::Text("Cpu: %.3fms", cpuTime * 1.0e3);
            cpuTime = 0.0f;

            ImGui::Checkbox("Render Debug", &renderDebug);
        }
        ImGui::End();

        ImGui::Render();

        cpuTime += clockTick(&cpuClock);
        GpuCmd* cmd = gpuFrameBegin(&window, 1);
        clockTick(&cpuClock);
        if (windowImageView(window) != nullptr)
        {
            GpuRenderAttachment colorAttachment{};
            colorAttachment.image = windowImageView(window);

            GpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

            gpuRenderPassBegin(cmd, &pass);

            renderLayer2D(cmd, &camera, &backgroundLayer);
            renderLayer2D(cmd, &camera, &spriteLayer);

            if (renderDebug)
            {
                renderDebug2D(cmd, &camera, &backgroundLayer);
                renderDebug2D(cmd, &camera, &spriteLayer);
            }

            renderImGui(cmd);

            gpuRenderPassEnd(cmd);
        }
        cpuTime += clockTick(&cpuClock);

        gpuFrameEnd(cmd);
    }

quit:
    quit = true;
    gpuWaitIdle();
}

