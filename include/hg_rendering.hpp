/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_RENDERING_HPP
#define HG_RENDERING_HPP

#include "hg_core.hpp"
#include "hg_math.hpp"
#include "hg_gpu.hpp"
#include "hg_window.hpp"
#include "hg_assets.hpp"
#include "hg_ecs.hpp"

/**
 * The types of camera projections
 */
enum HgCameraType : u32 {
    HgCameraType_perspective,
    HgCameraType_orthographic,
};

/**
 * A perspective camera
 */
struct HgCameraPerspective {
    /**
     * The field of view
     */
    f32 fov;
    /**
     * The aspect ratio
     */
    f32 aspect;
    /**
     * The near clipping plane
     */
    f32 near;
    /**
     * The far clipping plane
     */
    f32 far;
};

/**
 * An orthographic camera
 */
struct HgCameraOrthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left, right, top, bottom, near, far;
};

/**
 * A camera component
 */
struct HgCamera {
    /**
     * The gpu view projection data, created/destroyed on add/remove
     */
    HgGpuBuffer vpBuffer;
    /**
     * The type of camera
     */
    HgCameraType type;
    /**
     * An orthographic camera
     */
    HgCameraOrthographic orthographic;
    /**
     * A perspective camera
     */
    HgCameraPerspective perspective;
};

/**
 * HgCamera ecs add implementation
 */
template<>
void hgEcsAddImpl(HgCamera* camera);

/**
 * HgCamera ecs remove implementation
 */
template<>
void hgEcsRemoveImpl(HgCamera* camera);

/**
 * Update the camera's gpu side data, must have a camera and transform
 */
void hgCameraUpdate(HgEcs* ecs, HgEntity e);

/**
 * A sprite component
 */
struct HgSprite {
    /**
     * The texture to draw from
     */
    HgGpuTextureHandle texture = {};
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    HgVec2 uvPos{0.0f, 0.0f};
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    HgVec2 uvSize{1.0f, 1.0f};
};

/**
 * Initialize the sprite pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void hgSpritesInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the sprite pipeline
 */
void hgSpritesDeinit();

/**
 * Issue draw commands for all HgSprite2D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgSpritesDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * A skybox component
 */
struct HgSkybox {
    /**
     * The cubemap texture
     */
    HgGpuTextureHandle texture;
};

/**
 * Initialize the skybox pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, if used
 */
void hgSkyboxInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the skybox pipeline
 */
void hgSkyboxDeinit();

/**
 * Draw a skybox from a texture
 *
 * Parameters
 * - ecs The ecs with the camera
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgSkyboxDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * A directional light component
 */
struct HgDirLight {
    /**
     * The direction of the light
     */
    HgVec3 dir;
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * A point light component, should have a transform
 */
struct HgPointLight {
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * A 3D model component
 */
struct HgModel {
    /**
     * The model to render
     */
    HgGpuMeshHandle model;
    /**
     * The model's color map
     */
    HgGpuTextureHandle colorMap;
    /**
     * The model's normal map
     */
    HgGpuTextureHandle normalMap;
};

/**
 * Initialize the 3D model pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void hgModelsInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the 3D model pipeline
 */
void hgModelsDeinit();

/**
 * Issue draw commands for all HgModel3D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgModelsDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * Initialize ImGui platform backend
 *
 * Note, requires GLFW on Linux (for now)
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void hgImGuiInit(
    HgWindow window,
    HgFormat colorFormat,
    HgFormat depthFormat = HgFormat_undefined,
    HgFormat stencilFormat = HgFormat_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void hgImGuiDeinit();

/**
 * Create an ImGui texture
 */
void* hgImGuiTextureCreate(HgGpuView view, HgGpuLayout layout);

/**
 * Create an ImGui texture
 */
void hgImGuiTextureDestroy(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void hgImGuiNewFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void hgImGuiDraw(HgGpuCmd* cmd);

#endif // HG_RENDERING_HPP
