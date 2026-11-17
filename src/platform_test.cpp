#include <hurdygurdy.hpp>

#include <memory>
#include <string>

using namespace hg;

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

static AudioPlayer audio{};
static std::unique_ptr<AudioDevice> audioDevice{};
static std::string backendError{};
static bool toneActive = false;
static f32 toneT = 0.0f;
static f32 musicGain = 0.5f;

static std::unique_ptr<InputDevice> input{};
static std::unique_ptr<Window> window{};
static std::unique_ptr<Window> secondWindow{};
static std::string inputBackendName = "none";
static std::string inputBackendError{};static u32 width, height;
static Camera camera{Orthographic{{{0, 0}, {1.0f, 1.0f}}}};

static const char* eventName(EventType type)
{
    switch (type)
    {
        case EventType_none: return "none";
        case EventType_quit: return "quit";
        case EventType_text: return "text";
        case EventType_keyPress: return "keyPress";
        case EventType_keyRelease: return "keyRelease";
        case EventType_mouseMoved: return "mouseMoved";
        case EventType_wheelMoved: return "wheelMoved";
        case EventType_windowClosed: return "windowClosed";
        case EventType_windowFocused: return "windowFocused";
        case EventType_windowUnfocused: return "windowUnfocused";
        case EventType_windowResized: return "windowResized";
        case EventType_clipboardUpdate: return "clipboardUpdate";
        case EventType_count: return "count";
    }
    return "?";
}

static const char* buttonName(Button b)
{
    switch (b)
    {
        case Button_none: return "none";
        case Button_0: return "0";
        case Button_1: return "1";
        case Button_2: return "2";
        case Button_3: return "3";
        case Button_4: return "4";
        case Button_5: return "5";
        case Button_6: return "6";
        case Button_7: return "7";
        case Button_8: return "8";
        case Button_9: return "9";
        case Button_q: return "q";
        case Button_w: return "w";
        case Button_e: return "e";
        case Button_r: return "r";
        case Button_t: return "t";
        case Button_y: return "y";
        case Button_u: return "u";
        case Button_i: return "i";
        case Button_o: return "o";
        case Button_p: return "p";
        case Button_a: return "a";
        case Button_s: return "s";
        case Button_d: return "d";
        case Button_f: return "f";
        case Button_g: return "g";
        case Button_h: return "h";
        case Button_j: return "j";
        case Button_k: return "k";
        case Button_l: return "l";
        case Button_z: return "z";
        case Button_x: return "x";
        case Button_c: return "c";
        case Button_v: return "v";
        case Button_b: return "b";
        case Button_n: return "n";
        case Button_m: return "m";
        case Button_semicolon: return ";";
        case Button_apostrophe: return "'";
        case Button_comma: return ",";
        case Button_period: return ".";
        case Button_grave: return "`";
        case Button_lbracket: return "[";
        case Button_rbracket: return "]";
        case Button_equal: return "=";
        case Button_minus: return "-";
        case Button_slash: return "/";
        case Button_backslash: return "\\";
        case Button_up: return "up";
        case Button_down: return "down";
        case Button_left: return "left";
        case Button_right: return "right";
        case Button_escape: return "escape";
        case Button_space: return "space";
        case Button_enter: return "enter";
        case Button_backspace: return "backspace";
        case Button_kdelete: return "delete";
        case Button_insert: return "insert";
        case Button_tab: return "tab";
        case Button_home: return "home";
        case Button_end: return "end";
        case Button_pageup: return "pageup";
        case Button_pagedown: return "pagedown";
        case Button_f1: return "f1";
        case Button_f2: return "f2";
        case Button_f3: return "f3";
        case Button_f4: return "f4";
        case Button_f5: return "f5";
        case Button_f6: return "f6";
        case Button_f7: return "f7";
        case Button_f8: return "f8";
        case Button_f9: return "f9";
        case Button_f10: return "f10";
        case Button_f11: return "f11";
        case Button_f12: return "f12";
        case Button_printscreen: return "printscreen";
        case Button_context: return "context";
        case Button_numpad0: return "kp0";
        case Button_numpad1: return "kp1";
        case Button_numpad2: return "kp2";
        case Button_numpad3: return "kp3";
        case Button_numpad4: return "kp4";
        case Button_numpad5: return "kp5";
        case Button_numpad6: return "kp6";
        case Button_numpad7: return "kp7";
        case Button_numpad8: return "kp8";
        case Button_numpad9: return "kp9";
        case Button_numpaddecimal: return "kp.";
        case Button_numpaddiv: return "kp/";
        case Button_numpadmul: return "kp*";
        case Button_numpadminus: return "kp-";
        case Button_numpadplus: return "kp+";
        case Button_numpadenter: return "kpenter";
        case Button_lshift: return "lshift";
        case Button_rshift: return "rshift";
        case Button_lctrl: return "lctrl";
        case Button_rctrl: return "rctrl";
        case Button_lalt: return "lalt";
        case Button_ralt: return "ralt";
        case Button_lsuper: return "lsuper";
        case Button_rsuper: return "rsuper";
        case Button_capslock: return "capslock";
        case Button_numlock: return "numlock";
        case Button_scrolllock: return "scrolllock";
        case Button_pause: return "pause";
        case Button_mouse1: return "mouse1";
        case Button_mouse2: return "mouse2";
        case Button_mouse3: return "mouse3";
        case Button_mouse4: return "mouse4";
        case Button_mouse5: return "mouse5";
        default: break;
    }
    return "?";
}

static const char* cursorName(CursorType t)
{
    switch (t)
    {
        case CursorType_arrow: return "arrow";
        case CursorType_textInput: return "textInput";
        case CursorType_resizeAll: return "resizeAll";
        case CursorType_resizeNS: return "resizeNS";
        case CursorType_resizeEW: return "resizeEW";
        case CursorType_resizeNESW: return "resizeNESW";
        case CursorType_resizeNWSE: return "resizeNWSE";
        case CursorType_hand: return "hand";
        case CursorType_wait: return "wait";
        case CursorType_progress: return "progress";
        case CursorType_notAllowed: return "notAllowed";
        case CursorType_count: break;
    }
    return "?";
}

static void audioCallback(std::span<f32> buf, AudioConfig config)
{
    audio.update(buf, config);

    if (toneActive)
    {
        for (u64 i = 0; i < buf.size(); i += config.channels)
        {
            f32 val = std::sin(toneT * 440.0f * pif * 2.0f) * 0.3f;
            for (u32 c = 0; c < config.channels; ++c)
                buf[i + c] += val;
            toneT += 1.0f / assertCast<f32>(config.sampleRate);
        }
    }
}

static const char* backendName(AudioBackend backend)
{
    switch (backend)
    {
        case AudioBackend::none: return "none";
        case AudioBackend::pipewire: return "pipewire";
        case AudioBackend::wasapi: return "wasapi";
        case AudioBackend::sdl3: return "sdl3";
    }
    return "?";
}

static const char* inputBackendNameStr(InputBackend backend)
{
    switch (backend)
    {
        case InputBackend::none: return "none";
        case InputBackend::wayland: return "wayland";
        case InputBackend::x11: return "x11";
        case InputBackend::win32: return "win32";
        case InputBackend::sdl3: return "sdl3";
    }
    return "?";
}

static void switchInputBackend(InputBackend backend)
{
    // wait for in-flight frames, they still reference the resources about to be destroyed
    gpuWaitIdle();

    // release resources held by the app before the GPU goes away
    camera.vpBuffer = GpuBuffer{};

    // full teardown: windows and the GPU instance belong to the backend
    deinitImGui();
    secondWindow = nullptr;
    window = nullptr;
    input = nullptr;
    deinitGpu();

    auto rebuild = []() -> bool
    {
        input = InputDevice::create();
        if (input == nullptr)
        {
            inputBackendError = std::string{getError()};
            return false;
        }

        if (!initGpu(input->getVulkanExtensions()))
        {
            input = nullptr;
            inputBackendError = std::string{getError()};
            return false;
        }

        window = input->createWindow({});
        if (window == nullptr)
        {
            inputBackendError = std::string{getError()};
            input = nullptr;
            deinitGpu();
            return false;
        }

        window->setTitle("Platform API Test");
        return true;
    };

    InputDevice::hintBackend(backend);
    bool ok = rebuild();
    std::string firstError = inputBackendError;
    bool usedFallback = false;

    if (!ok)
    {
        // requested backend failed, fall back through the remaining ones
        usedFallback = true;
        const InputBackend fallbacks[] = {InputBackend::wayland, InputBackend::x11, InputBackend::win32, InputBackend::sdl3};
        for (InputBackend fallback : fallbacks)
        {
            if (fallback == backend)
                continue;
            InputDevice::hintBackend(fallback);
            if (rebuild())
            {
                ok = true;
                break;
            }
        }
    }

    if (!ok)
        HG_PANIC("Could not recreate input device: {}\n", inputBackendError.c_str());

    inputBackendName = inputBackendNameStr(input->getBackend());
    inputBackendError = usedFallback ? "requested backend failed: " + firstError : std::string{};

    window->size(&width, &height);
    camera.projection = Orthographic{{{0, 0}, {assertCast<f32>(width) / assertCast<f32>(height), 1.0f}}};
    camera.update();

    initImGui(*window, *input, window->swapchainFormat());
}

static bool switchAudioBackend(AudioBackend backend)
{
    // destroy first: a second pipewire device would otherwise share the old
    // device's thread loop, which its destructor tears down
    audioDevice = nullptr;

    AudioDevice::hintBackend(backend);
    std::unique_ptr<AudioDevice> newDevice = AudioDevice::create();
    if (newDevice == nullptr)
    {
        backendError = std::string{getError()};

        AudioDevice::hintBackend(AudioBackend::none);
        newDevice = AudioDevice::create();
        if (newDevice == nullptr)
        {
            backendError = std::string{getError()};
            return false;
        }
    }

    backendError = {};
    newDevice->setCallback(audioCallback, {.channels = 2, .sampleRate = 48000});
    audioDevice = std::move(newDevice);
    return true;
}

int main()
{
    input = InputDevice::create();
    if (input == nullptr)
        HG_PANIC("Could not create input device: {}\n", std::string{getError()}.c_str());
    inputBackendName = inputBackendNameStr(input->getBackend());

    if (!initGpu(input->getVulkanExtensions()))
        HG_PANIC("Could not initialize GPU: {}\n", std::string{getError()}.c_str());

    window = input->createWindow({});
    if (window == nullptr)
        HG_PANIC("Could not create window: {}\n", std::string{getError()}.c_str());
    window->setTitle("Platform API Test");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    initImGui(*window, *input, window->swapchainFormat());

    audioDevice = AudioDevice::create();
    if (audioDevice == nullptr)
        HG_PANIC("Could not create audio device: {}\n", std::string{getError()}.c_str());
    audioDevice->setCallback(audioCallback, {.channels = 2, .sampleRate = 48000});
    HG_DEFER(if (audioDevice) audioDevice->unsetCallback());

    window->size(&width, &height);
    camera.projection = Orthographic{{{0, 0}, {assertCast<f32>(width) / assertCast<f32>(height), 1.0f}}};

    bool showEvents = true;
    bool showKeyboard = true;
    bool showMousePanel = true;
    bool showWindowPanel = true;
    bool showAudio = true;
    bool showBackend = true;
    bool showDisplay = true;
    bool showCursorPanel = true;
    bool showTimers = true;

    Clock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();
        {
            ProfilerScopeTimer timer{"Cpu"};
                input->processEvents();
                if (input->wasQuit() || window->wasClosed())
                    goto quit;

                window->size(&width, &height);
                camera.projection = Orthographic{{{0, 0}, {assertCast<f32>(width) / assertCast<f32>(height), 1.0f}}};
                camera.update();

                beginImGuiFrame();
                ImGui::NewFrame();

                if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("Windows"))
                {
                    ImGui::MenuItem("Events", nullptr, &showEvents);
                    ImGui::MenuItem("Keyboard", nullptr, &showKeyboard);
                    ImGui::MenuItem("Mouse", nullptr, &showMousePanel);
                    ImGui::MenuItem("Window", nullptr, &showWindowPanel);
                    ImGui::MenuItem("Audio", nullptr, &showAudio);
                    ImGui::MenuItem("Backend", nullptr, &showBackend);
                    ImGui::MenuItem("Display", nullptr, &showDisplay);
                    ImGui::MenuItem("Cursor", nullptr, &showCursorPanel);
                    ImGui::MenuItem("Timers", nullptr, &showTimers);
                    ImGui::EndMenu();
                }
                ImGui::TextDisabled("---");
                ImGui::SameLine();
                ImGui::Text("%.1f FPS", 1.0 / delta);
                ImGui::EndMainMenuBar();
            }

            if (showEvents)
            {
                if (ImGui::Begin("Events", &showEvents))
                {
                    std::span<Event> events = input->getEvents();
                    ImGui::Text("Event count: %d", assertCast<int>(events.size()));
                    if (ImGui::BeginTable("events", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
                    {
                        ImGui::TableSetupColumn("Type");
                        ImGui::TableSetupColumn("Detail");
                        ImGui::TableSetupColumn("Value 1");
                        ImGui::TableSetupColumn("Value 2");
                        ImGui::TableHeadersRow();
                        for (u64 i = 0; i < events.size(); ++i)
                        {
                            const Event& e = events[i];
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(eventName(e.type));
                            ImGui::TableNextColumn();
                            switch (e.type)
                            {
                                case EventType_keyPress:
                                case EventType_keyRelease:
                                    ImGui::TextUnformatted(buttonName(e.button));
                                    break;
                                case EventType_mouseMoved:
                                    ImGui::Text("d: %.1f, %.1f", e.mouse.delta.x(), e.mouse.delta.y());
                                    ImGui::TableNextColumn();
                                    ImGui::Text("p: %.1f, %.1f", e.mouse.pos.x(), e.mouse.pos.y());
                                    ImGui::TableNextColumn();
                                    ImGui::Text("g: %.1f, %.1f", e.mouse.globalPos.x(), e.mouse.globalPos.y());
                                    break;
                                case EventType_wheelMoved:
                                    ImGui::Text("d: %.1f, %.1f", e.wheel.delta.x(), e.wheel.delta.y());
                                    break;
                                case EventType_text:
                                    ImGui::Text("\"%s\"", e.text);
                                    break;
                                default:
                                    break;
                            }
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::End();
            }

            if (showKeyboard)
            {
                if (ImGui::Begin("Keyboard", &showKeyboard))
                {
                    ImGui::Text("Pressed This Frame");
                    for (u32 i = 1; i < Button_count; ++i)
                    {
                        Button b = static_cast<Button>(i);
                        if (input->wasButtonPressed(b))
                        {
                            ImGui::SameLine();
                            ImGui::Text("%s", buttonName(b));
                        }
                    }

                    ImGui::Text("Released This Frame");
                    for (u32 i = 1; i < Button_count; ++i)
                    {
                        Button b = static_cast<Button>(i);
                        if (input->wasButtonReleased(b))
                        {
                            ImGui::SameLine();
                            ImGui::Text("%s", buttonName(b));
                        }
                    }

                    ImGui::Text("Held Down");
                    for (u32 i = 1; i < Button_count; ++i)
                    {
                        Button b = static_cast<Button>(i);
                        if (input->isButtonDown(b))
                        {
                            ImGui::SameLine();
                            ImGui::Text("[%s]", buttonName(b));
                        }
                    }
                }
                ImGui::End();
            }

            if (showMousePanel)
            {
                if (ImGui::Begin("Mouse", &showMousePanel))
                {
                    Vec2 mp = input->mousePos();
                    Vec2 md = input->mouseDelta();
                    Vec2 wd = input->wheelDelta();

                    ImGui::Text("pos: %.1f, %.1f", mp.x(), mp.y());
                    ImGui::Text("delta: %.1f, %.1f", md.x(), md.y());
                    ImGui::Text("wheel: %.1f, %.1f", wd.x(), wd.y());

                    ImGui::Text("Buttons");
                    for (u32 i = static_cast<u32>(Button_mouse1); i <= static_cast<u32>(Button_mouse5); ++i)
                    {
                        Button b = static_cast<Button>(i);
                        ImGui::Text("%s: down=%d pressed=%d released=%d",
                            buttonName(b),
                            input->isButtonDown(b),
                            input->wasButtonPressed(b),
                            input->wasButtonReleased(b));
                    }

                    if (secondWindow != nullptr)
                    {
                        ImGui::Text("Second Window Mouse");
                        Vec2 swmp = secondWindow->mousePos();
                        Vec2 swmd = secondWindow->mouseDelta();
                        ImGui::Text("pos: %.1f, %.1f  delta: %.1f, %.1f",
                            swmp.x(),
                            swmp.y(),
                            swmd.x(),
                            swmd.y());
                    }
                }
                ImGui::End();
            }

            if (showWindowPanel)
            {
                if (ImGui::Begin("Window", &showWindowPanel))
                {
                    ImGui::Text("State");
                    ImGui::Text("isFocused: %d", window->isFocused());
                    ImGui::Text("wasFocusGained: %d", window->wasFocusGained());
                    ImGui::Text("wasFocusLost: %d", window->wasFocusLost());
                    ImGui::Text("wasResized: %d", window->wasResized());
                    ImGui::Text("wasClosed: %d", window->wasClosed());

                    u32 ww, wh;
                    window->size(&ww, &wh);
                    ImGui::Text("size: %u, %u", ww, wh);

                    if (secondWindow != nullptr)
                    {
                        ImGui::Text("Second Window");
                        ImGui::Text("focused=%d resized=%d closed=%d",
                            secondWindow->isFocused(),
                            secondWindow->wasResized(), secondWindow->wasClosed());
                        u32 sw, sh;
                        secondWindow->size(&sw, &sh);
                        ImGui::Text("size: %u, %u", sw, sh);
                    }

                    ImGui::Text("Actions");
                    if (ImGui::Button("Fullscreen On")) window->setFullscreen(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Fullscreen Off")) window->setFullscreen(false);

                    ImGui::Text("Second Window");
                    if (secondWindow == nullptr)
                    {
                        if (ImGui::Button("Create"))
                        {
                            secondWindow = input->createWindow({});
                            if (secondWindow != nullptr)
                                secondWindow->setTitle("Second Window");
                        }
                    }
                    else
                    {
                        if (ImGui::Button("Close"))
                            secondWindow = nullptr;
                        ImGui::SameLine();
                        if (ImGui::Button("SetTitle"))
                            secondWindow->setTitle("Renamed!");
                    }

                    std::span<Event> wevts = window->events();
                    ImGui::Text("Window Events");
                    ImGui::Text("Count: %d", assertCast<int>(wevts.size()));
                }
                ImGui::End();
            }

            if (showAudio)
            {
                if (ImGui::Begin("Audio", &showAudio))
                {
                    if (!toneActive)
                    {
                        if (ImGui::Button("Start Tone (440Hz)"))
                        {
                            toneActive = true;
                        }
                    }
                    else
                    {
                        if (ImGui::Button("Stop Tone"))
                        {
                            toneActive = false;
                        }
                    }

                    ImGui::Text("AudioPlayer");
                    if (ImGui::Button("Play Sound"))
                    {
                        static Asset<Sound> snd;
                        if (snd.data == nullptr)
                        {
                            snd = newAsset<Sound>();
                            snd->data = {0, 8000};
                            snd->frequency = 48000;
                            snd->channels = 1;

                            for (u64 i = 0; i < 8000; ++i)
                            {
                                f32 t = assertCast<f32>(i) * pif * 2.0f / 8000.0f;
                                snd->data.emplace_back(noiseNorm(42u, t) / (t + 0.1f));
                            }
                        }
                        audio.playSound(snd, 0.5f);
                    }

                    static Asset<Sound> mus;
                    if (ImGui::Button("Play Music"))
                    {
                        if (mus.data == nullptr)
                        {
                            mus = newAsset<Sound>();
                            mus->data = {0, 48000 * 2};
                            mus->frequency = 48000;
                            mus->channels = 1;

                            for (u64 i = 0; i < 48000 * 2; ++i)
                            {
                                f32 t = assertCast<f32>(i) * pif * 2.0f / 48000.0f;
                                f32 val = 0;
                                for (u32 j = 1; j <= 8; ++j)
                                {
                                    f32 x = assertCast<f32>(j);
                                    val += 0.3f / x * std::sin(220.f * t * x);
                                }
                                mus->data.emplace_back(val);
                            }
                        }
                        audio.playMusic(mus, musicGain);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Pause Music"))
                    {
                        if (mus.data != nullptr)
                            audio.pauseMusic(mus);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Kill Music"))
                    {
                        if (mus.data != nullptr)
                            audio.killMusic(mus);
                    }

                    if (ImGui::SliderFloat("Gain", &musicGain, 0.0f, 2.0f))
                    {
                        if (mus.data != nullptr)
                            audio.setMusicGain(mus, musicGain);
                    }
                }
                ImGui::End();
            }

            if (showBackend)
            {
                if (ImGui::Begin("Backend", &showBackend))
                {
                    ImGui::Text("Input");
                    ImGui::Text("Current: %s", inputBackendName.c_str());

                    ImGui::PushID("InputBackend");
                    if (ImGui::Button("Wayland"))
                        switchInputBackend(InputBackend::wayland);
                    ImGui::SameLine();
                    if (ImGui::Button("X11"))
                        switchInputBackend(InputBackend::x11);
                    ImGui::SameLine();
                    if (ImGui::Button("Win32"))
                        switchInputBackend(InputBackend::win32);
                    ImGui::SameLine();
                    if (ImGui::Button("SDL3"))
                        switchInputBackend(InputBackend::sdl3);
                    ImGui::PopID();

                    if (inputBackendError.length() > 0)
                        ImGui::Text("Error: %s", inputBackendError.c_str());

                    ImGui::Separator();

                    ImGui::Text("Audio");
                    if (audioDevice != nullptr)
                        ImGui::Text("Current: %s", backendName(audioDevice->getBackend()));
                    else
                        ImGui::Text("Current: none");

                    ImGui::PushID("AudioBackend");
                    if (ImGui::Button("Pipewire"))
                        switchAudioBackend(AudioBackend::pipewire);
                    ImGui::SameLine();
                    if (ImGui::Button("SDL3"))
                        switchAudioBackend(AudioBackend::sdl3);
                    ImGui::SameLine();
                    if (ImGui::Button("WASAPI"))
                        switchAudioBackend(AudioBackend::wasapi);
                    ImGui::PopID();
                    if (backendError.length() > 0)
                        ImGui::Text("Error: %s", backendError.c_str());
                }
                ImGui::End();
            }

            if (showDisplay)
            {
                if (ImGui::Begin("Display", &showDisplay))
                {
                    std::span<DisplayInfo> displays = input->displayInfo();
                    ImGui::Text("Count: %d", assertCast<int>(displays.size()));
                    for (u64 i = 0; i < displays.size(); ++i)
                    {
                        if (ImGui::TreeNodeEx((void*)i, 0, "Display %d", assertCast<int>(i)))
                        {
                            const DisplayInfo& d = displays[i];
                            ImGui::Text("pos: %d, %d  size: %u, %u", d.posX, d.posY, d.sizeW, d.sizeH);
                            ImGui::Text("work: %d, %d  %u, %u", d.workPosX, d.workPosY, d.workSizeW, d.workSizeH);
                            ImGui::Text("dpiScale: %.2f", d.dpiScale);
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::End();
            }

            if (showCursorPanel)
            {
                if (ImGui::Begin("Cursor", &showCursorPanel))
                {
                    for (u32 i = 0; i < CursorType_count; ++i)
                    {
                        CursorType t = static_cast<CursorType>(i);
                        if (i > 0) ImGui::SameLine();
                        if (ImGui::Button(cursorName(t)))
                            input->setCursor(t);
                    }
                    ImGui::Separator();
                    if (ImGui::Button("Show")) input->showCursor(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Hide")) input->showCursor(false);
                }
                ImGui::End();
            }
        }

        if (showTimers)
        {
            if (ImGui::Begin("Timers", &showTimers))
            {
                ImGui::Text("%.1f FPS (%.3fms)", 1.0 / delta, delta * 1000.0);
                ImGui::Separator();
                Profiler::forEachTimer([&](const std::string& name, f64 time)
                {
                    ImGui::Text("%.*s: %.3fms", assertCast<int>(name.length()), name.data(), time * 1.e3f);
                });
                Profiler::forEachCounter([&](const std::string& name, u32 count)
                {
                    ImGui::Text("%.*s: %d", assertCast<int>(name.length()), name.data(), count);
                });
                Profiler::clear();
            }
            ImGui::End();
        }

        ImGui::Render();

        GpuCmd* cmd;
        {
            ProfilerScopeTimer timer{"Gpu"};
            if (secondWindow != nullptr)
            {
                Window* windows[] = {window.get(), secondWindow.get()};
                cmd = gpuBeginFrame(windows);
            }
            else
            {
                Window* windows[] = {window.get()};
                cmd = gpuBeginFrame(windows);
            }
        }

        if (window->renderTarget() != nullptr)
        {
            ProfilerScopeTimer timer{"Cpu"};
            GpuAttachment colorAttachment{};
            colorAttachment.image = window->renderTarget();
            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};
            gpuBeginRenderPass(cmd, pass);
            renderImGui(cmd);
            gpuEndRenderPass(cmd);
        }

        if (secondWindow != nullptr && secondWindow->renderTarget() != nullptr)
        {
            ProfilerScopeTimer timer{"Cpu"};
            GpuAttachment colorAttachment{};
            colorAttachment.image = secondWindow->renderTarget();
            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};
            gpuBeginRenderPass(cmd, pass);
            gpuEndRenderPass(cmd);
        }

        {
            ProfilerScopeTimer timer{"Gpu"};
            gpuEndFrame(cmd);
        }
    }

quit:
    gpuWaitIdle();
    camera.vpBuffer = GpuBuffer{};
    secondWindow = nullptr;
    window = nullptr;
    deinitImGui();
    input = nullptr;
    deinitGpu();
    ImGui::DestroyContext();
}
