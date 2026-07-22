# GPU API Integration Tests

End-to-end scenarios combining multiple API features in realistic workloads.
Each test exercises a complete real-world use case through the API surface
at `include/hurdygurdy.hpp:1657-3031`.

---

# Agent Context -- How to Write These Tests

## File Structure

All tests go into `src/test.cpp`, a monolithic file. The build uses CMake
presets: `debug`, `release`, `san`, `tsan`.

Test shaders are GLSL in `src/` and compiled to SPIR-V headers at build time
via `glslc` + the `embed` tool. List new shaders in `SHADERS` in CMakeLists.txt.

Existing test shaders in `src/`:

| File | Purpose |
|------|---------|
| `test_compute.comp` | SSBO read+add+write via push constant descriptors |
| `test_tri.vert` | Full-screen triangle (positions hardcoded) |
| `test_tri.frag` | Constant color output `vec4(0.2, 0.4, 0.6, 1.0)` |

## Test Macro

```cpp
#define TEST(cond) do { \
    if (!(cond)) \
        HG_PANIC("Test failed in " __FILE__ ":%d %s() \"" #cond "\"\n", \
                 __LINE__, __func__); \
} while(0)
```

## Test File Boilerplate

```cpp
#undef HG_NO_LOGGING
#define HG_LOGGING 1
#include "hurdygurdy.hpp"
// ... includes ...

#define TEST(cond) ...
using namespace hg;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");
    // ... tests ...
    printf("HurdyGurdy: All tests passed\n");
}
```

Shader includes are needed only in the GPU test section:
```cpp
#include "test_compute.comp.spv.h"
#include "test_tri.vert.spv.h"
#include "test_tri.frag.spv.h"
```
These generated headers contain `static const u8 shaderName[] = { ... };`.
The variable name matches the file with dots replaced: `test_tri.vert.spv.h`
produces `test_tri_vert_spv`.

## Test Pattern for GPU Tests

All existing GPU tests are scoped blocks inside `main()`. The general pattern:

```cpp
{
    // 1. Create GPU resources (GpuBuffer, GpuImage, GpuView, GpuPipeline)
    // 2. Write input data via buf.write() / view.write()
    // 3. gpuCmdBegin()
    // 4. gpuMemoryBarrier() -- transition resources to correct stages
    // 5. gpuComputePass() or gpuRenderPassBegin() -- declare resource deps
    // 6. gpuBindPipeline()
    // 7. gpuPushConstants() if needed
    // 8. gpuDispatch() or gpuDraw()
    // 9. gpuRenderPassEnd()
    // 10. gpuCmdEnd()
    // 11. Read back results via buf.read() / view.read()
    // 12. TEST() assertions on the results
}
```

No window is needed. All rendering goes to offscreen `GpuImage` with
`GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc`.
All compute goes to `GpuBuffer` with `GpuBufferUsage_storageBuffer`.

## GLSL Bindless Resource Bindings (from `include/hurdygurdy.glsl`)

| Macro | Binding Index | GLSL layout |
|-------|---------------|-------------|
| `HgCombinedImageSampler` | 0 | `layout(set=0, binding=0)` |
| `HgStorageImage` | 1 | `layout(set=0, binding=1, ...) uniform writeonly image2D` |
| `HgUniformBuffer` | 2 | `layout(set=0, binding=2) uniform` |
| `HgStorageBuffer` | 3 | `layout(set=0, binding=3) buffer` |

Descriptors are bindless: pass the index from `buf.uniformDescriptor()`,
`buf.storageDescriptor()`, `view.samplerDescriptor()`, or
`view.storageDescriptor()` via push constants.

Compute shader example (`test_compute.comp`):
```glsl
layout (push_constant) uniform Push {
    uint addVal;
    uint inIdx;
    uint outIdx;
} push;

layout (HgStorageBuffer) buffer InBuf { uint data[]; } inBufs[];
layout (HgStorageBuffer) buffer OutBuf { uint data[]; } outBufs[];

void main() {
    uint idx = gl_GlobalInvocationID.x;
    outBufs[push.outIdx].data[idx] = inBufs[push.inIdx].data[idx] + push.addVal;
}
```

Graphics pipeline example -- full-screen triangle (`test_tri.vert`):
```glsl
void main() {
    vec2 positions[3] = vec2[](vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0));
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

## Key Resource Lifecycle Rules

- `GpuImage` must outlive any `GpuView` referencing it.
- Default-constructed objects (`GpuBuffer{}`, `GpuPipeline{}`) have `data == nullptr`.
  Binding them is safe (no-op) but sets error.
- Move operations leave source with `data == nullptr`.
- `gpuCmdBegin()` returns a non-null `GpuCmd*`.
- `gpuCmdEnd()` submits and waits for completion (implicit sync).
- `gpuWaitIdle()` waits for all pending GPU work.

## Passing Resources to Render/Compute Passes

```cpp
GpuBuffer* storageBufs[] = {&bufA, &bufB};
GpuView* sampledImages[] = {&texView};

GpuComputePass pass{};
pass.storageBuffers = storageBufs;
pass.sampledImages = sampledImages;
gpuComputePass(cmd, pass);

// For rendering:
GpuRenderPass pass{};
pass.colorAttachments = {&attachment, 1};
pass.uniformBuffers = {&uniformBuf, 1};
pass.sampledImages = {&texView, 1};
gpuRenderPassBegin(cmd, pass);
```

## Memory Barriers

Resources need explicit barriers between usage stages:

```cpp
// Buffer barrier
GpuBufferBarrier bb{};
bb.buffer = &buf;
bb.nextStage = GpuStage_computeShader;
bb.nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
gpuMemoryBarrier(cmd, {&bb, 1}, {});

// Image barrier (includes layout transition)
GpuImageBarrier ib{};
ib.image = &view;
ib.nextStage = GpuStage_transfer;
ib.nextAccess = GpuAccess_transferRead;
ib.nextLayout = GpuLayout_transferSrc;
gpuMemoryBarrier(cmd, {}, {&ib, 1});

// Combined
gpuMemoryBarrier(cmd, {&bb, 1}, {&ib, 1});
```

## Common Pitfalls

1. **Missing barrier before dispatch/render pass** -- resource not visible to
   the shader, produces garbage or hangs.
2. **Wrong `GpuLayout` in image barrier** -- Vulkan validation error or
   undefined data.
3. **Image lacks `transferSrc` usage** -- `read()` will fail.
4. **Image lacks `colorAttachment` or `storage` usage** -- can't use as
   render target or storage image.
5. **Writing to a `deviceOnly` buffer** -- must go through staging buffer
   or `frequentUpdate`; but `deviceOnly` `write()` maps staging internally.
6. **View with wrong `GpuAspect`** -- depth aspect for depth images, color
   for color images.
7. **Push constant struct alignment** -- must match shader `layout(push_constant)`
   exactly. Use `u32` for descriptor indices (they are passed as raw indices).
8. **Command buffer reuse** -- `gpuCmdBegin`/`gpuCmdEnd` is one-shot.
   Each call pair creates a fresh submission.
9. **Descriptor index collisions** -- don't hardcode; use the accessor
   functions (`uniformDescriptor()`, `storageDescriptor()`, etc.) at runtime.
10. **`gpuRenderPassBegin` with `image` field as `GpuView*`** -- the view
    references the image internally. The attachment image is the view, not
    the raw image.

## Adding a New Test Shader

1. Write `src/test_my_shader.vert` (or `.frag`, `.comp`) with `#version 460`.
2. Add it to the `SHADERS` list in `CMakeLists.txt`.
3. `#include "test_my_shader.vert.spv.h"` in `test.cpp`.
4. Use the variable `test_my_shader_vert_spv` and its size.

Remember to rebuild after adding a new shader (custom command runs at cmake
build time).

---

# Integration Test Scenarios

Each test below is a complete real-world use case exercising multiple API
features in sequence.

## 1. Full-Screen Triangle (Minimal Render)

**Purpose:** Verify the entire offscreen rendering pipeline: image creation, view
creation, pipeline creation, render pass, draw, and readback.

**Preconditions:** `init()` succeeded.

**Steps:**
1. Create a 4x4 `GpuImage` with `Format_r8g8b8a8_unorm`,
   `GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc`.
2. Create a `GpuView` on that image with `GpuAspect_color`.
3. Build a `GpuGraphicsPipelineCreateInfo` with:
   - full-screen triangle vertex shader
   - fragment shader outputting a constant color (e.g. `vec4(0.2, 0.4, 0.6, 1.0)`)
   - `colorAttachmentFormats` = `{Format_r8g8b8a8_unorm}`
4. Call `gpuCmdBegin()`.
5. Set up a `GpuRenderAttachment`:
   - `image` = view
   - `loadOp` = `GpuLoadOp_clear`, `storeOp` = `GpuStoreOp_store`
   - clear color = red `(1,0,0,1)`
6. Call `gpuRenderPassBegin(cmd, pass)`.
7. Call `gpuBindPipeline(cmd, pipe)`.
8. Call `gpuDraw(cmd, 0, 3, 0, 1)`.
9. Call `gpuRenderPassEnd(cmd)`.
10. Call `gpuCmdEnd(cmd)`.
11. Read back pixels with `view.read(dst)`.
12. Assert every pixel == `0xFF996633`.

**Verification:** GPU wait idle after cmdEnd (implicit by design). All pixels match
expected shader output. Clear color is overwritten by triangle.

---

## 2. Compute SSBO Add (Buffer → Push Constant → Buffer)

**Purpose:** Verify compute dispatch with push constants, SSBO input, and SSBO
output. Covers `GpuBuffer`, `gpuBindPipeline`, `gpuPushConstants`, `gpuDispatch`,
`gpuMemoryBarrier`, and `gpuComputePass`.

**Steps:**
1. Create `inBuf` (64 x `u32`, `GpuBufferUsage_storageBuffer`, `GpuMemoryUsage_frequentUpdate`).
2. Create `outBuf` (64 x `u32`, `GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc`).
3. Write sequential values `0..63` into `inBuf`.
4. Define a `Push` struct with `addVal`, `inIdx` (descriptor), `outIdx` (descriptor).
5. Create `GpuPipeline` with compute shader that reads `inBuf[globalInvocationX]`,
   adds `push.addVal`, writes to `outBuf[globalInvocationX]`.
6. `gpuCmdBegin()`.
7. Issue buffer barriers for `inBuf` (`shaderRead`) and `outBuf` (`shaderRead | shaderWrite`).
8. Call `gpuComputePass(cmd, pass)` with `storageBuffers = {&inBuf, &outBuf}`.
9. `gpuBindPipeline`, `gpuPushConstants`, `gpuDispatch(cmd, 64, 1, 1)`.
10. `gpuCmdEnd(cmd)`.
11. Read `outBuf` and verify `output[i] == input[i] + addVal`.

**Verification:** Every output element equals input plus addVal (e.g. 100).

---

## 3. Offscreen Render with Depth Test

**Purpose:** Verify depth attachment creation, depth testing in a graphics
pipeline, and readback of depth values.

**Steps:**
1. Create color image (4x4, `Format_r8g8b8a8_unorm`,
   `colorAttachment | transferSrc`).
2. Create depth image (4x4, `Format_d32_sfloat`,
   `depthStencilAttachment | transferSrc`).
3. Create depth view with `GpuAspect_depth`.
4. Build pipeline with `depthAttachmentFormat = Format_d32_sfloat`,
   `enableDepthRead = true`, `enableDepthWrite = true`.
5. Vertex shader outputs positions at varying depths (z = 0.25 and z = 0.75).
6. `gpuRenderPassBegin` with both color and depth attachments.
   Depth loadOp = `clear`, clear depth = 1.0.
7. Draw two triangles (one at 0.25, one at 0.75).
8. `gpuRenderPassEnd`, `gpuCmdEnd`.
9. Readback color attachment: closer triangle overwrites farther.
10. Readback depth attachment: values should be 0.25 in the closer region.

**Verification:** Color readback shows the closer triangle's color where they
overlap. Depth readback shows 0.25 where the closer triangle was drawn.

---

## 4. Cubemap Cross Write and Per-Face Readback

**Purpose:** Verify `writeCubemap` correctly maps a 2D cross-layout source into
6 cubemap array layers, and that per-layer `read()` returns the expected data.

**Steps:**
1. Create cubemap image: `arrayLayers = 6`, `flags = cubeCompatible`,
   `Format_r8g8b8a8_unorm`, `transferSrc | transferDst | sampled`.
2. Create cubemap view: `type = cube`, `layerCount = 6`.
3. Build a byte array representing a 4-column by 3-row cross, each face
   4x4 pixels, with distinct RGBA colors per face:
   - layer 0 (+X): red, layer 1 (-X): green, layer 2 (+Y): blue,
     layer 3 (-Y): yellow, layer 4 (+Z): magenta, layer 5 (-Z): cyan.
   Place each face at the correct (cx, cy) position per
   `writeCubemap`'s documented cross layout.
4. Call `cubeView.writeCubemap(cross)`.
5. Create 6 separate single-layer `GpuView`s (`type = 2D`, `layerCount = 1`,
   `baseArrayLayer = i`).
6. For each layer, issue a `gpuMemoryBarrier` to `transferSrc`, then `read()`.
7. Assert each face readback matches the expected RGBA.

**Verification:** Each cubemap face reads back the exact color written via the
cross-layout source.

---

## 5. Mipmap Generation and Verification (All Levels)

**Purpose:** Verify `genMipmaps` produces correct downsampled content across all
levels, and that the base level is unmodified.

**Steps:**
1. Create 8x8 image with 4 mip levels,
   `transferSrc | transferDst | sampled`.
2. Create a full mip-chain view (`levelCount = 4`).
3. Write a checkerboard pattern to the base level:
   - 4x4 checker of red `(1,0,0,1)` and green `(0,1,0,1)`.
4. Call `genMipmaps()`.
5. Read back base level: verify checkerboard unchanged.
6. Create per-level views (`baseMipLevel = N`, `levelCount = 1`).
7. Read back mip 1 (4x4): verify it's the average of 2x2 blocks
   (approximately `(0.5, 0.5, 0, 1)` for each 2x2 of red+green).
8. Read back mip 2 (2x2): verify further averaged.
9. Read back mip 3 (1x1): verify single average color.

**Verification:** Base level preserved. Each mip level is a correctly averaged
downsample of the previous.

---

## 6. Multi-Draw with Instancing

**Purpose:** Exercise instanced draws with vertex/index buffers, verifying that
each instance receives the correct per-instance data.

**Steps:**
1. Create a `GpuBuffer` with vertex data (a unit quad: 4 vertices).
2. Create an index buffer `GpuBuffer` (`transferSrc | indexBuffer`,
   host-visible) with 6 indices.
3. Create a storage buffer with per-instance data (e.g. screen-space offsets).
4. Write instance data: 4 instances with offsets `(0,0)`, `(0.5,0)`, `(0,0.5)`,
   `(0.5,0.5)`.
5. Build a graphics pipeline that reads the instance data SSBO via
   `gl_InstanceIndex`.
6. Create a 4x4 color attachment, clear to black.
7. `gpuRenderPassBegin`, bind pipeline, `gpuDraw(cmd, 0, 4, 0, 4)`.
8. `gpuRenderPassEnd`, `gpuCmdEnd`.
9. Readback: verify 4 colored quads at the expected positions.

**Verification:** 4 quads rendered at distinct positions based on instance data.

---

## 7. Buffer Upload + Image Blit (Transfer Pipeline)

**Purpose:** Verify manual buffer-to-image copy (blit) via transfer commands.
Covers `gpuMemoryBarrier` with transfer stages and image layout transitions.

**Steps:**
1. Create a staging `GpuBuffer` (256 bytes, `transferSrc`,
   `GpuMemoryUsage_frequentUpdate`).
2. Fill it with a 4x4 RGBA8 pattern (e.g. gradient or checkerboard).
3. Create a 4x4 `GpuImage` (`transferSrc | transferDst | sampled`).
4. `gpuCmdBegin()`.
5. Barrier: transition image from `undefined` to `transferDst`.
6. Issue buffer-to-image copy via `gpuCmdCopyBufferToImage` (assumed API) or
   manually via `view.write(stagingBuf)`.
7. Barrier: transition image from `transferDst` to `transferSrc`.
8. `gpuCmdEnd()`.
9. Create a `GpuView` and read back the image.
10. Assert pixel data matches the original pattern.

**Verification:** Image content after blit matches original buffer content.

---

## 8. Multi-Viewport / Multi-Scissor Render

**Purpose:** Verify `gpuSetViewport` and `gpuSetScissor` correct rendering to
sub-regions of an attachment.

**Steps:**
1. Create a 16x16 color attachment.
2. Build a solid-color pipeline (outputs red).
3. `gpuRenderPassBegin`.
4. Set viewport to left half `(0, 0, 8, 16)`, scissor to same.
5. Draw full-screen triangle in left half.
6. Change shader push constant (or use second pipeline) to output blue.
7. Set viewport to right half `(8, 0, 8, 16)`, scissor to same.
8. Draw triangle in right half.
9. `gpuRenderPassEnd`, `gpuCmdEnd`.
10. Readback: left 8 columns are red, right 8 columns are blue.

**Verification:** Two viewports render different colors to the correct halves.
Pixels outside scissor are unchanged (clear color).

---

## 9. Compute Image Filter (Blur)

**Purpose:** Compute shader reading a `sampledImage`, writing to a `storageImage`.
Exercises `GpuComputePass` with image dependencies.

**Steps:**
1. Create a 16x16 source image (`transferDst | sampled | storage`), write a
   checkerboard pattern.
2. Create a 16x16 destination image (`transferSrc | storage`).
3. Build compute pipeline with:
   - `layout(set=0, binding=0) uniform sampler2D srcImg;`
   - `layout(set=0, binding=1, rgba8) uniform writeonly image2D dstImg;`
   - A 3x3 box blur kernel.
4. `gpuCmdBegin()`.
5. Barrier: transition source from `general` to `shaderReadOnly`.
6. Barrier: transition destination from `undefined` to `general`.
7. Create compute pass with `sampledImages = {&srcView}`, `storageImages = {&dstView}`.
8. Bind pipeline, push constants (if any), dispatch 16x16x1.
9. Barrier: transition destination from `general` to `transferSrc`.
10. `gpuCmdEnd()`.
11. Read back destination via view, verify blur (corners should be averaged).

**Verification:** Destination image contains a blurred version of source.

---

## 10. Render to Texture, Sample in Second Pass (Post-Processing)

**Purpose:** End-to-end multi-pass rendering: render scene to offscreen color
attachment, then use that attachment as a sampled texture in a second pass.
Covers image layout transitions between passes and `GpuRenderPass` with
`sampledImages` dependencies.

**Steps:**
1. Create a 4x4 color attachment (`colorAttachment | sampled | transferSrc`).
2. **Pass 1 (scene):**
   - Create color view.
   - Pipeline: renders a solid white triangle.
   - `gpuRenderPassBegin` with this color attachment.
   - Draw, end pass.
3. **Barrier:** transition color attachment from `colorAttachment` to
   `shaderReadOnly`.
4. **Create result image** (4x4, `transferSrc | transferDst | colorAttachment`).
5. **Pass 2 (post-process):**
   - Pipeline: samples the first-pass result, inverts colors
     `(1.0 - sampledColor)`.
   - `gpuRenderPassBegin` with result image as color target.
   - Pass includes `sampledImages = {&firstPassView}`.
   - Draw full-screen quad, end pass.
6. Barrier: transition result to `transferSrc`.
7. `gpuCmdEnd()`.
8. Read back result: each pixel should be ~`(0.8, 0.6, 0.4, 0.0)` (the inverse
   of the triangle's `(0.2, 0.4, 0.6, 1.0)`).

**Verification:** Second pass correctly samples the first pass output and applies
the post-process effect.

---

## 11. Simultaneous Multi-Window Rendering

**Purpose:** Verify `gpuFrameBegin` with multiple windows and per-window rendering.

**Preconditions:** Two windows created via `Window::create`.

**Steps:**
1. Create two `Window` objects (320x240 each).
2. For N frames:
   a. Poll events.
   b. `GpuCmd* cmd = gpuFrameBegin({&win1, &win2})`.
   c. For each window, if `imageView()` is non-null:
      - Begin render pass targeting the window's swapchain image.
      - Draw to it.
      - End render pass.
   d. `gpuFrameEnd(cmd)`.
3. `gpuWaitIdle()`.
4. Verify no crashes, validation errors, or device loss.

**Verification:** Both windows display content without errors. At least 60 frames
rendered.

---

## 12. Buffer Readback After Compute (Staging Read)

**Purpose:** Verify `GpuMemoryUsage_stagingRead` for reading compute output back
to CPU via a staging buffer.

**Steps:**
1. Create output `GpuBuffer` (256 bytes, `storageBuffer | transferDst`,
   device-local).
2. Create staging `GpuBuffer` (256 bytes, `transferDst`,
   `GpuMemoryUsage_stagingRead`).
3. Compute dispatch fills output buffer.
4. `gpuCmdBegin()`.
5. Barrier: output buffer to `transferSrc`.
6. `gpuCmdCopyBuffer(outputBuf, stagingBuf, 256)` (assumed API) or
   manual copy.
7. Barrier: staging buffer to `hostRead`.
8. `gpuCmdEnd()`.
9. Read staging buffer via `stagingBuf.read(dst)`.
10. Assert content matches expected compute output.

**Verification:** Data written by compute on GPU is readable on CPU via staging
buffer.

---

## 13. Pipeline Barrier Granularity (Buffer Sub-Ranges)

**Purpose:** Verify barriers correctly handle the same buffer used for different
purposes in different dispatch calls (two compute dispatches chained via barrier).

**Steps:**
1. Create `bufA`, `bufB`, `bufC` (each 256 bytes, `storageBuffer`,
   `frequentUpdate`).
2. Write known data to `bufA`.
3. **Dispatch 1:** reads `bufA`, writes `bufB`. Barrier main buffer regions
   between dispatches.
4. Barrier: `bufB` to `shaderRead`, `bufA` no longer needed.
5. **Dispatch 2:** reads `bufB`, writes `bufC`.
6. Barrier: `bufC` to `transferSrc`.
7. Read back `bufC` and verify: `bufC[i] == bufA[i] + addVal1 + addVal2`.

**Verification:** Two compute dispatches correctly chain through buffer barriers
without data hazards.

---

## 14. Image Array Rendering (Layered Rendering)

**Purpose:** Verify `layerCount > 1` in `GpuRenderPass` for layered rendering to
an image array.

**Steps:**
1. Create image array: `arrayLayers = 4`, `Format_r8g8b8a8_unorm`,
   `colorAttachment | transferSrc`.
2. Create array view with `layerCount = 4`.
3. Allocate per-layer output staging buffers.
4. **For each layer** (via `gpuRenderPassBegin` with `layerCount`):
   - Clear to a unique color per layer.
   - Draw triangle.
5. `gpuRenderPassEnd`.
6. Create 4 single-layer views, read back each.
7. Assert each layer has the expected triangle color and the clear color
   from other layers does not bleed.

**Verification:** Each array layer renders independently; `layerCount` correctly
iterates render passes across layers.

---

# Unit Tests for Existing Smoke Tests

The 11 test blocks below replace smoke tests in `src/test.cpp 2208-2823`.
Each verifies functional correctness, not just "does not crash."

---

## U1. GpuBuffer Write/Read with Non-Zero Offsets

**Replaces** `GpuBuffer write/read: host-visible` (line 2307) and `staging`
(line 2317).

**Steps:**
1. Create `GpuBuffer` (256 bytes, `transferSrc | transferDst`,
   `frequentUpdate`).
2. Write `0x11111111` at offset 0.
3. Write `0x22222222` at offset 64.
4. Write `0x33333333` at offset 128.
5. Read back 4 bytes from offset 0: assert `0x11111111`.
6. Read back 4 bytes from offset 64: assert `0x22222222`.
7. Read back 4 bytes from offset 128: assert `0x33333333`.
8. Read back 8 bytes from offset 60 (crossing into offset-64 write):
   assert yields the tail of `0x11111111` + head of `0x22222222`.
9. **Repeat steps 1-8** with `GpuMemoryUsage_deviceOnly` (staging path).

**Verification:** Independent buffer regions are independently writable and
readable. Non-zero offsets work. Staging path matches host-visible path.

---

## U2. GpuView Extended Config Affects Sampler Output

**Replaces** `GpuView extended create info` (line 2413).

**Steps:**
1. Create 4x4 `GpuImage` (`transferSrc | transferDst | sampled | colorAttachment`).
2. Write a checkerboard pattern to it (2x2 black, 2x2 white).
3. Create `GpuView` with `GpuFilter_linear`, `GpuSamplerEdgeMode_clampToEdge`.
4. Create a 2x2 color attachment.
5. Build a pipeline that samples the view at half-pixel coordinates and outputs
   the sampled color.
6. `gpuRenderPassBegin`, bind pipeline, draw full-screen quad, end pass.
7. Read back the 2x2 result.
8. Verify bilinear interpolation: the sample at `(0.25, 0.25)` from a 4x4
   texture yields a blend of 4 texels, not a nearest-neighbor result.
9. **Repeat** with `GpuFilter_nearest`: verify nearest-neighbor produces
   different values at the same coordinates.

**Verification:** `GpuFilter` actually affects how texels are sampled.
`GpuSamplerEdgeMode` could be verified by sampling outside `[0,1]` and
checking clamp behavior. This proves the extended create info isn't ignored.

---

## U3. Command Buffer Executes Recorded Commands

**Replaces** `gpuCmdBegin / gpuCmdEnd` (line 2428).

**Steps:**
1. Create `GpuBuffer` (64 bytes, `transferDst`, device-only).
2. Create staging `GpuBuffer` (64 bytes, `transferSrc`, `frequentUpdate`).
3. Write known data into staging buffer at offset 0.
4. `gpuCmdBegin()`.
5. Issue a barrier: staging to `transferSrc`.
6. **If `gpuCmdCopyBuffer` exists:** copy staging->device.
   **Otherwise:** dispatch a compute shader that copies staging->device.
7. Issue a barrier: device buffer to `transferSrc`.
8. `gpuCmdEnd(cmd)`.
9. Read from device buffer. Assert data matches what was written to staging.

**Verification:** Commands recorded between `gpuCmdBegin` and `gpuCmdEnd` are
actually executed and produce observable results. Without this, the test
exposes a no-op command buffer.

---

## U4. Buffer Barrier Synchronizes Compute Write Then Read

**Replaces** `gpuMemoryBarrier: buffers` (line 2435).

**Steps:**
1. Create `GpuBuffer` (256 bytes, `storageBuffer`, device-only).
2. Write known data via staging transfer (or use `frequentUpdate`).
3. `gpuCmdBegin()`.
4. **Dispatch 1:** barrier to `shaderWrite | shaderRead`, bind compute pipeline
   that increments every element by 1, dispatch.
5. **Barrier:** buffer to `shaderRead`.
6. **Dispatch 2:** bind compute pipeline that reads elements and writes them
   to a second output buffer.
7. `gpuCmdEnd()`.
8. Read result. Assert `result[i] == original[i] + 1`.
9. **Repeat without the barrier between Dispatch 1 and Dispatch 2:**
   expect incorrect results (data hazard). This confirms the barrier was
   what made step 8 pass.

**Verification:** The barrier actually forces visibility of the first dispatch's
writes before the second dispatch reads them. Without the barrier, the second
dispatch sees stale data.

---

## U5. Image Barrier Transitions Layout Correctly

**Replaces** `gpuMemoryBarrier: images` (line 2448).

**Steps:**
1. Create 4x4 `GpuImage` (`transferSrc | transferDst | sampled | colorAttachment`).
2. Create `GpuView` on it.
3. Write a known pattern via `view.write(src)`.
4. `gpuCmdBegin()`.
5. Barrier: transition image from `undefined` to `transferDst`.
6. Write a different pattern (e.g. all `0xFF0000FF` red) via a second
   `view.write()` call.
7. Barrier: `transferDst` -> `transferSrc`.
8. `gpuCmdEnd()`.
9. Read back. Assert the last-written pattern is correct, proving the layout
   transitions did not corrupt data.

**Verification:** The image layout actually transitions; data written after
a `transferDst` barrier survives a subsequent `transferSrc` barrier without
corruption. The texture is readable after the final layout.

---

## U6. Combined Buffer+Image Barrier in a Dependency Chain

**Replaces** `gpuMemoryBarrier: buffers + images combined` (line 2463).

**Steps:**
1. Create `storageBuf` (256 bytes, `storageBuffer`, device-only).
2. Create 4x4 `GpuImage` (`transferSrc | transferDst | storage`).
3. Write input data into `storageBuf`.
4. `gpuCmdBegin()`.
5. **Barrier:** `storageBuf` to `shaderRead | shaderWrite`, image to `general`.
6. **Dispatch 1:** compute shader reads `storageBuf`, writes result to image
   (via `imageStore`).
7. **Barrier:** `storageBuf` to `transferSrc`, image to `transferSrc`.
8. Read back both resources.
9. Verify `storageBuf` still contains input (or modified as expected).
10. Verify image contains the computed output.

**Verification:** A single `gpuMemoryBarrier` call correctly handles both
buffer and image barriers simultaneously. The combined barrier transitions both
resources to the correct stages/layouts for the subsequent operations.

---

## U7. Compute Pass Dispatch Produces Correct Output

**Replaces** `gpuComputePass` (line 2483).

**Steps:**
1. Create `inBuf` (256 bytes, `storageBuffer`, `frequentUpdate`).
2. Create `outBuf` (256 bytes, `storageBuffer | transferSrc`, device-only).
3. Write `0, 1, 2, ..., 63` into `inBuf`.
4. Create compute pipeline that reads `inBuf`, writes `inBuf[i] * 2` to `outBuf[i]`.
5. `gpuCmdBegin()`.
6. Barrier both buffers to `shaderRead` / `shaderWrite`.
7. `gpuComputePass(cmd, pass)` with `storageBuffers = {&inBuf, &outBuf}`.
8. `gpuBindPipeline`, `gpuDispatch(cmd, 64, 1, 1)`.
9. `gpuCmdEnd()`.
10. Read `outBuf`. Assert `outBuf[i] == inBuf[i] * 2`.

**Verification:** The compute pass structure actually makes resources visible to
the shader and dispatch produces correct results. Without this, the `GpuComputePass`
is just recording barriers that may never take effect.

---

## U8. gpuWaitIdle Waits for Pending Work

**Replaces** `gpuWaitIdle` (line 2629).

**Steps:**
1. Create `GpuBuffer` (64 bytes, `storageBuffer`, device-only).
2. Create staging buffer (64 bytes, `transferSrc`, `frequentUpdate`).
3. Write known data into staging.
4. Submit a compute dispatch that copies/modifies data (via a one-time command
   buffer: `gpuCmdBegin` + dispatch + `gpuCmdEnd`).
5. **Immediately** call `gpuWaitIdle()`.
6. Read the buffer. Assert the computation completed (data is modified from
   original).
7. **Without** `gpuWaitIdle`, the readback would race with the GPU.
   With `gpuWaitIdle`, the result is guaranteed visible.

**Verification:** After `gpuWaitIdle()`, all previously submitted work is
complete and visible to the host. This catches cases where `gpuWaitIdle` returns
before GPU work finishes.

---

## U9. Uniform Buffer Passes Data to Vertex Shader

**Replaces** `GpuBuffer::uniformDescriptor` (line 2634).

**Steps:**
1. Create `uniformBuf` (64 bytes, `GpuBufferUsage_uniformBuffer`,
   `frequentUpdate`).
2. Write a `Vec4` color (e.g. `{0.5, 0.3, 0.7, 1.0}`) into the buffer.
3. Create 4x4 color attachment.
4. Build a graphics pipeline with a uniform buffer binding in the vertex shader.
   The vertex shader propagates the color to the fragment shader via `location`.
5. `gpuRenderPassBegin`, include `uniformBuf` in `pass.uniformBuffers`.
6. Bind pipeline, draw triangle.
7. `gpuRenderPassEnd`, `gpuCmdEnd`.
8. Read back pixels. Assert they match the color from the uniform buffer
   (within rounding for `unorm8`).

**Verification:** The descriptor returned by `uniformDescriptor()` is actually
the correct binding point for uniform data. The shader receives the intended
value.

---

## U10. Storage Image Write via Compute Shader

**Replaces** `GpuView::storageDescriptor` (line 2642).

**Steps:**
1. Create 4x4 `GpuImage` (`transferSrc | transferDst | storage | sampled`).
2. Create `GpuView` with `GpuFilter_nearest`.
3. Build compute pipeline that writes a checkerboard pattern to the storage
   image via `imageStore`.
4. `gpuCmdBegin()`.
5. Barrier: image to `general`.
6. `gpuComputePass` with `storageImages = {&view}`.
7. Bind pipeline, dispatch 4x4x1.
8. Barrier: image to `transferSrc`.
9. `gpuCmdEnd()`.
10. Read back the image via `view.read(dst)`.
11. Assert each pixel matches the checkerboard pattern.

**Verification:** The descriptor returned by `storageDescriptor()` is the correct
binding point for `image2D` stores. Compute shader can write to the image via
this descriptor.

---
