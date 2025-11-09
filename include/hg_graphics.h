#ifndef HG_GRAPHICS_H
#define HG_GRAPHICS_H

#include "hg_enums.h"
#include "hg_utils.h"
#include "hg_init.h"

/**
 * Waits for the graphics subsystem
 *
 * Can be called to ensure that resources are not being used before destruction
 *
 * \param hg The HurdyGurdy context, must not be NULL
 */
void hg_graphics_wait(
    const HurdyGurdy* hg
);

/**
 * A buffer on the GPU
 */
typedef struct HgBuffer HgBuffer;

/**
 * Configuration for an HgBuffer
 *
 * size is the size of the buffer in bytes, must be greater than 0
 * usage lists how the buffer will be used, must not be HG_BUFFER_USAGE_NONE
 * memory_type is how to store memory, defaults to HG_MEMORY_TYPE_DEVICE_LOCAL
 */
typedef struct HgBufferConfig {
    usize size;
    HgBufferUsageFlags usage;
    HgGpuMemoryType memory_type;
} HgBufferConfig;

/**
 * Creates an HgBuffer
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param config The configuration for the buffer
 * \return The created buffer, is never NULL
 */
HgBuffer* hg_buffer_create(
    const HurdyGurdy* hg,
    const HgBufferConfig* config
);

/**
 * Destroys an HgBuffer
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param buffer The buffer to destroy, must not be NULL
 */
void hg_buffer_destroy(
    const HurdyGurdy* hg,
    HgBuffer* buffer
);

/**
 * Writes data to an HgBuffer
 *
 * If the buffer memory type is HG_GPU_MEMORY_TYPE_LINEAR_ACCESS, the offset
 * must be 0
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param dst The buffer to write to, must not be NULL
 * \param offset The offset into the dst buffer
 * \param src The data to write, must not be NULL
 * \param size The size of the data to read, may be 0 to do nothing, or SIZE_MAX
 * to copy the size of the buffer
 */
void hg_buffer_write(
    const HurdyGurdy* hg,
    HgBuffer* dst,
    usize offset,
    const void* src,
    usize size
);

/**
 * Reads data from an HgBuffer
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param dst The data to read, must not be NULL
 * \param src The buffer to read from, must not be NULL
 * \param offset The offset into the src buffer
 * \param size The size of the data to read, may be 0 to do nothing, or SIZE_MAX
 * to copy the size of the buffer
 */
void hg_buffer_read(
    const HurdyGurdy* hg,
    const HgBuffer* src,
    usize offset,
    void* dst,
    usize size
);

/**
 * A texture on the GPU
 */
typedef struct HgTexture HgTexture;

/**
 * Configuration for an HgTexture
 *
 * width, height, and depth are the size in pixels, must not be 0
 * dimensions is 1, 2, or 3, for 1D, 2D or 3D, defaults to 2
 * mip_levels creates space for mipmaps, which can be filled in with
 * hg_texture_generate_mipmaps(), defaults to 1 (no mips)
 * format is the format of each pixel, must not be to HG_FORMAT_UNDEFINED
 * usage lists how the texture will be used, must not be HG_TEXTURE_USAGE_NONE
 * edge_mode is how sampling behaves outside the edge of the texture behaves,
 * defaults to HG_SAMPLER_EDGE_MODE_REPEAT
 * bilinear_filter is whether samples are smoothed
 * make_cubemap creates a cubemap, width must equal height, depth must be 1,
 * dimensions must be 2D, and mip_levels must be 1
 */
typedef struct HgTextureConfig {
    u32 width;
    u32 height;
    u32 depth;
    u32 dimensions;
    u32 mip_levels;
    HgFormat format;
    HgTextureUsageFlags usage;
    HgSamplerEdgeMode edge_mode;
    bool bilinear_filter;
    bool make_cubemap;
} HgTextureConfig;

/**
 * Creates an HgTexture
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param config The texture configuration, must not be NULL
 * \return The created texture, is never NULL
 */
HgTexture* hg_texture_create(
    const HurdyGurdy* hg,
    const HgTextureConfig* config
);

/**
 * Destroys an HgTexture
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param texture The texture to destroy, must not be NULL
 */
void hg_texture_destroy(
    const HurdyGurdy* hg,
    HgTexture* texture
);

/**
 * Writes data to an HgTexture
 *
 * Copies into the whole texture, so src must be large enough
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param dst The texture to write to, must not be NULL
 * \param src The data to write, may be NULL to only set layout
 * \param layout The layout the texture will be set to after writing
 */
void hg_texture_write(
    const HurdyGurdy* hg,
    HgTexture* dst,
    const void* src,
    HgTextureLayout layout
);

/**
 * Reads data from an HgTexture
 *
 * The size of the dst buffer must match the size of the texture
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param dst A buffer to read to, must not be NULL
 * \param src The texture to read from, must not be NULL
 * \param layout The layout the texture will be set to after reading
 */
void hg_texture_read(
    const HurdyGurdy* hg,
    HgTexture* src,
    void* dst,
    HgTextureLayout layout
);

/**
 * Gets the maximum number of mip levels a texture can have
 *
 * \param width The width of the texture, must be greater than 0
 * \param height The height of the texture, must be greater than 0
 * \param depth The depth of the texture, must be greater than 0
 * \return The maximum number of mip levels
 */
u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth);

/**
 * Generates mipmaps in a texture
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param texture The texture to generate mipmaps in, must not be NULL
 * \param layout The layout the texture will be set to after generating mipmaps
 */
void hg_texture_generate_mipmaps(
    const HurdyGurdy* hg,
    HgTexture* texture,
    HgTextureLayout layout
);

/**
 * A shader on the GPU
 */
typedef struct HgShader HgShader;

/**
 * An attribute of a vertex binding
 */
typedef struct HgVertexAttribute {
    HgFormat format;
    u32 offset;
} HgVertexAttribute;

/**
 * A description of a vertex binding
 */
typedef struct HgVertexBinding {
    HgVertexAttribute* attributes;
    u32 attribute_count;
    u32 stride;
    HgVertexInputRate input_rate;
} HgVertexBinding;

/**
 * A binding of a descriptor set
 */
typedef struct HgDescriptorSetBinding {
    HgDescriptorType descriptor_type;
    u32 descriptor_count;
} HgDescriptorSetBinding;

/**
 * A description of a descriptor set
 */
typedef struct HgDescriptorSet {
    HgDescriptorSetBinding* bindings;
    u32 binding_count;
} HgDescriptorSet;

/**
 * Configuration for an HgShader
 *
 * color_format is the format of the render target, must not be
 * HG_FORMAT_UNDEFINED
 * depth_format is the format of the depth buffer, if it is HG_FORMAT_UNDEFINED,
 * then there is no depth buffer
 *
 * vertex_shader is a pointer to spirv bytecode, must not be NULL
 * vertex_shader_size is the size in bytes, must be greater than 0
 * fragment_shader is a pointer to spirv bytecode, must not be NULL
 * fragment_shader_size is the size in bytes, must be greater than 0
 *
 * vertex_bindings points to descriptions of the vertex bindings, may be NULL
 * vertex_binding_count is the number of vertex bindings, may be 0
 * desciptor_sets points to descriptions of the descriptor sets, may be NULL
 * descriptor_set_count is the number of descriptor sets, may be 0
 * push_constant_size is the size in bytes of the push constant, if 0, then
 * there is no push constant
 *
 * topology is how vertices are interpreted, HG_PRIMITIVE_TOPOLOGY_POINT_LIST
 * cull_mode enables face culling, defaults to HG_CULL_MODE_NONE
 * enable_color_blend enables color blending using pixel alpha values
 */
typedef struct HgShaderConfig {
    HgFormat color_format;
    HgFormat depth_format;

    const byte* vertex_shader;
    u32 vertex_shader_size;
    const byte* fragment_shader;
    u32 fragment_shader_size;

    HgVertexBinding* vertex_bindings;
    u32 vertex_binding_count;
    HgDescriptorSet* descriptor_sets;
    u32 descriptor_set_count;
    u32 push_constant_size;

    HgPrimitiveTopology topology;
    HgCullModeFlagBits cull_mode;
    bool enable_color_blend;
} HgShaderConfig;

/**
 * Creates an HgShader for graphics
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param config The shader configuration, must not be NULL
 * \return The created shader, is never NULL
 */
HgShader* hg_shader_create(
    const HurdyGurdy* hg,
    const HgShaderConfig* config
);

/**
 * Configuration for an HgShader for compute
 *
 * shader is a pointer to spirv bytecode, must not be NULL
 * shader_size is the size in bytes, must be greater than 0
 * desciptor_sets points to descriptions of the descriptor sets, may be NULL
 * descriptor_set_count is the number of descriptor sets, may be 0
 * push_constant_size is the size in bytes of the push constant, if 0, then
 * there is no push constant
 */
typedef struct HgComputeShaderConfig {
    const byte* shader;
    usize shader_size;
    HgDescriptorSet* descriptor_sets;
    u32 descriptor_set_count;
    u32 push_constant_size;
} HgComputeShaderConfig;

/**
 * Creates an HgShader for compute
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param config The shader configuration, must not be NULL
 * \return The created shader, is never NULL
 */
HgShader* hg_compute_shader_create(
    const HurdyGurdy* hg,
    const HgComputeShaderConfig* config
);

/**
 * Destroys an HgShader
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param shader The shader to destroy, must not be NULL
 */
void hg_shader_destroy(
    const HurdyGurdy* hg,
    HgShader* shader
);

/**
 * A command buffer
 */
typedef struct HgCommands HgCommands;

/**
 * Begins a generic command buffer
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \return The created command buffer, is never NULL
 */
HgCommands* hg_commands_begin(
    const HurdyGurdy* hg
);

/**
 * Ends a command buffer created by hg_commands_begin()
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param commands The command buffer, must not be NULL
 */
void hg_commands_end(
    const HurdyGurdy* hg,
    HgCommands* commands
);

/**
 * Begins a render pass
 *
 * \param commands The command buffer, must not be NULL
 * \param target The target texture to render to, must not be NULL
 * \param depth_buffer The depth buffer to render to, may be NULL if a depth
 * buffer is not being used
 * \param clear_target Whether to clear the target texture before rendering
 * \param clear_depth Whether to clear the depth buffer before rendering,
 * ignored if depth_buffer is NULL
 */
void hg_renderpass_begin(
    HgCommands* commands,
    HgTexture* target,
    HgTexture* depth_buffer,
    bool clear_target,
    bool clear_depth
);

/**
 * Ends a render pass
 *
 * \param command The command buffer, must not be NULL
 */
void hg_renderpass_end(
    HgCommands* commands
);

/**
 * Binds a shader
 *
 * \param commands The command buffer, must not be NULL
 * \param shader The shader to bind, must not be NULL
 */
void hg_shader_bind(
    HgCommands* commands,
    HgShader* shader
);

/**
 * Unbinds a shader
 *
 * Using this function is optional
 *
 * \param commands The command buffer, must not be NULL
 */
void hg_shader_unbind(
    HgCommands* commands
);

/**
 * A descriptor to bind to a descriptor set
 */
typedef struct HgDescriptor {
    HgDescriptorType type;
    u32 count;
    HgBuffer** buffers;
    HgTexture** textures;
} HgDescriptor;

/**
 * Binds a descriptor set
 *
 * Must be called with a shader bound, after hg_shader_bind()
 *
 * \param commands The command buffer, must not be NULL
 * \param set_index The index of the descriptor set in the shader
 * \param descriptors The descriptors to bind, must not be NULL
 * \param descriptor_count The number of descriptors to bind
 */
void hg_descriptor_set_bind(
    HgCommands* commands,
    u32 set_index,
    HgDescriptor* descriptors,
    u32 descriptor_count
);

/**
 * Binds a push constant
 *
 * Must be called with a shader bound, after hg_shader_bind()
 *
 * \param commands The command buffer, must not be NULL
 * \param data The data to bind, must not be NULL
 * \param size The size of the data to bind, must be greater than 0
 */
void hg_push_constant_bind(
    HgCommands* commands,
    void* data,
    u32 size
);

/**
 * Draws a set of vertices
 *
 * Must be called during a renderpass, after hg_renderpass_begin()
 * Must be called with a shader bound, after hg_shader_bind()
 *
 * \param command The command buffer, must not be NULL
 * \param vertex_buffer The vertices to draw, may be NULL
 * \param index_buffer Indices to order the vertices, may be NULL
 * \param vertex_count The number of vertices to draw, ignored if index_buffer
 * is present
 */
void hg_draw(
    HgCommands* commands,
    HgBuffer* vertex_buffer,
    HgBuffer* index_buffer,
    u32 vertex_count
);

/**
 * Dispatches a compute shader
 *
 * Must be called with a shader bound, after hg_shader_bind()
 *
 * \param commands The command buffer, must not be NULL
 * \param group_count_x The number of groups to dispatch in the x direction
 * \param group_count_y The number of groups to dispatch in the y direction
 * \param group_count_z The number of groups to dispatch in the z direction
 */
void hg_compute_dispatch(
    HgCommands* commands,
    u32 group_count_x,
    u32 group_count_y,
    u32 group_count_z
);

/**
 * Copies from one buffer to another
 *
 * \param commands The command buffer, must not be NULL
 * \param dst The destination written to, must not be NULL
 * \param dst_offset The offset into dst
 * \param src The source read from, must not be NULL
 * \param src_offset The offset into src
 * \param size The number of bytes to copy
 */
void hg_buffer_copy(
    HgCommands* commands,
    HgBuffer* dst,
    usize dst_offset,
    HgBuffer* src,
    usize src_offset,
    usize size
);

/**
 * Configuration for hg_texture_blit()
 *
 * Coordinates are in normalized space, from 0.0 to 1.0
 *
 * x, y, and z are the begin coordinates of the target area
 * w, h, and d are the width, height, and depth of the target area
 * mip_level and array_layer are ignored if the texture does not use them
 */
typedef struct HgBlitConfig {
    HgTexture* texture;
    f32 x, y, z, w, h, d;
    u32 mip_level;
    u32 array_layer;
} HgBlitNormConfig;

/**
 * Copies from one texture to another
 *
 * \param commands The command buffer, must not be NULL
 * \param dst The destination written to, must not be NULL
 * \param src The source read from, must not be NULL
 * \param bilinear_filter Whether to smooth out the result
 */
void hg_texture_blit(
    HgCommands* commands,
    HgBlitNormConfig* dst,
    HgBlitNormConfig* src,
    bool bilinear_filter
);

/**
 * Copies into a buffer from a texture
 *
 * Copies the whole texture, so dst must be large enough
 *
 * \param commands The command buffer, must not be NULL
 * \param dst The destination written to, must not be NULL
 * \param src The source read from, must not be NULL
 */
void hg_buffer_copy_from_texture(
    HgCommands* commands,
    HgBuffer* dst,
    HgTexture* src
);

/**
 * Copies into a texture from a buffer
 *
 * Copies the size of the whole texture, so src must be large enough
 *
 * \param commands The command buffer, must not be NULL
 * \param dst The destination written to, must not be NULL
 * \param src The source read from, must not be NULL
 */
void hg_texture_copy_from_buffer(
    HgCommands* commands,
    HgTexture* dst,
    HgBuffer* src
);

/**
 * Creates a buffer memory barrier
 *
 * Ensures read/writes are complete and cashes are flushed
 *
 * \param commands The command buffer, must not be NULL
 * \param buffer The buffer to use, must not be NULL
 */
void hg_memory_barrier_buffer(
    const HgCommands* commands,
    HgBuffer* buffer
);

/**
 * Creates a texture memory barrier
 *
 * Ensures read/writes are complete and cashes are flushed
 * Changes texture layout manually
 *
 * Note, texture barriers for render targets and depth buffers are done
 * automatically in a frame
 *
 * \param commands The command buffer, must not be NULL
 * \param texture The texture to use, must not be NULL
 * \param begin_layout The layout the texture was in before the barrier, may be
 * NULL if the memory need not be preserved
 * \param end_layout The layout the texture will be set to
 */
void hg_memory_barrier_texture(
    const HgCommands* commands,
    HgTexture* texture,
    HgTextureLayout begin_layout,
    HgTextureLayout end_layout
);

#endif // HG_GRAPHICS_H
