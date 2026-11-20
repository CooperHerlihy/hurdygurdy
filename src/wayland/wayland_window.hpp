#pragma once

#include "hg/window.hpp"
#include "hg/hash.hpp"

#include <span>
#include <string_view>
#include <vector>

#include <wayland-client-core.h>

struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_seat;
struct wl_keyboard;
struct wl_pointer;
struct wl_output;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_offer;
struct wl_surface;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

#include <xkbcommon/xkbcommon.h>

namespace hg {

class WindowWayland;

class InputDeviceWayland : public InputDevice {
private:
    struct wl_display* display = nullptr;
    struct wl_compositor* compositor = nullptr;
    struct wl_shm* shm = nullptr;
    struct xdg_wm_base* xdgWmBase = nullptr;
    struct wl_seat* seat = nullptr;
    struct wl_keyboard* keyboard = nullptr;
    struct wl_pointer* pointer = nullptr;
    struct wl_data_device_manager* dataDeviceManager = nullptr;
    struct wl_data_device* dataDevice = nullptr;

    std::vector<DisplayInfo> displays{};

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[static_cast<u32>(Button::count)]{};
    bool wasKeyDown[static_cast<u32>(Button::count)]{};
    Vec2 mouseDeltaAccum{};
    Vec2 wheelDeltaAccum{};

    HashMap<u64, WindowWayland*> windows{};
    WindowWayland* activeWindow = nullptr;

    std::vector<char> clipboard{};

    // Incoming clipboard transfer
    struct wl_data_offer* pendingOffer = nullptr;
    int clipboardFd = -1;
    bool clipboardHasText = false;
    std::vector<char> clipboardIncoming{};

    struct xkb_context* xkbContext = nullptr;
    struct xkb_keymap* xkbKeymap = nullptr;
    struct xkb_state* xkbState = nullptr;
    u32 xkbModifiers = 0;

    u32 pointerEnterSerial = 0;
    u32 lastSerial = 0;
    struct wl_surface* pointerFocus = nullptr;

    // Output tracking
    struct wl_output* outputs[16]{};
    u32 outputCount = 0;

    bool globalsReady = false;

    void readClipboard();

public:
    static std::unique_ptr<InputDeviceWayland> create();
    ~InputDeviceWayland() override;

    InputBackend getBackend() override
    {
        return InputBackend::wayland;
    }

    std::span<std::string_view> getVulkanExtensions() override;
    std::span<DisplayInfo> displayInfo() override;
    void processEvents() override;
    std::span<const Event> getEvents() override;
    bool wasQuit() override;
    std::unique_ptr<Window> createWindow(Gpu* gpu, const WindowConfig& config) override;
    bool isButtonDown(Button button) override;
    bool wasButtonPressed(Button button) override;
    bool wasButtonReleased(Button button) override;
    Vec2 mousePos() override;
    Vec2 mouseDelta() override;
    Vec2 wheelDelta() override;
    void setCursor(CursorType type) override;
    void showCursor(bool show = true) override;

    // Wayland protocol callbacks
    static void keyboardKeymap(void* data, struct wl_keyboard* keyboard,
        uint32_t format, int fd, uint32_t size);
    static void keyboardEnter(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, struct wl_surface* surface, struct wl_array* keys);
    static void keyboardLeave(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, struct wl_surface* surface);
    static void keyboardKey(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void keyboardModifiers(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
        uint32_t modsLocked, uint32_t group);
    static void keyboardRepeatInfo(void* data, struct wl_keyboard* keyboard,
        int32_t rate, int32_t delay);
    static void pointerEnter(void* data, struct wl_pointer* pointer,
        uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
    static void pointerLeave(void* data, struct wl_pointer* pointer,
        uint32_t serial, struct wl_surface* surface);
    static void pointerMotion(void* data, struct wl_pointer* pointer,
        uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
    static void pointerButton(void* data, struct wl_pointer* pointer,
        uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void pointerAxis(void* data, struct wl_pointer* pointer,
        uint32_t time, uint32_t axis, wl_fixed_t value);
    static void seatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities);
    static void seatName(void* data, struct wl_seat* seat, const char* name);
    static void outputGeometry(void* data, struct wl_output* output,
        int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight,
        int32_t subpixel, const char* make, const char* model, int32_t transform);
    static void outputMode(void* data, struct wl_output* output,
        uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    static void outputDone(void* data, struct wl_output* output);
    static void outputScale(void* data, struct wl_output* output, int32_t factor);
    static void outputName(void* data, struct wl_output* output, const char* name);
    static void outputDescription(void* data, struct wl_output* output, const char* description);
    static void registryGlobal(void* data, struct wl_registry* registry,
        uint32_t name, const char* interface, uint32_t version);
    static void registryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name);
    static void xdgWmBasePing(void* data, struct xdg_wm_base* xdgWmBase, uint32_t serial);
    static void xdgToplevelConfigure(void* data, struct xdg_toplevel* toplevel,
        int32_t width, int32_t height, struct wl_array* states);
    static void xdgToplevelClose(void* data, struct xdg_toplevel* toplevel);
    static void xdgToplevelConfigureBounds(void* data, struct xdg_toplevel* toplevel,
        int32_t width, int32_t height);
    static void xdgToplevelWmCapabilities(void* data, struct xdg_toplevel* toplevel,
        struct wl_array* capabilities);
    static void xdgSurfaceConfigure(void* data, struct xdg_surface* xdgSurface, uint32_t serial);
    static void dataOfferOffer(void* data, struct wl_data_offer* offer, const char* mimeType);
    static void dataDeviceDataOffer(void* data, struct wl_data_device* device,
        struct wl_data_offer* offer);
    static void dataDeviceEnter(void* data, struct wl_data_device* device,
        uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
        struct wl_data_offer* offer);
    static void dataDeviceMotion(void* data, struct wl_data_device* device,
        uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void dataDeviceLeave(void* data, struct wl_data_device* device);
    static void dataDeviceDrop(void* data, struct wl_data_device* device);
    static void dataDeviceSelection(void* data, struct wl_data_device* device,
        struct wl_data_offer* offer);

    friend class WindowWayland;
};

class WindowWayland : public Window {
private:
    std::unique_ptr<GpuSwapchain> swap{};

    struct wl_surface* wlSurface = nullptr;
    struct xdg_surface* xdgSurface = nullptr;
    struct xdg_toplevel* xdgToplevel = nullptr;

    std::vector<Event> eventQueue{};
    Vec2 mouse{};
    u32 width = 0;
    u32 height = 0;
    bool closed = false;
    bool focused = false;
    bool focusGained = false;
    bool focusLost = false;
    bool resized = false;

public:
    explicit WindowWayland(InputDevice* device) noexcept
        : Window(device)
    {}
    ~WindowWayland() override;

    GpuSwapchain* swapchain() const override;
    void setTitle(std::string_view title) override;
    std::span<const Event> getEvents() const override;
    bool wasClosed() const override;
    bool isFocused() const override;
    bool wasFocusGained() const override;
    bool wasFocusLost() const override;
    bool wasResized() const override;
    void size(u32* width, u32* height) const override;
    void setFullscreen(bool set = true) override;
    Vec2 mousePos() const override;
    Vec2 mouseDelta() const override;

    friend class InputDeviceWayland;
};

} // namespace hg
