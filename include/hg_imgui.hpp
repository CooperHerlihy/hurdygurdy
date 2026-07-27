#pragma once

#include "hg_gpu.hpp"
#include "hg_window.hpp"

namespace hg {

/**
 * Initialize ImGui platform backend
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void deinitImGui();

/**
 * Create an ImGui texture
 */
void* createImGuiTexture(const GpuView& view, GpuLayout layout);

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
void renderImGui(GpuCmd* cmd);

} // namespace hg

