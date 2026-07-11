/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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

#ifndef HG_GPU_HPP
#define HG_GPU_HPP

#include "hg_core.hpp"

namespace hg {

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 *
 * Returns
 * - Whether init succeeded
 */
bool gpuInit();

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void gpuDeinit();

/**
 * Wait for the GPU to finish work
 */
void gpuWaitIdle();

/**
 * Pixel formats
 */
enum Format : u32 {
    Format_undefined = 0,
    Format_r4g4_unorm_pack8 = 1,
    Format_r4g4b4a4_unorm_pack16 = 2,
    Format_b4g4r4a4_unorm_pack16 = 3,
    Format_r5g6b5_unorm_pack16 = 4,
    Format_b5g6r5_unorm_pack16 = 5,
    Format_r5g5b5a1_unorm_pack16 = 6,
    Format_b5g5r5a1_unorm_pack16 = 7,
    Format_a1r5g5b5_unorm_pack16 = 8,
    Format_r8_unorm = 9,
    Format_r8_snorm = 10,
    Format_r8_uscaled = 11,
    Format_r8_sscaled = 12,
    Format_r8_uint = 13,
    Format_r8_sint = 14,
    Format_r8_srgb = 15,
    Format_r8g8_unorm = 16,
    Format_r8g8_snorm = 17,
    Format_r8g8_uscaled = 18,
    Format_r8g8_sscaled = 19,
    Format_r8g8_uint = 20,
    Format_r8g8_sint = 21,
    Format_r8g8_srgb = 22,
    Format_r8g8b8_unorm = 23,
    Format_r8g8b8_snorm = 24,
    Format_r8g8b8_uscaled = 25,
    Format_r8g8b8_sscaled = 26,
    Format_r8g8b8_uint = 27,
    Format_r8g8b8_sint = 28,
    Format_r8g8b8_srgb = 29,
    Format_b8g8r8_unorm = 30,
    Format_b8g8r8_snorm = 31,
    Format_b8g8r8_uscaled = 32,
    Format_b8g8r8_sscaled = 33,
    Format_b8g8r8_uint = 34,
    Format_b8g8r8_sint = 35,
    Format_b8g8r8_srgb = 36,
    Format_r8g8b8a8_unorm = 37,
    Format_r8g8b8a8_snorm = 38,
    Format_r8g8b8a8_uscaled = 39,
    Format_r8g8b8a8_sscaled = 40,
    Format_r8g8b8a8_uint = 41,
    Format_r8g8b8a8_sint = 42,
    Format_r8g8b8a8_srgb = 43,
    Format_b8g8r8a8_unorm = 44,
    Format_b8g8r8a8_snorm = 45,
    Format_b8g8r8a8_uscaled = 46,
    Format_b8g8r8a8_sscaled = 47,
    Format_b8g8r8a8_uint = 48,
    Format_b8g8r8a8_sint = 49,
    Format_b8g8r8a8_srgb = 50,
    Format_a8b8g8r8_unorm_pack32 = 51,
    Format_a8b8g8r8_snorm_pack32 = 52,
    Format_a8b8g8r8_uscaled_pack32 = 53,
    Format_a8b8g8r8_sscaled_pack32 = 54,
    Format_a8b8g8r8_uint_pack32 = 55,
    Format_a8b8g8r8_sint_pack32 = 56,
    Format_a8b8g8r8_srgb_pack32 = 57,
    Format_a2r10g10b10_unorm_pack32 = 58,
    Format_a2r10g10b10_snorm_pack32 = 59,
    Format_a2r10g10b10_uscaled_pack32 = 60,
    Format_a2r10g10b10_sscaled_pack32 = 61,
    Format_a2r10g10b10_uint_pack32 = 62,
    Format_a2r10g10b10_sint_pack32 = 63,
    Format_a2b10g10r10_unorm_pack32 = 64,
    Format_a2b10g10r10_snorm_pack32 = 65,
    Format_a2b10g10r10_uscaled_pack32 = 66,
    Format_a2b10g10r10_sscaled_pack32 = 67,
    Format_a2b10g10r10_uint_pack32 = 68,
    Format_a2b10g10r10_sint_pack32 = 69,
    Format_r16_unorm = 70,
    Format_r16_snorm = 71,
    Format_r16_uscaled = 72,
    Format_r16_sscaled = 73,
    Format_r16_uint = 74,
    Format_r16_sint = 75,
    Format_r16_sfloat = 76,
    Format_r16g16_unorm = 77,
    Format_r16g16_snorm = 78,
    Format_r16g16_uscaled = 79,
    Format_r16g16_sscaled = 80,
    Format_r16g16_uint = 81,
    Format_r16g16_sint = 82,
    Format_r16g16_sfloat = 83,
    Format_r16g16b16_unorm = 84,
    Format_r16g16b16_snorm = 85,
    Format_r16g16b16_uscaled = 86,
    Format_r16g16b16_sscaled = 87,
    Format_r16g16b16_uint = 88,
    Format_r16g16b16_sint = 89,
    Format_r16g16b16_sfloat = 90,
    Format_r16g16b16a16_unorm = 91,
    Format_r16g16b16a16_snorm = 92,
    Format_r16g16b16a16_uscaled = 93,
    Format_r16g16b16a16_sscaled = 94,
    Format_r16g16b16a16_uint = 95,
    Format_r16g16b16a16_sint = 96,
    Format_r16g16b16a16_sfloat = 97,
    Format_r32_uint = 98,
    Format_r32_sint = 99,
    Format_r32_sfloat = 100,
    Format_r32g32_uint = 101,
    Format_r32g32_sint = 102,
    Format_r32g32_sfloat = 103,
    Format_r32g32b32_uint = 104,
    Format_r32g32b32_sint = 105,
    Format_r32g32b32_sfloat = 106,
    Format_r32g32b32a32_uint = 107,
    Format_r32g32b32a32_sint = 108,
    Format_r32g32b32a32_sfloat = 109,
    Format_r64_uint = 110,
    Format_r64_sint = 111,
    Format_r64_sfloat = 112,
    Format_r64g64_uint = 113,
    Format_r64g64_sint = 114,
    Format_r64g64_sfloat = 115,
    Format_r64g64b64_uint = 116,
    Format_r64g64b64_sint = 117,
    Format_r64g64b64_sfloat = 118,
    Format_r64g64b64a64_uint = 119,
    Format_r64g64b64a64_sint = 120,
    Format_r64g64b64a64_sfloat = 121,
    Format_b10g11r11_ufloat_pack32 = 122,
    Format_e5b9g9r9_ufloat_pack32 = 123,
    Format_d16_unorm = 124,
    Format_x8_d24_unorm_pack32 = 125,
    Format_d32_sfloat = 126,
    Format_s8_uint = 127,
    Format_d16_unorm_s8_uint = 128,
    Format_d24_unorm_s8_uint = 129,
    Format_d32_sfloat_s8_uint = 130,
    Format_bc1_rgb_unorm_block = 131,
    Format_bc1_rgb_srgb_block = 132,
    Format_bc1_rgba_unorm_block = 133,
    Format_bc1_rgba_srgb_block = 134,
    Format_bc2_unorm_block = 135,
    Format_bc2_srgb_block = 136,
    Format_bc3_unorm_block = 137,
    Format_bc3_srgb_block = 138,
    Format_bc4_unorm_block = 139,
    Format_bc4_snorm_block = 140,
    Format_bc5_unorm_block = 141,
    Format_bc5_snorm_block = 142,
    Format_bc6h_ufloat_block = 143,
    Format_bc6h_sfloat_block = 144,
    Format_bc7_unorm_block = 145,
    Format_bc7_srgb_block = 146,
    Format_etc2_r8g8b8_unorm_block = 147,
    Format_etc2_r8g8b8_srgb_block = 148,
    Format_etc2_r8g8b8a1_unorm_block = 149,
    Format_etc2_r8g8b8a1_srgb_block = 150,
    Format_etc2_r8g8b8a8_unorm_block = 151,
    Format_etc2_r8g8b8a8_srgb_block = 152,
    Format_eac_r11_unorm_block = 153,
    Format_eac_r11_snorm_block = 154,
    Format_eac_r11g11_unorm_block = 155,
    Format_eac_r11g11_snorm_block = 156,
    Format_astc_4x4_unorm_block = 157,
    Format_astc_4x4_srgb_block = 158,
    Format_astc_5x4_unorm_block = 159,
    Format_astc_5x4_srgb_block = 160,
    Format_astc_5x5_unorm_block = 161,
    Format_astc_5x5_srgb_block = 162,
    Format_astc_6x5_unorm_block = 163,
    Format_astc_6x5_srgb_block = 164,
    Format_astc_6x6_unorm_block = 165,
    Format_astc_6x6_srgb_block = 166,
    Format_astc_8x5_unorm_block = 167,
    Format_astc_8x5_srgb_block = 168,
    Format_astc_8x6_unorm_block = 169,
    Format_astc_8x6_srgb_block = 170,
    Format_astc_8x8_unorm_block = 171,
    Format_astc_8x8_srgb_block = 172,
    Format_astc_10x5_unorm_block = 173,
    Format_astc_10x5_srgb_block = 174,
    Format_astc_10x6_unorm_block = 175,
    Format_astc_10x6_srgb_block = 176,
    Format_astc_10x8_unorm_block = 177,
    Format_astc_10x8_srgb_block = 178,
    Format_astc_10x10_unorm_block = 179,
    Format_astc_10x10_srgb_block = 180,
    Format_astc_12x10_unorm_block = 181,
    Format_astc_12x10_srgb_block = 182,
    Format_astc_12x12_unorm_block = 183,
    Format_astc_12x12_srgb_block = 184,
    Format_g8b8g8r8_422_unorm = 1000156000,
    Format_b8g8r8g8_422_unorm = 1000156001,
    Format_g8_b8_r8_3plane_420_unorm = 1000156002,
    Format_g8_b8r8_2plane_420_unorm = 1000156003,
    Format_g8_b8_r8_3plane_422_unorm = 1000156004,
    Format_g8_b8r8_2plane_422_unorm = 1000156005,
    Format_g8_b8_r8_3plane_444_unorm = 1000156006,
    Format_r10x6_unorm_pack16 = 1000156007,
    Format_r10x6g10x6_unorm_2pack16 = 1000156008,
    Format_r10x6g10x6b10x6a10x6_unorm_4pack16 = 1000156009,
    Format_g10x6b10x6g10x6r10x6_422_unorm_4pack16 = 1000156010,
    Format_b10x6g10x6r10x6g10x6_422_unorm_4pack16 = 1000156011,
    Format_g10x6_b10x6_r10x6_3plane_420_unorm_3pack16 = 1000156012,
    Format_g10x6_b10x6r10x6_2plane_420_unorm_3pack16 = 1000156013,
    Format_g10x6_b10x6_r10x6_3plane_422_unorm_3pack16 = 1000156014,
    Format_g10x6_b10x6r10x6_2plane_422_unorm_3pack16 = 1000156015,
    Format_g10x6_b10x6_r10x6_3plane_444_unorm_3pack16 = 1000156016,
    Format_r12x4_unorm_pack16 = 1000156017,
    Format_r12x4g12x4_unorm_2pack16 = 1000156018,
    Format_r12x4g12x4b12x4a12x4_unorm_4pack16 = 1000156019,
    Format_g12x4b12x4g12x4r12x4_422_unorm_4pack16 = 1000156020,
    Format_b12x4g12x4r12x4g12x4_422_unorm_4pack16 = 1000156021,
    Format_g12x4_b12x4_r12x4_3plane_420_unorm_3pack16 = 1000156022,
    Format_g12x4_b12x4r12x4_2plane_420_unorm_3pack16 = 1000156023,
    Format_g12x4_b12x4_r12x4_3plane_422_unorm_3pack16 = 1000156024,
    Format_g12x4_b12x4r12x4_2plane_422_unorm_3pack16 = 1000156025,
    Format_g12x4_b12x4_r12x4_3plane_444_unorm_3pack16 = 1000156026,
    Format_g16b16g16r16_422_unorm = 1000156027,
    Format_b16g16r16g16_422_unorm = 1000156028,
    Format_g16_b16_r16_3plane_420_unorm = 1000156029,
    Format_g16_b16r16_2plane_420_unorm = 1000156030,
    Format_g16_b16_r16_3plane_422_unorm = 1000156031,
    Format_g16_b16r16_2plane_422_unorm = 1000156032,
    Format_g16_b16_r16_3plane_444_unorm = 1000156033,
    Format_g8_b8r8_2plane_444_unorm = 1000330000,
    Format_g10x6_b10x6r10x6_2plane_444_unorm_3pack16 = 1000330001,
    Format_g12x4_b12x4r12x4_2plane_444_unorm_3pack16 = 1000330002,
    Format_g16_b16r16_2plane_444_unorm = 1000330003,
    Format_a4r4g4b4_unorm_pack16 = 1000340000,
    Format_a4b4g4r4_unorm_pack16 = 1000340001,
    Format_astc_4x4_sfloat_block = 1000066000,
    Format_astc_5x4_sfloat_block = 1000066001,
    Format_astc_5x5_sfloat_block = 1000066002,
    Format_astc_6x5_sfloat_block = 1000066003,
    Format_astc_6x6_sfloat_block = 1000066004,
    Format_astc_8x5_sfloat_block = 1000066005,
    Format_astc_8x6_sfloat_block = 1000066006,
    Format_astc_8x8_sfloat_block = 1000066007,
    Format_astc_10x5_sfloat_block = 1000066008,
    Format_astc_10x6_sfloat_block = 1000066009,
    Format_astc_10x8_sfloat_block = 1000066010,
    Format_astc_10x10_sfloat_block = 1000066011,
    Format_astc_12x10_sfloat_block = 1000066012,
    Format_astc_12x12_sfloat_block = 1000066013,
    Format_a1b5g5r5_unorm_pack16 = 1000470000,
    Format_a8_unorm = 1000470001,
    Format_pvrtc1_2bpp_unorm_block_img = 1000054000,
    Format_pvrtc1_4bpp_unorm_block_img = 1000054001,
    Format_pvrtc2_2bpp_unorm_block_img = 1000054002,
    Format_pvrtc2_4bpp_unorm_block_img = 1000054003,
    Format_pvrtc1_2bpp_srgb_block_img = 1000054004,
    Format_pvrtc1_4bpp_srgb_block_img = 1000054005,
    Format_pvrtc2_2bpp_srgb_block_img = 1000054006,
    Format_pvrtc2_4bpp_srgb_block_img = 1000054007,
    Format_r8_bool_arm = 1000460000,
    Format_r16g16_sfixed5_nv = 1000464000,
    Format_r10x6_uint_pack16_arm = 1000609000,
    Format_r10x6g10x6_uint_2pack16_arm = 1000609001,
    Format_r10x6g10x6b10x6a10x6_uint_4pack16_arm = 1000609002,
    Format_r12x4_uint_pack16_arm = 1000609003,
    Format_r12x4g12x4_uint_2pack16_arm = 1000609004,
    Format_r12x4g12x4b12x4a12x4_uint_4pack16_arm = 1000609005,
    Format_r14x2_uint_pack16_arm = 1000609006,
    Format_r14x2g14x2_uint_2pack16_arm = 1000609007,
    Format_r14x2g14x2b14x2a14x2_uint_4pack16_arm = 1000609008,
    Format_r14x2_unorm_pack16_arm = 1000609009,
    Format_r14x2g14x2_unorm_2pack16_arm = 1000609010,
    Format_r14x2g14x2b14x2a14x2_unorm_4pack16_arm = 1000609011,
    Format_g14x2_b14x2r14x2_2plane_420_unorm_3pack16_arm = 1000609012,
    Format_g14x2_b14x2r14x2_2plane_422_unorm_3pack16_arm = 1000609013,
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
 * Where in the pipeline a resource can be accessed
 */
enum GpuStage : u32 {
    GpuStage_none = 0,
    GpuStage_topOfPipe = 0x00000001,
    GpuStage_drawIndirect = 0x00000002,
    GpuStage_vertexInput = 0x00000004,
    GpuStage_vertexShader = 0x00000008,
    GpuStage_tessellationControlShader = 0x00000010,
    GpuStage_tessellationEvaluationShader = 0x00000020,
    GpuStage_geometryShader = 0x00000040,
    GpuStage_fragmentShader = 0x00000080,
    GpuStage_earlyFragmentTests = 0x00000100,
    GpuStage_lateFragmentTests = 0x00000200,
    GpuStage_colorAttachmentOutput = 0x00000400,
    GpuStage_computeShader = 0x00000800,
    GpuStage_transfer = 0x00001000,
    GpuStage_bottomOfPipe = 0x00002000,
    GpuStage_host = 0x00004000,
    GpuStage_allGraphics = 0x00008000,
    GpuStage_allCommands = 0x00010000,
};
typedef u32 GpuStageFlags;

/**
 * How a resource can be accessed
 */
enum GpuAccess : u32 {
    GpuAccess_none = 0,
    GpuAccess_indirectCommandRead = 0x00000001,
    GpuAccess_indexRead = 0x00000002,
    GpuAccess_vertexAttributeRead = 0x00000004,
    GpuAccess_uniformRead = 0x00000008,
    GpuAccess_inputAttachmentRead = 0x00000010,
    GpuAccess_shaderRead = 0x00000020,
    GpuAccess_shaderWrite = 0x00000040,
    GpuAccess_colorAttachmentRead = 0x00000080,
    GpuAccess_colorAttachmentWrite = 0x00000100,
    GpuAccess_depthStencilAttachmentRead = 0x00000200,
    GpuAccess_depthStencilAttachmentWrite = 0x00000400,
    GpuAccess_transferRead = 0x00000800,
    GpuAccess_transferWrite = 0x00001000,
    GpuAccess_hostRead = 0x00002000,
    GpuAccess_hostWrite = 0x00004000,
    GpuAccess_memoryRead = 0x00008000,
    GpuAccess_memoryWrite = 0x00010000,
};
typedef u32 GpuAccessFlags;

/**
 * A gpu buffer
 */
struct GpuBuffer;

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
typedef u32 GpuBufferUsageFlags;

/**
 * How a gpu buffer will be accessed
 */
enum GpuMemoryUsage : u32 {
    /**
     * It will only be accessed from the device
     */
    GpuMemoryUsage_deviceOnly = 0,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    GpuMemoryUsage_stagingWrite = 1,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    GpuMemoryUsage_stagingRead = 2,
    /**
     * It will be frequently written from the host and read on the device
     */
    GpuMemoryUsage_frequentUpdate = 3,
};

/**
 * How a gpu buffer can be accessed
 */
enum GpuMemoryHostAccess : u32 {
    /**
     * The buffer cannot be accessed by the host
     */
    GpuMemoryHostAccess_none = 0x0,
    /**
     * The buffer can be written to by the host
     */
    GpuMemoryHostAccess_write = 0x1,
    /**
     * The buffer can be read from by the host
     */
    GpuMemoryHostAccess_read = 0x2,
};

/**
 * Create a gpu buffer
 *
 * Parameters
 * - size The size in bytes of the buffer
 * - usageFlags How the buffer will be used
 * - access How the buffer should be accessed
 */
GpuBuffer* gpuBufferCreate(
    u64 size,
    GpuBufferUsageFlags usageFlags,
    GpuMemoryUsage access = GpuMemoryUsage_deviceOnly);

/**
 * Destroy a gpu buffer
 */
void gpuBufferDestroy(GpuBuffer* buffer);

/**
 * Get the uniform buffer descriptor index from the buffer
 */
u32 gpuBufferUniformDescriptor(GpuBuffer* buffer);

/**
 * Get the storage buffer descriptor index from the buffer
 */
u32 gpuBufferStorageDescriptor(GpuBuffer* buffer);

/**
 * Writes to a gpu buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void gpuBufferWrite(GpuBuffer* dst, u64 offset, const void* src, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void gpuBufferRead(void* dst, GpuBuffer* src, u64 offset, u64 size);

/**
 * A gpu image
 */
struct GpuImage;

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
typedef u32 GpuImageUsageFlags;

/**
 * The layout of an image
 */
enum GpuLayout : u32 {
    GpuLayout_undefined = 0,
    GpuLayout_general = 1,
    GpuLayout_colorAttachment = 2,
    GpuLayout_depthStencilAttachment = 3,
    GpuLayout_depthStencilReadOnly = 4,
    GpuLayout_shaderReadOnly = 5,
    GpuLayout_transferSrc = 6,
    GpuLayout_transferDst = 7,
    GpuLayout_preinitialized = 8,
    GpuLayout_presentSrc = 1000001002,
};

/**
 * Extra config flags for image creation
 */
enum GpuImageConfig : u32 {
    GpuImageConfig_cubeCompatible = 0x00000010,
};
typedef u32 GpuImageConfigFlags;

/**
 * Create a gpu image assuming most defaults
 */
GpuImage* gpuImageCreate(u32 width, u32 height, Format format, GpuImageUsageFlags usage);

/**
 * Config for gpuImageCreateEx
 */
struct GpuImageCreateEx {
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
 * Create a gpu image with extended options
 */
GpuImage* gpuImageCreateEx(const GpuImageCreateEx* create);

/**
 * Destroy a gpu image
 */
void gpuImageDestroy(GpuImage* image);

/**
 * Get the width of an image
 */
u32 gpuImageWidth(GpuImage* image);

/**
 * Get the height of an image
 */
u32 gpuImageHeight(GpuImage* image);

/**
 * A gpu view
 */
struct GpuView;

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
typedef u32 GpuAspectFlags;

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
 * Create a gpu image view
 */
GpuView* gpuViewCreate(
    GpuImage* image,
    GpuAspectFlags aspectFlags,
    GpuFilter filter = GpuFilter_nearest);

/**
 * Config for gpuViewCreateEx
 */
struct GpuViewCreateEx {
    GpuImage* image;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
    GpuAspectFlags aspectFlags;
    GpuViewType type;
    GpuFilter filter;
    GpuSamplerEdgeMode edgeMode;
    GpuSamplerBorder border;
};

/**
 * Create a gpu image view with extended config
 */
GpuView* gpuViewCreateEx(const GpuViewCreateEx* config);

/**
 * Destroy a gpu image view
 */
void gpuViewDestroy(GpuView* view);

/**
 * Get the width of an image
 */
u32 gpuViewWidth(GpuView* view);

/**
 * Get the height of an image
 */
u32 gpuViewHeight(GpuView* view);

/**
 * Get the image sampler descriptor index from the image view
 */
u32 gpuImageSamplerDescriptor(GpuView* view);

/**
 * Get the storage image descriptor index from the image view
 */
u32 gpuImageStorageDescriptor(GpuView* view);

/**
 * Write to a gpu image
 *
 * Note, only fills the base mip level
 *
 * Parameters
 * - dst The image to write to
 * - src The data to read from
 */
void gpuImageWrite(GpuView* dst, const void* src);

/**
 * Write to a gpu image cubemap
 *
 * Note, dst must have at least 6 array layers to fill
 *
 * Only the base mip level is filled
 *
 * srcData is assumed to be layed out as:
 *  #
 * ####
 *  #
 *
 * Parameters
 * - dst The image to write to
 * - subresource The subresource of the image to write to
 * - src The data to read from
 */
void gpuImageWriteCubemap(GpuView* dst, const void* src);

/**
 * Read from a gpu image
 *
 * Note, only the base mip level is read
 *
 * Parameters
 * - src The pointer to write to
 * - dst The image to read from
 * - subresource The subresource of the image to read from
 */
void gpuImageRead(void* dst, GpuView* src);

/**
 * Generates mipmaps from the base level
 *
 * Note, dst should only have 1 array layer
 *
 * Parameters
 * - image The image to generate mipmaps for
 */
void gpuImageGenMipmaps(GpuView* dst);

/**
 * A gpu pipeline
 */
struct GpuPipeline;

/**
 * A push constant range in a pipeline
 */
struct GpuPushRange {
    /**
     * The offset in bytes
     */
    u32 offset;
    /**
     * The size of the push in bytes
     */
    u32 size;
};

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
typedef u32 GpuCullFlags;

/**
 * Config for createGraphicsPipeline
 */
struct CreateGpuGraphicsPipeline {
    /**
     * The vertex shader code
     */
    const void* vertexShader = nullptr;
    /**
     * The size in bytes of the vertex shader code
     */
    u64 vertexShaderSize = 0;
    /**
     * The fragment shader code
     */
    const void* fragmentShader = nullptr;
    /**
     * The size in bytes of the fragment shader code
     */
    u64 fragmentShaderSize = 0;
    /**
     * The size of the push constant
     */
    u32 pushConstantSize = 0;
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    const Format* colorAttachmentFormats = nullptr;
    /**
     * The number of color attachment formats
     */
    u32 colorAttachmentCount = 0;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    Format depthAttachmentFormat = Format_undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
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
     */
    const bool* colorBlendEnables = nullptr;
};

/**
 * Create a graphics pipeline
 *
 * Parameters
 * - config The pipeline configuration
 */
GpuPipeline* gpuPipelineCreateGraphics(const CreateGpuGraphicsPipeline* config);

/**
 * Create a compute pipeline
 *
 * Parameters
 * - pushSize The size of the push constant
 * - shaderCode The compute shader, must not be nullptr
 * - shaderCodeSize The size in bytes of shaderCode
 */
GpuPipeline* gpuPipelineCreateCompute(u32 pushSize, const u8* shaderCode, u64 shaderCodeSize);

/**
 * Destroy a graphics or compute pipeline
 */
void gpuPipelineDestroy(GpuPipeline* pipeline);

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
void gpuBindPipeline(GpuCmd* cmd, GpuPipeline* pipeline);

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
void gpuPushConstants(GpuCmd* cmd, GpuPipeline* pipeline, void* push, u32 size);

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
 * An image dependency barrier
 */
struct GpuImageBarrier {
    /**
     * The image to sychronize
     */
    GpuView* image;
    /**
     * Where the image will be used next
     */
    GpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    GpuAccessFlags nextAccess;
    /**
     * The next layout the image needs to be in
     */
    GpuLayout nextLayout;
};

/**
 * A buffer dependency barrier
 */
struct GpuBufferBarrier {
    /**
     * The buffer to sychronize
     */
    GpuBuffer* buffer;
    /**
     * Where the image will be used next
     */
    GpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    GpuAccessFlags nextAccess;
};

/**
 * Creates a barrier for resource uses that are not part of a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - bufferBarriers The buffer barriers
 * - bufferBarrierCount The number of buffer barriers
 * - imageBarriers The image barriers
 * - imageBarrierCount The number of image barriers
 */
void gpuMemoryBarrier(
    GpuCmd* cmd,
    const GpuBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const GpuImageBarrier* imageBarriers,
    u32 imageBarrierCount);

/**
 * A compute pass description
 */
struct GpuComputePass {
    /**
     * The uniforms buffer dependencies
     */
    GpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    GpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    GpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    GpuView* storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
};

/**
 * Performs memory barriers for compute shader resources
 *
 * Parameters
 * - cmd The command buffer
 * - pass The compute pass description
 */
void gpuComputePass(GpuCmd* cmd, const GpuComputePass* pass);

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
 * The value to clear color attachments to
 */
union GpuClearValueVolor {
    /**
     * The value as f32
     */
    f32 float32[4];
    /**
     * The value as i32
     */
    i32 int32[4];
    /**
     * The value as u32
     */
    u32 uint32[4];
};

/**
 * The value to clear depth and stencil attachments to
 */
struct GpuClearValueDepthStencil {
    /**
     * The depth value
     */
    f32 depth;
    /**
     * The stencil value
     */
    u32 stencil;
};

/**
 * The value to clear a render attachment to
 */
union GpuClearValue {
    /**
     * The value for color attachments
     */
    GpuClearValueVolor color;
    /**
     * The value for depth and stencil attachments
     */
    GpuClearValueDepthStencil depthStencil;
};

/**
 * A rendering attachment
 */
struct GpuRenderAttachment {
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
 * A render pass description
 */
struct GpuRenderPass {
    /**
     * The uniforms buffer dependencies
     */
    GpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    GpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    GpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    GpuView* storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
    /**
     * The color images to write to
     */
    const GpuRenderAttachment* colorAttachments = nullptr;
    /**
     * The number of color attachments
     */
    u32 colorAttachmentCount = 0;
    /**
     * The number of layers in each color attachment to write to
     */
    u32 layerCount = 1;
    /**
     * The depth attachment, if any
     */
    const GpuRenderAttachment* depthAttachment = nullptr;
    /**
     * The stencil attachment, if any
     */
    const GpuRenderAttachment* stencilAttachment = nullptr;
};

/**
 * Performs render barrier and begins a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - pass The render pass description
 */
void gpuRenderPassBegin(GpuCmd* cmd, const GpuRenderPass* pass);

/**
 * Ends the render pass
 *
 * Parameters
 * - cmd The command buffer
 */
void gpuRenderPassEnd(GpuCmd* cmd);

/**
 * Set the rendering viewport, should be called after gpuRenderPassBegin
 */
void gpuSetViewport(GpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f);

/**
 * Set the rendering scissor, should be called after gpuRenderPassBegin
 */
void gpuSetScissor(GpuCmd* cmd, i32 x, i32 y, u32 width, u32 height);

} // namespace hg

#endif // GPU_HPP
