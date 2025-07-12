#include "hg_window.h"

namespace hg {

Result<Window> Window::create(const bool fullscreen, const i32 width, const i32 height) {
    if (!fullscreen) {
        ASSERT(width > 0);
        ASSERT(height > 0);
    }

    auto window = ok<Window>();

    if (fullscreen) {
        const auto monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr)
            return Err::MonitorUnvailable;

        const auto video_mode = glfwGetVideoMode(monitor);
        if (video_mode == nullptr)
            ERROR("Could not find video mode");

        window->m_window = glfwCreateWindow(video_mode->width, video_mode->width, "Hurdy Gurdy", monitor, nullptr);
        if (window->m_window == nullptr)
            ERROR("Could not create window");

    } else {
        window->m_window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
        if (window->m_window == nullptr)
            ERROR("Could not create window");
    }

    ASSERT(window->m_window != nullptr);
    return window;
}

} // namespace hg
