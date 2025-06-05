#pragma once

#include "hg_external.h"
#include "hg_utils.h"

namespace hg {

struct Engine {
    vk::Instance instance = {};
    vk::DebugUtilsMessengerEXT debug_messenger = {};
    vk::PhysicalDevice gpu = {};
    vk::Device device = {};
    VmaAllocator allocator = nullptr;

    u32 queue_family_index = UINT32_MAX;
    vk::Queue queue = {};

    vk::CommandPool command_pool = {};
    vk::CommandPool single_time_command_pool = {};

    [[nodiscard]] static Engine create();
    void destroy() const;
};

constexpr u32 MaxFramesInFlight = 2;
constexpr u32 MaxSwapchainImages = 3;

class Window {
public:
    GLFWwindow* window = nullptr;
    vk::SurfaceKHR surface = {};

    vk::SwapchainKHR swapchain = {};
    vk::Extent2D extent = {};
    vk::Format image_format = vk::Format::eUndefined;
    u32 image_count = 0;
    u32 current_image_index = 0;
    std::array<vk::Image, MaxSwapchainImages> swapchain_images = {};
    std::array<vk::ImageView, MaxSwapchainImages> swapchain_views = {};

    vk::CommandBuffer& current_cmd() { return m_command_buffers[m_current_frame_index]; }
    vk::Image& current_image() { return swapchain_images[current_image_index]; }
    vk::ImageView& current_view() { return swapchain_views[current_image_index]; }
    vk::Fence& is_frame_finished() { return m_frame_finished_fences[m_current_frame_index]; }
    vk::Semaphore& is_image_available() { return m_image_available_semaphores[m_current_frame_index]; }
    vk::Semaphore& is_ready_to_present() { return m_ready_to_present_semaphores[current_image_index]; }

    [[nodiscard]] static Window create(const Engine& engine, i32 width, i32 height);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine);

    [[nodiscard]] vk::CommandBuffer begin_frame(const Engine& engine);
    [[nodiscard]] bool end_frame(const Engine& engine);
    [[nodiscard]] bool submit_frame(const Engine& engine, const auto& commands) {
        const auto cmd = begin_frame(engine);
        commands(cmd);
        return end_frame(engine);
    }

private:
    u32 m_current_frame_index = 0;
    std::array<vk::CommandBuffer, MaxFramesInFlight> m_command_buffers = {};
    std::array<vk::Fence, MaxFramesInFlight> m_frame_finished_fences = {};
    std::array<vk::Semaphore, MaxFramesInFlight> m_image_available_semaphores = {};
    std::array<vk::Semaphore, MaxSwapchainImages> m_ready_to_present_semaphores = {};
    bool m_recording = false;
};

} // namespace hg
