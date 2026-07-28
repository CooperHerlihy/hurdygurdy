#include "tests.hpp"

#include "test/compute.comp.spv.h"
#include "test/tri.vert.spv.h"
#include "test/tri.frag.spv.h"
#include "test/sampler.frag.spv.h"
#include "test/mul.comp.spv.h"
#include "test/buf_to_img.comp.spv.h"
#include "test/uniform.vert.spv.h"
#include "test/uniform.frag.spv.h"
#include "test/storage_img.comp.spv.h"
#include "test/depth.vert.spv.h"
#include "test/depth.frag.spv.h"
#include "test/instance.vert.spv.h"
#include "test/instance.frag.spv.h"
#include "test/blur.comp.spv.h"
#include "test/invert.frag.spv.h"

void testGpu()
{
    // ============================================================================
    // GPU API
    // ============================================================================
    //
    // Tests for the GPU abstraction layer (GpuBuffer, GpuImage, GpuView,
    // GpuPipeline, command buffers, barriers, compute, offscreen rendering).
    //
    // All GPU tests use offscreen resources; no window is needed.

    Span<const u8> shTriVert    = {test_tri_vert_spv,      sizeof(test_tri_vert_spv)};
    Span<const u8> shTriFrag    = {test_tri_frag_spv,      sizeof(test_tri_frag_spv)};
    Span<const u8> shDepthVert  = {test_depth_vert_spv,    sizeof(test_depth_vert_spv)};
    Span<const u8> shDepthFrag  = {test_depth_frag_spv,    sizeof(test_depth_frag_spv)};
    Span<const u8> shSamplerFrag= {test_sampler_frag_spv,  sizeof(test_sampler_frag_spv)};

    Format kColorFmt = Format_r8g8b8a8_unorm;

    // ---- shared helper lambdas ----
    auto makeColorAtt = [](GpuView* view, f32 r, f32 g, f32 b, f32 a,
                           GpuLoadOp loadOp = GpuLoadOp_clear,
                           GpuStoreOp storeOp = GpuStoreOp_store)
    {
        GpuRenderAttachment att{};
        att.image = view;
        att.loadOp = loadOp;
        att.storeOp = storeOp;
        att.clearValue.color.float32[0] = r;
        att.clearValue.color.float32[1] = g;
        att.clearValue.color.float32[2] = b;
        att.clearValue.color.float32[3] = a;
        return att;
    };

    auto makeDepthAtt = [](GpuView* view, f32 clearDepth = 1.0f)
    {
        GpuRenderAttachment att{};
        att.image = view;
        att.loadOp = GpuLoadOp_clear;
        att.storeOp = GpuStoreOp_store;
        att.clearValue.depthStencil.depth = clearDepth;
        return att;
    };

    auto simpleColorPass = [](GpuRenderAttachment* colorAtt,
                              GpuView** sampledImages = nullptr,
                              u32 sampledCount = 0,
                              GpuBuffer** uniformBufs = nullptr,
                              u32 uniformCount = 0)
    {
        GpuRenderPass pass{};
        pass.colorAttachments = {colorAtt, 1};
        if (sampledImages) pass.sampledImages = {sampledImages, sampledCount};
        if (uniformBufs)   pass.uniformBuffers = {uniformBufs, uniformCount};
        return pass;
    };

    auto makeSimplePipeline = [](Span<const u8> vertShader, Span<const u8> fragShader,
                                 u32 pushSize = 0, GpuTopology topo = GpuTopology_triangleList)
    {
        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = vertShader;
        ci.fragmentShader = fragShader;
        Format cf = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&cf, 1};
        ci.pushConstantSize = pushSize;
        ci.topology = topo;
        return GpuPipeline::graphics(ci);
    };

    auto makeColorTarget = [](u32 w, u32 h)
    {
        return GpuImage::create(w, h, Format_r8g8b8a8_unorm, GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
    };

    auto barrierToTransferRead = [](GpuCmd* cmd, GpuView* view)
    {
        GpuImageBarrier ib{};
        ib.image = view;
        ib.nextStage = GpuStage_transfer;
        ib.nextAccess = GpuAccess_transferRead;
        ib.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});
    };

    // formatToSize
    {
        // Common 8-bit formats
        TEST(formatToSize(Format_r8_unorm) == 1);
        TEST(formatToSize(Format_r8_snorm) == 1);
        TEST(formatToSize(Format_r8g8_unorm) == 2);
        TEST(formatToSize(Format_r8g8b8a8_unorm) == 4);
        TEST(formatToSize(Format_r8g8b8a8_snorm) == 4);
        TEST(formatToSize(Format_r8g8b8a8_srgb) == 4);
        // 16-bit float / depth formats
        TEST(formatToSize(Format_r16_sfloat) == 2);
        TEST(formatToSize(Format_r16g16_sfloat) == 4);
        TEST(formatToSize(Format_r16g16b16a16_sfloat) == 8);
        TEST(formatToSize(Format_d16_unorm) == 2);
        // 32-bit float / int formats
        TEST(formatToSize(Format_r32_sfloat) == 4);
        TEST(formatToSize(Format_r32g32_sfloat) == 8);
        TEST(formatToSize(Format_r32g32b32_sfloat) == 12);
        TEST(formatToSize(Format_r32g32b32a32_sfloat) == 16);
        TEST(formatToSize(Format_r32_sint) == 4);
        TEST(formatToSize(Format_r32g32b32a32_uint) == 16);
        // Depth-stencil
        TEST(formatToSize(Format_d24_unorm_s8_uint) == 4);
        TEST(formatToSize(Format_d32_sfloat) == 4);
        TEST(formatToSize(Format_d32_sfloat_s8_uint) == 5);
        // Block-compressed
        TEST(formatToSize(Format_bc1_rgba_unorm_block) == 8);
        TEST(formatToSize(Format_bc3_unorm_block) == 16);
        TEST(formatToSize(Format_bc5_unorm_block) == 16);
        TEST(formatToSize(Format_bc7_unorm_block) == 16);
    }

    // getMaxMipmaps
    {
        TEST(getMaxMipmaps(1, 1, 1) == 1);
        TEST(getMaxMipmaps(2, 1, 1) == 2);
        TEST(getMaxMipmaps(64, 64, 1) == 7);
        TEST(getMaxMipmaps(128, 64, 1) == 8);
        TEST(getMaxMipmaps(256, 256, 256) == 9);
        TEST(getMaxMipmaps(0, 0, 0) == 0);
        TEST(getMaxMipmaps(1, 0, 1) == 1);
    }

    // GpuBuffer lifecycle
    {
        // Default construction
        GpuBuffer empty{};
        TEST(empty.data == nullptr);

        // Create a device-local buffer
        GpuBuffer devBuf = GpuBuffer::create(256, GpuBufferUsage_transferSrc | GpuBufferUsage_transferDst);
        TEST(devBuf.data != nullptr);

        // Create a host-visible buffer
        GpuBuffer hostBuf = GpuBuffer::create(128, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
        TEST(hostBuf.data != nullptr);

        // Move construction
        GpuBuffer moved{std::move(devBuf)};
        TEST(devBuf.data == nullptr);
        TEST(moved.data != nullptr);

        // Move assignment
        GpuBuffer dest{};
        dest = std::move(hostBuf);
        TEST(hostBuf.data == nullptr);
        TEST(dest.data != nullptr);

        // Destruction via scope exit
    }

    // GpuBuffer Write/Read with Non-Zero Offsets
    {
        // Host-visible path
        {
            GpuBuffer buf = GpuBuffer::create(256,
                    GpuBufferUsage_transferSrc | GpuBufferUsage_transferDst,
                    GpuMemoryUsage_frequentUpdate);

            // Write 68 bytes of 0x11111111 starting at offset 0
            u32 fillVal = 0x11111111;
            for (u32 off = 0; off < 68; off += 4)
                buf.write(&fillVal, off, sizeof(fillVal));

            // Overwrite at offset 64 and 128
            u32 v2 = 0x22222222;
            u32 v3 = 0x33333333;
            buf.write(&v2, 64, sizeof(v2));
            buf.write(&v3, 128, sizeof(v3));

            u32 r1 = 0, r2 = 0, r3 = 0;
            buf.read(&r1, 0, sizeof(r1));
            buf.read(&r2, 64, sizeof(r2));
            buf.read(&r3, 128, sizeof(r3));
            TEST(r1 == 0x11111111);
            TEST(r2 == 0x22222222);
            TEST(r3 == 0x33333333);

            // Cross-boundary read: 8 bytes starting at offset 60
            // Bytes 60-63 are the tail of 0x11111111 fill,
            // bytes 64-67 are the head of the 0x22222222 write
            u64 cross = 0;
            buf.read(&cross, 60, sizeof(cross));
            u32 tail = static_cast<u32>(cross & 0xFFFFFFFF);
            u32 head = static_cast<u32>((cross >> 32) & 0xFFFFFFFF);
            TEST(tail == 0x11111111);
            TEST(head == 0x22222222);
        }

        // Device-only (staging) path
        {
            GpuBuffer buf = GpuBuffer::create(256,
                    GpuBufferUsage_transferSrc | GpuBufferUsage_transferDst);

            u32 fillVal = 0x11111111;
            for (u32 off = 0; off < 68; off += 4)
                buf.write(&fillVal, off, sizeof(fillVal));

            u32 v2 = 0x22222222;
            u32 v3 = 0x33333333;
            buf.write(&v2, 64, sizeof(v2));
            buf.write(&v3, 128, sizeof(v3));

            u32 r1 = 0, r2 = 0, r3 = 0;
            buf.read(&r1, 0, sizeof(r1));
            buf.read(&r2, 64, sizeof(r2));
            buf.read(&r3, 128, sizeof(r3));
            TEST(r1 == 0x11111111);
            TEST(r2 == 0x22222222);
            TEST(r3 == 0x33333333);

            u64 cross = 0;
            buf.read(&cross, 60, sizeof(cross));
            u32 tail = static_cast<u32>(cross & 0xFFFFFFFF);
            u32 head = static_cast<u32>((cross >> 32) & 0xFFFFFFFF);
            TEST(tail == 0x11111111);
            TEST(head == 0x22222222);
        }
    }

    // GpuBuffer write/read: multiple values
    {
        GpuBuffer buf = GpuBuffer::create(256, GpuBufferUsage_transferSrc | GpuBufferUsage_transferDst, GpuMemoryUsage_frequentUpdate);
        u32 src[16] = {};
        for (u32 i = 0; i < 16; ++i)
            src[i] = i * 3 + 7;
        buf.write(src, 0, sizeof(src));
        u32 dst[16] = {};
        buf.read(dst, 0, sizeof(dst));
        for (u32 i = 0; i < 16; ++i)
            TEST(dst[i] == i * 3 + 7);
    }

    // GpuImage lifecycle
    {
        GpuImage empty{};
        TEST(empty.data == nullptr);

        // Simple constructor
        GpuImage img = GpuImage::create(16, 16, Format_r8g8b8a8_unorm, GpuImageUsage_transferSrc);
        TEST(img.data != nullptr);
        TEST(img.width() == 16);
        TEST(img.height() == 16);

        // Extended constructor with mip levels
        GpuImageCreateInfo ci{};
        ci.width = 64;
        ci.height = 64;
        ci.format = Format_r32_sfloat;
        ci.usage = GpuImageUsage_transferSrc | GpuImageUsage_transferDst;
        ci.mipLevels = 4;
        GpuImage mipImg = GpuImage::createEx(ci);
        TEST(mipImg.data != nullptr);
        TEST(mipImg.width() == 64);
        TEST(mipImg.height() == 64);

        // Move construction
        GpuImage moved{std::move(img)};
        TEST(img.data == nullptr);
        TEST(moved.data != nullptr);

        // Move assignment
        GpuImage dest{};
        dest = std::move(mipImg);
        TEST(mipImg.data == nullptr);
        TEST(dest.data != nullptr);
    }

    // GpuView lifecycle
    {
        GpuImage img = GpuImage::create(16, 16, Format_r8g8b8a8_unorm,
            GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled);

        GpuView view = GpuView::create(img, GpuAspect_color);
        TEST(view.data != nullptr);
        TEST(view.width() == 16);
        TEST(view.height() == 16);

        // Move construction
        GpuView moved{std::move(view)};
        TEST(view.data == nullptr);
        TEST(moved.data != nullptr);

        // Move assignment
        GpuView dest{};
        dest = std::move(moved);
        TEST(moved.data == nullptr);
        TEST(dest.data != nullptr);
    }

    // GpuView write/read
    {
        GpuImage img = GpuImage::create(16, 16, Format_r8g8b8a8_unorm,
            GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled);
        GpuView view = GpuView::create(img, GpuAspect_color);

        u32 src[16 * 16] = {};
        for (u32 i = 0; i < 16 * 16; ++i)
            src[i] = 0x30405060;
        view.write(src);
        u32 dst[16 * 16] = {};
        view.read(dst);
        for (u32 i = 0; i < 16 * 16; ++i)
            TEST(dst[i] == 0x30405060);
    }

    // GpuView Extended Config Affects Sampler Output
    {
        static constexpr u32 texSize = 4;

        GpuImage texImg = GpuImage::create(texSize, texSize, Format_r8g8b8a8_unorm,
            GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled);

        u32 checker[texSize * texSize] = {};
        for (u32 y = 0; y < texSize; ++y)
            for (u32 x = 0; x < texSize; ++x)
                checker[y * texSize + x] = ((x + y) & 1)
                    ? 0xFFFFFFFF
                    : 0x000000FF;

        GpuViewCreateInfo vci{};
        vci.image = &texImg;
        vci.aspectFlags = GpuAspect_color;
        vci.type = GpuViewType_2D;
        vci.filter = GpuFilter_linear;
        vci.edgeMode = GpuSamplerEdgeMode_clampToEdge;
        GpuView view = GpuView::createEx(vci);
        view.write(checker);

        // Verify descriptor is valid
        // Read back texture data to verify write
        u32 readback[texSize * texSize] = {};
        view.read(readback);
        bool match = true;
        for (u32 i = 0; i < texSize * texSize; ++i)
            if (readback[i] != checker[i])
                match = false;
        TEST(match);

        // Verify sampled rendering produces non-clear color
        static constexpr u32 outSize = 2;
        GpuImage outImg = GpuImage::create(outSize, outSize, Format_r8g8b8a8_unorm,
            GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
        GpuView outView = GpuView::create(outImg, GpuAspect_color);

        struct Push {
            f32 uvX;
            f32 uvY;
            u32 texIdx;
        };
        Push push{0.375f, 0.125f, view.samplerDescriptor()};

        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = {test_tri_vert_spv, sizeof(test_tri_vert_spv)};
        ci.fragmentShader = {test_sampler_frag_spv, sizeof(test_sampler_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&colorFmt, 1};
        ci.pushConstantSize = sizeof(Push);

        GpuPipeline pipe = GpuPipeline::graphics(ci);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment colorAtt{};
        colorAtt.image = &outView;
        colorAtt.loadOp = GpuLoadOp_clear;
        colorAtt.storeOp = GpuStoreOp_store;
        colorAtt.clearValue.color.float32[0] = 0.0f;
        colorAtt.clearValue.color.float32[1] = 0.0f;
        colorAtt.clearValue.color.float32[2] = 0.0f;
        colorAtt.clearValue.color.float32[3] = 1.0f;

        GpuRenderPass pass{};
        pass.colorAttachments = {&colorAtt, 1};
        GpuView* sampledImages[] = {&view};
        pass.sampledImages = sampledImages;

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDraw(cmd, 0, 3, 0, 1);
        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 result[outSize * outSize] = {};
        outView.read(result);
        // Sampling center of white texel (1,0) → white (0xFFFFFFFF)
        TEST(result[0] == 0xFFFFFFFF);
    }

    // Command Buffer Executes Recorded Commands
    {
        static constexpr u32 bufSize = 64;

        GpuBuffer devBuf = GpuBuffer::create(bufSize, GpuBufferUsage_transferDst | GpuBufferUsage_transferSrc
                | GpuBufferUsage_storageBuffer);
        GpuBuffer staging = GpuBuffer::create(bufSize, GpuBufferUsage_transferSrc | GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);

        u32 known = 0xDECAF123;
        staging.write(&known, 0, sizeof(known));

        GpuPipeline pipe = GpuPipeline::compute({test_compute_comp_spv, sizeof(test_compute_comp_spv)}, 12);

        struct Push {
            u32 addVal;
            u32 inIdx;
            u32 outIdx;
        };
        Push push{0, staging.storageDescriptor(), devBuf.storageDescriptor()};

        GpuCmd* cmd = gpuCmdBegin();
        TEST(cmd != nullptr);

        GpuBufferBarrier stagingBarrier{};
        stagingBarrier.buffer = &staging;
        stagingBarrier.nextStage = GpuStage_computeShader;
        stagingBarrier.nextAccess = GpuAccess_shaderRead;
        gpuMemoryBarrier(cmd, {&stagingBarrier, 1}, {});

        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDispatch(cmd, 1, 1, 1);

        GpuBufferBarrier devBarrier{};
        devBarrier.buffer = &devBuf;
        devBarrier.nextStage = GpuStage_transfer;
        devBarrier.nextAccess = GpuAccess_transferRead;
        gpuMemoryBarrier(cmd, {&devBarrier, 1}, {});

        gpuCmdEnd(cmd);

        u32 result = 0;
        devBuf.read(&result, 0, sizeof(result));
        TEST(result == 0xDECAF123);
    }

    // Buffer Barrier Synchronizes Compute Write Then Read
    {
        static constexpr u32 elemCount = 64;
        static constexpr u32 bufSize = elemCount * sizeof(u32);

        GpuBuffer buf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc,
                GpuMemoryUsage_frequentUpdate);
        GpuBuffer outBuf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i;
        buf.write(input, 0, bufSize);

        struct Push {
            u32 addVal;
            u32 inIdx;
            u32 outIdx;
        };

        GpuPipeline incPipe = GpuPipeline::compute({test_compute_comp_spv, sizeof(test_compute_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        // Barrier: buf to shaderWrite|shaderRead for Dispatch 1
        GpuBufferBarrier bb{};
        bb.buffer = &buf;
        bb.nextStage = GpuStage_computeShader;
        bb.nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        gpuMemoryBarrier(cmd, {&bb, 1}, {});

        // Dispatch 1: increment every element by 1 (in-place)
        {
            Push push{1, buf.storageDescriptor(), buf.storageDescriptor()};
            gpuBindPipeline(cmd, incPipe);
            gpuPushConstants(cmd, incPipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        // Barrier: buf to shaderRead for Dispatch 2
        bb.nextAccess = GpuAccess_shaderRead;
        gpuMemoryBarrier(cmd, {&bb, 1}, {});

        // Dispatch 2: copy from buf to outBuf (addVal=0)
        {
            Push push{0, buf.storageDescriptor(), outBuf.storageDescriptor()};
            gpuBindPipeline(cmd, incPipe);
            gpuPushConstants(cmd, incPipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        gpuCmdEnd(cmd);

        u32 result[elemCount] = {};
        outBuf.read(result, 0, bufSize);
        for (u32 i = 0; i < elemCount; ++i)
            TEST(result[i] == input[i] + 1);
    }

    // Image Barrier Transitions Layout Correctly
    {
        static constexpr u32 imgSize = 4;

        GpuImage img = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
            GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled | GpuImageUsage_colorAttachment);
        GpuView view = GpuView::create(img, GpuAspect_color);

        // Write a known pattern
        u32 red[imgSize * imgSize] = {};
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            red[i] = 0xFF0000FF;
        view.write(red);

        // Do a round-trip layout transition:
        // transferDst (after write) -> general -> transferSrc
        GpuCmd* cmd = gpuCmdBegin();

        GpuImageBarrier ib{};
        ib.image = &view;
        ib.nextStage = GpuStage_computeShader;
        ib.nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        ib.nextLayout = GpuLayout_general;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        ib.nextStage = GpuStage_transfer;
        ib.nextAccess = GpuAccess_transferRead;
        ib.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        gpuCmdEnd(cmd);

        // Read back — data should survive the transitions
        u32 result[imgSize * imgSize] = {};
        view.read(result);
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(result[i] == 0xFF0000FF);
    }

    // Combined Buffer+Image Barrier in a Dependency Chain
    {
        static constexpr u32 bufSize = 256;
        static constexpr u32 imgSize = 4;

        GpuBuffer storageBuf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc,
                GpuMemoryUsage_frequentUpdate);
        GpuImage img = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
            GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_storage);
        GpuView imgView = GpuView::create(img, GpuAspect_color);

        // Write 16 RGBA pixels as u32 values into the buffer
        u32 pixelData[imgSize * imgSize] = {};
        for (u32 i = 0; i < imgSize * imgSize; ++i)
        {
            u8 r = static_cast<u8>((i * 37) & 0xFF);
            u8 g = static_cast<u8>((i * 71) & 0xFF);
            u8 b = static_cast<u8>((i * 101) & 0xFF);
            pixelData[i] = r | (static_cast<u32>(g) << 8)
                         | (static_cast<u32>(b) << 16) | (0xFFu << 24);
        }
        storageBuf.write(pixelData, 0, sizeof(pixelData));

        struct Push {
            u32 bufIdx;
            u32 imgIdx;
        };
        GpuPipeline pipe = GpuPipeline::compute({test_buf_to_img_comp_spv, sizeof(test_buf_to_img_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        // Combined barrier: buffer to shaderRead|shaderWrite, image to general
        GpuBufferBarrier bb{};
        bb.buffer = &storageBuf;
        bb.nextStage = GpuStage_computeShader;
        bb.nextAccess = GpuAccess_shaderRead;
        GpuImageBarrier ib{};
        ib.image = &imgView;
        ib.nextStage = GpuStage_computeShader;
        ib.nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        ib.nextLayout = GpuLayout_general;
        gpuMemoryBarrier(cmd, {&bb, 1}, {&ib, 1});

        // Dispatch: read buffer -> write image
        {
            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&storageBuf};
            GpuView* storageImages[] = {&imgView};
            pass.storageBuffers = storageBufs;
            pass.storageImages = storageImages;
            gpuComputePass(cmd, pass);

            Push push{storageBuf.storageDescriptor(), imgView.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, imgSize, imgSize, 1);
        }

        // Barrier: both to transferSrc for readback
        bb.nextAccess = GpuAccess_transferRead;
        bb.nextStage = GpuStage_transfer;
        ib.nextAccess = GpuAccess_transferRead;
        ib.nextStage = GpuStage_transfer;
        ib.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {&bb, 1}, {&ib, 1});

        gpuCmdEnd(cmd);

        // Read back buffer — should still contain original data
        u32 bufResult[imgSize * imgSize] = {};
        storageBuf.read(bufResult, 0, sizeof(bufResult));
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(bufResult[i] == pixelData[i]);

        // Read back image — should match buffer data
        u32 imgResult[imgSize * imgSize] = {};
        imgView.read(imgResult);
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(imgResult[i] == pixelData[i]);
    }

    // Compute Pass Dispatch Produces Correct Output
    {
        static constexpr u32 elemCount = 64;
        static constexpr u32 bufSize = elemCount * sizeof(u32);

        GpuBuffer inBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
        GpuBuffer outBuf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i;
        inBuf.write(input, 0, bufSize);

        struct Push {
            u32 inIdx;
            u32 outIdx;
        };
        GpuPipeline pipe = GpuPipeline::compute({test_mul_comp_spv, sizeof(test_mul_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = &inBuf;
        barriers[0].nextStage = GpuStage_computeShader;
        barriers[0].nextAccess = GpuAccess_shaderRead;
        barriers[1].buffer = &outBuf;
        barriers[1].nextStage = GpuStage_computeShader;
        barriers[1].nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        gpuMemoryBarrier(cmd, {barriers, 2}, {});

        {
            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&inBuf, &outBuf};
            pass.storageBuffers = storageBufs;
            gpuComputePass(cmd, pass);

            Push push{inBuf.storageDescriptor(), outBuf.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        gpuCmdEnd(cmd);

        u32 output[elemCount] = {};
        outBuf.read(output, 0, bufSize);
        for (u32 i = 0; i < elemCount; ++i)
            TEST(output[i] == input[i] * 2);
    }

    // GpuPipeline compute: lifecycle
    {
        GpuPipeline pipe = GpuPipeline::compute({test_compute_comp_spv, sizeof(test_compute_comp_spv)}, 12);
        TEST(pipe.data != nullptr);

        // Move construction
        GpuPipeline moved{std::move(pipe)};
        TEST(pipe.data == nullptr);
        TEST(moved.data != nullptr);

        // Move assignment
        GpuPipeline dest{};
        dest = std::move(moved);
        TEST(moved.data == nullptr);
        TEST(dest.data != nullptr);
    }

    // Compute dispatch: SSBO input → push constant → SSBO output
    {
        static constexpr u32 elemCount = 64;
        static constexpr u32 bufSize = elemCount * sizeof(u32);
        u32 addVal = 100;

        GpuBuffer inBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
        GpuBuffer outBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i;
        inBuf.write(input, 0, bufSize);

        struct Push {
            u32 addVal;
            u32 inIdx;
            u32 outIdx;
        };
        Push push{addVal, inBuf.storageDescriptor(), outBuf.storageDescriptor()};

        GpuPipeline pipe = GpuPipeline::compute({test_compute_comp_spv, sizeof(test_compute_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = &outBuf;
        barriers[0].nextStage = GpuStage_computeShader;
        barriers[0].nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        barriers[1].buffer = &inBuf;
        barriers[1].nextStage = GpuStage_computeShader;
        barriers[1].nextAccess = GpuAccess_shaderRead;
        gpuMemoryBarrier(cmd, {barriers, 2}, {});

        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDispatch(cmd, elemCount, 1, 1);

        gpuCmdEnd(cmd);

        u32 output[elemCount] = {};
        outBuf.read(output, 0, bufSize);

        for (u32 i = 0; i < elemCount; ++i)
            TEST(output[i] == input[i] + addVal);
    }

    // GpuPipeline graphics: lifecycle
    {
        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = {test_tri_vert_spv, sizeof(test_tri_vert_spv)};
        ci.fragmentShader = {test_tri_frag_spv, sizeof(test_tri_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&colorFmt, 1};

        GpuPipeline pipe = GpuPipeline::graphics(ci);
        TEST(pipe.data != nullptr);

        GpuPipeline moved{std::move(pipe)};
        TEST(pipe.data == nullptr);
        TEST(moved.data != nullptr);

        GpuPipeline dest{};
        dest = std::move(moved);
        TEST(moved.data == nullptr);
        TEST(dest.data != nullptr);
    }

    // Offscreen render: render a full-screen triangle to an image, read it back
    {
        static constexpr u32 imgSize = 4;

        GpuImage colorImg = makeColorTarget(imgSize, imgSize);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        GpuPipeline pipe = makeSimplePipeline(shTriVert, shTriFrag);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment colorAtt = makeColorAtt(&colorView, 1.0f, 0.0f, 0.0f, 1.0f);
        GpuRenderPass pass = simpleColorPass(&colorAtt);

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);
        gpuDraw(cmd, 0, 3, 0, 1);
        gpuRenderPassEnd(cmd);

        gpuCmdEnd(cmd);

        u32 pixelData[imgSize * imgSize] = {};
        colorView.read(pixelData);

        // Triangle covers the whole viewport; all pixels should be (0.2, 0.4, 0.6, 1.0)
        // packed RGBA → 0xFF996633
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(pixelData[i] == 0xFF996633);
    }

    // Compute SSBO Add (Buffer -> Push Constant -> Buffer)
    {
        static constexpr u32 elemCount = 64;
        static constexpr u32 bufSize = elemCount * sizeof(u32);
        u32 addVal = 100;

        GpuBuffer inBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer,
                GpuMemoryUsage_frequentUpdate);
        GpuBuffer outBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer
                | GpuBufferUsage_transferSrc);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i;
        inBuf.write(input, 0, bufSize);

        struct Push {
            u32 addVal;
            u32 inIdx;
            u32 outIdx;
        };
        Push push{addVal, inBuf.storageDescriptor(), outBuf.storageDescriptor()};

        GpuPipeline pipe = GpuPipeline::compute(                {test_compute_comp_spv, sizeof(test_compute_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = &inBuf;
        barriers[0].nextStage = GpuStage_computeShader;
        barriers[0].nextAccess = GpuAccess_shaderRead;
        barriers[1].buffer = &outBuf;
        barriers[1].nextStage = GpuStage_computeShader;
        barriers[1].nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        gpuMemoryBarrier(cmd, {barriers, 2}, {});

        {
            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&inBuf, &outBuf};
            pass.storageBuffers = storageBufs;
            gpuComputePass(cmd, pass);

            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        gpuCmdEnd(cmd);

        u32 output[elemCount] = {};
        outBuf.read(output, 0, bufSize);

        for (u32 i = 0; i < elemCount; ++i)
            TEST(output[i] == input[i] + addVal);
    }

    // Offscreen Render with Depth Test
    {
        static constexpr u32 imgSize = 4;

        GpuImage colorImg = makeColorTarget(imgSize, imgSize);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        GpuImage depthImg = GpuImage::create(imgSize, imgSize, Format_d32_sfloat,
                GpuImageUsage_depthStencilAttachment | GpuImageUsage_transferSrc);
        GpuView depthView = GpuView::create(depthImg, GpuAspect_depth);

        struct Push {
            float depth;
            float pad[3];
            float color[4];
        };

        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = shDepthVert;
        ci.fragmentShader = shDepthFrag;
        ci.colorAttachmentFormats = {&kColorFmt, 1};
        ci.depthAttachmentFormat = Format_d32_sfloat;
        ci.enableDepthRead = true;
        ci.enableDepthWrite = true;
        ci.pushConstantSize = sizeof(Push);

        GpuPipeline pipe = GpuPipeline::graphics(ci);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment colorAtt = makeColorAtt(&colorView, 0.0f, 0.0f, 0.0f, 0.0f);
        GpuRenderAttachment depthAtt = makeDepthAtt(&depthView);

        GpuRenderPass pass{};
        pass.colorAttachments = {&colorAtt, 1};
        pass.depthAttachment = &depthAtt;

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);

        Push farPush{0.75f, {}, {1.0f, 0.0f, 0.0f, 1.0f}};
        gpuPushConstants(cmd, pipe, &farPush, sizeof(farPush));
        gpuDraw(cmd, 0, 3, 0, 1);

        Push nearPush{0.25f, {}, {0.0f, 1.0f, 0.0f, 1.0f}};
        gpuPushConstants(cmd, pipe, &nearPush, sizeof(nearPush));
        gpuDraw(cmd, 0, 3, 0, 1);

        gpuRenderPassEnd(cmd);

        barrierToTransferRead(cmd, &depthView);

        gpuCmdEnd(cmd);

        u32 colorPixels[imgSize * imgSize] = {};
        colorView.read(colorPixels);

        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(colorPixels[i] == 0xFF00FF00);

        f32 depthPixels[imgSize * imgSize] = {};
        depthView.read(depthPixels);

        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(depthPixels[i] > 0.24f && depthPixels[i] < 0.26f);
    }

    // Multi-Draw with Instancing
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

        GpuBuffer instBuf = GpuBuffer::create(sizeof(instanceData),
                GpuBufferUsage_uniformBuffer, GpuMemoryUsage_frequentUpdate);
        instBuf.write(instanceData, 0, sizeof(instanceData));

        GpuImage colorImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        struct Push {
            u32 dataIdx;
        };

        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = {test_instance_vert_spv, sizeof(test_instance_vert_spv)};
        ci.fragmentShader = {test_instance_frag_spv, sizeof(test_instance_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&colorFmt, 1};
        ci.pushConstantSize = sizeof(Push);
        ci.topology = GpuTopology_triangleStrip;

        GpuPipeline pipe = GpuPipeline::graphics(ci);

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier bb{};
        bb.buffer = &instBuf;
        bb.nextStage = GpuStage_vertexShader;
        bb.nextAccess = GpuAccess_shaderRead;
        gpuMemoryBarrier(cmd, {&bb, 1}, {});

        GpuRenderAttachment colorAtt{};
        colorAtt.image = &colorView;
        colorAtt.loadOp = GpuLoadOp_clear;
        colorAtt.storeOp = GpuStoreOp_store;
        colorAtt.clearValue.color.float32[0] = 0.0f;
        colorAtt.clearValue.color.float32[1] = 0.0f;
        colorAtt.clearValue.color.float32[2] = 0.0f;
        colorAtt.clearValue.color.float32[3] = 0.0f;

        GpuRenderPass pass{};
        GpuBuffer* uniformBufs[] = {&instBuf};
        pass.uniformBuffers = uniformBufs;
        pass.colorAttachments = {&colorAtt, 1};

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);

        Push push{instBuf.uniformDescriptor()};
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDraw(cmd, 0, 4, 0, instanceCount);

        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        colorView.read(pixels);

        u32 green = 0xFF00FF00;
        TEST(pixels[2 * imgSize + 2] == green);
        TEST(pixels[2 * imgSize + 6] == green);
        TEST(pixels[6 * imgSize + 2] == green);
        TEST(pixels[6 * imgSize + 6] == green);
    }

    // Multi-Viewport / Multi-Scissor Render
    {
        static constexpr u32 imgSize = 16;

        GpuImage colorImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        struct Push {
            float depth;
            float pad[3];
            float color[4];
        };

        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = {test_depth_vert_spv, sizeof(test_depth_vert_spv)};
        ci.fragmentShader = {test_depth_frag_spv, sizeof(test_depth_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&colorFmt, 1};
        ci.pushConstantSize = sizeof(Push);

        GpuPipeline pipe = GpuPipeline::graphics(ci);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment colorAtt{};
        colorAtt.image = &colorView;
        colorAtt.loadOp = GpuLoadOp_clear;
        colorAtt.storeOp = GpuStoreOp_store;
        colorAtt.clearValue.color.float32[0] = 0.0f;
        colorAtt.clearValue.color.float32[1] = 0.0f;
        colorAtt.clearValue.color.float32[2] = 0.0f;
        colorAtt.clearValue.color.float32[3] = 1.0f;

        GpuRenderPass pass{};
        pass.colorAttachments = {&colorAtt, 1};

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);

        Push red{0.0f, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
        gpuSetViewport(cmd, 0.0f, 0.0f, 8.0f, 16.0f);
        gpuSetScissor(cmd, 0, 0, 8, 16);
        gpuPushConstants(cmd, pipe, &red, sizeof(red));
        gpuDraw(cmd, 0, 3, 0, 1);

        Push blue{0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
        gpuSetViewport(cmd, 8.0f, 0.0f, 8.0f, 16.0f);
        gpuSetScissor(cmd, 8, 0, 8, 16);
        gpuPushConstants(cmd, pipe, &blue, sizeof(blue));
        gpuDraw(cmd, 0, 3, 0, 1);

        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        colorView.read(pixels);

        u32 redPx = 0xFF0000FF;
        u32 bluePx = 0xFFFF0000;
        for (u32 y = 0; y < imgSize; ++y) {
            for (u32 x = 0; x < 8; ++x)
                TEST(pixels[y * imgSize + x] == redPx);
            for (u32 x = 8; x < imgSize; ++x)
                TEST(pixels[y * imgSize + x] == bluePx);
        }
    }

    // Compute Image Filter (Blur)
    {
        static constexpr u32 imgSize = 8;

        GpuImage srcImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_sampled | GpuImageUsage_transferDst | GpuImageUsage_transferSrc);
        GpuView srcView = GpuView::create(srcImg, GpuAspect_color);

        u32 white = 0xFFFFFFFF;
        u32 whiteImg[imgSize * imgSize];
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            whiteImg[i] = white;
        srcView.write(whiteImg);

        GpuImage dstImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_storage | GpuImageUsage_transferSrc);
        GpuViewCreateInfo dstCI{};
        dstCI.image = &dstImg;
        dstCI.aspectFlags = GpuAspect_color;
        dstCI.type = GpuViewType_2D;
        GpuView dstView = GpuView::createEx(dstCI);

        struct Push {
            u32 srcIdx;
            u32 dstIdx;
        };

        GpuPipeline pipe = GpuPipeline::compute({test_blur_comp_spv, sizeof(test_blur_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuImageBarrier srcBarrier{};
        srcBarrier.image = &srcView;
        srcBarrier.nextStage = GpuStage_computeShader;
        srcBarrier.nextAccess = GpuAccess_shaderRead;
        srcBarrier.nextLayout = GpuLayout_shaderReadOnly;
        GpuImageBarrier dstBarrier{};
        dstBarrier.image = &dstView;
        dstBarrier.nextStage = GpuStage_computeShader;
        dstBarrier.nextAccess = GpuAccess_shaderWrite;
        dstBarrier.nextLayout = GpuLayout_general;
        GpuImageBarrier imgBarriers[] = {srcBarrier, dstBarrier};
        gpuMemoryBarrier(cmd, {}, {imgBarriers, 2});

        {
            GpuComputePass pass{};
            GpuView* sampledImages[] = {&srcView};
            GpuView* storageImages[] = {&dstView};
            pass.sampledImages = sampledImages;
            pass.storageImages = storageImages;
            gpuComputePass(cmd, pass);

            Push push{srcView.samplerDescriptor(), dstView.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, imgSize, imgSize, 1);
        }

        GpuImageBarrier readBarrier{};
        readBarrier.image = &dstView;
        readBarrier.nextStage = GpuStage_transfer;
        readBarrier.nextAccess = GpuAccess_transferRead;
        readBarrier.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {}, {&readBarrier, 1});

        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        dstView.read(pixels);

        for (u32 y = 1; y < imgSize - 1; ++y)
            for (u32 x = 1; x < imgSize - 1; ++x)
                TEST(pixels[y * imgSize + x] == white);
    }

    // Render to Texture, Sample in Second Pass
    {
        static constexpr u32 imgSize = 4;

        GpuImage firstImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_sampled | GpuImageUsage_transferSrc);
        GpuView firstView = GpuView::create(firstImg, GpuAspect_color);

        GpuImage resultImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
        GpuView resultView = GpuView::create(resultImg, GpuAspect_color);

        GpuGraphicsPipelineCreateInfo pass1CI{};
        pass1CI.vertexShader = {test_tri_vert_spv, sizeof(test_tri_vert_spv)};
        pass1CI.fragmentShader = {test_tri_frag_spv, sizeof(test_tri_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        pass1CI.colorAttachmentFormats = {&colorFmt, 1};
        GpuPipeline pass1Pipe = GpuPipeline::graphics(pass1CI);

        struct Push2 { u32 texIdx; };
        GpuGraphicsPipelineCreateInfo pass2CI{};
        pass2CI.vertexShader = {test_tri_vert_spv, sizeof(test_tri_vert_spv)};
        pass2CI.fragmentShader = {test_invert_frag_spv, sizeof(test_invert_frag_spv)};
        pass2CI.colorAttachmentFormats = {&colorFmt, 1};
        pass2CI.pushConstantSize = sizeof(Push2);
        GpuPipeline pass2Pipe = GpuPipeline::graphics(pass2CI);

        GpuCmd* cmd = gpuCmdBegin();

        // Pass 1: render triangle to first attachment
        {
            GpuRenderAttachment colorAtt{};
            colorAtt.image = &firstView;
            colorAtt.loadOp = GpuLoadOp_clear;
            colorAtt.storeOp = GpuStoreOp_store;
            colorAtt.clearValue.color.float32[0] = 0.0f;
            colorAtt.clearValue.color.float32[1] = 0.0f;
            colorAtt.clearValue.color.float32[2] = 0.0f;
            colorAtt.clearValue.color.float32[3] = 1.0f;

            GpuRenderPass pass{};
            pass.colorAttachments = {&colorAtt, 1};

            gpuRenderPassBegin(cmd, pass);
            gpuBindPipeline(cmd, pass1Pipe);
            gpuDraw(cmd, 0, 3, 0, 1);
            gpuRenderPassEnd(cmd);
        }

        // Barrier: transition first attachment to shaderReadOnly
        GpuImageBarrier ib{};
        ib.image = &firstView;
        ib.nextStage = GpuStage_fragmentShader;
        ib.nextAccess = GpuAccess_shaderRead;
        ib.nextLayout = GpuLayout_shaderReadOnly;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        // Pass 2: sample first attachment, invert, write to result
        {
            GpuRenderAttachment colorAtt{};
            colorAtt.image = &resultView;
            colorAtt.loadOp = GpuLoadOp_clear;
            colorAtt.storeOp = GpuStoreOp_store;
            colorAtt.clearValue.color.float32[0] = 0.0f;
            colorAtt.clearValue.color.float32[1] = 0.0f;
            colorAtt.clearValue.color.float32[2] = 0.0f;
            colorAtt.clearValue.color.float32[3] = 1.0f;

            GpuRenderPass pass{};
            pass.colorAttachments = {&colorAtt, 1};
            GpuView* sampledImages[] = {&firstView};
            pass.sampledImages = sampledImages;

            gpuRenderPassBegin(cmd, pass);
            gpuBindPipeline(cmd, pass2Pipe);
            Push2 push{firstView.samplerDescriptor()};
            gpuPushConstants(cmd, pass2Pipe, &push, sizeof(push));
            gpuDraw(cmd, 0, 3, 0, 1);
            gpuRenderPassEnd(cmd);
        }

        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        resultView.read(pixels);

        // Invert of vec4(0.2, 0.4, 0.6, 1.0) = vec4(0.8, 0.6, 0.4, 0.0)
        // In RGBA8: R=204, G=153, B=102, A=0  => 0x006699CC
        u32 expected = 0x006699CC;
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(pixels[i] == expected);
    }

    // Buffer Readback After Compute (Staging Read)
    {
        static constexpr u32 elemCount = 32;
        static constexpr u32 bufSize = elemCount * sizeof(u32);

        GpuBuffer inBuf = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i + 1;
        inBuf.write(input, 0, bufSize);

        GpuBuffer outBuf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc,
                GpuMemoryUsage_stagingRead);

        struct Push { u32 inIdx; u32 outIdx; };
        GpuPipeline pipe = GpuPipeline::compute({test_mul_comp_spv, sizeof(test_mul_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier barriers[2] = {};
        barriers[0].buffer = &inBuf;
        barriers[0].nextStage = GpuStage_computeShader;
        barriers[0].nextAccess = GpuAccess_shaderRead;
        barriers[1].buffer = &outBuf;
        barriers[1].nextStage = GpuStage_computeShader;
        barriers[1].nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        gpuMemoryBarrier(cmd, {barriers, 2}, {});

        {
            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&inBuf, &outBuf};
            pass.storageBuffers = storageBufs;
            gpuComputePass(cmd, pass);

            Push push{inBuf.storageDescriptor(), outBuf.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        GpuBufferBarrier readBarrier{};
        readBarrier.buffer = &outBuf;
        readBarrier.nextStage = GpuStage_host;
        readBarrier.nextAccess = GpuAccess_hostRead;
        gpuMemoryBarrier(cmd, {&readBarrier, 1}, {});

        gpuCmdEnd(cmd);

        u32 output[elemCount] = {};
        outBuf.read(output, 0, bufSize);
        for (u32 i = 0; i < elemCount; ++i)
            TEST(output[i] == input[i] * 2);
    }

    // Pipeline Barrier Granularity (Chained Dispatches)
    {
        static constexpr u32 elemCount = 32;
        static constexpr u32 bufSize = elemCount * sizeof(u32);

        GpuBuffer bufA = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
        GpuBuffer bufB = GpuBuffer::create(bufSize, GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
        GpuBuffer bufC = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc,
                GpuMemoryUsage_stagingRead);

        u32 input[elemCount] = {};
        for (u32 i = 0; i < elemCount; ++i)
            input[i] = i + 1;
        bufA.write(input, 0, bufSize);

        struct Push { u32 inIdx; u32 outIdx; };
        GpuPipeline pipe = GpuPipeline::compute({test_mul_comp_spv, sizeof(test_mul_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        // Dispatch 1: bufA -> bufB (multiply by 2)
        {
            GpuBufferBarrier barriers[2] = {};
            barriers[0].buffer = &bufA;
            barriers[0].nextStage = GpuStage_computeShader;
            barriers[0].nextAccess = GpuAccess_shaderRead;
            barriers[1].buffer = &bufB;
            barriers[1].nextStage = GpuStage_computeShader;
            barriers[1].nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
            gpuMemoryBarrier(cmd, {barriers, 2}, {});

            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&bufA, &bufB};
            pass.storageBuffers = storageBufs;
            gpuComputePass(cmd, pass);

            Push push{bufA.storageDescriptor(), bufB.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        // Barrier: bufB -> shaderRead for dispatch 2
        {
            GpuBufferBarrier bb{};
            bb.buffer = &bufB;
            bb.nextStage = GpuStage_computeShader;
            bb.nextAccess = GpuAccess_shaderRead;
            GpuBufferBarrier bc{};
            bc.buffer = &bufC;
            bc.nextStage = GpuStage_computeShader;
            bc.nextAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
            gpuMemoryBarrier(cmd, {&bb, 1}, {});
        }

        // Dispatch 2: bufB -> bufC (multiply by 2 again)
        {
            GpuComputePass pass{};
            GpuBuffer* storageBufs[] = {&bufB, &bufC};
            pass.storageBuffers = storageBufs;
            gpuComputePass(cmd, pass);

            Push push{bufB.storageDescriptor(), bufC.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, elemCount, 1, 1);
        }

        // Barrier: bufC for host read
        {
            GpuBufferBarrier bc{};
            bc.buffer = &bufC;
            bc.nextStage = GpuStage_host;
            bc.nextAccess = GpuAccess_hostRead;
            gpuMemoryBarrier(cmd, {&bc, 1}, {});
        }

        gpuCmdEnd(cmd);

        u32 output[elemCount] = {};
        bufC.read(output, 0, bufSize);
        for (u32 i = 0; i < elemCount; ++i)
            TEST(output[i] == input[i] * 4);
    }

    // Image Array Rendering (Layered Rendering)
    {
        static constexpr u32 imgSize = 4;
        static constexpr u32 numLayers = 4;

        GpuImageCreateInfo imgCI{};
        imgCI.width = imgSize;
        imgCI.height = imgSize;
        imgCI.format = Format_r8g8b8a8_unorm;
        imgCI.arrayLayers = numLayers;
        imgCI.usage = GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc;
        GpuImage arrayImg = GpuImage::createEx(imgCI);

        // Hoisted: pipeline and attachment template (invariant across layers)
        GpuPipeline pipe = makeSimplePipeline(shTriVert, shTriFrag);
        GpuRenderAttachment colorAtt{};
        colorAtt.loadOp = GpuLoadOp_clear;
        colorAtt.storeOp = GpuStoreOp_store;
        colorAtt.clearValue.color.float32[0] = 0.0f;
        colorAtt.clearValue.color.float32[1] = 0.0f;
        colorAtt.clearValue.color.float32[2] = 0.0f;
        colorAtt.clearValue.color.float32[3] = 1.0f;
        GpuRenderPass pass{};
        pass.colorAttachments = {&colorAtt, 1};

        for (u32 layer = 0; layer < numLayers; ++layer) {
            GpuViewCreateInfo viewCI{};
            viewCI.image = &arrayImg;
            viewCI.baseArrayLayer = layer;
            viewCI.layerCount = 1;
            viewCI.aspectFlags = GpuAspect_color;
            viewCI.type = GpuViewType_2D;
            GpuView layerView = GpuView::createEx(viewCI);
            colorAtt.image = &layerView;

            GpuCmd* cmd = gpuCmdBegin();
            gpuRenderPassBegin(cmd, pass);
            gpuBindPipeline(cmd, pipe);
            gpuDraw(cmd, 0, 3, 0, 1);
            gpuRenderPassEnd(cmd);
            gpuCmdEnd(cmd);

            u32 pixels[imgSize * imgSize] = {};
            layerView.read(pixels);

            for (u32 i = 0; i < imgSize * imgSize; ++i)
                TEST(pixels[i] == 0xFF996633);
        }
    }

    // gpuWaitIdle Waits for Pending Work
    {
        static constexpr u32 bufSize = 64;

        GpuBuffer devBuf = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer | GpuBufferUsage_transferSrc);
        GpuBuffer staging = GpuBuffer::create(bufSize,
                GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);

        u32 known = 0xDEADBEEF;
        staging.write(&known, 0, sizeof(known));

        GpuPipeline pipe = GpuPipeline::compute({test_compute_comp_spv, sizeof(test_compute_comp_spv)}, 12);

        struct Push {
            u32 addVal;
            u32 inIdx;
            u32 outIdx;
        };
        Push push{0, staging.storageDescriptor(), devBuf.storageDescriptor()};

        GpuCmd* cmd = gpuCmdBegin();

        GpuBufferBarrier stagingBarrier{};
        stagingBarrier.buffer = &staging;
        stagingBarrier.nextStage = GpuStage_computeShader;
        stagingBarrier.nextAccess = GpuAccess_shaderRead;
        gpuMemoryBarrier(cmd, {&stagingBarrier, 1}, {});

        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDispatch(cmd, 1, 1, 1);

        GpuBufferBarrier devBarrier{};
        devBarrier.buffer = &devBuf;
        devBarrier.nextStage = GpuStage_transfer;
        devBarrier.nextAccess = GpuAccess_transferRead;
        gpuMemoryBarrier(cmd, {&devBarrier, 1}, {});

        gpuCmdEnd(cmd);

        // Immediately wait — this should make compute results visible
        gpuWaitIdle();

        u32 result = 0;
        devBuf.read(&result, 0, sizeof(result));
        TEST(result == 0xDEADBEEF);
    }

    // Uniform Buffer Passes Data to Vertex Shader
    {
        static constexpr u32 imgSize = 4;

        // Color as f32[4]: (0.5, 0.3, 0.7, 1.0)
        f32 colorData[4] = {0.5f, 0.3f, 0.7f, 1.0f};

        GpuBuffer uniformBuf = GpuBuffer::create(64, GpuBufferUsage_uniformBuffer, GpuMemoryUsage_frequentUpdate);
        uniformBuf.write(colorData, 0, sizeof(colorData));

        GpuImage colorImg = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        struct Push {
            u32 colorIdx;
        };

        GpuGraphicsPipelineCreateInfo ci{};
        ci.vertexShader = {test_uniform_vert_spv, sizeof(test_uniform_vert_spv)};
        ci.fragmentShader = {test_uniform_frag_spv, sizeof(test_uniform_frag_spv)};
        Format colorFmt = Format_r8g8b8a8_unorm;
        ci.colorAttachmentFormats = {&colorFmt, 1};
        ci.pushConstantSize = sizeof(Push);

        GpuPipeline pipe = GpuPipeline::graphics(ci);

        Push push{uniformBuf.uniformDescriptor()};

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment colorAtt{};
        colorAtt.image = &colorView;
        colorAtt.loadOp = GpuLoadOp_clear;
        colorAtt.storeOp = GpuStoreOp_store;
        colorAtt.clearValue.color.float32[0] = 0.0f;
        colorAtt.clearValue.color.float32[1] = 0.0f;
        colorAtt.clearValue.color.float32[2] = 0.0f;
        colorAtt.clearValue.color.float32[3] = 0.0f;

        GpuRenderPass pass{};
        GpuBuffer* uniformBufs[] = {&uniformBuf};
        pass.uniformBuffers = uniformBufs;
        pass.colorAttachments = {&colorAtt, 1};

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDraw(cmd, 0, 3, 0, 1);
        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        colorView.read(pixels);

        // Expected: f32(0.5, 0.3, 0.7, 1.0) -> unorm8(128, 76, 178, 255)
        for (u32 i = 0; i < imgSize * imgSize; ++i)
        {
            // Allow small rounding differences
            u8 r = static_cast<u8>((pixels[i] >> 0) & 0xFF);
            u8 g = static_cast<u8>((pixels[i] >> 8) & 0xFF);
            u8 b = static_cast<u8>((pixels[i] >> 16) & 0xFF);
            TEST(r >= 125 && r <= 130);
            TEST(g >= 74 && g <= 79);
            TEST(b >= 176 && b <= 181);
            TEST(((pixels[i] >> 24) & 0xFF) == 0xFF);
        }
    }

    // Storage Image Write via Compute Shader
    {
        static constexpr u32 imgSize = 4;

        GpuImage img = GpuImage::create(imgSize, imgSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_transferSrc | GpuImageUsage_transferDst
                | GpuImageUsage_storage | GpuImageUsage_sampled);
        GpuView view = GpuView::create(img, GpuAspect_color, GpuFilter_nearest);

        struct Push {
            u32 imgIdx;
        };

        GpuPipeline pipe = GpuPipeline::compute({test_storage_img_comp_spv, sizeof(test_storage_img_comp_spv)}, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        // Barrier: image to general for storage write
        GpuImageBarrier ib{};
        ib.image = &view;
        ib.nextStage = GpuStage_computeShader;
        ib.nextAccess = GpuAccess_shaderWrite;
        ib.nextLayout = GpuLayout_general;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        {
            GpuComputePass pass{};
            GpuView* storageImages[] = {&view};
            pass.storageImages = storageImages;
            gpuComputePass(cmd, pass);

            Push push{view.storageDescriptor()};
            gpuBindPipeline(cmd, pipe);
            gpuPushConstants(cmd, pipe, &push, sizeof(push));
            gpuDispatch(cmd, imgSize, imgSize, 1);
        }

        // Barrier: image to transferSrc for readback
        ib.nextStage = GpuStage_transfer;
        ib.nextAccess = GpuAccess_transferRead;
        ib.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        view.read(pixels);

        // Shader writes: (x+y)&1==0 -> vec4(1,1,1,1)=0xFFFFFFFF, else vec4(0,0,0,1)=0xFF000000
        u32 expected[imgSize * imgSize] = {};
        for (u32 y = 0; y < imgSize; ++y)
            for (u32 x = 0; x < imgSize; ++x)
                expected[y * imgSize + x] = ((x + y) & 1)
                    ? 0xFF000000  // black (R=0,G=0,B=0,A=255)
                    : 0xFFFFFFFF; // white

        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(pixels[i] == expected[i]);
    }

    // GpuView::writeCubemap
    {
        static constexpr u32 faceSize = 4;
        static constexpr u32 bpp = 4;

        GpuImageCreateInfo ci{};
        ci.width = faceSize;
        ci.height = faceSize;
        ci.format = Format_r8g8b8a8_unorm;
        ci.usage = GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled;
        ci.arrayLayers = 6;
        ci.flags = GpuImageConfig_cubeCompatible;
        GpuImage cubeImg = GpuImage::createEx(ci);

        GpuViewCreateInfo vci{};
        vci.image = &cubeImg;
        vci.aspectFlags = GpuAspect_color;
        vci.type = GpuViewType_cube;
        vci.layerCount = 6;
        GpuView cubeView = GpuView::createEx(vci);

        // Build cubemap cross: 4 columns × 3 rows (each face is faceSize × faceSize)
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
        //   layer 0 (+X) → src (2, 1)   layer 3 (+Y) → src (1, 0)
        //   layer 1 (-X) → src (0, 1)   layer 4 (+Z) → src (1, 1)
        //   layer 2 (-Y) → src (1, 2)   layer 5 (-Z) → src (3, 1)
        fillFace(2, 1, 0xFF, 0x00, 0x00, 0xFF); // +X (layer 0): red
        fillFace(0, 1, 0x00, 0xFF, 0x00, 0xFF); // -X (layer 1): green
        fillFace(1, 2, 0x00, 0x00, 0xFF, 0xFF); // -Y (layer 2): blue
        fillFace(1, 0, 0xFF, 0xFF, 0x00, 0xFF); // +Y (layer 3): yellow
        fillFace(1, 1, 0xFF, 0x00, 0xFF, 0xFF); // +Z (layer 4): magenta
        fillFace(3, 1, 0x00, 0xFF, 0xFF, 0xFF); // -Z (layer 5): cyan

        cubeView.writeCubemap(cross);

        // Read back layer 0 (+X) via a single-layer view
        GpuViewCreateInfo readVci{};
        readVci.image = &cubeImg;
        readVci.aspectFlags = GpuAspect_color;
        readVci.type = GpuViewType_2D;
        readVci.baseArrayLayer = 0;
        readVci.layerCount = 1;
        GpuView readView = GpuView::createEx(readVci);

        GpuCmd* cmd = gpuCmdBegin();
        barrierToTransferRead(cmd, &readView);
        gpuCmdEnd(cmd);

        u32 face[faceSize * faceSize] = {};
        readView.read(face);
        for (u32 i = 0; i < faceSize * faceSize; ++i)
            TEST(face[i] == 0xFF0000FF); // RGBA: R=0xFF, G=0x00, B=0x00, A=0xFF
    }

    // GpuView::genMipmaps — verify base level not corrupted
    {
        static constexpr u32 imgSize = 8;
        static constexpr u32 mipLevels = 4;

        GpuImageCreateInfo ci{};
        ci.width = imgSize;
        ci.height = imgSize;
        ci.format = Format_r8g8b8a8_unorm;
        ci.usage = GpuImageUsage_transferSrc | GpuImageUsage_transferDst | GpuImageUsage_sampled;
        ci.mipLevels = mipLevels;
        GpuImage mipImg = GpuImage::createEx(ci);

        GpuViewCreateInfo vci{};
        vci.image = &mipImg;
        vci.aspectFlags = GpuAspect_color;
        vci.type = GpuViewType_2D;
        vci.levelCount = mipLevels;
        GpuView mipView = GpuView::createEx(vci);

        u32 yellow = 0xFF00FFFF;
        u32 src[imgSize * imgSize] = {};
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            src[i] = yellow;
        mipView.write(src);

        mipView.genMipmaps();

        u32 dst[imgSize * imgSize] = {};
        mipView.read(dst);
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(dst[i] == yellow);
    }

    // GpuView::genMipmaps — verify mip 1 content via read()
    {
        static constexpr u32 w = 4;
        static constexpr u32 h = 4;
        static constexpr u32 mipLevels = 3;

        GpuImageCreateInfo ci{};
        ci.width = w;
        ci.height = h;
        ci.format = Format_r8g8b8a8_unorm;
        ci.usage = GpuImageUsage_transferSrc | GpuImageUsage_transferDst
                 | GpuImageUsage_sampled;
        ci.mipLevels = mipLevels;
        GpuImage img = GpuImage::createEx(ci);

        GpuViewCreateInfo fullVci{};
        fullVci.image = &img;
        fullVci.aspectFlags = GpuAspect_color;
        fullVci.type = GpuViewType_2D;
        fullVci.levelCount = mipLevels;
        GpuView fullView = GpuView::createEx(fullVci);

        u32 red   = 0xFF0000FF;
        u32 green = 0xFF00FF00;
        u32 src[w * h] = {};
        for (u32 y = 0; y < h; ++y)
            for (u32 x = 0; x < w; ++x)
                src[y * w + x] = (x < 2 && y < 2) ? red : green;
        fullView.write(src);

        fullView.genMipmaps();

        // Read mip 1 (2×2) via a view targeting only that level
        GpuViewCreateInfo mip1Vci{};
        mip1Vci.image = &img;
        mip1Vci.aspectFlags = GpuAspect_color;
        mip1Vci.type = GpuViewType_2D;
        mip1Vci.baseMipLevel = 1;
        mip1Vci.levelCount = 1;
        GpuView mip1View = GpuView::createEx(mip1Vci);

        GpuCmd* cmd = gpuCmdBegin();
        barrierToTransferRead(cmd, &mip1View);
        gpuCmdEnd(cmd);

        u32 mip1[4] = {};
        mip1View.read(mip1);

        // Mip 1 (2×2): top-left averages the red 2×2 block → red;
        // the other three average green quadrants → green
        TEST(mip1[0] == red);
        TEST(mip1[1] == green);
        TEST(mip1[2] == green);
        TEST(mip1[3] == green);
    }

    // GpuLoadOp_load: render pass begins without clearing, preserves existing content
    {
        static constexpr u32 w = 4;
        static constexpr u32 h = 4;
        static constexpr u32 halfW = 2;

        GpuImage img = GpuImage::create(w, h, Format_r8g8b8a8_unorm,
                GpuImageUsage_colorAttachment | GpuImageUsage_transferSrc
                | GpuImageUsage_transferDst);
        GpuView view = GpuView::create(img, GpuAspect_color);

        // Fill image with known pattern (red)
        u32 redPx = 0xFF0000FF;
        u32 src[w * h];
        for (u32 i = 0; i < w * h; ++i)
            src[i] = redPx;
        view.write(src);

        GpuPipeline pipe = makeSimplePipeline(shTriVert, shTriFrag);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment att = makeColorAtt(&view, 0.0f, 0.0f, 0.0f, 0.0f,
                                                GpuLoadOp_load, GpuStoreOp_store);
        GpuRenderPass pass = simpleColorPass(&att);

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);

        // Viewport covers left half only; right half should retain original red
        gpuSetViewport(cmd, 0.0f, 0.0f, static_cast<f32>(halfW), static_cast<f32>(h));
        gpuSetScissor(cmd, 0, 0, halfW, h);
        gpuDraw(cmd, 0, 3, 0, 1);

        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 pixels[w * h] = {};
        view.read(pixels);

        u32 triColor = 0xFF996633;
        for (u32 y = 0; y < h; ++y)
            for (u32 x = 0; x < w; ++x)
                TEST(pixels[y * w + x] == (x < halfW ? triColor : redPx));
    }

    // GpuView mirroredRepeat edge mode
    {
        static constexpr u32 texSize = 2;
        static constexpr u32 outSize = 2;

        GpuImage texImg = GpuImage::create(texSize, texSize, Format_r8g8b8a8_unorm,
                GpuImageUsage_transferSrc | GpuImageUsage_transferDst
                | GpuImageUsage_sampled);

        u32 pattern[texSize * texSize] = {0xFF0000FF, 0xFF00FF00,
                                           0xFFFF0000, 0xFFFFFFFF};
        GpuViewCreateInfo vci{};
        vci.image = &texImg;
        vci.aspectFlags = GpuAspect_color;
        vci.type = GpuViewType_2D;
        vci.filter = GpuFilter_nearest;
        vci.edgeMode = GpuSamplerEdgeMode_mirroredRepeat;
        GpuView view = GpuView::createEx(vci);
        view.write(pattern);

        GpuImage outImg = makeColorTarget(outSize, outSize);
        GpuView outView = GpuView::create(outImg, GpuAspect_color);

        // UV=(1.25, 0.25) -> mirroredRepeat: floor(1.25)=1(odd) -> 1-0.25=0.75,
        // floor(0.25)=0(even) -> 0.25.  Nearest texel to (0.75, 0.25) = (1,0) = green
        struct Push { f32 uvX; f32 uvY; u32 texIdx; };
        Push push{1.25f, 0.25f, view.samplerDescriptor()};

        GpuPipeline pipe = makeSimplePipeline(shTriVert, shSamplerFrag, sizeof(Push));

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment att = makeColorAtt(&outView, 0.0f, 0.0f, 0.0f, 0.0f);
        GpuRenderPass pass = simpleColorPass(&att);
        GpuView* sampledImages[] = {&view};
        pass.sampledImages = sampledImages;

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);
        gpuPushConstants(cmd, pipe, &push, sizeof(push));
        gpuDraw(cmd, 0, 3, 0, 1);
        gpuRenderPassEnd(cmd);
        gpuCmdEnd(cmd);

        u32 result[outSize * outSize] = {};
        outView.read(result);
        for (u32 i = 0; i < outSize * outSize; ++i)
            TEST(result[i] == 0xFF00FF00); // green from mirrored repeat
    }

    // Untested barrier stage/access flags: allGraphics + colorAttachmentWrite
    {
        static constexpr u32 imgSize = 4;

        GpuImage colorImg = makeColorTarget(imgSize, imgSize);
        GpuView colorView = GpuView::create(colorImg, GpuAspect_color);

        GpuPipeline pipe = makeSimplePipeline(shTriVert, shTriFrag);

        GpuCmd* cmd = gpuCmdBegin();

        GpuRenderAttachment att = makeColorAtt(&colorView, 1.0f, 0.0f, 0.0f, 1.0f);
        GpuRenderPass pass = simpleColorPass(&att);

        gpuRenderPassBegin(cmd, pass);
        gpuBindPipeline(cmd, pipe);
        gpuDraw(cmd, 0, 3, 0, 1);
        gpuRenderPassEnd(cmd);

        // Barrier with allGraphics / colorAttachmentWrite
        GpuImageBarrier ib{};
        ib.image = &colorView;
        ib.nextStage = GpuStage_allGraphics;
        ib.nextAccess = GpuAccess_colorAttachmentWrite;
        ib.nextLayout = GpuLayout_transferSrc;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        // Barrier from read-after-write: allCommands + memoryRead
        ib.nextStage = GpuStage_allCommands;
        ib.nextAccess = GpuAccess_memoryRead;
        ib.nextLayout = GpuLayout_general;
        gpuMemoryBarrier(cmd, {}, {&ib, 1});

        gpuCmdEnd(cmd);

        u32 pixels[imgSize * imgSize] = {};
        colorView.read(pixels);
        for (u32 i = 0; i < imgSize * imgSize; ++i)
            TEST(pixels[i] == 0xFF996633);
    }
}

