#pragma once

#include "hg/window.hpp"
#include "hg/gpu.hpp"
// #include "hg/memory.hpp"

namespace hg::wayland {

bool loadWayland();
bool initWayland();
void deinitWayland();
std::span<std::string_view> getPlatformVulkanExtensions();

std::span<DisplayInfo> displayInfo();
void setCursor(CursorType type);
void showCursor(bool show);
void processEvents();
std::span<Event> getEvents();
bool wasQuit();

bool isButtonDown(Button key);
bool wasButtonPressed(Button key);
bool wasButtonReleased(Button key);
Vec2 mousePos();
Vec2 mouseDelta();
Vec2 wheelDelta();

Window windowCreate(const WindowConfig& config);
void windowDestroy(void* data);
GpuSwapchain& windowSwapchain(void* data);
void windowSetTitle(void* data, std::string_view title);
std::span<Event> windowEvents(void* data);
bool windowWasClosed(void* data);
bool windowIsFocused(void* data);
bool windowWasFocusGained(void* data);
bool windowWasFocusLost(void* data);
bool windowWasResized(void* data);
void windowGetSize(void* data, u32* w, u32* h);
void windowSetFullscreen(void* data, bool set);
Vec2 windowMousePos(void* data);
Vec2 windowMouseDelta(void* data);

std::string_view getClipboardText();
void setClipboardText(std::string_view text);
void openURL(std::string_view url);

} // namespace hg::wayland
