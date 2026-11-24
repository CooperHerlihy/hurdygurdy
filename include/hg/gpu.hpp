#pragma once

#include "hg/inttypes.hpp"

#include <span>
#include <memory>
#include <string_view>
#include <type_traits>

namespace hg {

/**
 * Pixel formats
 */
enum class Format : u32 {
    /**
     * No format
     */
    undefined = 0,
    /**
     * 8 bit
     */
    r8_unorm,
    rg8_unorm,
    rgba8_unorm,
    bgra8_unorm,
    rgba8_srgb,
    bgra8_srgb,
    /**
     * 16 bit
     */
    r16_unorm,
    r16_sfloat,
    rg16_unorm,
    rg16_sfloat,
    rgba16_unorm,
    rgba16_sfloat,
    /**
     * 32 bit
     */
    r32_uint,
    r32_sfloat,
    rg32_sfloat,
    rgba32_sfloat,
    /**
     * Packed
     */
    a2b10g10r10_unorm_pack32,
    b10g11r11_ufloat_pack32,
    /**
     * Depth/stencil
     */
    d16_unorm,
    d32_sfloat,
    s8_uint,
    d16_unorm_s8_uint,
    d24_unorm_s8_uint,
    d32_sfloat_s8_uint,
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

/**
 * How a gpu buffer will be used
 */
struct GpuBufferUsage {
    enum : u32 {
        transferSrc = 0x00000001,
        transferDst = 0x00000002,
        uniformTexelBuffer = 0x00000004,
        storageTexelBuffer = 0x00000008,
        uniformBuffer = 0x00000010,
        storageBuffer = 0x00000020,
        indirectBuffer = 0x00000100,
    };
};
using GpuBufferUsageFlags = u32;

/**
 * How a gpu buffer will be accessed
 */
enum class GpuMemoryUsage : u32 {
    /**
     * It will only be accessed from the device
     */
    deviceOnly,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    stagingWrite,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    stagingRead,
    /**
     * It will be frequently written from the host and read on the device
     */
    frequentUpdate,
};

/**
 * A buffer on the gpu
 */
class GpuBuffer;

/**
 * How an image will be used
 */
struct GpuImageUsage {
    enum : u32 {
        transferSrc = 0x00000001,
        transferDst = 0x00000002,
        sampled = 0x00000004,
        storage = 0x00000008,
        colorAttachment = 0x00000010,
        depthStencilAttachment = 0x00000020,
        transientAttachment = 0x00000040,
        inputAttachment = 0x00000080,
        hostTransfer = 0x00400000,
    };
};
using GpuImageUsageFlags = u32;

/**
 * The layout of an image
 */
enum class GpuLayout : u32 {
    undefined = 0,
    general,
    colorAttachment,
    depthStencilAttachment,
    shaderReadOnly,
    transferSrc,
    transferDst,
    presentSrc,
};

/**
 * Extra config flags for image creation
 */
struct GpuImageConfig {
    enum : u32 {
        cubeCompatible = 0x00000010,
    };
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
    Format format = Format::undefined;
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
 * An image on the gpu
 */
class GpuImage;

/**
 * The dimensionality of an image
 */
enum class GpuViewType : u32 {
    image1D = 0,
    image2D = 1,
    image3D = 2,
    cube = 3,
    array1D = 4,
    array2D = 5,
    cubeArray = 6,
};

/**
 * How a sampler interpolates between pixels
 */
enum class GpuFilter : u32 {
    nearest = 0,
    linear = 1,
    count,
};

/**
 * How a sampler samples off the image's edge
 */
enum class GpuSamplerEdgeMode : u32 {
    repeat = 0,
    mirroredRepeat = 1,
    clampToEdge = 2,
    clampToBorder = 3,
    mirrorClampToEdge = 4,
    count,
};

/**
 * The border color if the sampler edge mode has a border
 */
enum class GpuSamplerBorder : u32 {
    floatTransparentBlack = 0,
    intTransparentBlack = 1,
    floatOpaqueBlack = 2,
    intOpaqueBlack = 3,
    floatOpaqueWhite = 4,
    intOpaqueWhite = 5,
    count,
};

/**
 * Config for GpuView
 */
struct GpuViewCreateInfo {
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
     * The dimensionality of the image
     */
    GpuViewType type = GpuViewType::image2D;
    /**
     * The sampler filter
     */
    GpuFilter filter = GpuFilter::nearest;
    /**
     * How the sampler handles edges
     */
    GpuSamplerEdgeMode edgeMode = GpuSamplerEdgeMode::repeat;
    /**
     * The border color if edge mode uses it
     */
    GpuSamplerBorder border = GpuSamplerBorder::floatTransparentBlack;
};

/**
 * Calculates the maximum number of mipmap levels that an image could have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 getMaxMipmaps(u32 width, u32 height, u32 depth);

/**
 * A view into a GpuImage
 */
class GpuView;

/**
 * How the vertex list is interpreted
 */
enum class GpuTopology : u32 {
    pointList = 0,
    lineList = 1,
    lineStrip = 2,
    triangleList = 3,
    triangleStrip = 4,
    triangleFan = 5,
    lineListWithAdjacency = 6,
    lineStripWithAdjacency = 7,
    triangleListWithAdjacency = 8,
    triangleStripWithAdjacency = 9,
    patchList = 10,
};

/**
 * How to treat vertices
 */
enum class GpuPolygonMode : u32 {
    fill = 0,
    line = 1,
    point = 2,
};

/**
 * The cull mode
 */
enum class GpuCullMode : u32 {
    none = 0,
    front = 1,
    back = 2,
    both = 3,
};

/**
 * Config for GpuPipeline graphics
 */
struct GpuGraphicsPipelineCreateInfo {
    /**
     * The vertex shader code
     */
    std::span<const byte> vertexShader{};
    /**
     * The fragment shader code
     */
    std::span<const byte> fragmentShader{};
    /**
     * The size of the push constant
     */
    u32 pushConstantSize = 0;
    /**
     * The format of the color attachments, none can be undefined
     */
    std::span<const Format> colorAttachmentFormats{};
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    Format depthAttachmentFormat = Format::undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     *
     * TODO: Stencil state is not yet implemented
     */
    Format stencilAttachmentFormat = Format::undefined;
    /**
     * How to interpret vertices into topology
     */
    GpuTopology topology = GpuTopology::triangleList;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    GpuPolygonMode polygonMode = GpuPolygonMode::fill;
    /**
     * Enables back/front face culling
     */
    GpuCullMode cullMode = GpuCullMode::none;
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
    std::span<const bool> colorBlendEnables{};
};

/**
 * A gpu pipeline to execute shaders
 */
class GpuPipeline;

/**
 * A gpu command buffer
 */
class GpuCommandBuffer;

/**
 * The present mode for the swapchain
 */
enum class GpuPresentMode : u32 {
    immediate = 0,
    mailbox = 1,
    fifo = 2,
    fifoRelaxed = 3,
};

/**
 * A window swapchain
 */
class GpuSwapchain;

/**
 * Forward declaration of Window
 */
class Window;

/**
 * The graphics device
 */
class Gpu {
public:
    /**
     * Create the gpu device
     *
     * Parameters
     * - extensions The Vulkan instance extensions required by the windowing backend
     */
    static std::unique_ptr<Gpu> create(std::span<std::string_view> extensions);

    /**
     * Destroy the gpu device
     */
    virtual ~Gpu() noexcept = default;

    /**
     * Wait for the gpu to finish work
     */
    virtual void waitIdle() = 0;

    /**
     * Create a gpu buffer
     *
     * Parameters
     * - size The size in bytes of the buffer
     * - usageFlags How the buffer will be used
     * - memoryUsage How the buffer should be accessed
     */
    virtual std::unique_ptr<GpuBuffer> createBuffer(
        u64 size,
        GpuBufferUsageFlags usageFlags,
        GpuMemoryUsage memoryUsage = GpuMemoryUsage::deviceOnly) = 0;

    /**
     * Create a gpu image assuming most defaults
     */
    virtual std::unique_ptr<GpuImage> createImage(u32 width, u32 height, Format format, GpuImageUsageFlags usage) = 0;

    /**
     * Create a gpu image with extended options
     */
    virtual std::unique_ptr<GpuImage> createImage(const GpuImageCreateInfo& config) = 0;

    /**
     * Create a graphics pipeline
     */
    virtual std::unique_ptr<GpuPipeline> createGraphicsPipeline(const GpuGraphicsPipelineCreateInfo& config) = 0;

    /**
     * Create a compute pipeline
     */
    virtual std::unique_ptr<GpuPipeline> createComputePipeline(std::span<const byte> shaderCode, u32 pushSize = 0) = 0;

    /**
     * Begin a command buffer to be executed once
     *
     * Returns
     * - The command buffer to record, will never be nullptr
     */
    virtual std::unique_ptr<GpuCommandBuffer> beginCommands() = 0;

    /**
     * Execute the command buffer and wait for completion
     *
     * Parameters
     * - cmd The command buffer from beginCmd, must not be nullptr
     */
    virtual void endCommands(std::unique_ptr<GpuCommandBuffer> cmd) = 0;

    /**
     * Acquire an image from each swapchain and begin a command buffer
     *
     * Parameters
     * - windows The windows that will be rendered to
     */
    virtual std::unique_ptr<GpuCommandBuffer> beginFrame(std::span<Window*> windows) = 0;

    /**
     * Finishes recording the command buffer and presents the window images
     *
     * Parameters
     * - cmd The command buffer given from beginFrame
     */
    virtual void endFrame(std::unique_ptr<GpuCommandBuffer> cmd) = 0;

    /**
     * Create a gpu swapchain
     */
    virtual std::unique_ptr<GpuSwapchain> createSwapchain(
        void* surface,
        u32 width,
        u32 height,
        GpuPresentMode presentMode,
        GpuImageUsageFlags imageUsage) = 0;
};

/**
 * A gpu buffer
 */
class GpuBuffer {
public:
    /**
     * Destroy the buffer
     */
    virtual ~GpuBuffer() noexcept = default;

    /**
     * Get the size of the buffer in bytes
     */
    virtual u64 size() const = 0;

    /**
     * Get the uniform buffer descriptor index from the buffer
     */
    virtual u32 uniformDescriptor() const = 0;

    /**
     * Get the storage buffer descriptor index from the buffer
     */
    virtual u32 storageDescriptor() const = 0;

    /**
     * Writes to the gpu buffer
     *
     * Parameters
     * - src The data to write
     * - offset The offset in bytes into the gpu buffer
     */
    virtual void write(std::span<const byte> src, u64 offset) = 0;

    /**
     * Reads from the gpu buffer
     *
     * Parameters
     * - dst The destination to read to
     * - offset The offset in bytes into the gpu buffer
     */
    virtual void read(std::span<byte> dst, u64 offset) const = 0;
};

/**
 * A region of a gpu image
 */
struct GpuImageRegion {
    /**
     * The beginning of the region in the x direction
     */
    i32 x = 0;
    /**
     * The beginning of the region in the y direction
     */
    i32 y = 0;
    /**
     * The beginning of the region in the z direction
     */
    i32 z = 0;
    /**
     * The width of the region extending from x
     */
    u32 width = 0;
    /**
     * The height of the region extending from y
     */
    u32 height = 0;
    /**
     * The depth of the region extending from z
     */
    u32 depth = 0;
    /**
     * The mip level
     */
    u32 mipLevel = 0;
    /**
     * The beginning array layer
     */
    u32 arrayLayer = 0;
    /**
     * The extent of array layers
     */
    u32 layerCount = 1;
};

/**
 * A gpu image
 */
class GpuImage {
public:
    /**
     * Destroy the image
     */
    virtual ~GpuImage() noexcept = default;

    /**
     * Get the format of the image
     */
    virtual Format format() const = 0;

    /**
     * Get the width of the image
     */
    virtual u32 width() const = 0;

    /**
     * Get the height of the image
     */
    virtual u32 height() const = 0;

    /**
     * Get the height of the image
     */
    virtual u32 depth() const = 0;

    /**
     * Get the number of mipmaps in the image
     */
    virtual u32 mipmapCount() const = 0;

    /**
     * Get the number of array layers in the image
     */
    virtual u32 layerCount() const = 0;

    /**
     * Create a view into the image
     */
    virtual std::unique_ptr<GpuView> createView(GpuFilter filter = GpuFilter::nearest) = 0;

    /**
     * Create a view with extended config
     */
    virtual std::unique_ptr<GpuView> createView(const GpuViewCreateInfo& config) = 0;

    /**
     * Write to a section of the image from a buffer
     *
     * Note, src must be the same size as the box
     */
    virtual void write(std::span<const byte> src, const GpuImageRegion& dst) = 0;

    /**
     * Write to the whole image from a buffer
     *
     * Note, src must be the same size as the image, including array layers
     */
    virtual void writeWhole(std::span<const byte> src, u32 mipLevel = 0) = 0;

    /**
     * Write to the image as a cubemap
     *
     * Note, there must be 6 array layers
     * Note, src must be the size of the image * 12
     *
     * src is assumed to be layed out as:
     *  #
     * ####
     *  #
     */
    virtual void writeCubemap(std::span<const byte> src, u32 mipLevel = 0) = 0;

    /**
     * Generate mipmaps from the base level
     */
    virtual void generateMipmaps(u32 arrayLayer = 0) = 0;

    /**
     * Read from a section of the image
     *
     * Note, byte should be reinterpreted as pixels
     */
    virtual void read(std::span<byte> dst, const GpuImageRegion& src) = 0;

    /**
     * Read from the whole image, including all array layers
     *
     * Note, byte should be reinterpreted as pixels
     */
    virtual void readWhole(std::span<byte> dst, u32 mipLevel = 0) = 0;

    /**
     * Read from the image as a cubemap
     *
     * Note, there must be 6 array layers
     * Note, only reads one mip layer
     *
     * The vector is layed out as:
     *  #
     * ####
     *  #
     *
     * Note, byte should be reinterpreted as pixels
     */
    virtual void readCubemap(std::span<byte> dst, u32 mipLevel = 0) = 0;
};

/**
 * A view into a gpu image for shaders
 *
 * The view references the GpuImage it was created from internally.
 * The GpuImage must outlive any GpuView that references it.
 */
class GpuView {
public:
    /**
     * Destroy the gpu view
     */
    virtual ~GpuView() noexcept = default;

    /**
     * Get the format of an image
     */
    virtual Format format() const = 0;

    /**
     * Get the width of an image
     */
    virtual u32 width() const = 0;

    /**
     * Get the height of an image
     */
    virtual u32 height() const = 0;

    /**
     * Get the depth of an image
     */
    virtual u32 depth() const = 0;

    /**
     * Get the number of mipmaps in the image
     */
    virtual u32 mipmapCount() const = 0;

    /**
     * Get the number of array layers in the image
     */
    virtual u32 layerCount() const = 0;

    /**
     * Get the image sampler descriptor index from the image view
     */
    virtual u32 samplerDescriptor() const = 0;

    /**
     * Get the storage image descriptor index from the image view
     */
    virtual u32 storageDescriptor() const = 0;
};

/**
 * A gpu pipeline
 */
class GpuPipeline {
public:
    /**
     * Destroy a graphics or compute pipeline
     */
    virtual ~GpuPipeline() noexcept = default;
};

/**
 * How a gpu resource will be accessed
 */
enum class GpuAccess : u32 {
    indirectBuffer,
    uniformBufferCompute,
    uniformBufferVertex,
    uniformBufferFragment,
    uniformBufferAllGraphics,
    storageBufferCompute,
    storageBufferVertex,
    storageBufferFragment,
    storageBufferAllGraphics,
    sampledImageCompute,
    sampledImageFragment,
    storageImageCompute,
    storageImageFragment,
    colorAttachment,
    depthStencilAttachment,
    transferSrc,
    transferDst,
    hostRead,
};

/**
 * An image dependency barrier
 */
struct GpuImageBarrier {
    GpuView* image = nullptr;
    GpuAccess nextAccess = GpuAccess::transferDst;
    GpuLayout nextLayout = GpuLayout::undefined;
};

/**
 * A buffer dependency barrier
 */
struct GpuBufferBarrier {
    GpuBuffer* buffer = nullptr;
    GpuAccess nextAccess = GpuAccess::transferDst;
};

/**
 * The operation to load a render attachment
 */
enum class GpuLoadOp : u32 {
    load = 0,
    clear = 1,
    dontCare = 2,
};

/**
 * The operation to store a render attachment
 */
enum class GpuStoreOp : u32 {
    store = 0,
    dontCare = 1,
};

/**
 * The value to clear depth and stencil attachments to
 */
struct GpuClearValueDepthStencil {
    f32 depth = 0.0f;
    u32 stencil = 0;
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
    GpuLoadOp loadOp = GpuLoadOp::clear;
    /**
     * How the image will be stored
     */
    GpuStoreOp storeOp = GpuStoreOp::store;
    /**
     * What to clear the image to if cleared
     */
    union {
        /**
         * The color to clear to
         */
        f32 clearColor[4];
        /**
         * The depth stencil values to clear to
         */
        GpuClearValueDepthStencil clearDepthStencil;
    };
};

/**
 * A render/compute pass description
 */
struct GpuPass {
    /**
     * The uniform buffer dependencies
     */
    std::span<GpuBuffer*> uniformBuffers{};
    /**
     * The storage buffer dependencies
     */
    std::span<GpuBuffer*> storageBuffers{};
    /**
     * The sampled image dependencies
     */
    std::span<GpuView*> sampledImages{};
    /**
     * The storage image dependencies
     */
    std::span<GpuView*> storageImages{};
    /**
     * The color images to write to
     */
    std::span<const GpuAttachment> colorAttachments{};
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
 * A gpu command buffer
 */
class GpuCommandBuffer {
public:
    /**
     * Destroy a graphics or compute pipeline
     */
    virtual ~GpuCommandBuffer() noexcept = default;

    /**
     * Creates a barrier for resource uses that are not part of a render pass
     *
     * Parameters
     * - bufferBarriers The buffer barriers
     * - imageBarriers The image barriers
     */
    virtual void memoryBarrier (
        std::span<const GpuBufferBarrier> bufferBarriers,
        std::span<const GpuImageBarrier> imageBarriers) = 0;

    /**
     * Copies a section of a buffer to another
     *
     * Parameters
     * - dst The buffer to copy to
     * - dstOffset The distance in bytes into dst to start copying
     * - src The buffer to copy from
     * - srcOffset The distance in bytes into src to start copying
     * - size The size in bytes to copy
     */
    virtual void copyBuffer(
        const GpuBuffer* dst,
        usize dstOffset,
        const GpuBuffer* src,
        usize srcOffset,
        usize size) = 0;

    /**
     * Copies part of a 3d image to part of another 3d image
     *
     * Both images must be the same format
     * Both regions must be the same size
     *
     * Parameters
     * - dst The image to copy to
     * - dstRegion The part of dst to copy to
     * - src The image to copy from
     * - srcRegion The part of src to copy from
     */
    virtual void copyImage(
        const GpuImage* dst,
        const GpuImageRegion& dstRegion,
        const GpuImage* src,
        const GpuImageRegion& srcRegion) = 0;

    /**
     * Copies a whole image to another whole image, including all array layers
     *
     * Both images must be the same size and format
     *
     * Parameters
     * - dst The image to copy to
     * - src The image to copy from
     */
    virtual void copyImageWhole(
        const GpuImage* dst,
        const GpuImage* src) = 0;

    /**
     * Blit part of a 3d image to part of another 3d image
     *
     * Images and boxes may be different formats and sizes
     *
     * Parameters
     * - dst The image to copy to
     * - dstRegion The region in dst to copy to
     * - src The image to copy from
     * - srcRegion The region in src to copy from
     */
    virtual void blitImage(
        const GpuImage* dst,
        const GpuImageRegion& dstRegion,
        const GpuImage* src,
        const GpuImageRegion& srcRegion) = 0;

    /**
     * Blit a whole image to another whole image
     *
     * Images and boxes may be different formats and sizes
     *
     * Parameters
     * - dst The image to copy to
     * - src The image to copy from
     */
    virtual void blitImageWhole(
        const GpuImage* dst,
        const GpuImage* src) = 0;

    /**
     * Performs memory barriers for compute shader resources
     *
     * Note, all attachments in the pass description are ignored
     */
    virtual void computePass(const GpuPass& pass) = 0;

    /**
     * Performs render barrier and begins a render pass
     */
    virtual void beginRenderPass(const GpuPass& pass) = 0;

    /**
     * Ends the render pass
     */
    virtual void endRenderPass() = 0;

    /**
     * Set the rendering viewport, should be called after gpuRenderPassBegin
     */
    virtual void gpuSetViewport(f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f) = 0;

    /**
     * Set the rendering scissor, should be called after gpuRenderPassBegin
     */
    virtual void gpuSetScissor(i32 x, i32 y, u32 width, u32 height) = 0;

    /**
     * Bind a graphics or compute pipeline
     */
    virtual void bindPipeline(const GpuPipeline* pipeline) = 0;

    /**
     * Push constants to the shader
     *
     * Parameters
     * - pipeline The pipeline to push to
     * - push The data to push, must fit the pipeline's push constant size
     */
    virtual void pushConstants(const GpuPipeline* pipeline, std::span<const byte> push) = 0;

    /**
     * Push constants to the shader
     *
     * Parameters
     * - pipeline The pipeline to push to
     * - push The data to push, must fit the pipeline's push constant size
     */
    template<typename T> requires std::is_trivially_copyable_v<T>
    void pushConstants(const GpuPipeline* pipeline, const T& push)
    {
        pushConstants(pipeline, std::as_bytes(std::span{&push, 1}));
    }

    /**
     * Issue a draw call
     *
     * Parameters
     * - vertexBegin The index of the first vertex to draw
     * - vertexCount The number of vertices to draw
     * - instanceBegin The index of the first instance to draw
     * - instanceCount The number of instances to draw
     */
    virtual void draw(u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount) = 0;

    /**
     * Dispatch a compute shader
     *
     * Parameters
     * - groupCountX The number of workgroups in the x dimension
     * - groupCountY The number of workgroups in the y dimension
     * - groupCountZ The number of workgroups in the z dimension
     */
    virtual void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) = 0;
};

/**
 * A gpu swapchain, used internally for window implementations
 */
class GpuSwapchain {
public:
    /**
     * Destroy the gpu swapchain
     */
    virtual ~GpuSwapchain() noexcept = default;

    /**
     * Recreate the swapchain with a new size
     */
    virtual void resize(u32 width, u32 height) = 0;

    /**
     * Get the current swapchain width
     */
    virtual u32 width() const = 0;

    /**
     * Get the current swapchain height
     */
    virtual u32 height() const = 0;

    /**
     * Get the current swapchain format
     */
    virtual Format format() const = 0;

    /**
     * Get the current swapchain image count
     */
    virtual u32 imageCount() const = 0;

    /**
     * Returns whether an image is available this frame
     */
    virtual bool frameAvailable() const = 0;

    /**
     * Get the current swapchain image
     *
     * Note, frameAvailable must return true
     */
    virtual GpuImage* currentImage() const = 0;

    /**
     * Get the current swapchain image view
     *
     * Note, frameAvailable must return true
     */
    virtual GpuView* currentView() const = 0;
};

} // namespace hg
