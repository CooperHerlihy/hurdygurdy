#pragma once

#include "hg/window.hpp"
#include "hg/gpu.hpp"
#include "hg/span.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"

namespace hg::wayland {

bool loadWayland();
bool initWayland();
void deinitWayland();
Span<StringView> getPlatformVulkanExtensions(Arena* arena);

Span<DisplayInfo> displayInfo();
void setCursor(CursorType type);
void showCursor(bool show);
void processEvents();
Span<Event> getEvents();
bool wasQuit();

u32 connectedGamepadCount();
bool isGamepadConnected(u32 gamepad);
bool isGamepadButtonDown(u32 gamepad, Button key);
bool wasGamepadButtonPressed(u32 gamepad, GamepadButton key);
bool wasGamepadButtonReleased(u32 gamepad, GamepadButton key);
Vec2 gamepadLeftStick(u32 gamepad);
Vec2 gamepadRightStick(u32 gamepad);
f32 gamepadLeftTrigger(u32 gamepad);
f32 gamepadRightTrigger(u32 gamepad);

bool isButtonDown(Button key);
bool wasButtonPressed(Button key);
bool wasButtonReleased(Button key);
Vec2 mousePos();
Vec2 mouseDelta();
Vec2 wheelDelta();

Window windowCreate(const WindowConfig& config);
void windowDestroy(void* data);
GpuSwapchain& windowSwapchain(void* data);
void windowSetTitle(void* data, StringView title);
Span<Event> windowEvents(void* data);
bool windowWasClosed(void* data);
bool windowIsFocused(void* data);
bool windowWasFocusGained(void* data);
bool windowWasFocusLost(void* data);




bool windowWasResized(void* data);
void windowGetSize(void* data, u32* w, u32* h);

void windowMaximize(void* data);


void windowMinimize(void* data);
void windowRestore(void* data);
void windowSetFullscreen(void* data, bool set);
Vec2 windowMousePos(void* data);
Vec2 windowMouseDelta(void* data);

StringView getClipboardText();
void setClipboardText(StringView text);
void openURL(StringView url);

} // namespace hg::wayland
