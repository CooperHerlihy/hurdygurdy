#ifndef HG_GRAPHICS_H
#define HG_GRAPHICS_H

#include "hg_utils.h"
#include "hg_graphics_enums.h"

void hg_graphics_init(void);
void hg_graphics_shutdown(void);
void hg_graphics_wait(void);

typedef struct HgWindowConfig {
    const char* title;
    u32 width;
    u32 height;
    bool windowed;
} HgWindowConfig;

void hg_window_open(const HgWindowConfig* config);
void hg_window_close(void);

void hg_window_update_size(void);
void hg_window_get_size(u32* width, u32* height);

typedef struct HgBuffer HgBuffer;

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

typedef struct HgTextureConfig {
    u32 width;
    u32 height;
    u32 depth;
    u32 array_layers;
    u32 mip_levels;
    HgFormat format;
    HgTextureAspectFlags aspect;
    HgTextureUsageFlags usage;
    HgSamplerEdgeMode edge_mode;
    bool bilinear_filter;
    bool make_cubemap;
} HgTextureConfig;

HgTexture* hg_texture_create(const HgTextureConfig* config);
void hg_texture_destroy(HgTexture* texture);

void hg_texture_write(HgTexture* dst, const void* src, HgTextureLayout layout);
void hg_texture_read(HgTexture* src, void* dst, HgTextureLayout layout);

u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth);
void hg_texture_generate_mipmaps(HgTexture* texture, HgTextureLayout layout);

typedef struct HgShader HgShader;

typedef struct HgVertexAttribute {
    HgFormat format;
    u32 offset;
} HgVertexAttribute;

typedef struct HgVertexBinding {
    HgVertexAttribute* attributes;
    u32 attribute_count;
    u32 stride;
    HgVertexInputRate input_rate;
} HgVertexBinding;

typedef struct HgDescriptorSetBinding {
    HgDescriptorType descriptor_type;
    u32 descriptor_count;
} HgDescriptorSetBinding;

typedef struct HgDescriptorSet {
    HgDescriptorSetBinding* bindings;
    u32 binding_count;
} HgDescriptorSet;

typedef struct HgShaderConfig {
    HgFormat color_format;
    HgFormat depth_format;

    const byte* spirv_vertex_shader;
    const byte* spirv_fragment_shader;
    u32 vertex_shader_size;
    u32 fragment_shader_size;

    HgVertexBinding* vertex_bindings;
    HgDescriptorSet* descriptor_sets;
    u32 vertex_binding_count;
    u32 descriptor_set_count;

    u32 push_constant_size;

    HgPrimitiveTopology topology;
    HgCullModeFlagBits cull_mode;
    bool enable_color_blend;
    bool enable_depth_buffer;
} HgShaderConfig;

HgShader* hg_shader_create(const HgShaderConfig* config);
void hg_shader_destroy(HgShader* shader);

bool hg_commands_begin(void);
bool hg_commands_end(void);

void hg_renderpass_begin(HgTexture* target, HgTexture* depth_buffer);
void hg_renderpass_end(void);

void hg_shader_bind(HgShader* shader);
void hg_shader_unbind(void);

typedef struct HgDescriptor {
    HgDescriptorType type;
    u32 count;
    union {
        HgBuffer** buffers;
        HgTexture** textures;
    };
} HgDescriptor;

void hg_bind_descriptor_set(u32 set_index, HgDescriptor* descriptors, u32 descriptor_count);

void hg_draw_indexed(HgBuffer* vertex_buffer, HgBuffer* index_buffer, void* push_data, u32 push_size);
void hg_draw(HgBuffer* vertex_buffer, u32 count, void* push_data, u32 push_size);

#endif // HG_GRAPHICS_H
