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
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    Window window = Window::create("Hg Minimal Example", 1200, 800, {}).expect("Could not create window\n");

    f32 musicData[2000];
    Asset<Sound> music = newAsset<Sound>();
    music->data = musicData;
    music->size = sizeof(musicData);
    music->frequency = 8000;
    music->channels = 1;

    for (u32 i = 0; i < std::size(musicData); ++i)
    {
        f32 t = static_cast<f32>(i) * static_cast<f32>(HG_PI) * 2.0f / 8000.0f;
        musicData[i] = 0;
        for (u32 j = 1; j <= 64; ++j)
        {
            f32 x = static_cast<f32>(j);
            musicData[i] += 1.0f / x * std::sin(100.f * t * x);
        }
    }

    f32 soundData[2000];
    Asset<Sound> sound = newAsset<Sound>();
    sound->data = soundData;
    sound->size = sizeof(soundData);
    sound->frequency = 8000;
    sound->channels = 1;

    for (u32 i = 0; i < std::size(soundData); ++i)
    {
        f32 t = static_cast<f32>(i) * static_cast<f32>(HG_PI) * 2.0f / 8000.0f;
        soundData[i] = noiseNorm(42.0f, t) / (t + 0.1f);
    }

    AudioPlayer audio = audioPlayerCreate();
    audioPlayerMusic(&audio, &music);
    audioPlayerSetMusicGain(&audio, &music, 0.3f);
    audioPlayerMusicPause(&audio, &music);

    rendererInit2D(window.imageFormat());
    HG_DEFER(rendererDeinit2D());

    u32 width = window.width();
    u32 height = window.height();

    Camera camera = cameraCreate();

    Layer2D backgroundLayer = layerCreate2D();
    HG_DEFER(layerDestroy2D(&backgroundLayer));

    layerClear2D(&backgroundLayer);
    Vec2 backgroundBegin = Vec2{static_cast<f32>(width) / static_cast<f32>(height) - 0.5f, 0.5f} / 2.0f;
    drawRect2D(&backgroundLayer,
        {.002f, 0, .012f, 1},
        {backgroundBegin, backgroundBegin + Vec2{0.5f, 0.5f}});

    Layer2D spriteLayer = layerCreate2D();
    HG_DEFER(layerDestroy2D(&spriteLayer));

    Sprite2D sprite = {nullptr, {Vec2{0}, Vec2{1}}};
    Vec2 spriteSize{0.1f, 0.1f};
    Vec2 spritePos = (Vec2{static_cast<f32>(width) / static_cast<f32>(height), 1} - spriteSize) / 2.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    HG_DEFER(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    initImGui(window, window.imageFormat());
    HG_DEFER(deinitImGui());

    // temporary, trick the OS into thinking we're important
    callPar(nullptr, nullptr, [](void*)
    {
        while(!quit)
        {
            _mm_pause();
        }
    });

    Clock gameClock{};
    for (;;)
    {
        cpuClock.tick();
        f64 delta = gameClock.tick();

        processEvents();
        if (wasQuit() || window.wasClosed())
            goto quit;

        audioPlayerUpdate(&audio);

        beginImGuiFrame();
        ImGui::NewFrame();

        width = window.width();
        height = window.height();

        cameraSetOrthographic(&camera, static_cast<f32>(width) / static_cast<f32>(height), 1.0f);

        if (window.isButtonDown(Button_lmouse))
        {
            f32 moveSpeed = 1.0f;
            camera.position.x -= window.mouseDX() * moveSpeed;
            camera.position.y -= window.mouseDY() * moveSpeed;
        }

        cameraUpdate(&camera);

        Vec2 spriteMove = {
            static_cast<f32>(window.isButtonDown(Button_d) - window.isButtonDown(Button_a)),
            static_cast<f32>(window.isButtonDown(Button_s) - window.isButtonDown(Button_w)),
        };
        if (spriteMove != Vec2{0.0f})
        {
            f32 moveSpeed = 0.4f;
            spritePos += vecNorm2(spriteMove) * moveSpeed * static_cast<f32>(delta);
        }

        layerClear2D(&spriteLayer);

        drawSprite2D(&spriteLayer, &sprite, {spritePos, spritePos + spriteSize});

        Span<WindowEvent> events = window.events();
        for (WindowEvent event : events)
        {
            if (event.type == WindowEventType_buttonPress &&
                event.button.button == Button_space)
                audioPlayerSound(&audio, &sound, 0.5f);
        }

        if (window.isButtonDown(Button_m))
            audioPlayerMusic(&audio, &music);
        else
            audioPlayerMusicPause(&audio, &music);

        if (ImGui::Begin("Info"))
        {
            ImGui::Text("Total: %.3fms", delta * 1.0e3);

            cpuTime += cpuClock.tick();
            ImGui::Text("Cpu: %.3fms", cpuTime * 1.0e3);
            cpuTime = 0.0f;

            ImGui::Checkbox("Render Debug", &renderDebug);
        }
        ImGui::End();

        ImGui::Render();

        cpuTime += cpuClock.tick();
        Window* windows[] = {&window};
        GpuCmd* cmd = gpuFrameBegin(windows);
        cpuClock.tick();
        if (window.imageView() != nullptr)
        {
            GpuRenderAttachment colorAttachment{};
            colorAttachment.image = window.imageView();

            GpuRenderPass pass{};
            pass.colorAttachments = &colorAttachment;
            pass.colorAttachmentCount = 1;

            gpuRenderPassBegin(cmd, pass);

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
        cpuTime += cpuClock.tick();

        gpuFrameEnd(cmd);
    }

quit:
    quit = true;
    gpuWaitIdle();
}

