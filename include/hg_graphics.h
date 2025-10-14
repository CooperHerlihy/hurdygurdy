#ifndef HG_GRAPHICS_H
#define HG_GRAPHICS_H

#include "hg_utils.h"

HgError hg_init_graphics(void);
void hg_shutdown_graphics(void);
void hg_wait_graphics(void);

typedef struct HgWindow HgWindow;

typedef struct HgWindowConfig {
    const char* title;
    u32 width;
    u32 height;
    bool windowed;
} HgWindowConfig;

HgWindow* hg_window_create(const HgWindowConfig* config);
void hg_window_destroy(HgWindow* window);
void hg_window_update_size(HgWindow* window);
void hg_window_get_size(HgWindow* window, u32* width, u32* height);

typedef enum HgGpuMemoryType {
    HG_GPU_MEMORY_TYPE_DEVICE_LOCAL = 0,
    HG_GPU_MEMORY_TYPE_RANDOM_ACCESS,
    HG_GPU_MEMORY_TYPE_LINEAR_ACCESS,
} HgGpuMemoryType;

typedef struct HgBuffer HgBuffer;

typedef enum HgBufferUsageFlags {
    HG_BUFFER_USAGE_NONE = 0x0,
    HG_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x1,
    HG_BUFFER_USAGE_TRANSFER_DST_BIT = 0x2,
    HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x4,
    HG_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x8,
    HG_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x10,
} HgBufferUsageFlags;

typedef struct HgBufferConfig {
    usize size;
    HgBufferUsageFlags usage;
    HgGpuMemoryType memory_type;
} HgBufferConfig;

HgBuffer* hg_buffer_create(const HgBufferConfig* config);
void hg_buffer_destroy(HgBuffer* buffer);

void hg_buffer_write(HgBuffer* dst, usize offset, const void* src, usize size);
void hg_buffer_read(const HgBuffer* src, usize offset, usize size, void* dst);

typedef struct HgTexture HgTexture;

typedef enum HgFormat {
    HG_FORMAT_UNDEFINED = 0,
    HG_FORMAT_R4G4_UNORM_PACK8 = 1,
    HG_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
    HG_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
    HG_FORMAT_R5G6B5_UNORM_PACK16 = 4,
    HG_FORMAT_B5G6R5_UNORM_PACK16 = 5,
    HG_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
    HG_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
    HG_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
    HG_FORMAT_R8_UNORM = 9,
    HG_FORMAT_R8_SNORM = 10,
    HG_FORMAT_R8_USCALED = 11,
    HG_FORMAT_R8_SSCALED = 12,
    HG_FORMAT_R8_UINT = 13,
    HG_FORMAT_R8_SINT = 14,
    HG_FORMAT_R8_SRGB = 15,
    HG_FORMAT_R8G8_UNORM = 16,
    HG_FORMAT_R8G8_SNORM = 17,
    HG_FORMAT_R8G8_USCALED = 18,
    HG_FORMAT_R8G8_SSCALED = 19,
    HG_FORMAT_R8G8_UINT = 20,
    HG_FORMAT_R8G8_SINT = 21,
    HG_FORMAT_R8G8_SRGB = 22,
    HG_FORMAT_R8G8B8_UNORM = 23,
    HG_FORMAT_R8G8B8_SNORM = 24,
    HG_FORMAT_R8G8B8_USCALED = 25,
    HG_FORMAT_R8G8B8_SSCALED = 26,
    HG_FORMAT_R8G8B8_UINT = 27,
    HG_FORMAT_R8G8B8_SINT = 28,
    HG_FORMAT_R8G8B8_SRGB = 29,
    HG_FORMAT_B8G8R8_UNORM = 30,
    HG_FORMAT_B8G8R8_SNORM = 31,
    HG_FORMAT_B8G8R8_USCALED = 32,
    HG_FORMAT_B8G8R8_SSCALED = 33,
    HG_FORMAT_B8G8R8_UINT = 34,
    HG_FORMAT_B8G8R8_SINT = 35,
    HG_FORMAT_B8G8R8_SRGB = 36,
    HG_FORMAT_R8G8B8A8_UNORM = 37,
    HG_FORMAT_R8G8B8A8_SNORM = 38,
    HG_FORMAT_R8G8B8A8_USCALED = 39,
    HG_FORMAT_R8G8B8A8_SSCALED = 40,
    HG_FORMAT_R8G8B8A8_UINT = 41,
    HG_FORMAT_R8G8B8A8_SINT = 42,
    HG_FORMAT_R8G8B8A8_SRGB = 43,
    HG_FORMAT_B8G8R8A8_UNORM = 44,
    HG_FORMAT_B8G8R8A8_SNORM = 45,
    HG_FORMAT_B8G8R8A8_USCALED = 46,
    HG_FORMAT_B8G8R8A8_SSCALED = 47,
    HG_FORMAT_B8G8R8A8_UINT = 48,
    HG_FORMAT_B8G8R8A8_SINT = 49,
    HG_FORMAT_B8G8R8A8_SRGB = 50,
    HG_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
    HG_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
    HG_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
    HG_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
    HG_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
    HG_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
    HG_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
    HG_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
    HG_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
    HG_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
    HG_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
    HG_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
    HG_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
    HG_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
    HG_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
    HG_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
    HG_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
    HG_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
    HG_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
    HG_FORMAT_R16_UNORM = 70,
    HG_FORMAT_R16_SNORM = 71,
    HG_FORMAT_R16_USCALED = 72,
    HG_FORMAT_R16_SSCALED = 73,
    HG_FORMAT_R16_UINT = 74,
    HG_FORMAT_R16_SINT = 75,
    HG_FORMAT_R16_SFLOAT = 76,
    HG_FORMAT_R16G16_UNORM = 77,
    HG_FORMAT_R16G16_SNORM = 78,
    HG_FORMAT_R16G16_USCALED = 79,
    HG_FORMAT_R16G16_SSCALED = 80,
    HG_FORMAT_R16G16_UINT = 81,
    HG_FORMAT_R16G16_SINT = 82,
    HG_FORMAT_R16G16_SFLOAT = 83,
    HG_FORMAT_R16G16B16_UNORM = 84,
    HG_FORMAT_R16G16B16_SNORM = 85,
    HG_FORMAT_R16G16B16_USCALED = 86,
    HG_FORMAT_R16G16B16_SSCALED = 87,
    HG_FORMAT_R16G16B16_UINT = 88,
    HG_FORMAT_R16G16B16_SINT = 89,
    HG_FORMAT_R16G16B16_SFLOAT = 90,
    HG_FORMAT_R16G16B16A16_UNORM = 91,
    HG_FORMAT_R16G16B16A16_SNORM = 92,
    HG_FORMAT_R16G16B16A16_USCALED = 93,
    HG_FORMAT_R16G16B16A16_SSCALED = 94,
    HG_FORMAT_R16G16B16A16_UINT = 95,
    HG_FORMAT_R16G16B16A16_SINT = 96,
    HG_FORMAT_R16G16B16A16_SFLOAT = 97,
    HG_FORMAT_R32_UINT = 98,
    HG_FORMAT_R32_SINT = 99,
    HG_FORMAT_R32_SFLOAT = 100,
    HG_FORMAT_R32G32_UINT = 101,
    HG_FORMAT_R32G32_SINT = 102,
    HG_FORMAT_R32G32_SFLOAT = 103,
    HG_FORMAT_R32G32B32_UINT = 104,
    HG_FORMAT_R32G32B32_SINT = 105,
    HG_FORMAT_R32G32B32_SFLOAT = 106,
    HG_FORMAT_R32G32B32A32_UINT = 107,
    HG_FORMAT_R32G32B32A32_SINT = 108,
    HG_FORMAT_R32G32B32A32_SFLOAT = 109,
    HG_FORMAT_R64_UINT = 110,
    HG_FORMAT_R64_SINT = 111,
    HG_FORMAT_R64_SFLOAT = 112,
    HG_FORMAT_R64G64_UINT = 113,
    HG_FORMAT_R64G64_SINT = 114,
    HG_FORMAT_R64G64_SFLOAT = 115,
    HG_FORMAT_R64G64B64_UINT = 116,
    HG_FORMAT_R64G64B64_SINT = 117,
    HG_FORMAT_R64G64B64_SFLOAT = 118,
    HG_FORMAT_R64G64B64A64_UINT = 119,
    HG_FORMAT_R64G64B64A64_SINT = 120,
    HG_FORMAT_R64G64B64A64_SFLOAT = 121,
    HG_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
    HG_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
    HG_FORMAT_D16_UNORM = 124,
    HG_FORMAT_X8_D24_UNORM_PACK32 = 125,
    HG_FORMAT_D32_SFLOAT = 126,
    HG_FORMAT_S8_UINT = 127,
    HG_FORMAT_D16_UNORM_S8_UINT = 128,
    HG_FORMAT_D24_UNORM_S8_UINT = 129,
    HG_FORMAT_D32_SFLOAT_S8_UINT = 130,
} HgFormat;

typedef enum HgTextureAspectFlags {
    HG_TEXTURE_ASPECT_NONE = 0x0,
    HG_TEXTURE_ASPECT_COLOR_BIT = 0x1,
    HG_TEXTURE_ASPECT_DEPTH_BIT = 0x2,
} HgTextureAspectFlags;

typedef enum HgTextureUsageFlags {
    HG_TEXTURE_USAGE_NONE = 0x0,
    HG_TEXTURE_USAGE_TRANSFER_SRC_BIT = 0x1,
    HG_TEXTURE_USAGE_TRANSFER_DST_BIT = 0x2,
    HG_TEXTURE_USAGE_SAMPLED_BIT = 0x4,
    HG_TEXTURE_USAGE_RENDER_TARGET_BIT = 0x10,
    HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT = 0x20,
} HgTextureUsageFlags;

typedef enum HgTextureLayout {
    HG_IMAGE_LAYOUT_UNDEFINED = 0,
    HG_IMAGE_LAYOUT_GENERAL = 1,
    HG_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    HG_IMAGE_LAYOUT_DEPTH_BUFFER_OPTIMAL = 3,
    HG_IMAGE_LAYOUT_DEPTH_BUFFER_READ_ONLY_OPTIMAL = 4,
    HG_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    HG_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
    HG_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
} HgTextureLayout;

typedef struct HgTextureConfig {
    u32 width;
    u32 height;
    u32 depth;
    u32 array_layers;
    u32 mip_levels;
    HgFormat format;
    HgTextureAspectFlags aspect;
    HgTextureUsageFlags usage;
    bool make_cubemap;
} HgTextureConfig;

HgTexture* hg_texture_create(const HgTextureConfig* config);
void hg_texture_destroy(HgTexture* texture);

void hg_texture_write(HgTexture* dst, const void* src, HgTextureLayout layout);
u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth);
void hg_texture_generate_mipmaps(HgTexture* texture, HgTextureLayout layout);

typedef struct HgSampler HgSampler;

typedef enum HgSamplerEdgeMode {
    HG_SAMPLER_EDGE_MODE_REPEAT = 0,
    HG_SAMPLER_EDGE_MODE_MIRRORED_REPEAT = 1,
    HG_SAMPLER_EDGE_MODE_CLAMP_TO_EDGE = 2,
    HG_SAMPLER_EDGE_MODE_CLAMP_TO_BORDER = 3,
    HG_SAMPLER_EDGE_MODE_MIRROR_CLAMP_TO_EDGE = 4,
} HgSamplerEdgeMode;

typedef struct HgSamplerConfig {
    bool bilinear_filter;
    HgSamplerEdgeMode edge_mode;
    u32 mip_levels;
} HgSamplerConfig;

HgSampler* hg_sampler_create(const HgSamplerConfig* config);
void hg_sampler_destroy(HgSampler* sampler);

typedef struct HgShader HgShader;

typedef struct HgVertexInputBindingDescription {
    u32 binding;
    u32 stride;
    bool input_rate_instance;
} HgVertexInputBindingDescription;

typedef struct HgVertexInputAttributeDescription {
    u32 location;
    u32 binding;
    HgFormat format;
    u32 offset;
} HgVertexInputAttributeDescription;

typedef enum HgDescriptorType {
    HG_DESCRIPTOR_TYPE_BUFFER = 0,
    HG_DESCRIPTOR_TYPE_TEXTURE,
    HG_DESCRIPTOR_TYPE_SAMPLER,
} HgDescriptorType;

typedef struct HgDescriptorSetBinding {
    u32 set;
    u32 binding;
    HgDescriptorType descriptor_type;
    u32 descriptor_count;
} HgDescriptorSetBinding;

typedef enum HgShaderStageFlags {
    HG_SHADER_STAGE_NONE = 0,
    HG_SHADER_STAGE_VERTEX_BIT = 0x1,
    // HG_SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x2,
    // HG_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x4,
    // HG_SHADER_STAGE_GEOMETRY_BIT = 0x8,
    HG_SHADER_STAGE_FRAGMENT_BIT = 0x10,
    HG_SHADER_STAGE_COMPUTE_BIT = 0x20,
    HG_SHADER_STAGE_ALL_GRAPHICS = 0x1F,
} HgShaderStageFlags;

typedef struct HgPushConstantRange {
    HgShaderStageFlags stage_flags;
    u32 offset;
    u32 size;
} HgPushConstantRange;

typedef enum HgPrimitiveTopology {
    HG_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    HG_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    HG_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    HG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    HG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
} HgPrimitiveTopology;

typedef enum HgCullModeFlagBits {
    HG_CULL_MODE_NONE = 0,
    HG_CULL_MODE_FRONT_BIT = 0x1,
    HG_CULL_MODE_BACK_BIT = 0x2,
    HG_CULL_MODE_FRONT_AND_BACK = 0x3,
} HgCullModeFlagBits;

typedef struct HgShaderConfig {
    HgFormat color_format;
    HgFormat depth_format;

    const byte* spirv_vertex_shader;
    const byte* spirv_fragment_shader;
    u32 vertex_shader_size;
    u32 fragment_shader_size;

    HgVertexInputBindingDescription* vertex_bindings;
    HgVertexInputAttributeDescription* vertex_attributes;
    u32 vertex_binding_count;
    u32 vertex_attribute_count;

    HgDescriptorSetBinding* descriptor_set_bindings;
    HgPushConstantRange* push_constants;
    u32 descriptor_binding_count;
    u32 push_constant_count;

    HgPrimitiveTopology topology;
    HgCullModeFlagBits cull_mode;
    u32 MSAA_sample_count;
    bool enable_depth_buffer;
    bool enable_color_blend;
} HgShaderConfig;

HgShader* hg_shader_create(const HgShaderConfig* config);
void hg_shader_destroy(HgShader* shader);

typedef struct HgDescriptorSet HgDescriptorSet;

HgDescriptorSet* hg_allocate_descriptor_set(HgShader* shader, u32 set_index);
// free descriptor sets : TODO

void hg_write_descriptor_set(
    HgDescriptorSet* descriptor_set,
    u32 binding,
    u32 array_index,
    HgDescriptorType type,
    HgBuffer* buffer,
    HgTexture* texture,
    HgSampler* sampler
);

typedef struct HgObject {
    HgBuffer* vertex_buffer;
    HgBuffer* index_buffer;
    HgDescriptorSet** descriptor_sets;
    void* push_data;
    u32 descriptor_set_count;
    u32 push_size;
} HgObject;

typedef struct HgPipeline {
    HgShader* shader;
    HgObject* objects;
    HgDescriptorSet** global_descriptor_sets;
    u32 global_descriptor_set_count;
    u32 object_count;
} HgPipeline;

typedef struct HgRenderPass {
    HgTexture* target;
    HgTexture* depth_buffer;
    HgPipeline* pipelines;
    u32 pipeline_count;
} HgRenderPass;

typedef struct HgRenderDescription {
    HgRenderPass* render_passes;
    u32 render_pass_count;
} HgRenderDescription;

bool hg_draw(HgWindow* window, const HgRenderDescription* scene);

#endif // HG_GRAPHICS_H
