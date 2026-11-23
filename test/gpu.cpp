#include "tests.hpp"
#include "hg/gpu.hpp"

#include "shaders/compute.comp.spv.h"
#include "shaders/tri.vert.spv.h"
#include "shaders/tri.frag.spv.h"
#include "shaders/sampler.frag.spv.h"
#include "shaders/mul.comp.spv.h"
#include "shaders/buf_to_img.comp.spv.h"
#include "shaders/uniform.vert.spv.h"
#include "shaders/uniform.frag.spv.h"
#include "shaders/storage_img.comp.spv.h"
#include "shaders/depth.vert.spv.h"
#include "shaders/depth.frag.spv.h"
#include "shaders/instance.vert.spv.h"
#include "shaders/instance.frag.spv.h"
#include "shaders/blur.comp.spv.h"
#include "shaders/invert.frag.spv.h"

#include <algorithm>
#include <iterator>

using namespace hg;

// ---- shared shader spans ----
static std::span<const byte> shTriVert = std::as_bytes(std::span{tri_vert_spv});
static std::span<const byte> shTriFrag = std::as_bytes(std::span{tri_frag_spv});
static std::span<const byte> shDepthVert = std::as_bytes(std::span{depth_vert_spv});
static std::span<const byte> shDepthFrag = std::as_bytes(std::span{depth_frag_spv});
static std::span<const byte> shSamplerFrag = std::as_bytes(std::span{sampler_frag_spv});

static Format kColorFmt = Format::rgba8_unorm;

// ---- shared helper functions ----
static GpuAttachment makeColorAtt(GpuView* view, f32 r, f32 g, f32 b, f32 a,
                                        GpuLoadOp loadOp = GpuLoadOp::clear,
                                        GpuStoreOp storeOp = GpuStoreOp::store)
{
    GpuAttachment att{};
    att.image = view;
    att.loadOp = loadOp;
    att.storeOp = storeOp;
    att.clearColor[0] = r;
    att.clearColor[1] = g;
    att.clearColor[2] = b;
    att.clearColor[3] = a;
    return att;
}

static GpuAttachment makeDepthAtt(GpuView* view, f32 clearDepth = 1.0f)
{
    GpuAttachment att{};
    att.image = view;
    att.loadOp = GpuLoadOp::clear;
    att.storeOp = GpuStoreOp::store;
    att.clearDepthStencil.depth = clearDepth;
    return att;
}

static GpuPass simpleColorPass(GpuAttachment* colorAtt,
                                      GpuView** sampledImages = nullptr,
                                      u32 sampledCount = 0,
                                      GpuBuffer** uniformBufs = nullptr,
                                      u32 uniformCount = 0)
{
    GpuPass pass{};
    pass.colorAttachments = {colorAtt, 1};
    if (sampledImages) pass.sampledImages = {sampledImages, sampledCount};
    if (uniformBufs)   pass.uniformBuffers = {uniformBufs, uniformCount};
    return pass;
}

static std::unique_ptr<GpuPipeline> makeSimplePipeline(std::span<const byte> vertShader, std::span<const byte> fragShader,
                                      u32 pushSize = 0, GpuTopology topo = GpuTopology::triangleList)
{
    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = vertShader;
    ci.fragmentShader = fragShader;
    Format cf = Format::rgba8_unorm;
    ci.colorAttachmentFormats = {&cf, 1};
    ci.pushConstantSize = pushSize;
    ci.topology = topo;
    return testGpu->createGraphicsPipeline(ci);
}

static std::unique_ptr<GpuImage> makeColorTarget(u32 w, u32 h)
{
    return testGpu->createImage(w, h, Format::rgba8_unorm, GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
}

static void barrierToTransferRead(GpuCommandBuffer* cmd, GpuView* view)
{
    GpuImageBarrier ib{};
    ib.image = view;
    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&ib, 1});
}

// ============================================================================
// Tests
// ============================================================================

TEST(testFormatToSize)
{
    // 8-bit
    ASSERT(formatToSize(Format::r8_unorm) == 1);
    ASSERT(formatToSize(Format::rg8_unorm) == 2);
    ASSERT(formatToSize(Format::rgba8_unorm) == 4);
    ASSERT(formatToSize(Format::rgba8_srgb) == 4);
    // 16-bit
    ASSERT(formatToSize(Format::r16_unorm) == 2);
    ASSERT(formatToSize(Format::r16_sfloat) == 2);
    ASSERT(formatToSize(Format::rg16_sfloat) == 4);
    ASSERT(formatToSize(Format::rgba16_sfloat) == 8);
    ASSERT(formatToSize(Format::d16_unorm) == 2);
    // 32-bit
    ASSERT(formatToSize(Format::r32_sfloat) == 4);
    ASSERT(formatToSize(Format::rg32_sfloat) == 8);
    ASSERT(formatToSize(Format::rgba32_sfloat) == 16);
    // Depth-stencil
    ASSERT(formatToSize(Format::d24_unorm_s8_uint) == 4);
    ASSERT(formatToSize(Format::d32_sfloat) == 4);
    ASSERT(formatToSize(Format::d32_sfloat_s8_uint) == 5);
    // Block-compressed
    ASSERT(formatToSize(Format::bc1_rgb_unorm_block) == 8);
    ASSERT(formatToSize(Format::bc3_unorm_block) == 16);
    ASSERT(formatToSize(Format::bc5_unorm_block) == 16);
    ASSERT(formatToSize(Format::bc7_unorm_block) == 16);
}

TEST(testGetMaxMipmaps)
{
    ASSERT(getMaxMipmaps(1, 1, 1) == 1);
    ASSERT(getMaxMipmaps(2, 1, 1) == 2);
    ASSERT(getMaxMipmaps(64, 64, 1) == 7);
    ASSERT(getMaxMipmaps(128, 64, 1) == 8);
    ASSERT(getMaxMipmaps(256, 256, 256) == 9);
    ASSERT(getMaxMipmaps(0, 0, 0) == 0);
    ASSERT(getMaxMipmaps(1, 0, 1) == 1);
}

TEST(testGpuBufferWriteReadOffsets)
{
    // Host-visible path
    {
        std::unique_ptr<GpuBuffer> buf = testGpu->createBuffer(256,
                GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst,
                GpuMemoryUsage::frequentUpdate);

        // Write 68 bytes of 0x11111111 starting at offset 0
        u32 fillVal = 0x11111111;
        for (u32 off = 0; off < 68; off += 4)
            buf->write(std::as_bytes(std::span{&fillVal, 1}), off);

        // Overwrite at offset 64 and 128
        u32 v2 = 0x22222222;
        u32 v3 = 0x33333333;
        buf->write(std::as_bytes(std::span{&v2, 1}), 64);
        buf->write(std::as_bytes(std::span{&v3, 1}), 128);

        u32 r1 = 0, r2 = 0, r3 = 0;
        buf->read(std::as_writable_bytes(std::span{&r1, 1}), 0);
        buf->read(std::as_writable_bytes(std::span{&r2, 1}), 64);
        buf->read(std::as_writable_bytes(std::span{&r3, 1}), 128);
        ASSERT(r1 == 0x11111111);
        ASSERT(r2 == 0x22222222);
        ASSERT(r3 == 0x33333333);

        // Cross-boundary read: 8 bytes starting at offset 60
        u64 cross = 0;
        buf->read(std::as_writable_bytes(std::span{&cross, 1}), 60);
        u32 tail = assertCast<u32>(cross & 0xFFFFFFFF);
        u32 head = assertCast<u32>((cross >> 32) & 0xFFFFFFFF);
        ASSERT(tail == 0x11111111);
        ASSERT(head == 0x22222222);
    }

    // Device-only (staging) path
    {
        std::unique_ptr<GpuBuffer> buf = testGpu->createBuffer(256,
                GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst);

        u32 fillVal = 0x11111111;
        for (u32 off = 0; off < 68; off += 4)
            buf->write(std::as_bytes(std::span{&fillVal, 1}), off);

        u32 v2 = 0x22222222;
        u32 v3 = 0x33333333;
        buf->write(std::as_bytes(std::span{&v2, 1}), 64);
        buf->write(std::as_bytes(std::span{&v3, 1}), 128);

        u32 r1 = 0, r2 = 0, r3 = 0;
        buf->read(std::as_writable_bytes(std::span{&r1, 1}), 0);
        buf->read(std::as_writable_bytes(std::span{&r2, 1}), 64);
        buf->read(std::as_writable_bytes(std::span{&r3, 1}), 128);
        ASSERT(r1 == 0x11111111);
        ASSERT(r2 == 0x22222222);
        ASSERT(r3 == 0x33333333);

        u64 cross = 0;
        buf->read(std::as_writable_bytes(std::span{&cross, 1}), 60);
        u32 tail = assertCast<u32>(cross & 0xFFFFFFFF);
        u32 head = assertCast<u32>((cross >> 32) & 0xFFFFFFFF);
        ASSERT(tail == 0x11111111);
        ASSERT(head == 0x22222222);
    }
}

TEST(testGpuBufferWriteReadMultiple)
{
    std::unique_ptr<GpuBuffer> buf = testGpu->createBuffer(256, GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst, GpuMemoryUsage::frequentUpdate);
    u32 src[16] = {};
    for (u32 i = 0; i < 16; ++i)
        src[i] = i * 3 + 7;
    buf->write(std::as_bytes(std::span{src}), 0);
    u32 dst[16] = {};
    buf->read(std::as_writable_bytes(std::span{dst}), 0);
    ASSERT(std::equal(std::begin(dst), std::end(dst), std::begin(src)));
}

TEST(testGpuViewWriteRead)
{
    std::unique_ptr<GpuImage> img = testGpu->createImage(16, 16, Format::rgba8_unorm,
        GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> view = img->createView();

    u32 src[16 * 16] = {};
    std::fill(std::begin(src), std::end(src), 0x30405060u);
    img->writeWhole(std::as_bytes(std::span{src}));
    u32 dst[16 * 16] = {};
    img->readWhole(std::as_writable_bytes(std::span{dst}));
    ASSERT(std::equal(std::begin(dst), std::end(dst), std::begin(src)));
}

TEST(testGpuViewExtendedConfigSampler)
{
    static constexpr u32 texSize = 4;

    std::unique_ptr<GpuImage> texImg = testGpu->createImage(texSize, texSize, Format::rgba8_unorm,
        GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);

    u32 checker[texSize * texSize] = {};
    for (u32 y = 0; y < texSize; ++y)
        for (u32 x = 0; x < texSize; ++x)
            checker[y * texSize + x] = ((x + y) & 1)
                ? 0xFFFFFFFF
                : 0x000000FF;

    GpuViewCreateInfo vci{};
    vci.type = GpuViewType::image2D;
    vci.filter = GpuFilter::linear;
    vci.edgeMode = GpuSamplerEdgeMode::clampToEdge;
    std::unique_ptr<GpuView> view = texImg->createView(vci);
    texImg->writeWhole(std::as_bytes(std::span{checker}));

    // Verify descriptor is valid
    // Read back texture data to verify write
    u32 readback[texSize * texSize] = {};
    texImg->readWhole(std::as_writable_bytes(std::span{readback}));
    ASSERT(std::equal(std::begin(readback), std::end(readback), std::begin(checker)));

    // Verify sampled rendering produces non-clear color
    static constexpr u32 outSize = 2;
    std::unique_ptr<GpuImage> outImg = testGpu->createImage(outSize, outSize, Format::rgba8_unorm,
        GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> outView = outImg->createView();

    struct Push {
        f32 uvX;
        f32 uvY;
        u32 texIdx;
    };
    Push push{0.375f, 0.125f, view->samplerDescriptor()};

    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = std::as_bytes(std::span{tri_vert_spv});
    ci.fragmentShader = std::as_bytes(std::span{sampler_frag_spv});
    Format colorFmt = Format::rgba8_unorm;
    ci.colorAttachmentFormats = {&colorFmt, 1};
    ci.pushConstantSize = sizeof(Push);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createGraphicsPipeline(ci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt{};
    colorAtt.image = outView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};
    GpuView* sampledImages[] = {view.get()};
    pass.sampledImages = sampledImages;

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->draw(0, 3, 0, 1);
    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 result[outSize * outSize] = {};
    outImg->readWhole(std::as_writable_bytes(std::span{result}));
    // Sampling center of white texel (1,0) -> white (0xFFFFFFFF)
    ASSERT(result[0] == 0xFFFFFFFF);
}

TEST(testCommandBufferExecutes)
{
    static constexpr u32 bufSize = 64;

    std::unique_ptr<GpuBuffer> devBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::transferDst | GpuBufferUsage::transferSrc
            | GpuBufferUsage::storageBuffer);
    std::unique_ptr<GpuBuffer> staging = testGpu->createBuffer(bufSize, GpuBufferUsage::transferSrc | GpuBufferUsage::storageBuffer,
            GpuMemoryUsage::frequentUpdate);

    u32 known = 0xDECAF123;
    staging->write(std::as_bytes(std::span{&known, 1}), 0);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{compute_comp_spv}), 12);

    struct Push {
        u32 addVal;
        u32 inIdx;
        u32 outIdx;
    };
    Push push{0, staging->storageDescriptor(), devBuf->storageDescriptor()};

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    ASSERT(cmd != nullptr);

    GpuBufferBarrier stagingBarrier{};
    stagingBarrier.buffer = staging.get();
    stagingBarrier.nextAccess = GpuAccess::uniformBufferCompute;
    cmd->memoryBarrier({&stagingBarrier, 1}, {});

    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->dispatch(1, 1, 1);

    GpuBufferBarrier devBarrier{};
    devBarrier.buffer = devBuf.get();
    devBarrier.nextAccess = GpuAccess::transferSrc;
    cmd->memoryBarrier({&devBarrier, 1}, {});

    testGpu->endCommands(std::move(cmd));

    u32 result = 0;
    devBuf->read(std::as_writable_bytes(std::span{&result, 1}), 0);
    ASSERT(result == 0xDECAF123);
}

TEST(testBufferBarrierComputeSync)
{
    static constexpr u32 elemCount = 64;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> buf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc,
            GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> outBuf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i;
    buf->write(std::as_bytes(std::span{input}), 0);

    struct Push {
        u32 addVal;
        u32 inIdx;
        u32 outIdx;
    };

    std::unique_ptr<GpuPipeline> incPipe = testGpu->createComputePipeline(std::as_bytes(std::span{compute_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    // Barrier: buf to shaderWrite|shaderRead for Dispatch 1
    GpuBufferBarrier bb{};
    bb.buffer = buf.get();
    bb.nextAccess = GpuAccess::storageBufferCompute;
    cmd->memoryBarrier({&bb, 1}, {});

    // Dispatch 1: increment every element by 1 (in-place)
    {
        Push push{1, buf->storageDescriptor(), buf->storageDescriptor()};
        cmd->bindPipeline(incPipe.get());
        cmd->pushConstants(incPipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    // Barrier: buf to shaderRead for Dispatch 2
    bb.nextAccess = GpuAccess::uniformBufferCompute;
    cmd->memoryBarrier({&bb, 1}, {});

    // Dispatch 2: copy from buf to outBuf (addVal=0)
    {
        Push push{0, buf->storageDescriptor(), outBuf->storageDescriptor()};
        cmd->bindPipeline(incPipe.get());
        cmd->pushConstants(incPipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    testGpu->endCommands(std::move(cmd));

    u32 result[elemCount] = {};
    outBuf->read(std::as_writable_bytes(std::span{result}), 0);
    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(result[i] == input[i] + 1);
}

TEST(testImageBarrierLayoutTransition)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> img = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
        GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled | GpuImageUsage::colorAttachment);
    std::unique_ptr<GpuView> view = img->createView();

    // Write a known pattern
    u32 red[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        red[i] = 0xFF0000FF;
    img->writeWhole(std::as_bytes(std::span{red}));

    // Do a round-trip layout transition:
    // transferDst (after write) -> general -> transferSrc
    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuImageBarrier ib{};
    ib.image = view.get();
    ib.nextAccess = GpuAccess::storageImageCompute;
    ib.nextLayout = GpuLayout::general;
    cmd->memoryBarrier({}, {&ib, 1});

    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&ib, 1});

    testGpu->endCommands(std::move(cmd));

    // Read back -- data should survive the transitions
    u32 result[imgSize * imgSize] = {};
    img->readWhole(std::as_writable_bytes(std::span{result}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(result[i] == 0xFF0000FF);
}

TEST(testCombinedBufferImageBarrier)
{
    static constexpr u32 bufSize = 256;
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuBuffer> storageBuf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc,
            GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuImage> img = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
        GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::storage);
    std::unique_ptr<GpuView> imgView = img->createView();

    // Write 16 RGBA pixels as u32 values into the buffer
    u32 pixelData[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
    {
        u8 r = assertCast<u8>((i * 37) & 0xFF);
        u8 g = assertCast<u8>((i * 71) & 0xFF);
        u8 b = assertCast<u8>((i * 101) & 0xFF);
        pixelData[i] = r | (assertCast<u32>(g) << 8)
                     | (assertCast<u32>(b) << 16) | (0xFFu << 24);
    }
    storageBuf->write(std::as_bytes(std::span{pixelData}), 0);

    struct Push {
        u32 bufIdx;
        u32 imgIdx;
    };
    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{buf_to_img_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    // Combined barrier: buffer to shaderRead|shaderWrite, image to general
    GpuBufferBarrier bb{};
    bb.buffer = storageBuf.get();
    bb.nextAccess = GpuAccess::uniformBufferCompute;
    GpuImageBarrier ib{};
    ib.image = imgView.get();
    ib.nextAccess = GpuAccess::storageImageCompute;
    ib.nextLayout = GpuLayout::general;
    cmd->memoryBarrier({&bb, 1}, {&ib, 1});

    // Dispatch: read buffer -> write image
    {
        GpuPass pass{};
        GpuBuffer* storageBufs[] = {storageBuf.get()};
        GpuView* storageImages[] = {imgView.get()};
        pass.storageBuffers = storageBufs;
        pass.storageImages = storageImages;
        cmd->computePass(pass);

        Push push{storageBuf->storageDescriptor(), imgView->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(imgSize, imgSize, 1);
    }

    // Barrier: both to transferSrc for readback
    bb.nextAccess = GpuAccess::transferSrc;
    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({&bb, 1}, {&ib, 1});

    testGpu->endCommands(std::move(cmd));

    // Read back buffer -- should still contain original data
    u32 bufResult[imgSize * imgSize] = {};
    storageBuf->read(std::as_writable_bytes(std::span{bufResult}), 0);
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(bufResult[i] == pixelData[i]);

    // Read back image -- should match buffer data
    u32 imgResult[imgSize * imgSize] = {};
    img->readWhole(std::as_writable_bytes(std::span{imgResult}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(imgResult[i] == pixelData[i]);
}

TEST(testGpuCopyBufferRegion)
{
    static constexpr u32 elemCount = 64;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> src = testGpu->createBuffer(bufSize,
            GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst,
            GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> dst = testGpu->createBuffer(bufSize,
            GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst);

    u32 pattern[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        pattern[i] = i * 3 + 7;
    src->write(std::as_bytes(std::span{pattern}), 0);

    u32 sentinel[elemCount] = {};
    std::fill(std::begin(sentinel), std::end(sentinel), 0xDDDDDDDDu);
    dst->write(std::as_bytes(std::span{sentinel}), 0);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // dst bytes [8, 40) <- src bytes [64, 96)
    cmd->copyBuffer(dst.get(), 8, src.get(), 64, 32);
    // dst bytes [224, 256) <- src bytes [0, 32): region ends exactly at the
    // end of dst
    cmd->copyBuffer(dst.get(), 224, src.get(), 0, 32);
    testGpu->endCommands(std::move(cmd));

    u32 result[elemCount] = {};
    dst->read(std::as_writable_bytes(std::span{result}), 0);

    for (u32 i = 0; i < elemCount; ++i)
    {
        if (i >= 2 && i < 10)
            ASSERT(result[i] == (16 + (i - 2)) * 3 + 7);
        else if (i >= 56)
            ASSERT(result[i] == (i - 56) * 3 + 7);
        else
            ASSERT(result[i] == 0xDDDDDDDD);
    }
}

TEST(testGpuCopyBufferWhole)
{
    static constexpr u32 elemCount = 32;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> src = testGpu->createBuffer(bufSize,
            GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst,
            GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> dst = testGpu->createBuffer(bufSize,
            GpuBufferUsage::transferSrc | GpuBufferUsage::transferDst);

    u32 pattern[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        pattern[i] = 0xAAAA0000u | i;
    src->write(std::as_bytes(std::span{pattern}), 0);

    u32 sentinel[elemCount] = {};
    std::fill(std::begin(sentinel), std::end(sentinel), 0xDDDDDDDDu);
    dst->write(std::as_bytes(std::span{sentinel}), 0);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    cmd->copyBuffer(dst.get(), 0, src.get(), 0, bufSize);
    testGpu->endCommands(std::move(cmd));

    u32 result[elemCount] = {};
    dst->read(std::as_writable_bytes(std::span{result}), 0);
    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(result[i] == (0xAAAA0000u | i));
}

TEST(testGpuCopyImageRegion)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 sentinelVal = 0xFF00FF00;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    u32 pattern[imgSize * imgSize] = {};
    for (u32 y = 0; y < imgSize; ++y)
        for (u32 x = 0; x < imgSize; ++x)
            pattern[y * imgSize + x] = 0xFF000000u | (y * imgSize + x + 1);
    srcImg->writeWhole(std::as_bytes(std::span{pattern}));

    u32 sentinel[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        sentinel[i] = sentinelVal;
    dstImg->writeWhole(std::as_bytes(std::span{sentinel}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // Top-left quadrant of src -> bottom-right of dst:
    // both regions touch the image edges exactly
    GpuImageRegion dstRegion{4, 4, 0, 4, 4, 1};
    GpuImageRegion srcRegion{0, 0, 0, 4, 4, 1};
    cmd->copyImage(dstImg.get(), dstRegion, srcImg.get(), srcRegion);
    // Bottom-right quadrant of src -> top-left of dst
    dstRegion = {0, 0, 0, 4, 4, 1};
    srcRegion = {4, 4, 0, 4, 4, 1};
    cmd->copyImage(dstImg.get(), dstRegion, srcImg.get(), srcRegion);
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    for (u32 y = 0; y < imgSize; ++y)
    {
        for (u32 x = 0; x < imgSize; ++x)
        {
            if (x >= 4 && y >= 4)
                ASSERT(pixels[y * imgSize + x] == (0xFF000000u | ((y - 4) * imgSize + (x - 4) + 1)));
            else if (x < 4 && y < 4)
                ASSERT(pixels[y * imgSize + x] == (0xFF000000u | ((y + 4) * imgSize + (x + 4) + 1)));
            else
                ASSERT(pixels[y * imgSize + x] == sentinelVal);
        }
    }

    // The copy must not modify the source
    u32 srcPixels[imgSize * imgSize] = {};
    srcImg->readWhole(std::as_writable_bytes(std::span{srcPixels}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(srcPixels[i] == pattern[i]);
}

TEST(testGpuCopyImage3DSlice)
{
    static constexpr u32 w = 4;
    static constexpr u32 h = 4;
    static constexpr u32 d = 2;
    static constexpr u32 sentinelVal = 0xFF00FF00;

    GpuImageCreateInfo ci{};
    ci.dimensions = 3;
    ci.width = w;
    ci.height = h;
    ci.depth = d;
    ci.format = Format::rgba8_unorm;
    ci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled;
    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(ci);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(ci);

    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    u32 pattern[w * h * d] = {};
    for (u32 z = 0; z < d; ++z)
        for (u32 y = 0; y < h; ++y)
            for (u32 x = 0; x < w; ++x)
                pattern[(z * h + y) * w + x] = 0xFF000000u | ((z * h + y) * w + x + 1);
    srcImg->writeWhole(std::as_bytes(std::span{pattern}));

    u32 sentinel[w * h * d] = {};
    for (u32 i = 0; i < w * h * d; ++i)
        sentinel[i] = sentinelVal;
    dstImg->writeWhole(std::as_bytes(std::span{sentinel}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // Copy the z=1 slice of src over the z=0 slice of dst
    GpuImageRegion dstRegion{0, 0, 0, 4, 4, 1};
    GpuImageRegion srcRegion{0, 0, 1, 4, 4, 1};
    cmd->copyImage(dstImg.get(), dstRegion, srcImg.get(), srcRegion);
    testGpu->endCommands(std::move(cmd));

    u32 pixels[w * h * d] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    for (u32 y = 0; y < h; ++y)
    {
        for (u32 x = 0; x < w; ++x)
        {
            // z=0 gets the src z=1 slice, z=1 keeps the sentinel
            ASSERT(pixels[0 * h * w + y * w + x] == pattern[(1 * h + y) * w + x]);
            ASSERT(pixels[1 * h * w + y * w + x] == sentinelVal);
        }
    }
}

TEST(testGpuCopyImageWhole)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 sentinelVal = 0xFF00FF00;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    u32 pattern[imgSize * imgSize] = {};
    for (u32 y = 0; y < imgSize; ++y)
        for (u32 x = 0; x < imgSize; ++x)
            pattern[y * imgSize + x] = 0xFF000000u | (y * imgSize + x + 1);
    srcImg->writeWhole(std::as_bytes(std::span{pattern}));

    u32 sentinel[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        sentinel[i] = sentinelVal;
    dstImg->writeWhole(std::as_bytes(std::span{sentinel}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    cmd->copyImageWhole(dstImg.get(), srcImg.get());
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixels[i] == pattern[i]);

    // The copy must not modify the source
    u32 srcPixels[imgSize * imgSize] = {};
    srcImg->readWhole(std::as_writable_bytes(std::span{srcPixels}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(srcPixels[i] == pattern[i]);
}

TEST(testGpuBlitImageRegion)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 sentinelVal = 0xFF00FF00;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    u32 pattern[imgSize * imgSize] = {};
    for (u32 y = 0; y < imgSize; ++y)
        for (u32 x = 0; x < imgSize; ++x)
            pattern[y * imgSize + x] = 0xFF000000u | (y * imgSize + x + 1);
    srcImg->writeWhole(std::as_bytes(std::span{pattern}));

    u32 sentinel[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        sentinel[i] = sentinelVal;
    dstImg->writeWhole(std::as_bytes(std::span{sentinel}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // 4x4 box: src [1,5)x[2,6) -> dst [3,7)x[3,7)
    GpuImageRegion dstRegion{3, 3, 0, 4, 4, 1};
    GpuImageRegion srcRegion{1, 2, 0, 4, 4, 1};
    cmd->blitImage(dstImg.get(), dstRegion, srcImg.get(), srcRegion);
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    for (u32 y = 0; y < imgSize; ++y)
    {
        for (u32 x = 0; x < imgSize; ++x)
        {
            if (x >= 3 && x < 7 && y >= 3 && y < 7)
                ASSERT(pixels[y * imgSize + x] == pattern[((y - 1) * imgSize) + (x - 2)]);
            else
                ASSERT(pixels[y * imgSize + x] == sentinelVal);
        }
    }
}

TEST(testGpuBlitImageScaled)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 sentinelVal = 0xFF00FF00;
    static constexpr u32 red = 0xFF0000FF;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    // Solid color: the blit result must be identical for any filter mode
    u32 solid[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        solid[i] = red;
    srcImg->writeWhole(std::as_bytes(std::span{solid}));

    u32 sentinel[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        sentinel[i] = sentinelVal;
    dstImg->writeWhole(std::as_bytes(std::span{sentinel}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // Boxes of different sizes: 2x2 src box [1,3)x[1,3) scaled into the
    // 4x4 dst box [2,6)x[2,6)
    GpuImageRegion dstRegion{2, 2, 0, 4, 4, 1};
    GpuImageRegion srcRegion{1, 1, 0, 2, 2, 1};
    cmd->blitImage(dstImg.get(), dstRegion, srcImg.get(), srcRegion);
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    for (u32 y = 0; y < imgSize; ++y)
    {
        for (u32 x = 0; x < imgSize; ++x)
        {
            if (x >= 2 && x < 6 && y >= 2 && y < 6)
                ASSERT(pixels[y * imgSize + x] == red);
            else
                ASSERT(pixels[y * imgSize + x] == sentinelVal);
        }
    }
}

TEST(testGpuBlitImageWholeScaled)
{
    static constexpr u32 srcSize = 4;
    static constexpr u32 dstSize = 8;
    static constexpr u32 red = 0xFF0000FF;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(srcSize, srcSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(dstSize, dstSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    // Solid color: the blit result must be identical for any filter mode
    u32 solid[srcSize * srcSize] = {};
    for (u32 i = 0; i < srcSize * srcSize; ++i)
        solid[i] = red;
    srcImg->writeWhole(std::as_bytes(std::span{solid}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    // Whole image blit across different sizes: 4x4 -> 8x8
    cmd->blitImageWhole(dstImg.get(), srcImg.get());
    testGpu->endCommands(std::move(cmd));

    u32 pixels[dstSize * dstSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < dstSize * dstSize; ++i)
        ASSERT(pixels[i] == red);
}

TEST(testGpuBlitImageFormats)
{
    static constexpr u32 imgSize = 4;

    // The contract allows different formats: bgra8 -> rgba8
    //
    // Solid color R=0x33 G=0x66 B=0x99 A=0xFF:
    // - in bgra8 memory the bytes are B,G,R,A -> u32 0xFF336699
    // - in rgba8 memory the bytes are R,G,B,A -> u32 0xFF996633
    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::bgra8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> srcView = srcImg->createView();
    std::unique_ptr<GpuView> dstView = dstImg->createView();

    u32 solid[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        solid[i] = 0xFF336699;
    srcImg->writeWhole(std::as_bytes(std::span{solid}));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    cmd->blitImageWhole(dstImg.get(), srcImg.get());
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixels[i] == 0xFF996633);
}

TEST(testComputePassDispatch)
{
    static constexpr u32 elemCount = 64;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> inBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> outBuf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i;
    inBuf->write(std::as_bytes(std::span{input}), 0);

    struct Push {
        u32 inIdx;
        u32 outIdx;
    };
    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{mul_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier barriers[2] = {};
    barriers[0].buffer = inBuf.get();
    barriers[0].nextAccess = GpuAccess::uniformBufferCompute;
    barriers[1].buffer = outBuf.get();
    barriers[1].nextAccess = GpuAccess::storageBufferCompute;
    cmd->memoryBarrier({barriers, 2}, {});

    {
        GpuPass pass{};
        GpuBuffer* storageBufs[] = {inBuf.get(), outBuf.get()};
        pass.storageBuffers = storageBufs;
        cmd->computePass(pass);

        Push push{inBuf->storageDescriptor(), outBuf->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    testGpu->endCommands(std::move(cmd));

    u32 output[elemCount] = {};
    outBuf->read(std::as_writable_bytes(std::span{output}), 0);
    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(output[i] == input[i] * 2);
}

TEST(testComputeDispatchSSBO)
{
    static constexpr u32 elemCount = 64;
    static constexpr u32 bufSize = elemCount * sizeof(u32);
    u32 addVal = 100;

    std::unique_ptr<GpuBuffer> inBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> outBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i;
    inBuf->write(std::as_bytes(std::span{input}), 0);

    struct Push {
        u32 addVal;
        u32 inIdx;
        u32 outIdx;
    };
    Push push{addVal, inBuf->storageDescriptor(), outBuf->storageDescriptor()};

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{compute_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier barriers[2] = {};
    barriers[0].buffer = outBuf.get();
        barriers[0].nextAccess = GpuAccess::storageBufferCompute;
    barriers[1].buffer = inBuf.get();
        barriers[1].nextAccess = GpuAccess::uniformBufferCompute;
    cmd->memoryBarrier({barriers, 2}, {});

    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->dispatch(elemCount, 1, 1);

    testGpu->endCommands(std::move(cmd));

    u32 output[elemCount] = {};
    outBuf->read(std::as_writable_bytes(std::span{output}), 0);

    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(output[i] == input[i] + addVal);
}

TEST(testOffscreenRenderTriangle)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> colorImg = makeColorTarget(imgSize, imgSize);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    std::unique_ptr<GpuPipeline> pipe = makeSimplePipeline(shTriVert, shTriFrag);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt = makeColorAtt(colorView.get(), 1.0f, 0.0f, 0.0f, 1.0f);
    GpuPass pass = simpleColorPass(&colorAtt);

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());
    cmd->draw(0, 3, 0, 1);
    cmd->endRenderPass();

    testGpu->endCommands(std::move(cmd));

    u32 pixelData[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixelData}));

    // Triangle covers the whole viewport; all pixels should be (0.2, 0.4, 0.6, 1.0)
    // packed RGBA -> 0xFF996633
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixelData[i] == 0xFF996633);
}

TEST(testComputeSSBOAdd)
{
    static constexpr u32 elemCount = 64;
    static constexpr u32 bufSize = elemCount * sizeof(u32);
    u32 addVal = 100;

    std::unique_ptr<GpuBuffer> inBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer,
            GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> outBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer
            | GpuBufferUsage::transferSrc);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i;
    inBuf->write(std::as_bytes(std::span{input}), 0);

    struct Push {
        u32 addVal;
        u32 inIdx;
        u32 outIdx;
    };
    Push push{addVal, inBuf->storageDescriptor(), outBuf->storageDescriptor()};

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{compute_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier barriers[2] = {};
    barriers[0].buffer = inBuf.get();
    barriers[0].nextAccess = GpuAccess::uniformBufferCompute;
    barriers[1].buffer = outBuf.get();
    barriers[1].nextAccess = GpuAccess::storageBufferCompute;
    cmd->memoryBarrier({barriers, 2}, {});

    {
        GpuPass pass{};
        GpuBuffer* storageBufs[] = {inBuf.get(), outBuf.get()};
        pass.storageBuffers = storageBufs;
        cmd->computePass(pass);

        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    testGpu->endCommands(std::move(cmd));

    u32 output[elemCount] = {};
    outBuf->read(std::as_writable_bytes(std::span{output}), 0);

    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(output[i] == input[i] + addVal);
}

TEST(testOffscreenRenderDepthTest)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> colorImg = makeColorTarget(imgSize, imgSize);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    std::unique_ptr<GpuImage> depthImg = testGpu->createImage(imgSize, imgSize, Format::d32_sfloat,
            GpuImageUsage::depthStencilAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> depthView = depthImg->createView();

    struct Push {
        float depth;
        float pad[3];
        float color[4];
    };

    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = shDepthVert;
    ci.fragmentShader = shDepthFrag;
    ci.colorAttachmentFormats = {&kColorFmt, 1};
    ci.depthAttachmentFormat = Format::d32_sfloat;
    ci.enableDepthRead = true;
    ci.enableDepthWrite = true;
    ci.pushConstantSize = sizeof(Push);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createGraphicsPipeline(ci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt = makeColorAtt(colorView.get(), 0.0f, 0.0f, 0.0f, 0.0f);
    GpuAttachment depthAtt = makeDepthAtt(depthView.get());

    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};
    pass.depthAttachment = &depthAtt;

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());

    Push farPush{0.75f, {}, {1.0f, 0.0f, 0.0f, 1.0f}};
    cmd->pushConstants(pipe.get(), farPush);
    cmd->draw(0, 3, 0, 1);

    Push nearPush{0.25f, {}, {0.0f, 1.0f, 0.0f, 1.0f}};
    cmd->pushConstants(pipe.get(), nearPush);
    cmd->draw(0, 3, 0, 1);

    cmd->endRenderPass();

    barrierToTransferRead(cmd.get(), depthView.get());

    testGpu->endCommands(std::move(cmd));

    u32 colorPixels[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{colorPixels}));

    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(colorPixels[i] == 0xFF00FF00);

    f32 depthPixels[imgSize * imgSize] = {};
    depthImg->readWhole(std::as_writable_bytes(std::span{depthPixels}));

    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(depthPixels[i] > 0.24f && depthPixels[i] < 0.26f);
}

TEST(testMultiDrawInstancing)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 instanceCount = 4;

    struct InstanceData {
        float x, y, z, w;
    };
    InstanceData instanceData[instanceCount] = {
        {-1.0f, -1.0f, 0.0f, 1.0f},
        { 0.0f, -1.0f, 0.0f, 1.0f},
        {-1.0f,  0.0f, 0.0f, 1.0f},
        { 0.0f,  0.0f, 0.0f, 1.0f},
    };

    std::unique_ptr<GpuBuffer> instBuf = testGpu->createBuffer(sizeof(instanceData),
            GpuBufferUsage::uniformBuffer, GpuMemoryUsage::frequentUpdate);
    instBuf->write(std::as_bytes(std::span{instanceData}), 0);

    std::unique_ptr<GpuImage> colorImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    struct Push {
        u32 dataIdx;
    };

    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = std::as_bytes(std::span{instance_vert_spv});
    ci.fragmentShader = std::as_bytes(std::span{instance_frag_spv});
    Format colorFmt = Format::rgba8_unorm;
    ci.colorAttachmentFormats = {&colorFmt, 1};
    ci.pushConstantSize = sizeof(Push);
    ci.topology = GpuTopology::triangleStrip;

    std::unique_ptr<GpuPipeline> pipe = testGpu->createGraphicsPipeline(ci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier bb{};
    bb.buffer = instBuf.get();
    bb.nextAccess = GpuAccess::uniformBufferVertex;
    cmd->memoryBarrier({&bb, 1}, {});

    GpuAttachment colorAtt{};
    colorAtt.image = colorView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 0.0f;

    GpuPass pass{};
    GpuBuffer* uniformBufs[] = {instBuf.get()};
    pass.uniformBuffers = uniformBufs;
    pass.colorAttachments = {&colorAtt, 1};

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());

    Push push{instBuf->uniformDescriptor()};
    cmd->pushConstants(pipe.get(), push);
    cmd->draw(0, 4, 0, instanceCount);

    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    u32 green = 0xFF00FF00;
    ASSERT(pixels[2 * imgSize + 2] == green);
    ASSERT(pixels[2 * imgSize + 6] == green);
    ASSERT(pixels[6 * imgSize + 2] == green);
    ASSERT(pixels[6 * imgSize + 6] == green);
}

TEST(testMultiViewportScissor)
{
    static constexpr u32 imgSize = 16;

    std::unique_ptr<GpuImage> colorImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    struct Push {
        float depth;
        float pad[3];
        float color[4];
    };

    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = std::as_bytes(std::span{depth_vert_spv});
    ci.fragmentShader = std::as_bytes(std::span{depth_frag_spv});
    Format colorFmt = Format::rgba8_unorm;
    ci.colorAttachmentFormats = {&colorFmt, 1};
    ci.pushConstantSize = sizeof(Push);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createGraphicsPipeline(ci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt{};
    colorAtt.image = colorView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());

    Push red{0.0f, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
    cmd->gpuSetViewport(0.0f, 0.0f, 8.0f, 16.0f);
    cmd->gpuSetScissor(0, 0, 8, 16);
    cmd->pushConstants(pipe.get(), red);
    cmd->draw(0, 3, 0, 1);

    Push blue{0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
    cmd->gpuSetViewport(8.0f, 0.0f, 8.0f, 16.0f);
    cmd->gpuSetScissor(8, 0, 8, 16);
    cmd->pushConstants(pipe.get(), blue);
    cmd->draw(0, 3, 0, 1);

    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    u32 redPx = 0xFF0000FF;
    u32 bluePx = 0xFFFF0000;
    for (u32 y = 0; y < imgSize; ++y) {
        for (u32 x = 0; x < 8; ++x)
            ASSERT(pixels[y * imgSize + x] == redPx);
        for (u32 x = 8; x < imgSize; ++x)
            ASSERT(pixels[y * imgSize + x] == bluePx);
    }
}

TEST(testComputeImageFilterBlur)
{
    static constexpr u32 imgSize = 8;

    std::unique_ptr<GpuImage> srcImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::sampled | GpuImageUsage::transferDst | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> srcView = srcImg->createView();

    u32 white = 0xFFFFFFFF;
    u32 whiteImg[imgSize * imgSize];
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        whiteImg[i] = white;
    srcImg->writeWhole(std::as_bytes(std::span{whiteImg}));

    std::unique_ptr<GpuImage> dstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::storage | GpuImageUsage::transferSrc);
    GpuViewCreateInfo dstCI{};
    dstCI.type = GpuViewType::image2D;
    std::unique_ptr<GpuView> dstView = dstImg->createView(dstCI);

    struct Push {
        u32 srcIdx;
        u32 dstIdx;
    };

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{blur_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuImageBarrier srcBarrier{};
    srcBarrier.image = srcView.get();
    srcBarrier.nextAccess = GpuAccess::sampledImageCompute;
    srcBarrier.nextLayout = GpuLayout::shaderReadOnly;
    GpuImageBarrier dstBarrier{};
    dstBarrier.image = dstView.get();
    dstBarrier.nextAccess = GpuAccess::storageImageCompute;
    dstBarrier.nextLayout = GpuLayout::general;
    GpuImageBarrier imgBarriers[] = {srcBarrier, dstBarrier};
    cmd->memoryBarrier({}, {imgBarriers, 2});

    {
        GpuPass pass{};
        GpuView* sampledImages[] = {srcView.get()};
        GpuView* storageImages[] = {dstView.get()};
        pass.sampledImages = sampledImages;
        pass.storageImages = storageImages;
        cmd->computePass(pass);

        Push push{srcView->samplerDescriptor(), dstView->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(imgSize, imgSize, 1);
    }

    GpuImageBarrier readBarrier{};
    readBarrier.image = dstView.get();
    readBarrier.nextAccess = GpuAccess::transferSrc;
    readBarrier.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&readBarrier, 1});

    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    dstImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    for (u32 y = 1; y < imgSize - 1; ++y)
        for (u32 x = 1; x < imgSize - 1; ++x)
            ASSERT(pixels[y * imgSize + x] == white);
}

TEST(testRenderToTextureSample)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> firstImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::sampled | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> firstView = firstImg->createView();

    std::unique_ptr<GpuImage> resultImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> resultView = resultImg->createView();

    GpuGraphicsPipelineCreateInfo pass1CI{};
    pass1CI.vertexShader = std::as_bytes(std::span{tri_vert_spv});
    pass1CI.fragmentShader = std::as_bytes(std::span{tri_frag_spv});
    Format colorFmt = Format::rgba8_unorm;
    pass1CI.colorAttachmentFormats = {&colorFmt, 1};
    std::unique_ptr<GpuPipeline> pass1Pipe = testGpu->createGraphicsPipeline(pass1CI);

    struct Push2 { u32 texIdx; };
    GpuGraphicsPipelineCreateInfo pass2CI{};
    pass2CI.vertexShader = std::as_bytes(std::span{tri_vert_spv});
    pass2CI.fragmentShader = std::as_bytes(std::span{invert_frag_spv});
    pass2CI.colorAttachmentFormats = {&colorFmt, 1};
    pass2CI.pushConstantSize = sizeof(Push2);
    std::unique_ptr<GpuPipeline> pass2Pipe = testGpu->createGraphicsPipeline(pass2CI);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    // Pass 1: render triangle to first attachment
    {
        GpuAttachment colorAtt{};
        colorAtt.image = firstView.get();
        colorAtt.loadOp = GpuLoadOp::clear;
        colorAtt.storeOp = GpuStoreOp::store;
        colorAtt.clearColor[0] = 0.0f;
        colorAtt.clearColor[1] = 0.0f;
        colorAtt.clearColor[2] = 0.0f;
        colorAtt.clearColor[3] = 1.0f;

        GpuPass pass{};
        pass.colorAttachments = {&colorAtt, 1};

        cmd->beginRenderPass(pass);
        cmd->bindPipeline(pass1Pipe.get());
        cmd->draw(0, 3, 0, 1);
        cmd->endRenderPass();
    }

    // Barrier: transition first attachment to shaderReadOnly
    GpuImageBarrier ib{};
    ib.image = firstView.get();
    ib.nextAccess = GpuAccess::sampledImageFragment;
    ib.nextLayout = GpuLayout::shaderReadOnly;
    cmd->memoryBarrier({}, {&ib, 1});

    // Pass 2: sample first attachment, invert, write to result
    {
        GpuAttachment colorAtt{};
        colorAtt.image = resultView.get();
        colorAtt.loadOp = GpuLoadOp::clear;
        colorAtt.storeOp = GpuStoreOp::store;
        colorAtt.clearColor[0] = 0.0f;
        colorAtt.clearColor[1] = 0.0f;
        colorAtt.clearColor[2] = 0.0f;
        colorAtt.clearColor[3] = 1.0f;

        GpuPass pass{};
        pass.colorAttachments = {&colorAtt, 1};
        GpuView* sampledImages[] = {firstView.get()};
        pass.sampledImages = sampledImages;

        cmd->beginRenderPass(pass);
        cmd->bindPipeline(pass2Pipe.get());
        Push2 push{firstView->samplerDescriptor()};
        cmd->pushConstants(pass2Pipe.get(), push);
        cmd->draw(0, 3, 0, 1);
        cmd->endRenderPass();
    }

    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    resultImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    // Invert of vec4(0.2, 0.4, 0.6, 1.0) = vec4(0.8, 0.6, 0.4, 0.0)
    // In RGBA8: R=204, G=153, B=102, A=0  => 0x006699CC
    u32 expected = 0x006699CC;
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixels[i] == expected);
}

TEST(testBufferReadbackStagingRead)
{
    static constexpr u32 elemCount = 32;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> inBuf = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i + 1;
    inBuf->write(std::as_bytes(std::span{input}), 0);

    std::unique_ptr<GpuBuffer> outBuf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc,
            GpuMemoryUsage::stagingRead);

    struct Push { u32 inIdx; u32 outIdx; };
    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{mul_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier barriers[2] = {};
    barriers[0].buffer = inBuf.get();
    barriers[0].nextAccess = GpuAccess::uniformBufferCompute;
    barriers[1].buffer = outBuf.get();
    barriers[1].nextAccess = GpuAccess::storageBufferCompute;
    cmd->memoryBarrier({barriers, 2}, {});

    {
        GpuPass pass{};
        GpuBuffer* storageBufs[] = {inBuf.get(), outBuf.get()};
        pass.storageBuffers = storageBufs;
        cmd->computePass(pass);

        Push push{inBuf->storageDescriptor(), outBuf->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    GpuBufferBarrier readBarrier{};
    readBarrier.buffer = outBuf.get();
    readBarrier.nextAccess = GpuAccess::hostRead;
    cmd->memoryBarrier({&readBarrier, 1}, {});

    testGpu->endCommands(std::move(cmd));

    u32 output[elemCount] = {};
    outBuf->read(std::as_writable_bytes(std::span{output}), 0);
    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(output[i] == input[i] * 2);
}

TEST(testPipelineBarrierChainedDispatches)
{
    static constexpr u32 elemCount = 32;
    static constexpr u32 bufSize = elemCount * sizeof(u32);

    std::unique_ptr<GpuBuffer> bufA = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> bufB = testGpu->createBuffer(bufSize, GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
    std::unique_ptr<GpuBuffer> bufC = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc,
            GpuMemoryUsage::stagingRead);

    u32 input[elemCount] = {};
    for (u32 i = 0; i < elemCount; ++i)
        input[i] = i + 1;
    bufA->write(std::as_bytes(std::span{input}), 0);

    struct Push { u32 inIdx; u32 outIdx; };
    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{mul_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    // Dispatch 1: bufA -> bufB (multiply by 2)
    {
        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = bufA.get();
        barriers[0].nextAccess = GpuAccess::uniformBufferCompute;
        barriers[1].buffer = bufB.get();
        barriers[1].nextAccess = GpuAccess::storageBufferCompute;
        cmd->memoryBarrier({barriers, 2}, {});

        GpuPass pass{};
        GpuBuffer* storageBufs[] = {bufA.get(), bufB.get()};
        pass.storageBuffers = storageBufs;
        cmd->computePass(pass);

        Push push{bufA->storageDescriptor(), bufB->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    // Barrier: bufB -> shaderRead for dispatch 2
    {
        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = bufB.get();
        barriers[0].nextAccess = GpuAccess::uniformBufferCompute;
        barriers[1].buffer = bufC.get();
        barriers[1].nextAccess = GpuAccess::storageBufferCompute;
        cmd->memoryBarrier(barriers, {});
    }

    // Dispatch 2: bufB -> bufC (multiply by 2 again)
    {
        GpuPass pass{};
        GpuBuffer* storageBufs[] = {bufB.get(), bufC.get()};
        pass.storageBuffers = storageBufs;
        cmd->computePass(pass);

        Push push{bufB->storageDescriptor(), bufC->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(elemCount, 1, 1);
    }

    // Barrier: bufC for host read
    {
        GpuBufferBarrier bc{};
        bc.buffer = bufC.get();
    bc.nextAccess = GpuAccess::hostRead;
        cmd->memoryBarrier({&bc, 1}, {});
    }

    testGpu->endCommands(std::move(cmd));

    u32 output[elemCount] = {};
    bufC->read(std::as_writable_bytes(std::span{output}), 0);
    for (u32 i = 0; i < elemCount; ++i)
        ASSERT(output[i] == input[i] * 4);
}

TEST(testImageArrayRendering)
{
    static constexpr u32 imgSize = 4;
    static constexpr u32 numLayers = 4;

    GpuImageCreateInfo imgCI{};
    imgCI.width = imgSize;
    imgCI.height = imgSize;
    imgCI.format = Format::rgba8_unorm;
    imgCI.arrayLayers = numLayers;
    imgCI.usage = GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc;
    std::unique_ptr<GpuImage> arrayImg = testGpu->createImage(imgCI);

    // Hoisted: pipeline and attachment template (invariant across layers)
    std::unique_ptr<GpuPipeline> pipe = makeSimplePipeline(shTriVert, shTriFrag);
    GpuAttachment colorAtt{};
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 1.0f;
    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};

    for (u32 layer = 0; layer < numLayers; ++layer) {
        GpuViewCreateInfo viewCI{};
        viewCI.baseArrayLayer = layer;
        viewCI.layerCount = 1;
        viewCI.type = GpuViewType::image2D;
        std::unique_ptr<GpuView> layerView = arrayImg->createView(viewCI);
        colorAtt.image = layerView.get();

        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        cmd->bindPipeline(pipe.get());
        cmd->draw(0, 3, 0, 1);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));

        u32 pixels[imgSize * imgSize] = {};
        GpuImageRegion readRegion{};
        readRegion.width = imgSize;
        readRegion.height = imgSize;
        readRegion.depth = 1;
        readRegion.arrayLayer = layer;
        readRegion.layerCount = 1;
        arrayImg->read(std::as_writable_bytes(std::span{pixels}), readRegion);

        for (u32 i = 0; i < imgSize * imgSize; ++i)
            ASSERT(pixels[i] == 0xFF996633);
    }
}

TEST(testGpuWaitIdle)
{
    static constexpr u32 bufSize = 64;

    std::unique_ptr<GpuBuffer> devBuf = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer | GpuBufferUsage::transferSrc);
    std::unique_ptr<GpuBuffer> staging = testGpu->createBuffer(bufSize,
            GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);

    u32 known = 0xDEADBEEF;
    staging->write(std::as_bytes(std::span{&known, 1}), 0);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{compute_comp_spv}), 12);

    struct Push {
        u32 addVal;
        u32 inIdx;
        u32 outIdx;
    };
    Push push{0, staging->storageDescriptor(), devBuf->storageDescriptor()};

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuBufferBarrier stagingBarrier{};
    stagingBarrier.buffer = staging.get();
    stagingBarrier.nextAccess = GpuAccess::uniformBufferCompute;
    cmd->memoryBarrier({&stagingBarrier, 1}, {});

    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->dispatch(1, 1, 1);

    GpuBufferBarrier devBarrier{};
    devBarrier.buffer = devBuf.get();
    devBarrier.nextAccess = GpuAccess::transferSrc;
    cmd->memoryBarrier({&devBarrier, 1}, {});

    testGpu->endCommands(std::move(cmd));

    // Immediately wait -- this should make compute results visible
    testGpu->waitIdle();

    u32 result = 0;
    devBuf->read(std::as_writable_bytes(std::span{&result, 1}), 0);
    ASSERT(result == 0xDEADBEEF);
}

TEST(testUniformBufferVertexShader)
{
    static constexpr u32 imgSize = 4;

    // Color as f32[4]: (0.5, 0.3, 0.7, 1.0)
    f32 colorData[4] = {0.5f, 0.3f, 0.7f, 1.0f};

    std::unique_ptr<GpuBuffer> uniformBuf = testGpu->createBuffer(64, GpuBufferUsage::uniformBuffer, GpuMemoryUsage::frequentUpdate);
    uniformBuf->write(std::as_bytes(std::span{colorData}), 0);

    std::unique_ptr<GpuImage> colorImg = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    struct Push {
        u32 colorIdx;
    };

    GpuGraphicsPipelineCreateInfo ci{};
    ci.vertexShader = std::as_bytes(std::span{uniform_vert_spv});
    ci.fragmentShader = std::as_bytes(std::span{uniform_frag_spv});
    Format colorFmt = Format::rgba8_unorm;
    ci.colorAttachmentFormats = {&colorFmt, 1};
    ci.pushConstantSize = sizeof(Push);

    std::unique_ptr<GpuPipeline> pipe = testGpu->createGraphicsPipeline(ci);

    Push push{uniformBuf->uniformDescriptor()};

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt{};
    colorAtt.image = colorView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 0.0f;

    GpuPass pass{};
    GpuBuffer* uniformBufs[] = {uniformBuf.get()};
    pass.uniformBuffers = uniformBufs;
    pass.colorAttachments = {&colorAtt, 1};

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->draw(0, 3, 0, 1);
    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    // Expected: f32(0.5, 0.3, 0.7, 1.0) -> unorm8(128, 76, 178, 255)
    for (u32 i = 0; i < imgSize * imgSize; ++i)
    {
        // Allow small rounding differences
        u8 r = assertCast<u8>((pixels[i] >> 0) & 0xFF);
        u8 g = assertCast<u8>((pixels[i] >> 8) & 0xFF);
        u8 b = assertCast<u8>((pixels[i] >> 16) & 0xFF);
        ASSERT(r >= 125 && r <= 130);
        ASSERT(g >= 74 && g <= 79);
        ASSERT(b >= 176 && b <= 181);
        ASSERT(((pixels[i] >> 24) & 0xFF) == 0xFF);
    }
}

TEST(testStorageImageComputeShader)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> img = testGpu->createImage(imgSize, imgSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst
            | GpuImageUsage::storage | GpuImageUsage::sampled);
    std::unique_ptr<GpuView> view = img->createView(GpuFilter::nearest);

    struct Push {
        u32 imgIdx;
    };

    std::unique_ptr<GpuPipeline> pipe = testGpu->createComputePipeline(std::as_bytes(std::span{storage_img_comp_spv}), sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    // Barrier: image to general for storage write
    GpuImageBarrier ib{};
    ib.image = view.get();
    ib.nextAccess = GpuAccess::storageImageCompute;
    ib.nextLayout = GpuLayout::general;
    cmd->memoryBarrier({}, {&ib, 1});

    {
        GpuPass pass{};
        GpuView* storageImages[] = {view.get()};
        pass.storageImages = storageImages;
        cmd->computePass(pass);

        Push push{view->storageDescriptor()};
        cmd->bindPipeline(pipe.get());
        cmd->pushConstants(pipe.get(), push);
        cmd->dispatch(imgSize, imgSize, 1);
    }

    // Barrier: image to transferSrc for readback
    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&ib, 1});

    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    img->readWhole(std::as_writable_bytes(std::span{pixels}));

    // Shader writes: (x+y)&1==0 -> vec4(1,1,1,1)=0xFFFFFFFF, else vec4(0,0,0,1)=0xFF000000
    u32 expected[imgSize * imgSize] = {};
    for (u32 y = 0; y < imgSize; ++y)
        for (u32 x = 0; x < imgSize; ++x)
            expected[y * imgSize + x] = ((x + y) & 1)
                ? 0xFF000000  // black (R=0,G=0,B=0,A=255)
                : 0xFFFFFFFF; // white

    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixels[i] == expected[i]);
}

TEST(testGpuViewWriteCubemap)
{
    static constexpr u32 faceSize = 4;
    static constexpr u32 bpp = 4;

    GpuImageCreateInfo ci{};
    ci.width = faceSize;
    ci.height = faceSize;
    ci.format = Format::rgba8_unorm;
    ci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled;
    ci.arrayLayers = 6;
    ci.flags = GpuImageConfig::cubeCompatible;
    std::unique_ptr<GpuImage> cubeImg = testGpu->createImage(ci);

    GpuViewCreateInfo vci{};
    vci.type = GpuViewType::cube;
    vci.layerCount = 6;
    std::unique_ptr<GpuView> cubeView = cubeImg->createView(vci);

    // Build cubemap cross: 4 columns x 3 rows (each face is faceSize x faceSize)
    u8 cross[faceSize * 4 * faceSize * 3 * bpp] = {};
    u32 pitch = faceSize * 4 * bpp; // bytes per row of cross
    u32 w = faceSize;
    u32 h = faceSize;

    auto fillFace = [&](u32 cx, u32 cy, u8 r, u8 g, u8 b, u8 a)
    {
        for (u32 y = 0; y < h; ++y)
            for (u32 x = 0; x < w; ++x)
            {
                u32 idx = (cy * h + y) * pitch + (cx * w + x) * bpp;
                cross[idx + 0] = r;
                cross[idx + 1] = g;
                cross[idx + 2] = b;
                cross[idx + 3] = a;
            }
    };

    // Face cross coordinates (from writeCubemap regions):
    //   layer 0 (+X) -> src (2, 1)   layer 3 (+Y) -> src (1, 0)
    //   layer 1 (-X) -> src (0, 1)   layer 4 (+Z) -> src (1, 1)
    //   layer 2 (-Y) -> src (1, 2)   layer 5 (-Z) -> src (3, 1)
    fillFace(2, 1, 0xFF, 0x00, 0x00, 0xFF); // +X (layer 0): red
    fillFace(0, 1, 0x00, 0xFF, 0x00, 0xFF); // -X (layer 1): green
    fillFace(1, 2, 0x00, 0x00, 0xFF, 0xFF); // -Y (layer 2): blue
    fillFace(1, 0, 0xFF, 0xFF, 0x00, 0xFF); // +Y (layer 3): yellow
    fillFace(1, 1, 0xFF, 0x00, 0xFF, 0xFF); // +Z (layer 4): magenta
    fillFace(3, 1, 0x00, 0xFF, 0xFF, 0xFF); // -Z (layer 5): cyan

    cubeImg->writeCubemap(std::as_bytes(std::span{cross}));

    // Read back layer 0 (+X) via a single-layer view
    GpuViewCreateInfo readVci{};
    readVci.type = GpuViewType::image2D;
    readVci.baseArrayLayer = 0;
    readVci.layerCount = 1;
    std::unique_ptr<GpuView> readView = cubeImg->createView(readVci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    barrierToTransferRead(cmd.get(), readView.get());
    testGpu->endCommands(std::move(cmd));

    u32 face[faceSize * faceSize] = {};
    GpuImageRegion readRegion{};
    readRegion.width = faceSize;
    readRegion.height = faceSize;
    readRegion.depth = 1;
    readRegion.layerCount = 1;
    cubeImg->read(std::as_writable_bytes(std::span{face}), readRegion);
    for (u32 i = 0; i < faceSize * faceSize; ++i)
        ASSERT(face[i] == 0xFF0000FF); // RGBA: R=0xFF, G=0x00, B=0x00, A=0xFF
}

TEST(testGpuViewGenMipmapsBaseLevel)
{
    static constexpr u32 imgSize = 8;
    static constexpr u32 mipLevels = 4;

    GpuImageCreateInfo ci{};
    ci.width = imgSize;
    ci.height = imgSize;
    ci.format = Format::rgba8_unorm;
    ci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled;
    ci.mipLevels = mipLevels;
    std::unique_ptr<GpuImage> mipImg = testGpu->createImage(ci);

    GpuViewCreateInfo vci{};
    vci.type = GpuViewType::image2D;
    vci.levelCount = mipLevels;
    std::unique_ptr<GpuView> mipView = mipImg->createView(vci);

    u32 yellow = 0xFF00FFFF;
    u32 src[imgSize * imgSize] = {};
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        src[i] = yellow;
    mipImg->writeWhole(std::as_bytes(std::span{src}));

    mipImg->generateMipmaps();

    u32 dst[imgSize * imgSize] = {};
    mipImg->readWhole(std::as_writable_bytes(std::span{dst}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(dst[i] == yellow);
}

TEST(testGpuViewGenMipmapsMip1Content)
{
    static constexpr u32 w = 4;
    static constexpr u32 h = 4;
    static constexpr u32 mipLevels = 3;

    GpuImageCreateInfo ci{};
    ci.width = w;
    ci.height = h;
    ci.format = Format::rgba8_unorm;
    ci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst
             | GpuImageUsage::sampled;
    ci.mipLevels = mipLevels;
    std::unique_ptr<GpuImage> img = testGpu->createImage(ci);

    GpuViewCreateInfo fullVci{};
    fullVci.type = GpuViewType::image2D;
    fullVci.levelCount = mipLevels;
    std::unique_ptr<GpuView> fullView = img->createView(fullVci);

    u32 red   = 0xFF0000FF;
    u32 green = 0xFF00FF00;
    u32 src[w * h] = {};
    for (u32 y = 0; y < h; ++y)
        for (u32 x = 0; x < w; ++x)
            src[y * w + x] = (x < 2 && y < 2) ? red : green;
    img->writeWhole(std::as_bytes(std::span{src}));

    img->generateMipmaps();

    // Read mip 1 (2x2) via a view targeting only that level
    GpuViewCreateInfo mip1Vci{};
    mip1Vci.type = GpuViewType::image2D;
    mip1Vci.baseMipLevel = 1;
    mip1Vci.levelCount = 1;
    std::unique_ptr<GpuView> mip1View = img->createView(mip1Vci);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
    barrierToTransferRead(cmd.get(), mip1View.get());
    testGpu->endCommands(std::move(cmd));

    u32 mip1[4] = {};
    img->readWhole(std::as_writable_bytes(std::span{mip1}), 1);

    // Mip 1 (2x2): top-left averages the red 2x2 block -> red;
    // the other three average green quadrants -> green
    ASSERT(mip1[0] == red);
    ASSERT(mip1[1] == green);
    ASSERT(mip1[2] == green);
    ASSERT(mip1[3] == green);
}

TEST(testGpuLoadOpLoadPreservesContent)
{
    static constexpr u32 w = 4;
    static constexpr u32 h = 4;
    static constexpr u32 halfW = 2;

    std::unique_ptr<GpuImage> img = testGpu->createImage(w, h, Format::rgba8_unorm,
            GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc
            | GpuImageUsage::transferDst);
    std::unique_ptr<GpuView> view = img->createView();

    // Fill image with known pattern (red)
    u32 redPx = 0xFF0000FF;
    u32 src[w * h];
    for (u32 i = 0; i < w * h; ++i)
        src[i] = redPx;
    img->writeWhole(std::as_bytes(std::span{src}));

    std::unique_ptr<GpuPipeline> pipe = makeSimplePipeline(shTriVert, shTriFrag);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment att = makeColorAtt(view.get(), 0.0f, 0.0f, 0.0f, 0.0f,
                                            GpuLoadOp::load, GpuStoreOp::store);
    GpuPass pass = simpleColorPass(&att);

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());

    // Viewport covers left half only; right half should retain original red
    cmd->gpuSetViewport(0.0f, 0.0f, assertCast<f32>(halfW), assertCast<f32>(h));
    cmd->gpuSetScissor(0, 0, halfW, h);
    cmd->draw(0, 3, 0, 1);

    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 pixels[w * h] = {};
    img->readWhole(std::as_writable_bytes(std::span{pixels}));

    u32 triColor = 0xFF996633;
    for (u32 y = 0; y < h; ++y)
        for (u32 x = 0; x < w; ++x)
            ASSERT(pixels[y * w + x] == (x < halfW ? triColor : redPx));
}

TEST(testGpuViewMirroredRepeat)
{
    static constexpr u32 texSize = 2;
    static constexpr u32 outSize = 2;

    std::unique_ptr<GpuImage> texImg = testGpu->createImage(texSize, texSize, Format::rgba8_unorm,
            GpuImageUsage::transferSrc | GpuImageUsage::transferDst
            | GpuImageUsage::sampled);

    u32 pattern[texSize * texSize] = {0xFF0000FF, 0xFF00FF00,
                                       0xFFFF0000, 0xFFFFFFFF};
    GpuViewCreateInfo vci{};
    vci.type = GpuViewType::image2D;
    vci.filter = GpuFilter::nearest;
    vci.edgeMode = GpuSamplerEdgeMode::mirroredRepeat;
    std::unique_ptr<GpuView> view = texImg->createView(vci);
    texImg->writeWhole(std::as_bytes(std::span{pattern}));

    std::unique_ptr<GpuImage> outImg = makeColorTarget(outSize, outSize);
    std::unique_ptr<GpuView> outView = outImg->createView();

    // UV=(1.25, 0.25) -> mirroredRepeat: floor(1.25)=1(odd) -> 1-0.25=0.75,
    // floor(0.25)=0(even) -> 0.25.  Nearest texel to (0.75, 0.25) = (1,0) = green
    struct Push { f32 uvX; f32 uvY; u32 texIdx; };
    Push push{1.25f, 0.25f, view->samplerDescriptor()};

    std::unique_ptr<GpuPipeline> pipe = makeSimplePipeline(shTriVert, shSamplerFrag, sizeof(Push));

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment att = makeColorAtt(outView.get(), 0.0f, 0.0f, 0.0f, 0.0f);
    GpuPass pass = simpleColorPass(&att);
    GpuView* sampledImages[] = {view.get()};
    pass.sampledImages = sampledImages;

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());
    cmd->pushConstants(pipe.get(), push);
    cmd->draw(0, 3, 0, 1);
    cmd->endRenderPass();
    testGpu->endCommands(std::move(cmd));

    u32 result[outSize * outSize] = {};
    outImg->readWhole(std::as_writable_bytes(std::span{result}));
    for (u32 i = 0; i < outSize * outSize; ++i)
        ASSERT(result[i] == 0xFF00FF00); // green from mirrored repeat
}

TEST(testBarrierFlagsAllGraphics)
{
    static constexpr u32 imgSize = 4;

    std::unique_ptr<GpuImage> colorImg = makeColorTarget(imgSize, imgSize);
    std::unique_ptr<GpuView> colorView = colorImg->createView();

    std::unique_ptr<GpuPipeline> pipe = makeSimplePipeline(shTriVert, shTriFrag);

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment att = makeColorAtt(colorView.get(), 1.0f, 0.0f, 0.0f, 1.0f);
    GpuPass pass = simpleColorPass(&att);

    cmd->beginRenderPass(pass);
    cmd->bindPipeline(pipe.get());
    cmd->draw(0, 3, 0, 1);
    cmd->endRenderPass();

    // Barrier with allGraphics / colorAttachmentWrite
    GpuImageBarrier ib{};
    ib.image = colorView.get();
    ib.nextAccess = GpuAccess::colorAttachment;
    ib.nextLayout = GpuLayout::transferSrc;
    cmd->memoryBarrier({}, {&ib, 1});

    // Barrier from read-after-write: allCommands + memoryRead
    ib.nextAccess = GpuAccess::transferSrc;
    ib.nextLayout = GpuLayout::general;
    cmd->memoryBarrier({}, {&ib, 1});

    testGpu->endCommands(std::move(cmd));

    u32 pixels[imgSize * imgSize] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < imgSize * imgSize; ++i)
        ASSERT(pixels[i] == 0xFF996633);
}

TEST(testGpuImageAccessors)
{
    GpuImageCreateInfo ci{};
    ci.width = 8;
    ci.height = 8;
    ci.format = Format::rgba8_unorm;
    ci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst | GpuImageUsage::sampled;
    ci.mipLevels = 4;
    std::unique_ptr<GpuImage> img = testGpu->createImage(ci);

    ASSERT(img->format() == Format::rgba8_unorm);
    ASSERT(img->width() == 8);
    ASSERT(img->height() == 8);
    ASSERT(img->depth() == 1);
    ASSERT(img->mipmapCount() == 4);
    ASSERT(img->layerCount() == 1);

    GpuViewCreateInfo vci{};
    vci.levelCount = 4;
    std::unique_ptr<GpuView> view = img->createView(vci);
    ASSERT(view->format() == Format::rgba8_unorm);
    ASSERT(view->width() == 8);
    ASSERT(view->height() == 8);
    ASSERT(view->depth() == 1);
    ASSERT(view->mipmapCount() == 4);
    ASSERT(view->layerCount() == 1);

    // A partial view reports only its configured mip and layer extent
    GpuViewCreateInfo pci{};
    pci.baseMipLevel = 1;
    pci.levelCount = 2;
    std::unique_ptr<GpuView> partial = img->createView(pci);
    ASSERT(partial->mipmapCount() == 2);
    ASSERT(partial->layerCount() == 1);

    // 3D image depth
    GpuImageCreateInfo dci{};
    dci.dimensions = 3;
    dci.width = 4;
    dci.height = 4;
    dci.depth = 2;
    dci.format = Format::rgba8_unorm;
    dci.usage = GpuImageUsage::transferSrc | GpuImageUsage::transferDst;
    std::unique_ptr<GpuImage> img3d = testGpu->createImage(dci);
    ASSERT(img3d->depth() == 2);
    ASSERT(img3d->width() == 4);
    ASSERT(img3d->height() == 4);
}
