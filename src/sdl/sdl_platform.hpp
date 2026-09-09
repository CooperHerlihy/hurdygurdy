#pragma once

#include "hg/window.hpp"
#include "hg/audio.hpp"
#include "hg/gpu.hpp"
#include "hg/span.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"

namespace hg::sdl {

bool initPlatform();
void deinitPlatform();
Span<StringView> getPlatformVulkanExtensions(Arena* arena);

Span<DisplayInfo> displayInfo();
void setCursor(CursorType type);
void showCursor(bool show);
String getClipboardText();
void setClipboardText(StringView text);
void openURL(StringView url);
void processEvents();
bool wasQuit();
Span<Event> getEvents();

u32 gamepadCount();
bool isGamepadActive(u32 gamepad);
bool isGamepadButtonDown(u32 gamepad, Button key);
Vec2 gamepadLeftStick(u32 gamepad);
Vec2 gamepadRightStick(u32 gamepad);
f32 gamepadLeftTrigger(u32 gamepad);
f32 gamepadRightTrigger(u32 gamepad);

bool isButtonDown(Button key);
bool wasButtonPressed(Button key);
bool wasButtonReleased(Button key);
Vec2 globalMousePos();
Vec2 mousePos();
Vec2 mouseDelta();
Vec2 wheelDelta();

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig);
void unsetAudioCallback();

void* windowCreate(const WindowConfig& config);
void windowDestroy(void* data);
GpuSwapchain& windowSwapchain(void* data);
GpuView* windowImageView(void* data);
Format windowImageFormat(void* data);
void windowSetTitle(void* data, StringView title);
void windowGetPos(void* data, i32* x, i32* y);
void windowSetPos(void* data, i32 x, i32 y);
void windowGetSize(void* data, u32* w, u32* h);
void windowSetSize(void* data, u32 w, u32 h);
bool windowIsFullscreen(void* data);
void windowSetFullscreen(void* data, bool set);
void windowSetResizable(void* data, bool set);
bool windowIsFocused(void* data);
bool windowWasClosed(void* data);
bool windowWasResized(void* data);
bool windowWasFocusGained(void* data);
bool windowWasFocusLost(void* data);
bool windowWasMoved(void* data);
bool windowIsMaximized(void* data);
bool windowIsMinimized(void* data);
void windowMaximize(void* data);
void windowMinimize(void* data);
void windowRestore(void* data);
Vec2 windowMousePos(void* data);
Span<Event> windowEvents(void* data);

} // namespace hg::sdl
