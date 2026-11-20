#pragma once

#include "hg/gpu.hpp"
#include "hg/window.hpp"

namespace hg {

/**
 * Initialize ImGui platform backend
 *
 * Parameters
 * - window The window for ImGui to use
 * - device The input device for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void initImGui(
    Window* window,
    InputDevice* device,
    Format colorFormat,
    Format depthFormat = Format::undefined,
    Format stencilFormat = Format::undefined);

/**
 * Deinitializes ImGui platform backend
 */
void deinitImGui();

/**
 * Create an ImGui texture
 */
void* createImGuiTexture(GpuView* view, GpuLayout layout);

/**
 * Create an ImGui texture
 */
void destroyImGuiTexture(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void beginImGuiFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void renderImGui(GpuCommandBuffer* cmd);

} // namespace hg

