#include <hurdygurdy.hpp>

using namespace hg;

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

static volatile bool quit = false;

static bool renderDebug = false;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    Window window = Window::create({
        // .preferredPresentMode = GpuPresentMode_mailbox,
    });
    window.setTitle("Hg Minimal Example");

    f32 musicData[8000];
    Asset<Sound> music = newAsset<Sound>();
    music->data = musicData;
    music->frequency = 48000;
    music->channels = 1;

    for (u32 i = 0; i < size(musicData); ++i)
    {
        f32 t = static_cast<f32>(i) * pif * 2.0f / 48000.0f;
        musicData[i] = 0;
        for (u32 j = 1; j <= 64; ++j)
        {
            f32 x = static_cast<f32>(j);
            musicData[i] += 1.0f / x * std::sin(100.f * t * x);
        }
    }

    f32 soundData[8000];
    Asset<Sound> sound = newAsset<Sound>();
    sound->data = soundData;
    sound->frequency = 48000;
    sound->channels = 1;

    for (u32 i = 0; i < size(soundData); ++i)
    {
        f32 t = static_cast<f32>(i) * pif * 2.0f / 8000.0f;
        soundData[i] = noiseNorm(42u, t) / (t + 0.1f);
    }

    AudioPlayer audio{};
    setAudioCallback(audio.callback, &audio);

    audio.playMusic(music);
    audio.setMusicGain(music, 0.3f);
    audio.pauseMusic(music);

    Renderer2D renderer{window.swapchain().format()};
    DebugRenderer2D debugRenderer{window.swapchain().format()};

    u32 width, height;
    window.size(&width, &height);

    Camera camera{};

    Layer2D backgroundLayer{};

    backgroundLayer.clear();

    Vec2 backgroundBegin = Vec2{static_cast<f32>(width) / static_cast<f32>(height) - 0.5f, 0.5f} / 2.0f;
    backgroundLayer.drawRect({.002f, 0, .012f, 1}, {backgroundBegin, backgroundBegin + Vec2{0.5f, 0.5f}});

    backgroundLayer.drawText("Hello World", Vec4{1}, TextBuilder{getDefaultFont()}
        .setTopLeft(Vec2{0})
        .setBounds(1, 0.25));

    Layer2D spriteLayer{};

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

    initImGui(window, window.swapchain().format());
    HG_DEFER(deinitImGui());

    beginImGuiFrame();
    ImGui::NewFrame();

    Clock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();
    {
        ProfilerScopeTimer timer{"Frame"};
        {
            ProfilerScopeTimer timer{"Cpu"};

            processEvents();
            if (wasQuit() || window.wasClosed())
                goto quit;

            window.size(&width, &height);
            camera.setOrthographic(static_cast<f32>(width) / static_cast<f32>(height), 1.0f);

            if (wasButtonPressed(Button_space))
                audio.playSound(sound, 1.0f);

            if (isButtonDown(Button_m))
                audio.playMusic(music);
            else
                audio.pauseMusic(music);

            if (isButtonDown(Button_lmouse))
            {
                Vec2 md = mouseDelta();
                f32 moveSpeed = 1.0f;
                camera.position.x -= md.x * moveSpeed;
                camera.position.y -= md.y * moveSpeed;
            }

            camera.update();

            Vec2 spriteMove = {
                static_cast<f32>(isButtonDown(Button_d) - isButtonDown(Button_a)),
                static_cast<f32>(isButtonDown(Button_s) - isButtonDown(Button_w)),
            };
            if (spriteMove != Vec2{0.0f})
            {
                f32 moveSpeed = 0.4f;
                spritePos += vecNorm2(spriteMove) * moveSpeed * static_cast<f32>(delta);
            }

            spriteLayer.clear();
            spriteLayer.drawSprite(sprite, {spritePos, spritePos + spriteSize});

            renderer.queueLayer(backgroundLayer);
            renderer.queueLayer(spriteLayer);
            debugRenderer.queueLayer(backgroundLayer);
            debugRenderer.queueLayer(spriteLayer);

            if (ImGui::Begin("Info"))
            {
                ImGui::Text("FPS: %.3f", 1.e0 / delta);
                ImGui::Checkbox("Render Debug", &renderDebug);
            }
            ImGui::End();

            ImGui::Render();
        }

        GpuCmd* cmd;
        {
            ProfilerScopeTimer timer{"Gpu"};
            Window* windows[] = {&window};
            cmd = gpuBeginFrame(windows);
        }
        if (window.swapchain().renderTarget() != nullptr)
        {
            ProfilerScopeTimer timer{"Cpu"};

            GpuAttachment colorAttachment{};
            colorAttachment.image = window.swapchain().renderTarget();

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};

            gpuBeginRenderPass(cmd, pass);

            renderer.render(cmd, camera);
            if (renderDebug)
                debugRenderer.render(cmd, camera);

            renderImGui(cmd);

            gpuEndRenderPass(cmd);
        }
        {
            ProfilerScopeTimer timer{"Gpu"};
            gpuEndFrame(cmd);
        }
    }
        beginImGuiFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Timers"))
        {
            Profiler::forEachTimer([&](const String& name, f64 time)
            {
                ImGui::Text("%.*s: %.3fms", (int)name.length, name.chars, time * 1.e3f);
            });
            Profiler::forEachCounter([&](const String& name, u32 count)
            {
                ImGui::Text("%.*s: %d", (int)name.length, name.chars, count);
            });
            Profiler::clear();
        }
        ImGui::End();
    }

quit:
    quit = true;
    gpuWaitIdle();
}

