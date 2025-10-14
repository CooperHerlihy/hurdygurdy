#ifndef HG_GRAPHICS_H
#define HG_GRAPHICS_H

#include "hg_utils.h"

#include <vulkan/vulkan.h>

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

typedef struct HgBufferConfig {
    usize size;
    VkBufferUsageFlags usage;
    HgGpuMemoryType memory_type;
} HgBufferConfig;

HgBuffer* hg_buffer_create(const HgBufferConfig* config);
void hg_buffer_destroy(HgBuffer* buffer);

void hg_buffer_write(HgBuffer* dst, usize offset, const void* src, usize size);
void hg_buffer_read(const HgBuffer* src, usize offset, usize size, void* dst);

typedef struct HgTexture HgTexture;

typedef struct HgTextureConfig {
    u32 width;
    u32 height;
    u32 depth;
    u32 array_layers;
    u32 mip_levels;
    VkFormat format;
    VkImageAspectFlags aspect;
    VkImageUsageFlags usage;
    bool make_cubemap;
} HgTextureConfig;

HgTexture* hg_texture_create(const HgTextureConfig* config);
void hg_texture_destroy(HgTexture* texture);

void hg_texture_write(HgTexture* dst, const void* src, VkImageLayout layout);
u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth);
void hg_texture_generate_mipmaps(HgTexture* texture, VkImageLayout layout);

typedef struct HgSampler HgSampler;

typedef struct HgSamplerConfig {
    VkFilter type;
    VkSamplerAddressMode edge_mode;
    u32 mip_levels;
} HgSamplerConfig;

HgSampler* hg_sampler_create(const HgSamplerConfig* config);
void hg_sampler_destroy(HgSampler* sampler);

typedef struct HgShader HgShader;

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

typedef struct HgShaderConfig {
    VkPipelineCache cache;

    VkFormat color_format;
    VkFormat depth_format;
    VkFormat stencil_format;

    const byte* spirv_vertex_shader;
    const byte* spirv_fragment_shader;
    u32 vertex_shader_size;
    u32 fragment_shader_size;

    VkVertexInputBindingDescription* vertex_bindings;
    VkVertexInputAttributeDescription* vertex_attributes;
    u32 vertex_binding_count;
    u32 vertex_attribute_count;

    HgDescriptorSetBinding* descriptor_set_bindings;
    VkPushConstantRange* push_constants;
    u32 descriptor_binding_count;
    u32 push_constant_count;

    VkPrimitiveTopology topology;
    VkCullModeFlagBits cull_mode;
    VkSampleCountFlagBits MSAA;
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
