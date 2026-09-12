#include <hurdygurdy.hpp>

using namespace hg;

#define IM_ASSERT HG_ASSERT
#include "imgui.h"

static AudioPlayer audio{};
static bool toneActive = false;
static f32 toneT = 0.0f;
static char clipboardBuf[256]{};
static f32 musicGain = 0.5f;

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
        case EventType_gamepadConnected: return "gamepadConnected";
        case EventType_gamepadDisconnected: return "gamepadDisconnected";
        case EventType_gamepadPress: return "gamepadPress";
        case EventType_gamepadRelease: return "gamepadRelease";
        case EventType_gamepadLeftStick: return "gamepadLeftStick";
        case EventType_gamepadRightStick: return "gamepadRightStick";
        case EventType_gamepadLeftTrigger: return "gamepadLeftTrigger";
        case EventType_gamepadRightTrigger: return "gamepadRightTrigger";
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

static const char* gamepadButtonName(GamepadButton b)
{
    switch (b)
    {
        case GamepadButton_none: return "none";
        case GamepadButton_south: return "south";
        case GamepadButton_east: return "east";
        case GamepadButton_west: return "west";
        case GamepadButton_north: return "north";
        case GamepadButton_back: return "back";
        case GamepadButton_guide: return "guide";
        case GamepadButton_start: return "start";
        case GamepadButton_leftStick: return "leftStick";
        case GamepadButton_rightStick: return "rightStick";
        case GamepadButton_leftShoulder: return "leftShoulder";
        case GamepadButton_rightShoulder: return "rightShoulder";
        case GamepadButton_dpadUp: return "dpadUp";
        case GamepadButton_dpadDown: return "dpadDown";
        case GamepadButton_dpadLeft: return "dpadLeft";
        case GamepadButton_dpadRight: return "dpadRight";
        case GamepadButton_count: break;
    }
    return "?";
}

static void audioCallback(void* userData, Span<f32> buf, AudioConfig config)
{
    static_cast<AudioPlayer*>(userData)->update(buf, config);

    if (toneActive)
    {
        for (u64 i = 0; i < buf.count; i += config.channels)
        {
            f32 val = std::sin(toneT * 440.0f * pif * 2.0f) * 0.3f;
            for (u32 c = 0; c < config.channels; ++c)
                buf[i + c] += val;
            toneT += 1.0f / static_cast<f32>(config.sampleRate);
        }
    }
}

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    Window window = Window::create();
    window.setTitle("Platform API Test");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    HG_DEFER(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    initImGui(window, window.swapchain().format());
    HG_DEFER(deinitImGui());

    setAudioCallback(audioCallback, &audio, {.channels = 2, .sampleRate = 48000});
    HG_DEFER(unsetAudioCallback());

    u32 width, height;
    window.size(&width, &height);
    Camera camera{};
    camera.setOrthographic(static_cast<f32>(width) / static_cast<f32>(height), 1.0f);

    bool showEvents = true;
    bool showKeyboard = true;
    bool showMousePanel = true;
    bool showGamepad = true;
    bool showWindowPanel = true;
    bool showAudio = true;
    bool showDisplay = true;
    bool showCursorPanel = true;
    bool showClipboard = true;
    bool showTimers = true;

    Window secondWindow{};

    Clock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();
        {
            ProfilerScopeTimer timer{"Cpu"};
                processEvents();
                if (wasQuit() || window.wasClosed())
                    goto quit;

                window.size(&width, &height);
                camera.setOrthographic(static_cast<f32>(width) / static_cast<f32>(height), 1.0f);
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
                    ImGui::MenuItem("Gamepad", nullptr, &showGamepad);
                    ImGui::MenuItem("Window", nullptr, &showWindowPanel);
                    ImGui::MenuItem("Audio", nullptr, &showAudio);
                    ImGui::MenuItem("Display", nullptr, &showDisplay);
                    ImGui::MenuItem("Cursor", nullptr, &showCursorPanel);
                    ImGui::MenuItem("Clipboard", nullptr, &showClipboard);
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
                    Span<Event> events = getEvents();
                    ImGui::Text("Event count: %d", (int)events.count);
                    if (ImGui::BeginTable("events", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
                    {
                        ImGui::TableSetupColumn("Type");
                        ImGui::TableSetupColumn("Detail");
                        ImGui::TableSetupColumn("Value 1");
                        ImGui::TableSetupColumn("Value 2");
                        ImGui::TableHeadersRow();
                        for (u64 i = 0; i < events.count; ++i)
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
                                    ImGui::Text("d: %.1f, %.1f", e.mouse.delta.x, e.mouse.delta.y);
                                    ImGui::TableNextColumn();
                                    ImGui::Text("p: %.1f, %.1f", e.mouse.pos.x, e.mouse.pos.y);
                                    ImGui::TableNextColumn();
                                    ImGui::Text("g: %.1f, %.1f", e.mouse.globalPos.x, e.mouse.globalPos.y);
                                    break;
                                case EventType_wheelMoved:
                                    ImGui::Text("d: %.1f, %.1f", e.wheel.delta.x, e.wheel.delta.y);
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
                        if (wasButtonPressed(b))
                        {
                            ImGui::SameLine();
                            ImGui::Text("%s", buttonName(b));
                        }
                    }

                    ImGui::Text("Released This Frame");
                    for (u32 i = 1; i < Button_count; ++i)
                    {
                        Button b = static_cast<Button>(i);
                        if (wasButtonReleased(b))
                        {
                            ImGui::SameLine();
                            ImGui::Text("%s", buttonName(b));
                        }
                    }

                    ImGui::Text("Held Down");
                    for (u32 i = 1; i < Button_count; ++i)
                    {
                        Button b = static_cast<Button>(i);
                        if (isButtonDown(b))
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
                    Vec2 mp = mousePos();
                    Vec2 md = mouseDelta();
                    Vec2 wd = wheelDelta();

                    ImGui::Text("pos: %.1f, %.1f", mp.x, mp.y);
                    ImGui::Text("delta: %.1f, %.1f", md.x, md.y);
                    ImGui::Text("wheel: %.1f, %.1f", wd.x, wd.y);

                    ImGui::Text("Buttons");
                    for (u32 i = static_cast<u32>(Button_mouse1); i <= static_cast<u32>(Button_mouse5); ++i)
                    {
                        Button b = static_cast<Button>(i);
                        ImGui::Text("%s: down=%d pressed=%d released=%d",
                            buttonName(b), isButtonDown(b), wasButtonPressed(b), wasButtonReleased(b));
                    }

                    if (secondWindow.data != nullptr)
                    {
                        ImGui::Text("Second Window Mouse");
                        Vec2 swmp = secondWindow.mousePos();
                        Vec2 swmd = secondWindow.mouseDelta();
                        ImGui::Text("pos: %.1f, %.1f  delta: %.1f, %.1f", swmp.x, swmp.y, swmd.x, swmd.y);
                    }
                }
                ImGui::End();
            }

            if (showGamepad)
            {
                if (ImGui::Begin("Gamepad", &showGamepad))
                {
                    u32 count = connectedGamepadCount();
                    ImGui::Text("Connected: %u", count);

                    for (u32 g = 0; g < count; ++g)
                    {
                        if (ImGui::TreeNodeEx((void*)(uintptr_t)g, 0, "Gamepad %u", g))
                        {
                            ImGui::Text("Connected: %s", isGamepadConnected(g) ? "yes" : "no");

                            ImGui::Text("Buttons");
                            for (u32 i = 1; i < GamepadButton_count; ++i)
                            {
                                GamepadButton b = static_cast<GamepadButton>(i);
                                bool down = isGamepadButtonDown(g, static_cast<Button>(b));
                                bool pressed = wasGamepadButtonPressed(g, b);
                                bool released = wasGamepadButtonReleased(g, b);
                                if (down || pressed || released)
                                    ImGui::Text("%s: d=%d p=%d r=%d", gamepadButtonName(b), down, pressed, released);
                            }

                            ImGui::Text("Sticks");
                            Vec2 ls = gamepadLeftStick(g);
                            Vec2 rs = gamepadRightStick(g);
                            ImGui::Text("Left:  %.3f, %.3f", ls.x, ls.y);
                            ImGui::Text("Right: %.3f, %.3f", rs.x, rs.y);

                            ImGui::Text("Triggers");
                            ImGui::Text("Left:  %.3f", gamepadLeftTrigger(g));
                            ImGui::Text("Right: %.3f", gamepadRightTrigger(g));

                            ImGui::TreePop();
                        }
                    }

                    if (count == 0)
                        ImGui::TextDisabled("No gamepads connected");
                }
                ImGui::End();
            }

            if (showWindowPanel)
            {
                if (ImGui::Begin("Window", &showWindowPanel))
                {
                    ImGui::Text("State");
                    ImGui::Text("isFocused: %d", window.isFocused());
                    ImGui::Text("wasFocusGained: %d", window.wasFocusGained());
                    ImGui::Text("wasFocusLost: %d", window.wasFocusLost());
                    ImGui::Text("wasResized: %d", window.wasResized());
                    ImGui::Text("wasClosed: %d", window.wasClosed());

                    u32 ww, wh;
                    window.size(&ww, &wh);
                    ImGui::Text("size: %u, %u", ww, wh);

                    if (secondWindow.data != nullptr)
                    {
                        ImGui::Text("Second Window");
                        ImGui::Text("focused=%d resized=%d closed=%d",
                            secondWindow.isFocused(),
                            secondWindow.wasResized(), secondWindow.wasClosed());
                        u32 sw, sh;
                        secondWindow.size(&sw, &sh);
                        ImGui::Text("size: %u, %u", sw, sh);
                    }

                    ImGui::Text("Actions");
                    if (ImGui::Button("Maximize")) window.maximize();
                    ImGui::SameLine();
                    if (ImGui::Button("Minimize")) window.minimize();
                    ImGui::SameLine();
                    if (ImGui::Button("Restore")) window.restore();

                    if (ImGui::Button("Fullscreen On")) window.setFullscreen(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Fullscreen Off")) window.setFullscreen(false);

                    ImGui::Text("Second Window");
                    if (secondWindow.data == nullptr)
                    {
                        if (ImGui::Button("Create"))
                        {
                            secondWindow = Window::create();
                            secondWindow.setTitle("Second Window");
                        }
                    }
                    else
                    {
                        if (ImGui::Button("Close"))
                            secondWindow = Window{};
                        ImGui::SameLine();
                        if (ImGui::Button("SetTitle"))
                            secondWindow.setTitle("Renamed!");
                    }

                    Span<Event> wevts = window.events();
                    ImGui::Text("Window Events");
                    ImGui::Text("Count: %d", (int)wevts.count);
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
                            static f32 soundData[8000];
                            for (u64 i = 0; i < size(soundData); ++i)
                            {
                                f32 t = static_cast<f32>(i) * pif * 2.0f / 8000.0f;
                                soundData[i] = noiseNorm(42u, t) / (t + 0.1f);
                            }
                            snd = newAsset<Sound>();
                            snd->data = soundData;
                            snd->frequency = 48000;
                            snd->channels = 1;
                        }
                        audio.playSound(snd, 0.5f);
                    }

                    if (ImGui::Button("Play Music"))
                    {
                        static Asset<Sound> mus;
                        if (mus.data == nullptr)
                        {
                            static f32 musicData[48000 * 2];
                            for (u64 i = 0; i < size(musicData); ++i)
                            {
                                f32 t = static_cast<f32>(i) * pif * 2.0f / 48000.0f;
                                musicData[i] = 0;
                                for (u32 j = 1; j <= 8; ++j)
                                {
                                    f32 x = static_cast<f32>(j);
                                    musicData[i] += 0.3f / x * std::sin(220.f * t * x);
                                }
                            }
                            mus = newAsset<Sound>();
                            mus->data = musicData;
                            mus->frequency = 48000;
                            mus->channels = 1;
                        }
                        audio.playMusic(mus, musicGain);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Pause Music"))
                    {
                        if (audio.music.count > 0)
                            audio.pauseMusic(audio.music.vals[0].asset);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Kill Music"))
                    {
                        if (audio.music.count > 0)
                            audio.killMusic(audio.music.vals[0].asset);
                    }

                    if (ImGui::SliderFloat("Gain", &musicGain, 0.0f, 2.0f))
                    {
                        if (audio.music.count > 0)
                            audio.setMusicGain(audio.music.vals[0].asset, musicGain);
                    }
                }
                ImGui::End();
            }

            if (showDisplay)
            {
                if (ImGui::Begin("Display", &showDisplay))
                {
                    Span<DisplayInfo> displays = displayInfo();
                    ImGui::Text("Count: %d", (int)displays.count);
                    for (u64 i = 0; i < displays.count; ++i)
                    {
                        if (ImGui::TreeNodeEx((void*)i, 0, "Display %d", (int)i))
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
                            setCursor(t);
                    }
                    ImGui::Separator();
                    if (ImGui::Button("Show")) hg::showCursor(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Hide")) hg::showCursor(false);
                }
                ImGui::End();
            }

            if (showClipboard)
            {
                if (ImGui::Begin("Clipboard", &showClipboard))
                {
                    if (ImGui::Button("Get"))
                    {
                        StringView sv = getClipboardText();
                        u64 len = sv.length < sizeof(clipboardBuf) - 1 ? sv.length : sizeof(clipboardBuf) - 1;
                        memcpy(clipboardBuf, sv.chars, len);
                        clipboardBuf[len] = '\0';
                    }
                    ImGui::InputText("##clip", clipboardBuf, sizeof(clipboardBuf));
                    if (ImGui::Button("Set"))
                        setClipboardText(clipboardBuf);
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

        ImGui::Render();

        GpuCmd* cmd;
        {
            ProfilerScopeTimer timer{"Gpu"};
            if (secondWindow.data != nullptr)
            {
                Window* windows[] = {&window, &secondWindow};
                cmd = gpuBeginFrame(windows);
            }
            else
            {
                Window* windows[] = {&window};
                cmd = gpuBeginFrame(windows);
            }
        }

        if (window.swapchain().renderTarget() != nullptr)
        {
            ProfilerScopeTimer timer{"Cpu"};
            GpuAttachment colorAttachment{};
            colorAttachment.image = window.swapchain().renderTarget();
            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};
            gpuBeginRenderPass(cmd, pass);
            renderImGui(cmd);
            gpuEndRenderPass(cmd);
        }

        if (secondWindow.data != nullptr && secondWindow.swapchain().renderTarget() != nullptr)
        {
            ProfilerScopeTimer timer{"Cpu"};
            GpuAttachment colorAttachment{};
            colorAttachment.image = secondWindow.swapchain().renderTarget();
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
    secondWindow = Window{};
    gpuWaitIdle();
}
