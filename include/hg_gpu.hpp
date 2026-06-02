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

#ifndef HG_GPU_HPP
#define HG_GPU_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_containers.hpp"

/**
 * Gpu init config
 */
struct HgGpuInit {
    /**
     * The maximum gpu buffers
     */
    u32 maxBuffers;
    /**
     * The maximum gpu images
     */
    u32 maxImages;
    /**
     * The maximum gpu views
     */
    u32 maxViews;
    /**
     * The maximum gpu pipelines
     */
    u32 maxPipelines;
    /**
     * The maximum frames rendered concurrently, should be 2
     */
    u32 maxFramesInFlight;
    /**
     * The maximum OS windows, same as in platform init
     */
    u32 maxWindows;
};

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 */
void hgGpuInit(HgArena* arena, HgGpuInit* config);

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void hgGpuDeinit();

/**
 * Wait for the GPU to finish work
 */
void hgGpuWaitIdle();

/**
 * Pixel formats
 */
enum HgFormat : u32 {
    HgFormat_undefined = 0,
    HgFormat_r4g4_unorm_pack8 = 1,
    HgFormat_r4g4b4a4_unorm_pack16 = 2,
    HgFormat_b4g4r4a4_unorm_pack16 = 3,
    HgFormat_r5g6b5_unorm_pack16 = 4,
    HgFormat_b5g6r5_unorm_pack16 = 5,
    HgFormat_r5g5b5a1_unorm_pack16 = 6,
    HgFormat_b5g5r5a1_unorm_pack16 = 7,
    HgFormat_a1r5g5b5_unorm_pack16 = 8,
    HgFormat_r8_unorm = 9,
    HgFormat_r8_snorm = 10,
    HgFormat_r8_uscaled = 11,
    HgFormat_r8_sscaled = 12,
    HgFormat_r8_uint = 13,
    HgFormat_r8_sint = 14,
    HgFormat_r8_srgb = 15,
    HgFormat_r8g8_unorm = 16,
    HgFormat_r8g8_snorm = 17,
    HgFormat_r8g8_uscaled = 18,
    HgFormat_r8g8_sscaled = 19,
    HgFormat_r8g8_uint = 20,
    HgFormat_r8g8_sint = 21,
    HgFormat_r8g8_srgb = 22,
    HgFormat_r8g8b8_unorm = 23,
    HgFormat_r8g8b8_snorm = 24,
    HgFormat_r8g8b8_uscaled = 25,
    HgFormat_r8g8b8_sscaled = 26,
    HgFormat_r8g8b8_uint = 27,
    HgFormat_r8g8b8_sint = 28,
    HgFormat_r8g8b8_srgb = 29,
    HgFormat_b8g8r8_unorm = 30,
    HgFormat_b8g8r8_snorm = 31,
    HgFormat_b8g8r8_uscaled = 32,
    HgFormat_b8g8r8_sscaled = 33,
    HgFormat_b8g8r8_uint = 34,
    HgFormat_b8g8r8_sint = 35,
    HgFormat_b8g8r8_srgb = 36,
    HgFormat_r8g8b8a8_unorm = 37,
    HgFormat_r8g8b8a8_snorm = 38,
    HgFormat_r8g8b8a8_uscaled = 39,
    HgFormat_r8g8b8a8_sscaled = 40,
    HgFormat_r8g8b8a8_uint = 41,
    HgFormat_r8g8b8a8_sint = 42,
    HgFormat_r8g8b8a8_srgb = 43,
    HgFormat_b8g8r8a8_unorm = 44,
    HgFormat_b8g8r8a8_snorm = 45,
    HgFormat_b8g8r8a8_uscaled = 46,
    HgFormat_b8g8r8a8_sscaled = 47,
    HgFormat_b8g8r8a8_uint = 48,
    HgFormat_b8g8r8a8_sint = 49,
    HgFormat_b8g8r8a8_srgb = 50,
    HgFormat_a8b8g8r8_unorm_pack32 = 51,
    HgFormat_a8b8g8r8_snorm_pack32 = 52,
    HgFormat_a8b8g8r8_uscaled_pack32 = 53,
    HgFormat_a8b8g8r8_sscaled_pack32 = 54,
    HgFormat_a8b8g8r8_uint_pack32 = 55,
    HgFormat_a8b8g8r8_sint_pack32 = 56,
    HgFormat_a8b8g8r8_srgb_pack32 = 57,
    HgFormat_a2r10g10b10_unorm_pack32 = 58,
    HgFormat_a2r10g10b10_snorm_pack32 = 59,
    HgFormat_a2r10g10b10_uscaled_pack32 = 60,
    HgFormat_a2r10g10b10_sscaled_pack32 = 61,
    HgFormat_a2r10g10b10_uint_pack32 = 62,
    HgFormat_a2r10g10b10_sint_pack32 = 63,
    HgFormat_a2b10g10r10_unorm_pack32 = 64,
    HgFormat_a2b10g10r10_snorm_pack32 = 65,
    HgFormat_a2b10g10r10_uscaled_pack32 = 66,
    HgFormat_a2b10g10r10_sscaled_pack32 = 67,
    HgFormat_a2b10g10r10_uint_pack32 = 68,
    HgFormat_a2b10g10r10_sint_pack32 = 69,
    HgFormat_r16_unorm = 70,
    HgFormat_r16_snorm = 71,
    HgFormat_r16_uscaled = 72,
    HgFormat_r16_sscaled = 73,
    HgFormat_r16_uint = 74,
    HgFormat_r16_sint = 75,
    HgFormat_r16_sfloat = 76,
    HgFormat_r16g16_unorm = 77,
    HgFormat_r16g16_snorm = 78,
    HgFormat_r16g16_uscaled = 79,
    HgFormat_r16g16_sscaled = 80,
    HgFormat_r16g16_uint = 81,
    HgFormat_r16g16_sint = 82,
    HgFormat_r16g16_sfloat = 83,
    HgFormat_r16g16b16_unorm = 84,
    HgFormat_r16g16b16_snorm = 85,
    HgFormat_r16g16b16_uscaled = 86,
    HgFormat_r16g16b16_sscaled = 87,
    HgFormat_r16g16b16_uint = 88,
    HgFormat_r16g16b16_sint = 89,
    HgFormat_r16g16b16_sfloat = 90,
    HgFormat_r16g16b16a16_unorm = 91,
    HgFormat_r16g16b16a16_snorm = 92,
    HgFormat_r16g16b16a16_uscaled = 93,
    HgFormat_r16g16b16a16_sscaled = 94,
    HgFormat_r16g16b16a16_uint = 95,
    HgFormat_r16g16b16a16_sint = 96,
    HgFormat_r16g16b16a16_sfloat = 97,
    HgFormat_r32_uint = 98,
    HgFormat_r32_sint = 99,
    HgFormat_r32_sfloat = 100,
    HgFormat_r32g32_uint = 101,
    HgFormat_r32g32_sint = 102,
    HgFormat_r32g32_sfloat = 103,
    HgFormat_r32g32b32_uint = 104,
    HgFormat_r32g32b32_sint = 105,
    HgFormat_r32g32b32_sfloat = 106,
    HgFormat_r32g32b32a32_uint = 107,
    HgFormat_r32g32b32a32_sint = 108,
    HgFormat_r32g32b32a32_sfloat = 109,
    HgFormat_r64_uint = 110,
    HgFormat_r64_sint = 111,
    HgFormat_r64_sfloat = 112,
    HgFormat_r64g64_uint = 113,
    HgFormat_r64g64_sint = 114,
    HgFormat_r64g64_sfloat = 115,
    HgFormat_r64g64b64_uint = 116,
    HgFormat_r64g64b64_sint = 117,
    HgFormat_r64g64b64_sfloat = 118,
    HgFormat_r64g64b64a64_uint = 119,
    HgFormat_r64g64b64a64_sint = 120,
    HgFormat_r64g64b64a64_sfloat = 121,
    HgFormat_b10g11r11_ufloat_pack32 = 122,
    HgFormat_e5b9g9r9_ufloat_pack32 = 123,
    HgFormat_d16_unorm = 124,
    HgFormat_x8_d24_unorm_pack32 = 125,
    HgFormat_d32_sfloat = 126,
    HgFormat_s8_uint = 127,
    HgFormat_d16_unorm_s8_uint = 128,
    HgFormat_d24_unorm_s8_uint = 129,
    HgFormat_d32_sfloat_s8_uint = 130,
    HgFormat_bc1_rgb_unorm_block = 131,
    HgFormat_bc1_rgb_srgb_block = 132,
    HgFormat_bc1_rgba_unorm_block = 133,
    HgFormat_bc1_rgba_srgb_block = 134,
    HgFormat_bc2_unorm_block = 135,
    HgFormat_bc2_srgb_block = 136,
    HgFormat_bc3_unorm_block = 137,
    HgFormat_bc3_srgb_block = 138,
    HgFormat_bc4_unorm_block = 139,
    HgFormat_bc4_snorm_block = 140,
    HgFormat_bc5_unorm_block = 141,
    HgFormat_bc5_snorm_block = 142,
    HgFormat_bc6h_ufloat_block = 143,
    HgFormat_bc6h_sfloat_block = 144,
    HgFormat_bc7_unorm_block = 145,
    HgFormat_bc7_srgb_block = 146,
    HgFormat_etc2_r8g8b8_unorm_block = 147,
    HgFormat_etc2_r8g8b8_srgb_block = 148,
    HgFormat_etc2_r8g8b8a1_unorm_block = 149,
    HgFormat_etc2_r8g8b8a1_srgb_block = 150,
    HgFormat_etc2_r8g8b8a8_unorm_block = 151,
    HgFormat_etc2_r8g8b8a8_srgb_block = 152,
    HgFormat_eac_r11_unorm_block = 153,
    HgFormat_eac_r11_snorm_block = 154,
    HgFormat_eac_r11g11_unorm_block = 155,
    HgFormat_eac_r11g11_snorm_block = 156,
    HgFormat_astc_4x4_unorm_block = 157,
    HgFormat_astc_4x4_srgb_block = 158,
    HgFormat_astc_5x4_unorm_block = 159,
    HgFormat_astc_5x4_srgb_block = 160,
    HgFormat_astc_5x5_unorm_block = 161,
    HgFormat_astc_5x5_srgb_block = 162,
    HgFormat_astc_6x5_unorm_block = 163,
    HgFormat_astc_6x5_srgb_block = 164,
    HgFormat_astc_6x6_unorm_block = 165,
    HgFormat_astc_6x6_srgb_block = 166,
    HgFormat_astc_8x5_unorm_block = 167,
    HgFormat_astc_8x5_srgb_block = 168,
    HgFormat_astc_8x6_unorm_block = 169,
    HgFormat_astc_8x6_srgb_block = 170,
    HgFormat_astc_8x8_unorm_block = 171,
    HgFormat_astc_8x8_srgb_block = 172,
    HgFormat_astc_10x5_unorm_block = 173,
    HgFormat_astc_10x5_srgb_block = 174,
    HgFormat_astc_10x6_unorm_block = 175,
    HgFormat_astc_10x6_srgb_block = 176,
    HgFormat_astc_10x8_unorm_block = 177,
    HgFormat_astc_10x8_srgb_block = 178,
    HgFormat_astc_10x10_unorm_block = 179,
    HgFormat_astc_10x10_srgb_block = 180,
    HgFormat_astc_12x10_unorm_block = 181,
    HgFormat_astc_12x10_srgb_block = 182,
    HgFormat_astc_12x12_unorm_block = 183,
    HgFormat_astc_12x12_srgb_block = 184,
    HgFormat_g8b8g8r8_422_unorm = 1000156000,
    HgFormat_b8g8r8g8_422_unorm = 1000156001,
    HgFormat_g8_b8_r8_3plane_420_unorm = 1000156002,
    HgFormat_g8_b8r8_2plane_420_unorm = 1000156003,
    HgFormat_g8_b8_r8_3plane_422_unorm = 1000156004,
    HgFormat_g8_b8r8_2plane_422_unorm = 1000156005,
    HgFormat_g8_b8_r8_3plane_444_unorm = 1000156006,
    HgFormat_r10x6_unorm_pack16 = 1000156007,
    HgFormat_r10x6g10x6_unorm_2pack16 = 1000156008,
    HgFormat_r10x6g10x6b10x6a10x6_unorm_4pack16 = 1000156009,
    HgFormat_g10x6b10x6g10x6r10x6_422_unorm_4pack16 = 1000156010,
    HgFormat_b10x6g10x6r10x6g10x6_422_unorm_4pack16 = 1000156011,
    HgFormat_g10x6_b10x6_r10x6_3plane_420_unorm_3pack16 = 1000156012,
    HgFormat_g10x6_b10x6r10x6_2plane_420_unorm_3pack16 = 1000156013,
    HgFormat_g10x6_b10x6_r10x6_3plane_422_unorm_3pack16 = 1000156014,
    HgFormat_g10x6_b10x6r10x6_2plane_422_unorm_3pack16 = 1000156015,
    HgFormat_g10x6_b10x6_r10x6_3plane_444_unorm_3pack16 = 1000156016,
    HgFormat_r12x4_unorm_pack16 = 1000156017,
    HgFormat_r12x4g12x4_unorm_2pack16 = 1000156018,
    HgFormat_r12x4g12x4b12x4a12x4_unorm_4pack16 = 1000156019,
    HgFormat_g12x4b12x4g12x4r12x4_422_unorm_4pack16 = 1000156020,
    HgFormat_b12x4g12x4r12x4g12x4_422_unorm_4pack16 = 1000156021,
    HgFormat_g12x4_b12x4_r12x4_3plane_420_unorm_3pack16 = 1000156022,
    HgFormat_g12x4_b12x4r12x4_2plane_420_unorm_3pack16 = 1000156023,
    HgFormat_g12x4_b12x4_r12x4_3plane_422_unorm_3pack16 = 1000156024,
    HgFormat_g12x4_b12x4r12x4_2plane_422_unorm_3pack16 = 1000156025,
    HgFormat_g12x4_b12x4_r12x4_3plane_444_unorm_3pack16 = 1000156026,
    HgFormat_g16b16g16r16_422_unorm = 1000156027,
    HgFormat_b16g16r16g16_422_unorm = 1000156028,
    HgFormat_g16_b16_r16_3plane_420_unorm = 1000156029,
    HgFormat_g16_b16r16_2plane_420_unorm = 1000156030,
    HgFormat_g16_b16_r16_3plane_422_unorm = 1000156031,
    HgFormat_g16_b16r16_2plane_422_unorm = 1000156032,
    HgFormat_g16_b16_r16_3plane_444_unorm = 1000156033,
    HgFormat_g8_b8r8_2plane_444_unorm = 1000330000,
    HgFormat_g10x6_b10x6r10x6_2plane_444_unorm_3pack16 = 1000330001,
    HgFormat_g12x4_b12x4r12x4_2plane_444_unorm_3pack16 = 1000330002,
    HgFormat_g16_b16r16_2plane_444_unorm = 1000330003,
    HgFormat_a4r4g4b4_unorm_pack16 = 1000340000,
    HgFormat_a4b4g4r4_unorm_pack16 = 1000340001,
    HgFormat_astc_4x4_sfloat_block = 1000066000,
    HgFormat_astc_5x4_sfloat_block = 1000066001,
    HgFormat_astc_5x5_sfloat_block = 1000066002,
    HgFormat_astc_6x5_sfloat_block = 1000066003,
    HgFormat_astc_6x6_sfloat_block = 1000066004,
    HgFormat_astc_8x5_sfloat_block = 1000066005,
    HgFormat_astc_8x6_sfloat_block = 1000066006,
    HgFormat_astc_8x8_sfloat_block = 1000066007,
    HgFormat_astc_10x5_sfloat_block = 1000066008,
    HgFormat_astc_10x6_sfloat_block = 1000066009,
    HgFormat_astc_10x8_sfloat_block = 1000066010,
    HgFormat_astc_10x10_sfloat_block = 1000066011,
    HgFormat_astc_12x10_sfloat_block = 1000066012,
    HgFormat_astc_12x12_sfloat_block = 1000066013,
    HgFormat_a1b5g5r5_unorm_pack16 = 1000470000,
    HgFormat_a8_unorm = 1000470001,
    HgFormat_pvrtc1_2bpp_unorm_block_img = 1000054000,
    HgFormat_pvrtc1_4bpp_unorm_block_img = 1000054001,
    HgFormat_pvrtc2_2bpp_unorm_block_img = 1000054002,
    HgFormat_pvrtc2_4bpp_unorm_block_img = 1000054003,
    HgFormat_pvrtc1_2bpp_srgb_block_img = 1000054004,
    HgFormat_pvrtc1_4bpp_srgb_block_img = 1000054005,
    HgFormat_pvrtc2_2bpp_srgb_block_img = 1000054006,
    HgFormat_pvrtc2_4bpp_srgb_block_img = 1000054007,
    HgFormat_r8_bool_arm = 1000460000,
    HgFormat_r16g16_sfixed5_nv = 1000464000,
    HgFormat_r10x6_uint_pack16_arm = 1000609000,
    HgFormat_r10x6g10x6_uint_2pack16_arm = 1000609001,
    HgFormat_r10x6g10x6b10x6a10x6_uint_4pack16_arm = 1000609002,
    HgFormat_r12x4_uint_pack16_arm = 1000609003,
    HgFormat_r12x4g12x4_uint_2pack16_arm = 1000609004,
    HgFormat_r12x4g12x4b12x4a12x4_uint_4pack16_arm = 1000609005,
    HgFormat_r14x2_uint_pack16_arm = 1000609006,
    HgFormat_r14x2g14x2_uint_2pack16_arm = 1000609007,
    HgFormat_r14x2g14x2b14x2a14x2_uint_4pack16_arm = 1000609008,
    HgFormat_r14x2_unorm_pack16_arm = 1000609009,
    HgFormat_r14x2g14x2_unorm_2pack16_arm = 1000609010,
    HgFormat_r14x2g14x2b14x2a14x2_unorm_4pack16_arm = 1000609011,
    HgFormat_g14x2_b14x2r14x2_2plane_420_unorm_3pack16_arm = 1000609012,
    HgFormat_g14x2_b14x2r14x2_2plane_422_unorm_3pack16_arm = 1000609013,
};

/**
 * Turns a HgFormat into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 hgFormatToSize(HgFormat format);

// Vulkan allocator : TODO?

/**
 * Where in the pipeline a resource can be accessed
 */
enum HgGpuStage : u32 {
    HgGpuStage_none = 0,
    HgGpuStage_topOfPipe = 0x00000001,
    HgGpuStage_drawIndirect = 0x00000002,
    HgGpuStage_vertexInput = 0x00000004,
    HgGpuStage_vertexShader = 0x00000008,
    HgGpuStage_tessellationControlShader = 0x00000010,
    HgGpuStage_tessellationEvaluationShader = 0x00000020,
    HgGpuStage_geometryShader = 0x00000040,
    HgGpuStage_fragmentShader = 0x00000080,
    HgGpuStage_earlyFragmentTests = 0x00000100,
    HgGpuStage_lateFragmentTests = 0x00000200,
    HgGpuStage_colorAttachmentOutput = 0x00000400,
    HgGpuStage_computeShader = 0x00000800,
    HgGpuStage_transfer = 0x00001000,
    HgGpuStage_bottomOfPipe = 0x00002000,
    HgGpuStage_host = 0x00004000,
    HgGpuStage_allGraphics = 0x00008000,
    HgGpuStage_allCommands = 0x00010000,
};
typedef u32 HgGpuStageFlags;

/**
 * How a resource can be accessed
 */
enum HgGpuAccess : u32 {
    HgGpuAccess_none = 0,
    HgGpuAccess_indirectCommandRead = 0x00000001,
    HgGpuAccess_indexRead = 0x00000002,
    HgGpuAccess_vertexAttributeRead = 0x00000004,
    HgGpuAccess_uniformRead = 0x00000008,
    HgGpuAccess_inputAttachmentRead = 0x00000010,
    HgGpuAccess_shaderRead = 0x00000020,
    HgGpuAccess_shaderWrite = 0x00000040,
    HgGpuAccess_colorAttachmentRead = 0x00000080,
    HgGpuAccess_colorAttachmentWrite = 0x00000100,
    HgGpuAccess_depthStencilAttachmentRead = 0x00000200,
    HgGpuAccess_depthStencilAttachmentWrite = 0x00000400,
    HgGpuAccess_transferRead = 0x00000800,
    HgGpuAccess_transferWrite = 0x00001000,
    HgGpuAccess_hostRead = 0x00002000,
    HgGpuAccess_hostWrite = 0x00004000,
    HgGpuAccess_memoryRead = 0x00008000,
    HgGpuAccess_memoryWrite = 0x00010000,
};
typedef u32 HgGpuAccessFlags;

/**
 * A gpu buffer
 */
struct HgGpuBuffer {
    HgHandle handle;
};

/**
 * How a gpu buffer will be used
 */
enum HgGpuBufferUsage : u32 {
    HgGpuBufferUsage_transferSrc = 0x00000001,
    HgGpuBufferUsage_transferDst = 0x00000002,
    HgGpuBufferUsage_uniformTexelBuffer = 0x00000004,
    HgGpuBufferUsage_storageTexelBuffer = 0x00000008,
    HgGpuBufferUsage_uniformBuffer = 0x00000010,
    HgGpuBufferUsage_storageBuffer = 0x00000020,
    HgGpuBufferUsage_indirectBuffer = 0x00000100,
};
typedef u32 HgGpuBufferUsageFlags;

/**
 * How a gpu buffer will be accessed
 */
enum HgGpuMemoryUsage : u32 {
    /**
     * It will only be accessed from the device
     */
    HgGpuMemoryUsage_deviceOnly = 0,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    HgGpuMemoryUsage_stagingWrite = 1,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    HgGpuMemoryUsage_stagingRead = 2,
    /**
     * It will be frequently written from the host and read on the device
     */
    HgGpuMemoryUsage_frequentUpdate = 3,
};

/**
 * How a gpu buffer can be accessed
 */
enum HgGpuMemoryHostAccess : u32 {
    /**
     * The buffer cannot be accessed by the host
     */
    HgGpuMemoryHostAccess_none = 0x0,
    /**
     * The buffer can be written to by the host
     */
    HgGpuMemoryHostAccess_write = 0x1,
    /**
     * The buffer can be read from by the host
     */
    HgGpuMemoryHostAccess_read = 0x2,
};

/**
 * Create a gpu buffer
 *
 * Parameters
 * - size The size in bytes of the buffer
 * - usageFlags How the buffer will be used
 * - access How the buffer should be accessed
 */
HgGpuBuffer hgGpuBufferCreate(
    u64 size,
    HgGpuBufferUsageFlags usageFlags,
    HgGpuMemoryUsage access = HgGpuMemoryUsage_deviceOnly);

/**
 * Destroy a gpu buffer
 */
void hgGpuBufferDestroy(HgGpuBuffer buffer);

/**
 * Get the uniform buffer descriptor index from the buffer
 */
u32 hgGpuBufferUniformDescriptor(HgGpuBuffer buffer);

/**
 * Get the storage buffer descriptor index from the buffer
 */
u32 hgGpuBufferStorageDescriptor(HgGpuBuffer buffer);

/**
 * Writes to a gpu buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void hgGpuBufferWrite(HgGpuBuffer dst, u64 offset, const void* src, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void hgGpuBufferRead(void* dst, HgGpuBuffer src, u64 offset, u64 size);

/**
 * A gpu image
 */
struct HgGpuImage {
    HgHandle handle;
};

/**
 * How an image will be used
 */
enum HgGpuImageUsage : u32 {
    HgGpuImageUsage_transferSrc = 0x00000001,
    HgGpuImageUsage_transferDst = 0x00000002,
    HgGpuImageUsage_sampled = 0x00000004,
    HgGpuImageUsage_storage = 0x00000008,
    HgGpuImageUsage_colorAttachment = 0x00000010,
    HgGpuImageUsage_depthStencilAttachment = 0x00000020,
    HgGpuImageUsage_transientAttachment = 0x00000040,
    HgGpuImageUsage_inputAttachment = 0x00000080,
    HgGpuImageUsage_hostTransfer = 0x00400000,
};
typedef u32 HgGpuImageUsageFlags;

/**
 * The layout of an image
 */
enum HgGpuLayout : u32 {
    HgGpuLayout_undefined = 0,
    HgGpuLayout_general = 1,
    HgGpuLayout_colorAttachment = 2,
    HgGpuLayout_depthStencilAttachment = 3,
    HgGpuLayout_depthStencilReadOnly = 4,
    HgGpuLayout_shaderReadOnly = 5,
    HgGpuLayout_transferSrc = 6,
    HgGpuLayout_transferDst = 7,
    HgGpuLayout_preinitialized = 8,
    HgGpuLayout_presentSrc = 1000001002,
};

/**
 * Extra config flags for image creation
 */
enum HgGpuImageConfig : u32 {
    HgGpuImageConfig_cubeCompatible = 0x00000010,
};
typedef u32 HgGpuImageConfigFlags;

/**
 * Create a gpu image assuming most defaults
 */
HgGpuImage hgGpuImageCreate(u32 width, u32 height, HgFormat format, HgGpuImageUsageFlags usage);

/**
 * Config for hgGpuImageCreateEx
 */
struct HgGpuImageCreateEx {
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
    HgFormat format = HgFormat_undefined;
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
    HgGpuImageUsageFlags usage = 0;
    /**
     * Extra config flags
     */
    HgGpuImageConfigFlags flags = 0;
};

/**
 * Create a gpu image with extended options
 */
HgGpuImage hgGpuImageCreateEx(const HgGpuImageCreateEx* create);

/**
 * Destroy a gpu image
 */
void hgGpuImageDestroy(HgGpuImage image);

/**
 * A gpu view
 */
struct HgGpuView {
    HgHandle handle;
};

/**
 * The dimensionality of an image
 */
enum HgGpuViewType : u32 {
    HgGpuViewType_1D = 0,
    HgGpuViewType_2D = 1,
    HgGpuViewType_3D = 2,
    HgGpuViewType_cube = 3,
    HgGpuViewType_1DArray = 4,
    HgGpuViewType_2DArray = 5,
    HgGpuViewType_cubeArray = 6,
};

/**
 * The aspect the image will be accessed in
 */
enum HgGpuAspect : u32 {
    HgGpuAspect_none = 0,
    HgGpuAspect_color = 0x00000001,
    HgGpuAspect_depth = 0x00000002,
    HgGpuAspect_stencil = 0x00000004,
    HgGpuAspect_metadata = 0x00000008,
    HgGpuAspect_plane0 = 0x00000010,
    HgGpuAspect_plane1 = 0x00000020,
    HgGpuAspect_plane2 = 0x00000040,
};
typedef u32 HgGpuAspectFlags;

/**
 * How a sampler interpolates between pixels
 */
enum HgGpuFilter : u32 {
    HgGpuFilter_nearest = 0,
    HgGpuFilter_linear = 1,
    HgGpuFilter_count,
};

/**
 * How a sampler samples off the image's edge
 */
enum HgGpuSamplerEdgeMode : u32 {
    HgGpuSamplerEdgeMode_repeat = 0,
    HgGpuSamplerEdgeMode_mirroredRepeat = 1,
    HgGpuSamplerEdgeMode_clampToEdge = 2,
    HgGpuSamplerEdgeMode_clampToBorder = 3,
    HgGpuSamplerEdgeMode_mirrorClampToEdge = 4,
    HgGpuSamplerEdgeMode_count,
};

/**
 * The border color if the sampler edge mode has a border
 */
enum HgGpuSamplerBorder : u32 {
    HgGpuSamplerBorder_floatTransparentBlack = 0,
    HgGpuSamplerBorder_intTransparentBlack = 1,
    HgGpuSamplerBorder_floatOpaqueBlack = 2,
    HgGpuSamplerBorder_intOpaqueBlack = 3,
    HgGpuSamplerBorder_floatOpaqueWhite = 4,
    HgGpuSamplerBorder_intOpaqueWhite = 5,
    HgGpuSamplerBorder_count,
};

/**
 * Create a gpu image view
 */
HgGpuView hgGpuViewCreate(
    HgGpuImage image,
    HgGpuAspectFlags aspectFlags,
    HgGpuFilter filter = HgGpuFilter_nearest);

/**
 * Config for hgGpuViewCreateEx
 */
struct HgGpuViewCreateEx {
    HgGpuImage image;
    u32 baseMipLevel;
    u32 levelCount;
    u32 baseArrayLayer;
    u32 layerCount;
    HgGpuAspectFlags aspectFlags;
    HgGpuViewType type;
    HgGpuFilter filter;
    HgGpuSamplerEdgeMode edgeMode;
    HgGpuSamplerBorder border;
};

/**
 * Create a gpu image view with extended config
 */
HgGpuView hgGpuViewCreateEx(const HgGpuViewCreateEx* config);

/**
 * Destroy a gpu image view
 */
void hgGpuViewDestroy(HgGpuView view);

/**
 * Get the image sampler descriptor index from the image view
 */
u32 hgGpuImageSamplerDescriptor(HgGpuView view);

/**
 * Get the storage image descriptor index from the image view
 */
u32 hgGpuImageStorageDescriptor(HgGpuView view);

/**
 * Write to a gpu image
 *
 * Note, only fills the base mip level
 *
 * Parameters
 * - dst The image to write to
 * - src The data to read from
 */
void hgGpuImageWrite(HgGpuView dst, const void* src);

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
void hgGpuImageWriteCubemap(HgGpuView dst, const void* src);

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
void hgGpuImageRead(void* dst, HgGpuView src);

/**
 * Generates mipmaps from the base level
 *
 * Note, dst should only have 1 array layer
 *
 * Parameters
 * - image The image to generate mipmaps for
 */
void hgGpuImageGenMipmaps(HgGpuView dst);

/**
 * A gpu pipeline
 */
struct HgGpuPipeline {
    HgHandle handle;
};

/**
 * A push constant range in a pipeline
 */
struct HgGpuPushRange {
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
enum HgGpuTopology : u32 {
    HgGpuTopology_pointList = 0,
    HgGpuTopology_lineList = 1,
    HgGpuTopology_lineStrip = 2,
    HgGpuTopology_triangleList = 3,
    HgGpuTopology_triangleStrip = 4,
    HgGpuTopology_triangleFan = 5,
    HgGpuTopology_lineListWithAdjacency = 6,
    HgGpuTopology_lineStripWithAdjacency = 7,
    HgGpuTopology_triangleListWithAdjacency = 8,
    HgGpuTopology_triangleStripWithAdjacency = 9,
    HgGpuTopology_patchList = 10,
};

/**
 * How to treat vertices
 */
enum HgGpuPolygonMode : u32 {
    HgGpuPolygonMode_fill = 0,
    HgGpuPolygonMode_line = 1,
    HgGpuPolygonMode_point = 2,
};

enum HgGpuCull : u32 {
    HgGpuCull_none = 0,
    HgGpuCull_front = 0x00000001,
    HgGpuCull_back = 0x00000002,
    HgGpuCull_both = 0x00000003,
};
typedef u32 HgGpuCullFlags;

/**
 * Config for hgCreateGraphicsPipeline
 */
struct HgCreateGpuGraphicsPipeline {
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
     * The format of the color attachments, none can be UNDEFINED
     */
    const HgFormat* colorAttachmentFormats = nullptr;
    /**
     * The number of color attachment formats
     */
    u32 colorAttachmentCount = 0;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    HgFormat depthAttachmentFormat = HgFormat_undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     */
    HgFormat stencilAttachmentFormat = HgFormat_undefined;
    /**
     * The push constant ranges, if any
     */
    const HgGpuPushRange* pushRanges = nullptr;
    /**
     * The number of push constant ranges
     */
    u32 pushRangeCount;
    /**
     * How to interpret vertices into topology
     */
    HgGpuTopology topology = HgGpuTopology_triangleList;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    HgGpuPolygonMode polygonMode = HgGpuPolygonMode_fill;
    /**
     * Enables back/front face culling
     */
    HgGpuCullFlags cullMode = HgGpuCull_none;
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
HgGpuPipeline hgGpuPipelineCreateGraphics(const HgCreateGpuGraphicsPipeline* config);

/**
 * Create a compute pipeline
 *
 * Parameters
 * - pushSize The size of the push constant
 * - shaderCode The compute shader, must not be nullptr
 * - shaderCodeSize The size in bytes of shaderCode
 */
HgGpuPipeline hgGpuPipelineCreateCompute(u32 pushSize, const u8* shaderCode, u64 shaderCodeSize);

/**
 * Destroy a graphics or compute pipeline
 */
void hgGpuPipelineDestroy(HgGpuPipeline pipeline);

/**
 * A gpu command buffer
 */
struct HgGpuCmd;

/**
 * Begin a command buffer to be executed once
 *
 * Returns
 * - The command buffer to record, will never be nullptr
 */
HgGpuCmd* hgGpuCmdBegin();

/**
 * Execute the command buffer and wait for completion
 *
 * Parameters
 * - cmd The command buffer from hgBeginGpuCommands, must not be nullptr
 */
void hgGpuCmdEnd(HgGpuCmd* cmd);

/**
 * Bind a graphics or compute pipeline
 */
void hgGpuBindPipeline(HgGpuCmd* cmd, HgGpuPipeline pipeline);

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
void hgGpuPushConstants(HgGpuCmd* cmd, HgGpuPipeline pipeline, u32 offset, void* push, u32 size);

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
void hgGpuDraw(HgGpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount);

/**
 * Dispatch a compute shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - groupCountX The number of workgroups in the x dimension
 * - groupCountY The number of workgroups in the y dimension
 * - groupCountZ The number of workgroups in the z dimension
 */
void hgGpuDispatch(HgGpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ);

/**
 * An image dependency barrier
 */
struct HgGpuImageBarrier {
    /**
     * The image to sychronize
     */
    HgGpuView image;
    /**
     * Where the image will be used next
     */
    HgGpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    HgGpuAccessFlags nextAccess;
    /**
     * The next layout the image needs to be in
     */
    HgGpuLayout nextLayout;
};

/**
 * A buffer dependency barrier
 */
struct HgGpuBufferBarrier {
    /**
     * The buffer to sychronize
     */
    HgGpuBuffer buffer;
    /**
     * Where the image will be used next
     */
    HgGpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    HgGpuAccessFlags nextAccess;
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
void hgGpuMemoryBarrier(
    HgGpuCmd* cmd,
    const HgGpuBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const HgGpuImageBarrier* imageBarriers,
    u32 imageBarrierCount);

/**
 * A compute pass description
 */
struct HgGpuComputePass {
    /**
     * The uniforms buffer dependencies
     */
    HgGpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    HgGpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    HgGpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    HgGpuView* storageImages = nullptr;
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
void hgGpuComputePass(HgGpuCmd* cmd, const HgGpuComputePass* pass);

/**
 * The operation to load a render attachment
 */
enum HgGpuLoadOp : u32 {
    HgGpuLoadOp_load = 0,
    HgGpuLoadOp_clear = 1,
    HgGpuLoadOp_dontCare = 2,
};

/**
 * The operation to store a render attachment
 */
enum HgGpuStoreOp : u32 {
    HgGpuStoreOp_store = 0,
    HgGpuStoreOp_dontCare = 1,
};

/**
 * The value to clear color attachments to
 */
union HgGpuClearValueVolor {
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
struct HgGpuClearValueDepthStencil {
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
union HgGpuClearValue {
    /**
     * The value for color attachments
     */
    HgGpuClearValueVolor color;
    /**
     * The value for depth and stencil attachments
     */
    HgGpuClearValueDepthStencil depthStencil;
};

/**
 * A rendering attachment
 */
struct HgGpuRenderAttachment {
    /**
     * The image attached, must not be nullptr
     */
    HgGpuView image{};
    /**
     * How the image will be loaded
     */
    HgGpuLoadOp loadOp = HgGpuLoadOp_clear;
    /**
     * How the image will be stored
     */
    HgGpuStoreOp storeOp = HgGpuStoreOp_store;
    /**
     * What to clear the image to if cleared
     */
    HgGpuClearValue clearValue = {};
};

/**
 * A render pass description
 */
struct HgGpuRenderPass {
    /**
     * The uniforms buffer dependencies
     */
    HgGpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    HgGpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    HgGpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    HgGpuView* storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
    /**
     * The color images to write to
     */
    const HgGpuRenderAttachment* colorAttachments = nullptr;
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
    const HgGpuRenderAttachment* depthAttachment = nullptr;
    /**
     * The stencil attachment, if any
     */
    const HgGpuRenderAttachment* stencilAttachment = nullptr;
};

/**
 * Performs render barrier and begins a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - width The width of the render area
 * - height The height of the render area
 * - pass The render pass description
 */
void hgGpuRenderPassBegin(HgGpuCmd* cmd, u32 width, u32 height, const HgGpuRenderPass* pass);

/**
 * Ends the render pass
 *
 * Parameters
 * - cmd The command buffer
 */
void hgGpuRenderPassEnd(HgGpuCmd* cmd);

#endif // HG_GPU_HPP
