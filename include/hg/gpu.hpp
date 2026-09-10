#pragma once

#include "hg/inttypes.hpp"
#include "hg/span.hpp"
#include "hg/smart_ptr.hpp"
#include "hg/math.hpp"

namespace hg {

/**
 * Wait for the GPU to finish work
 */
void gpuWaitIdle();

/**
 * Pixel formats
 */
enum Format : u32 {
    /**
     * No format
     */
    Format_undefined = 0,
    /**
     * 8 bit
     */
    Format_r8_unorm,
    Format_rg8_unorm,
    Format_rgba8_unorm,
    Format_bgra8_unorm,
    Format_rgba8_srgb,
    Format_bgra8_srgb,
    /**
     * 16 bit
     */
    Format_r16_unorm,
    Format_r16_sfloat,
    Format_rg16_unorm,
    Format_rg16_sfloat,
    Format_rgba16_unorm,
    Format_rgba16_sfloat,
    /**
     * 32 bit
     */
    Format_r32_uint,
    Format_r32_sfloat,
    Format_rg32_sfloat,
    Format_rgba32_sfloat,
    /**
     * Packed
     */
    Format_a2b10g10r10_unorm_pack32,
    Format_b10g11r11_ufloat_pack32,
    /**
     * Depth/stencil
     */
    Format_d16_unorm,
    Format_d32_sfloat,
    Format_s8_uint,
    Format_d16_unorm_s8_uint,
    Format_d24_unorm_s8_uint,
    Format_d32_sfloat_s8_uint,
    /**
     * BC compressed
     */
    Format_bc1_rgb_unorm_block,
    Format_bc3_unorm_block,
    Format_bc4_unorm_block,
    Format_bc5_unorm_block,
    Format_bc7_unorm_block,
    /**
     * ASTC compressed
     */
    Format_astc_4x4_unorm_block,
    Format_astc_8x8_unorm_block,
};

/**
 * Turns a Format into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 formatToSize(Format format);

// Vulkan allocator : TODO?

/**
 * How a gpu buffer will be used
 */
enum GpuBufferUsage : u32 {
    GpuBufferUsage_transferSrc = 0x00000001,
    GpuBufferUsage_transferDst = 0x00000002,
    GpuBufferUsage_uniformTexelBuffer = 0x00000004,
    GpuBufferUsage_storageTexelBuffer = 0x00000008,
    GpuBufferUsage_uniformBuffer = 0x00000010,
    GpuBufferUsage_storageBuffer = 0x00000020,
    GpuBufferUsage_indirectBuffer = 0x00000100,
};
using GpuBufferUsageFlags = u32;

/**
 * How a gpu buffer will be accessed
 */
enum GpuMemoryUsage : u32 {
    /**
     * It will only be accessed from the device
     */
    GpuMemoryUsage_deviceOnly,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    GpuMemoryUsage_stagingWrite,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    GpuMemoryUsage_stagingRead,
    /**
     * It will be frequently written from the host and read on the device
     */
    GpuMemoryUsage_frequentUpdate,
};

/**
 * GpuBuffer implementation data
 */
struct GpuBufferData;

/**
 * A gpu buffer
 */
struct GpuBuffer {
    /**
     * The implementation data
     */
    UniquePtr<GpuBufferData> data;

    /**
     * Construct empty
     */
    GpuBuffer() noexcept;

    /**
     * Create a gpu buffer
     *
     * Parameters
     * - size The size in bytes of the buffer
     * - usageFlags How the buffer will be used
     * - memoryUsage How the buffer should be accessed
     */
    static GpuBuffer create(
        u64 size,
        GpuBufferUsageFlags usageFlags,
        GpuMemoryUsage memoryUsage = GpuMemoryUsage_deviceOnly);

    /**
     * Destroy the gpu buffer
     */
    ~GpuBuffer() noexcept;

    /**
     * Get the uniform buffer descriptor index from the buffer
     */
    u32 uniformDescriptor() const;

    /**
     * Get the storage buffer descriptor index from the buffer
     */
    u32 storageDescriptor() const;

    /**
     * Writes to the gpu buffer
     *
     * Parameters
     * - src The data to write, must not be nullptr
     * - offset The offset in bytes into the gpu buffer
     * - size The size in bytes to write
     */
    void write(const void* src, u64 offset, u64 size);

    /**
     * Reads from the gpu buffer
     *
     * Parameters
     * - dst The location to write to, must not be nullptr
     * - offset The offset in bytes into the gpu buffer
     * - size The size in bytes to read
     */
    void read(void* dst, u64 offset, u64 size);

    /**
     * Move construct
     */
    GpuBuffer(GpuBuffer&&) noexcept;

    /**
     * Move assign
     */
    GpuBuffer& operator=(GpuBuffer&&) noexcept;

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
};

/**
 * How an image will be used
 */
enum GpuImageUsage : u32 {
    GpuImageUsage_transferSrc = 0x00000001,
    GpuImageUsage_transferDst = 0x00000002,
    GpuImageUsage_sampled = 0x00000004,
    GpuImageUsage_storage = 0x00000008,
    GpuImageUsage_colorAttachment = 0x00000010,
    GpuImageUsage_depthStencilAttachment = 0x00000020,
    GpuImageUsage_transientAttachment = 0x00000040,
    GpuImageUsage_inputAttachment = 0x00000080,
    GpuImageUsage_hostTransfer = 0x00400000,
};
using GpuImageUsageFlags = u32;

/**
 * The layout of an image
 */
enum GpuLayout : u32 {
    GpuLayout_undefined = 0,
    GpuLayout_general,
    GpuLayout_colorAttachment,
    GpuLayout_depthStencilAttachment,
    GpuLayout_shaderReadOnly,
    GpuLayout_transferSrc,
    GpuLayout_transferDst,
    GpuLayout_presentSrc,
};

/**
 * Extra config flags for image creation
 */
enum GpuImageConfig : u32 {
    GpuImageConfig_cubeCompatible = 0x00000010,
};
using GpuImageConfigFlags = u32;

/**
 * Config for GpuImage
 */
struct GpuImageCreateInfo {
    /**
     * The dimensions of the image
     */
    u32 dimensions = 2;
    /**
     * The width of the image
     */
    u32 width = 1;
    /**
     * The height of the image
     */
    u32 height = 1;
    /**
     * The depth of the image
     */
    u32 depth = 1;
    /**
     * The format of the image, must not be undefined
     */
    Format format = Format_undefined;
    /**
     * The number of mip level
     */
    u32 mipLevels = 1;
    /**
     * The number of array layers
     */
    u32 arrayLayers = 1;
    /**
     * The number of MSAA samples
     */
    u32 msaaSamples = 1;
    /**
     * How the image will be used, must not be 0
     */
    GpuImageUsageFlags usage = 0;
    /**
     * Extra config flags
     */
    GpuImageConfigFlags flags = 0;
};

/**
 * GpuImage implementation data
 */
struct GpuImageData;

/**
 * A gpu image
 */
struct GpuImage {
    /**
     * The implementation data
     */
    UniquePtr<GpuImageData> data;

    /**
     * Construct empty
     */
    GpuImage() noexcept;

    /**
     * Create a gpu image assuming most defaults
     */
    static GpuImage create(u32 width, u32 height, Format format, GpuImageUsageFlags usage);

    /**
     * Create a gpu image with extended options
     */
    static GpuImage createEx(const GpuImageCreateInfo& config);

    /**
     * Destroy a gpu image
     */
    ~GpuImage() noexcept;

    /**
     * Get the width of an image
     */
    u32 width() const;

    /**
     * Get the height of an image
     */
    u32 height() const;

    /**
     * Move construct
     */
    GpuImage(GpuImage&&) noexcept;

    /**
     * Move assign
     */
    GpuImage& operator=(GpuImage&&) noexcept;

    GpuImage(const GpuImage&) = delete;
    GpuImage& operator=(const GpuImage&) = delete;
};

/**
 * The dimensionality of an image
 */
enum GpuViewType : u32 {
    GpuViewType_1D = 0,
    GpuViewType_2D = 1,
    GpuViewType_3D = 2,
    GpuViewType_cube = 3,
    GpuViewType_1DArray = 4,
    GpuViewType_2DArray = 5,
    GpuViewType_cubeArray = 6,
};

/**
 * The aspect the image will be accessed in
 */
enum GpuAspect : u32 {
    GpuAspect_none = 0,
    GpuAspect_color = 0x00000001,
    GpuAspect_depth = 0x00000002,
    GpuAspect_stencil = 0x00000004,
    GpuAspect_metadata = 0x00000008,
    GpuAspect_plane0 = 0x00000010,
    GpuAspect_plane1 = 0x00000020,
    GpuAspect_plane2 = 0x00000040,
};
using GpuAspectFlags = u32;

/**
 * How a sampler interpolates between pixels
 */
enum GpuFilter : u32 {
    GpuFilter_nearest = 0,
    GpuFilter_linear = 1,
    GpuFilter_count,
};

/**
 * How a sampler samples off the image's edge
 */
enum GpuSamplerEdgeMode : u32 {
    GpuSamplerEdgeMode_repeat = 0,
    GpuSamplerEdgeMode_mirroredRepeat = 1,
    GpuSamplerEdgeMode_clampToEdge = 2,
    GpuSamplerEdgeMode_clampToBorder = 3,
    GpuSamplerEdgeMode_mirrorClampToEdge = 4,
    GpuSamplerEdgeMode_count,
};

/**
 * The border color if the sampler edge mode has a border
 */
enum GpuSamplerBorder : u32 {
    GpuSamplerBorder_floatTransparentBlack = 0,
    GpuSamplerBorder_intTransparentBlack = 1,
    GpuSamplerBorder_floatOpaqueBlack = 2,
    GpuSamplerBorder_intOpaqueBlack = 3,
    GpuSamplerBorder_floatOpaqueWhite = 4,
    GpuSamplerBorder_intOpaqueWhite = 5,
    GpuSamplerBorder_count,
};

/**
 * Config for GpuView
 */
struct GpuViewCreateInfo {
    /**
     * The image to view
     */
    GpuImage* image = nullptr;
    /**
     * The index of the base mipmap level
     */
    u32 baseMipLevel = 0;
    /**
     * The number of mip levels after base
     */
    u32 levelCount = 1;
    /**
     * The index of the base array layer
     */
    u32 baseArrayLayer = 0;
    /**
     * The numner of array layers after base
     */
    u32 layerCount = 1;
    /**
     * The aspect the image will be accessed in
     */
    GpuAspectFlags aspectFlags = 0;
    /**
     * The dimensionality of the image
     */
    GpuViewType type = {};
    /**
     * The sampler filter
     */
    GpuFilter filter = {};
    /**
     * How the sampler handles edges
     */
    GpuSamplerEdgeMode edgeMode = {};
    /**
     * The border color if edge mode uses it
     */
    GpuSamplerBorder border = {};
};

/**
 * GpuView implemenation data
 */
struct GpuViewData;

/**
 * A gpu view
 *
 * The view references the GpuImage it was created from internally.
 * The GpuImage must outlive any GpuView that references it.
 */
struct GpuView {
    /**
     * The implementation data
     */
    UniquePtr<GpuViewData> data;

    /**
     * Construct empty
     */
    GpuView() noexcept;

    /**
     * Create a gpu image view
     */
    static GpuView create(
        GpuImage& image,
        GpuAspectFlags aspectFlags,
        GpuFilter filter = GpuFilter_nearest);

    /**
     * Create a gpu image view with extended config
     */
    static GpuView createEx(const GpuViewCreateInfo& config);

    /**
     * Destroy the gpu view
     */
    ~GpuView() noexcept;

    /**
     * Get the width of an image
     */
    u32 width() const;

    /**
     * Get the height of an image
     */
    u32 height() const;

    /**
     * Get the image sampler descriptor index from the image view
     */
    u32 samplerDescriptor() const;

    /**
     * Get the storage image descriptor index from the image view
     */
    u32 storageDescriptor() const;

    /**
     * Write to a gpu image
     *
     * Note, only fills the base mip level
     */
    void write(const void* src);

    /**
     * Write to a gpu image cubemap
     *
     * Note, the view must have at least 6 array layers to fill
     *
     * Only the base mip level is filled
     *
     * src is assumed to be layed out as:
     *  #
     * ####
     *  #
     */
    void writeCubemap(const void* src);

    /**
     * Read from a gpu image
     *
     * Note, only the base mip level is read
     */
    void read(void* dst);

    /**
     * Generates mipmaps from the base level
     *
     * Note, the view should only have 1 array layer
     */
    void genMipmaps();

    /**
     * Move construct
     */
    GpuView(GpuView&&) noexcept;

    /**
     * Move assign
     */
    GpuView& operator=(GpuView&&) noexcept;

    GpuView(const GpuView&) = delete;
    GpuView& operator=(const GpuView&) = delete;
};

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 getMaxMipmaps(u32 width, u32 height, u32 depth);

/**
 * How the vertex list is interpreted
 */
enum GpuTopology : u32 {
    GpuTopology_pointList = 0,
    GpuTopology_lineList = 1,
    GpuTopology_lineStrip = 2,
    GpuTopology_triangleList = 3,
    GpuTopology_triangleStrip = 4,
    GpuTopology_triangleFan = 5,
    GpuTopology_lineListWithAdjacency = 6,
    GpuTopology_lineStripWithAdjacency = 7,
    GpuTopology_triangleListWithAdjacency = 8,
    GpuTopology_triangleStripWithAdjacency = 9,
    GpuTopology_patchList = 10,
};

/**
 * How to treat vertices
 */
enum GpuPolygonMode : u32 {
    GpuPolygonMode_fill = 0,
    GpuPolygonMode_line = 1,
    GpuPolygonMode_point = 2,
};

enum GpuCull : u32 {
    GpuCull_none = 0,
    GpuCull_front = 0x00000001,
    GpuCull_back = 0x00000002,
    GpuCull_both = 0x00000003,
};
using GpuCullFlags = u32;

/**
 * Config for GpuPipeline graphics
 */
struct GpuGraphicsPipelineCreateInfo {
    /**
     * The vertex shader code
     */
    Span<const u8> vertexShader{};
    /**
     * The fragment shader code
     */
    Span<const u8> fragmentShader{};
    /**
     * The size of the push constant
     */
    u32 pushConstantSize = 0;
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    Span<const Format> colorAttachmentFormats;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    Format depthAttachmentFormat = Format_undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     *
     * TODO: Stencil state is not yet implemented
     */
    Format stencilAttachmentFormat = Format_undefined;
    /**
     * How to interpret vertices into topology
     */
    GpuTopology topology = GpuTopology_triangleList;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    GpuPolygonMode polygonMode = GpuPolygonMode_fill;
    /**
     * Enables back/front face culling
     */
    GpuCullFlags cullMode = GpuCull_none;
    /**
     * How many samples are used in MSAA
     */
    u32 multisampleCount = 1;
    /**
     * Enables culling fragments by comparing to a depth buffer
     */
    bool enableDepthRead = false;
    /**
     * Enables writing the depth to the depth buffer
     */
    bool enableDepthWrite = false;
    /**
     * Enables color blending using pixel alpha values for each color attachment
     *
     * Empty disables all blend
     */
    Span<const bool> colorBlendEnables{};
};

/**
 * Config for GpuPipeline compute
 */
struct GpuComputePipelineCreateInfo {
    /**
     * The spirv compute shader code, must not be nullptr
     */
    Span<const u8> shaderCode{};
    /**
     * The size in bytes of the push constant, if any
     */
    u32 pushSize = 0;
};

/**
 * GpuPipeline implementation data
 */
struct GpuPipelineData;

/**
 * A gpu pipeline
 */
struct GpuPipeline {
    /**
     * Implementation data
     */
    UniquePtr<GpuPipelineData> data;

    /**
     * Construct empty
     */
    GpuPipeline() noexcept;

    /**
     * Create a graphics pipeline
     */
    static GpuPipeline graphics(const GpuGraphicsPipelineCreateInfo& config);

    /**
     * Create a compute pipeline
     */
    static GpuPipeline compute(Span<const u8> shaderCode, u32 pushSize = 0);

    /**
     * Destroy a graphics or compute pipeline
     */
    ~GpuPipeline() noexcept;

    /**
     * Move construct
     */
    GpuPipeline(GpuPipeline&&) noexcept;

    /**
     * Move assign
     */
    GpuPipeline& operator=(GpuPipeline&&) noexcept;

    GpuPipeline(const GpuPipeline&) = delete;
    GpuPipeline& operator=(const GpuPipeline&) = delete;
};

/**
 * A gpu command buffer
 */
struct GpuCmd;

/**
 * Begin a command buffer to be executed once
 *
 * Returns
 * - The command buffer to record, will never be nullptr
 */
GpuCmd* gpuCmdBegin();

/**
 * Execute the command buffer and wait for completion
 *
 * Parameters
 * - cmd The command buffer from beginGpuCommands, must not be nullptr
 */
void gpuCmdEnd(GpuCmd* cmd);

/**
 * Bind a graphics or compute pipeline
 */
void gpuBindPipeline(GpuCmd* cmd, const GpuPipeline& pipeline);

/**
 * Push constants to the shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - pipeline The pipeline to push to
 * - offset The offset into the push range
 * - size The size of the data
 * - push The data to push
 */
void gpuPushConstants(GpuCmd* cmd, const GpuPipeline& pipeline, void* push, u32 size);

/**
 * Issue a draw call
 *
 * Parameters
 * - cmd The command buffer to record to
 * - vertexBegin The index of the first vertex to draw
 * - vertexCount The number of vertices to draw
 * - instanceBegin The index of the first instance to draw
 * - instanceCount The number of instances to draw
 */
void gpuDraw(GpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount);

/**
 * Dispatch a compute shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - groupCountX The number of workgroups in the x dimension
 * - groupCountY The number of workgroups in the y dimension
 * - groupCountZ The number of workgroups in the z dimension
 */
void gpuDispatch(GpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ);

/**
 * How a gpu resource will be accessed
 */
enum GpuAccess : u32 {
    GpuAccess_indirectBuffer,
    GpuAccess_uniformBufferCompute,
    GpuAccess_uniformBufferVertex,
    GpuAccess_uniformBufferFragment,
    GpuAccess_uniformBufferAllGraphics,
    GpuAccess_storageBufferCompute,
    GpuAccess_storageBufferVertex,
    GpuAccess_storageBufferFragment,
    GpuAccess_storageBufferAllGraphics,
    GpuAccess_sampledImageCompute,
    GpuAccess_sampledImageFragment,
    GpuAccess_storageImageCompute,
    GpuAccess_storageImageFragment,
    GpuAccess_colorAttachment,
    GpuAccess_depthStencilAttachment,
    GpuAccess_transferSrc,
    GpuAccess_transferDst,
    GpuAccess_hostRead,
};

/**
 * An image dependency barrier
 */
struct GpuImageBarrier {
    GpuView* image = nullptr;
    GpuAccess nextAccess = GpuAccess_transferDst;
    GpuLayout nextLayout = GpuLayout_undefined;
};

/**
 * A buffer dependency barrier
 */
struct GpuBufferBarrier {
    GpuBuffer* buffer = nullptr;
    GpuAccess nextAccess = GpuAccess_transferDst;
};

/**
 * Creates a barrier for resource uses that are not part of a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - bufferBarriers The buffer barriers
 * - imageBarriers The image barriers
 */
void gpuMemoryBarrier(
    GpuCmd* cmd,
    Span<const GpuBufferBarrier> bufferBarriers,
    Span<const GpuImageBarrier> imageBarriers);

/**
 * The operation to load a render attachment
 */
enum GpuLoadOp : u32 {
    GpuLoadOp_load = 0,
    GpuLoadOp_clear = 1,
    GpuLoadOp_dontCare = 2,
};

/**
 * The operation to store a render attachment
 */
enum GpuStoreOp : u32 {
    GpuStoreOp_store = 0,
    GpuStoreOp_dontCare = 1,
};

/**
 * The value to clear depth and stencil attachments to
 */
struct GpuClearValueDepthStencil {
    f32 depth = 0.0f;
    u32 stencil = 0;
};

/**
 * The value to clear a render attachment to
 */
union GpuClearValue {
    Vec4 color;
    GpuClearValueDepthStencil depthStencil;
};

/**
 * A rendering attachment
 */
struct GpuAttachment {
    /**
     * The image attached, must not be nullptr
     */
    GpuView* image = nullptr;
    /**
     * How the image will be loaded
     */
    GpuLoadOp loadOp = GpuLoadOp_clear;
    /**
     * How the image will be stored
     */
    GpuStoreOp storeOp = GpuStoreOp_store;
    /**
     * What to clear the image to if cleared
     */
    GpuClearValue clearValue = {};
};

/**
 * A render/compute pass description
 */
struct GpuPass {
    /**
     * The uniform buffer dependencies
     */
    Span<GpuBuffer*> uniformBuffers{};
    /**
     * The storage buffer dependencies
     */
    Span<GpuBuffer*> storageBuffers{};
    /**
     * The sampled image dependencies
     */
    Span<GpuView*> sampledImages{};
    /**
     * The storage image dependencies
     */
    Span<GpuView*> storageImages{};
    /**
     * The color images to write to
     */
    Span<const GpuAttachment> colorAttachments{};
    /**
     * The number of layers in each color attachment to write to
     */
    u32 layerCount = 1;
    /**
     * The depth attachment, if any
     */
    const GpuAttachment* depthAttachment = nullptr;
    /**
     * The stencil attachment, if any
     */
    const GpuAttachment* stencilAttachment = nullptr;
};

/**
 * Performs memory barriers for compute shader resources
 *
 * Note, all attachments in the pass description are ignored
 *
 * Parameters
 * - cmd The command buffer
 * - pass The compute pass description
 */
void gpuComputePass(GpuCmd* cmd, const GpuPass& pass);

/**
 * Performs render barrier and begins a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - pass The render pass description
 */
void gpuBeginRenderPass(GpuCmd* cmd, const GpuPass& pass);

/**
 * Ends the render pass
 *
 * Parameters
 * - cmd The command buffer
 */
void gpuEndRenderPass(GpuCmd* cmd);

/**
 * Set the rendering viewport, should be called after gpuRenderPassBegin
 */
void gpuSetViewport(GpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f);

/**
 * Set the rendering scissor, should be called after gpuRenderPassBegin
 */
void gpuSetScissor(GpuCmd* cmd, i32 x, i32 y, u32 width, u32 height);

/**
 * The present mode for the swapchain
 */
enum GpuPresentMode : u32 {
    GpuPresentMode_immediate = 0,
    GpuPresentMode_mailbox = 1,
    GpuPresentMode_fifo = 2,
    GpuPresentMode_fifoRelaxed = 3,
};

/**
 * GpuSwapchain implementation data
 */
struct GpuSwapchainData;

/**
 * A gpu swapchain, used internally for window implementations
 */
struct GpuSwapchain {
    /**
     * The implementation data
     */
    UniquePtr<GpuSwapchainData> data;

    /**
     * Construct empty
     */
    GpuSwapchain() noexcept;

    /**
     * Create a gpu swapchain
     */
    static GpuSwapchain create(
        void* surface,
        u32 width,
        u32 height,
        GpuPresentMode presentMode,
        GpuImageUsageFlags imageUsage);

    /**
     * Recreate the swapchain with a new size
     */
    void resize(u32 width, u32 height);

    /**
     * Destroy the gpu swapchain
     */
    ~GpuSwapchain() noexcept;

    /**
     * Get the current swapchain size
     */
    void size(u32* width, u32* height) const;

    /**
     * Get the current swapchain image count
     */
    u32 imageCount() const;

    /**
     * Get the current swapchain image view
     */
    GpuView* renderTarget() const;

    /**
     * Get the current swapchain format
     */
    Format format() const;

    /**
     * Move construct
     */
    GpuSwapchain(GpuSwapchain&& other) noexcept;

    /**
     * Move assign
     */
    GpuSwapchain& operator=(GpuSwapchain&& other) noexcept;

    GpuSwapchain(const GpuSwapchain&) = delete;
    GpuSwapchain& operator=(const GpuSwapchain&) = delete;
};

/**
 * Forward declaration of Window
 */
struct Window;

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
GpuCmd* gpuBeginFrame(Span<Window*> windows);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void gpuEndFrame(GpuCmd* cmd);

} // namespace hg
