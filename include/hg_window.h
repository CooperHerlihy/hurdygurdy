#pragma once

#include "hg_utils.h"

#include <GLFW/glfw3.h>

namespace hg {

class Window {
public:
    [[nodiscard]] GLFWwindow* get() const {
        ASSERT(m_window != nullptr);
        return m_window;
    }

    [[nodiscard]] vk::Extent2D get_extent() const {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        return {to_u32(width), to_u32(height)};
    }

    [[nodiscard]] static Result<Window> create(bool fullscreen, i32 width, i32 height);
    void destroy() const {
        ASSERT(m_window != nullptr);
        glfwDestroyWindow(m_window);
    }

private:
    GLFWwindow* m_window = nullptr;
};

} // namespace hg
