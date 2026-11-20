#include "hg/imgui.hpp"

#include "internal.hpp"

#include "hg/utility.hpp"
#include "hg/window.hpp"
#include "hg/time.hpp"

#include <imgui.h>
#include <math.h>

namespace hg {

struct ImGuiState {
    Window* window = nullptr;
    InputDevice* device = nullptr;
    Clock clock{};
    CursorType cursors[ImGuiMouseCursor_COUNT]{};
    CursorType lastCursor = CursorType::count;
};

static ImGuiState state;

static ImGuiKey hgButtonToImGuiKey(Button button)
{
    switch (button)
    {
        case Button::b0: return ImGuiKey_0;
        case Button::b1: return ImGuiKey_1;
        case Button::b2: return ImGuiKey_2;
        case Button::b3: return ImGuiKey_3;
        case Button::b4: return ImGuiKey_4;
        case Button::b5: return ImGuiKey_5;
        case Button::b6: return ImGuiKey_6;
        case Button::b7: return ImGuiKey_7;
        case Button::b8: return ImGuiKey_8;
        case Button::b9: return ImGuiKey_9;
        case Button::a: return ImGuiKey_A;
        case Button::b: return ImGuiKey_B;
        case Button::c: return ImGuiKey_C;
        case Button::d: return ImGuiKey_D;
        case Button::e: return ImGuiKey_E;
        case Button::f: return ImGuiKey_F;
        case Button::g: return ImGuiKey_G;
        case Button::h: return ImGuiKey_H;
        case Button::i: return ImGuiKey_I;
        case Button::j: return ImGuiKey_J;
        case Button::k: return ImGuiKey_K;
        case Button::l: return ImGuiKey_L;
        case Button::m: return ImGuiKey_M;
        case Button::n: return ImGuiKey_N;
        case Button::o: return ImGuiKey_O;
        case Button::p: return ImGuiKey_P;
        case Button::q: return ImGuiKey_Q;
        case Button::r: return ImGuiKey_R;
        case Button::s: return ImGuiKey_S;
        case Button::t: return ImGuiKey_T;
        case Button::u: return ImGuiKey_U;
        case Button::v: return ImGuiKey_V;
        case Button::w: return ImGuiKey_W;
        case Button::x: return ImGuiKey_X;
        case Button::y: return ImGuiKey_Y;
        case Button::z: return ImGuiKey_Z;
        case Button::f1: return ImGuiKey_F1;
        case Button::f2: return ImGuiKey_F2;
        case Button::f3: return ImGuiKey_F3;
        case Button::f4: return ImGuiKey_F4;
        case Button::f5: return ImGuiKey_F5;
        case Button::f6: return ImGuiKey_F6;
        case Button::f7: return ImGuiKey_F7;
        case Button::f8: return ImGuiKey_F8;
        case Button::f9: return ImGuiKey_F9;
        case Button::f10: return ImGuiKey_F10;
        case Button::f11: return ImGuiKey_F11;
        case Button::f12: return ImGuiKey_F12;
        case Button::escape: return ImGuiKey_Escape;
        case Button::space: return ImGuiKey_Space;
        case Button::enter: return ImGuiKey_Enter;
        case Button::backspace: return ImGuiKey_Backspace;
        case Button::kdelete: return ImGuiKey_Delete;
        case Button::insert: return ImGuiKey_Insert;
        case Button::tab: return ImGuiKey_Tab;
        case Button::home: return ImGuiKey_Home;
        case Button::end: return ImGuiKey_End;
        case Button::pageup: return ImGuiKey_PageUp;
        case Button::pagedown: return ImGuiKey_PageDown;
        case Button::up: return ImGuiKey_UpArrow;
        case Button::down: return ImGuiKey_DownArrow;
        case Button::left: return ImGuiKey_LeftArrow;
        case Button::right: return ImGuiKey_RightArrow;
        case Button::lshift: return ImGuiKey_LeftShift;
        case Button::rshift: return ImGuiKey_RightShift;
        case Button::lctrl: return ImGuiKey_LeftCtrl;
        case Button::rctrl: return ImGuiKey_RightCtrl;
        case Button::lalt: return ImGuiKey_LeftAlt;
        case Button::ralt: return ImGuiKey_RightAlt;
        case Button::lsuper: return ImGuiKey_LeftSuper;
        case Button::rsuper: return ImGuiKey_RightSuper;
        case Button::capslock: return ImGuiKey_CapsLock;
        case Button::numlock: return ImGuiKey_NumLock;
        case Button::scrolllock: return ImGuiKey_ScrollLock;
        case Button::pause: return ImGuiKey_Pause;
        case Button::semicolon: return ImGuiKey_Semicolon;
        case Button::comma: return ImGuiKey_Comma;
        case Button::period: return ImGuiKey_Period;
        case Button::slash: return ImGuiKey_Slash;
        case Button::backslash: return ImGuiKey_Backslash;
        case Button::equal: return ImGuiKey_Equal;
        case Button::grave: return ImGuiKey_GraveAccent;
        case Button::apostrophe: return ImGuiKey_Apostrophe;
        case Button::lbracket: return ImGuiKey_LeftBracket;
        case Button::rbracket: return ImGuiKey_RightBracket;
        case Button::minus: return ImGuiKey_Minus;
        default: return ImGuiKey_None;
    }
}

void initImGui(
    Window* window,
    InputDevice* device,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();

    state.window = const_cast<Window*>(window);
    state.device = device;
    state.clock = Clock{};
    state.lastCursor = CursorType::count;

    io.BackendPlatformName = "imgui_hurdygurdy";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    state.cursors[ImGuiMouseCursor_Arrow] = CursorType::arrow;
    state.cursors[ImGuiMouseCursor_TextInput] = CursorType::textInput;
    state.cursors[ImGuiMouseCursor_ResizeAll] = CursorType::resizeAll;
    state.cursors[ImGuiMouseCursor_ResizeNS] = CursorType::resizeNS;
    state.cursors[ImGuiMouseCursor_ResizeEW] = CursorType::resizeEW;
    state.cursors[ImGuiMouseCursor_ResizeNESW] = CursorType::resizeNESW;
    state.cursors[ImGuiMouseCursor_ResizeNWSE] = CursorType::resizeNWSE;
    state.cursors[ImGuiMouseCursor_Hand] = CursorType::hand;
    state.cursors[ImGuiMouseCursor_Wait] = CursorType::wait;
    state.cursors[ImGuiMouseCursor_Progress] = CursorType::progress;
    state.cursors[ImGuiMouseCursor_NotAllowed] = CursorType::notAllowed;

    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();

    pio.Monitors.resize(0);
    std::span<DisplayInfo> displays = state.device->displayInfo();
    for (u32 i = 0; i < displays.size(); i++)
    {
        ImGuiPlatformMonitor monitor;

        monitor.DpiScale = displays[i].dpiScale;
        if (monitor.DpiScale <= 0.0f)
            continue;

        monitor.MainPos = monitor.WorkPos = ImVec2(
            assertCast<f32>(displays[i].posX),
            assertCast<f32>(displays[i].posY));
        monitor.MainSize = monitor.WorkSize = ImVec2(
            assertCast<f32>(displays[i].sizeW),
            assertCast<f32>(displays[i].sizeH));

        if (displays[i].workSizeW > 0 && displays[i].workSizeH > 0)
        {
            monitor.WorkPos = ImVec2(
                assertCast<f32>(displays[i].workPosX),
                assertCast<f32>(displays[i].workPosY));
            monitor.WorkSize = ImVec2(
                assertCast<f32>(displays[i].workSizeW),
                assertCast<f32>(displays[i].workSizeH));
        }

        monitor.PlatformHandle = reinterpret_cast<void*>(assertCast<intptr_t>(i));

        pio.Monitors.push_back(monitor);
    }

    internal::initImGuiGpu(
        window->swapchain(),
        colorFormat,
        depthFormat,
        stencilFormat);

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformUserData = state.window;
    mainViewport->PlatformHandle = state.window;
}

void deinitImGui()
{
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformUserData = nullptr;
    mainViewport->PlatformHandle = nullptr;

    internal::deinitImGuiGpu();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors
        | ImGuiBackendFlags_HasSetMousePos);

    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.ClearPlatformHandlers();
}

void beginImGuiFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    for (const Event& event : state.device->getEvents())
    {
        switch (event.type)
        {
            case EventType::keyPress:
            case EventType::keyRelease:
            {
                bool down = (event.type == EventType::keyPress);
                int mouseButton = -1;

                switch (event.button)
                {
                    case Button::mouse1: mouseButton = 0; break;
                    case Button::mouse2: mouseButton = 1; break;
                    case Button::mouse3: mouseButton = 2; break;
                    case Button::mouse4: mouseButton = 3; break;
                    case Button::mouse5: mouseButton = 4; break;
                    default: break;
                }

                if (mouseButton != -1)
                {
                    io.AddMouseButtonEvent(mouseButton, down);
                }
                else
                {
                    ImGuiKey key = hgButtonToImGuiKey(event.button);
                    if (key != ImGuiKey_None)
                        io.AddKeyEvent(key, down);

                    switch (event.button)
                    {
                        case Button::lctrl:
                        case Button::rctrl:
                            io.AddKeyEvent(ImGuiMod_Ctrl, down); break;
                        case Button::lshift:
                        case Button::rshift:
                            io.AddKeyEvent(ImGuiMod_Shift, down); break;
                        case Button::lalt:
                        case Button::ralt:
                            io.AddKeyEvent(ImGuiMod_Alt, down); break;
                        case Button::lsuper:
                        case Button::rsuper:
                            io.AddKeyEvent(ImGuiMod_Super, down); break;
                        default: break;
                    }
                }
            } break;

            case EventType::text:
            {
                io.AddInputCharactersUTF8(event.text);
            } break;

            default:
                break;
        }
    }

    Vec2 mouse = state.window->mousePos();
    io.AddMousePosEvent(mouse.x(), mouse.y());

    u32 width, height;
    state.window->size(&width, &height);
    io.DisplaySize = ImVec2(assertCast<f32>(width), assertCast<f32>(height));

    io.DeltaTime = assertCast<f32>(state.clock.tick());
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 1.0f / 60.0f;

    Vec2 wheel = state.device->wheelDelta();
    if (wheel.x() != 0.0f || wheel.y() != 0.0f)
        io.AddMouseWheelEvent(wheel.x(), wheel.y());

    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None)
        {
            state.device->showCursor(false);
        }
        else
        {
            CursorType expected = state.cursors[imguiCursor];
            if (state.lastCursor != expected)
            {
                state.device->setCursor(expected);
                state.lastCursor = expected;
            }
            state.device->showCursor(true);
        }
    }

    internal::beginImGuiFrameGpu();
}

} // namespace hg
