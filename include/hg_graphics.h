#ifndef HG_GRAPHICS_H
#define HG_GRAPHICS_H

#include "hg_utils.h"
#include "hg_graphics_enums.h"

/**
 * Initializes the graphics subsystem
 *
 * This function must be called before any other graphics function
 */
void hg_graphics_init(void);

/**
 * Shuts down the graphics subsystem
 *
 * This function can be called in order to reinitialize the graphics subsystem
 */
void hg_graphics_shutdown(void);

/**
 * Waits for the graphics subsystem to finish rendering
 *
 * This function can be called to ensure that the resources are not being used before destruction
 */
void hg_graphics_wait(void);

/**
 * Configuration for a window
 *
 * width and height are only used when windowed is true
 */
typedef struct HgWindowConfig {
    const char* title;
    u32 width;
    u32 height;
    bool windowed;
} HgWindowConfig;

/**
 * Opens the window
 *
 * This function must be called before rendering a frame
 *
 * \param config The window configuration, must not be NULL
 */
void hg_window_open(const HgWindowConfig* config);

/**
 * Closes the window
 *
 * This function can be called in order to reinitialize the window
 * This function must be called before shutting down the graphics subsystem
 */
void hg_window_close(void);

/**
 * Gets the size of the window
 *
 * \param width A pointer to store the width of the window, must not be NULL
 * \param height A pointer to store the height of the window, must not be NULL
 */
void hg_window_get_size(u32* width, u32* height);

/**
 * Updates the size of the window
 *
 * This function updates the swapchain to match the new size of the window
 * This function must be called after the window has changed size
 */
void hg_window_update_size(void);

/**
 * A buffer on the GPU
 */
typedef struct HgBuffer HgBuffer;

/**
 * Configuration for an HgBuffer
 *
 * size is the size of the buffer in bytes
 * size must be greater than 0
 * usage must not be 0
 */
typedef struct HgBufferConfig {
    usize size;
    HgBufferUsageFlags usage;
    HgGpuMemoryType memory_type;
} HgBufferConfig;

/**
 * Creates an HgBuffer
 *
 * \param config The buffer configuration, must not be NULL
 * \return The created buffer
 */
HgBuffer* hg_buffer_create(const HgBufferConfig* config);

/**
 * Destroys an HgBuffer
 *
 * \param buffer The buffer to destroy, must not be NULL
 */
void hg_buffer_destroy(HgBuffer* buffer);

/**
 * Writes data to an HgBuffer
 *
 * If the buffer memory type is HG_GPU_MEMORY_TYPE_LINEAR_ACCESS, the offset must be 0
 *
 * \param dst The buffer to write to, must not be NULL
 * \param offset The offset into the dst buffer
 * \param src The data to write, must not be NULL
 * \param size The size of the data to write, must be greater than 0
 */
void hg_buffer_write(HgBuffer* dst, usize offset, const void* src, usize size);

/**
 * Reads data from an HgBuffer
 *
 * \param dst The data to read, must not be NULL
 * \param size The size of the data to read, must be greater than 0
 * \param src The buffer to read from, must not be NULL
 * \param offset The offset into the src buffer
 */
void hg_buffer_read(void* dst, usize size, const HgBuffer* src, usize offset);

/**
 * A texture on the GPU
 */
typedef struct HgTexture HgTexture;

/**
 * Configuration for an HgTexture
 *
 * width, height, and depth must be greater than 0
 * format, aspect, and usage must not be 0
 * array_layers is ignored if make_cubemap is true
 * if make_cubemap is true, width and height must be equal, and depth must be 1
 */
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

/**
 * Creates an HgTexture
 *
 * \param config The texture configuration, must not be NULL
 * \return The created texture
 */
HgTexture* hg_texture_create(const HgTextureConfig* config);

/**
 * Destroys an HgTexture
 *
 * \param texture The texture to destroy, must not be NULL
 */
void hg_texture_destroy(HgTexture* texture);

/**
 * Writes data to an HgTexture
 *
 * The size of the data must match the size of the texture
 *
 * \param dst The texture to write to, must not be NULL
 * \param src The data to write, must not be NULL
 * \param layout The layout the texture will be set to after writing
 */
void hg_texture_write(HgTexture* dst, const void* src, HgTextureLayout layout);

/**
 * Reads data from an HgTexture
 *
 * The size of the dst buffer must match the size of the texture
 *
 * \param dst A buffer to read to, must not be NULL
 * \param src The texture to read from, must not be NULL
 * \param layout The layout the texture will be set to after reading
 */
void hg_texture_read(void* dst, HgTexture* src, HgTextureLayout layout);

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
 * \param texture The texture to generate mipmaps in, must not be NULL
 * \param layout The layout the texture will be set to after generating mipmaps
 */
void hg_texture_generate_mipmaps(HgTexture* texture, HgTextureLayout layout);

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
 * color_format must not be HG_FORMAT_UNDEFINED
 * if depth_format is HG_FORMAT_UNDEFINED, there is no depth buffer
 *
 * spirv_vertex_shader and spirv_fragment_shader must not be NULL
 * vertex_shader_size and fragment_shader_size must be greater than 0
 *
 * if vertex_bindings is NULL, there are no vertex bindings
 * otherwise, vertex_binding_count must be greater than 0
 *
 * if descriptor_sets is NULL, there are no descriptor sets
 * otherwise, descriptor_set_count must be greater than 0
 *
 * if push_constant_size is 0, there is no push constant
 */
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
} HgShaderConfig;

/**
 * Creates an HgShader for graphics
 *
 * \param config The shader configuration, must not be NULL
 * \return The created shader
 */
HgShader* hg_shader_create(const HgShaderConfig* config);

/**
 * Configuration for an HgShader for compute
 *
 * spirv_shader must not be NULL
 * shader_size must be greater than 0
 *
 * if descriptor_sets is NULL, there are no descriptor sets
 * otherwise, descriptor_set_count must be greater than 0
 *
 * if push_constant_size is 0, there is no push constant
 */
typedef struct HgComputeShaderConfig {
    const byte* spirv_shader;
    HgDescriptorSet* descriptor_sets;
    u32 shader_size;
    u32 descriptor_set_count;
    u32 push_constant_size;
} HgComputeShaderConfig;

/**
 * Creates an HgShader for compute
 *
 * \param config The shader configuration, must not be NULL
 * \return The created shader
 */
HgShader* hg_compute_shader_create(const HgComputeShaderConfig* config);

/**
 * Destroys an HgShader
 *
 * \param shader The shader to destroy, must not be NULL
 */
void hg_shader_destroy(HgShader* shader);

/**
 * Begins generic a command buffer
 *
 * cannot be called while recording a frame
 */
void hg_commands_begin(void);

/**
 * Ends a command buffer
 *
 * hg_commands_begin() must be called before this function
 */
void hg_commands_end(void);

/**
 * Begins a command buffer for this frame
 *
 * This function must be called before rendering a frame
 *
 * \return HG_SUCCESS if the frame was started successfully
 * \return HG_ERROR_WINDOW_INVALID if the window was closed
 */
HgError hg_frame_begin(void);

/**
 * Ends this frame and presents to the window
 *
 * hg_frame_begin() must be called before this function
 *
 * \param framebuffer The framebuffer to present to the window, does not present if NULL
 * \return HG_SUCCESS if the frame was ended successfully
 * \return HG_ERROR_WINDOW_INVALID if the window was closed
 */
HgError hg_frame_end(HgTexture* framebuffer);

/**
 * Begins a render pass in a frame
 *
 * \param target The target texture to render to, must not be NULL
 * \param depth_buffer The depth buffer to render to, may be NULL if no depth buffer is used
 * \param clear_target Whether to clear the target texture before rendering
 * \param clear_depth Whether to clear the depth buffer before rendering, ignored if depth_buffer is NULL
 */
void hg_renderpass_begin(HgTexture* target, HgTexture* depth_buffer, bool clear_target, bool clear_depth);

/**
 * Ends a render pass in a frame
 *
 * hg_renderpass_begin() must be called before this function
 */
void hg_renderpass_end(void);

/**
 * Binds a shader in a command buffer
 *
 * \param shader The shader to bind, must not be NULL
 */
void hg_shader_bind(HgShader* shader);

/**
 * Unbinds a shader in a command buffer
 *
 * Using this function is optional
 */
void hg_shader_unbind(void);

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
 * Binds a descriptor set in a command buffer
 *
 * \param set_index The index of the descriptor set in the shader
 * \param descriptors The descriptors to bind, must not be NULL
 * \param descriptor_count The number of descriptors to bind
 */
void hg_bind_descriptor_set(u32 set_index, HgDescriptor* descriptors, u32 descriptor_count);

/**
 * Binds a push constant in a command buffer
 *
 * \param data The data to bind, must not be NULL
 * \param size The size of the data to bind, must be greater than 0
 */
void hg_bind_push_constant(void* data, u32 size);

/**
 * Draws a set of vertices in a render pass, using a shader
 *
 * hg_renderpass_begin() must be called before this function
 * hg_shader_bind() must be called before this function
 *
 * \param vertex_buffer The vertex buffer to draw from, may be NULL if no vertex buffer is used
 * \param index_buffer The index buffer to draw from, may be NULL if no index buffer is used
 * \param vertex_count The number of vertices to draw, only used if index_buffer is NULL
 */
void hg_draw(HgBuffer* vertex_buffer, HgBuffer* index_buffer, u32 count);

/**
 * Dispatches a compute shader in a command buffer
 *
 * \param group_count_x The number of groups to dispatch in the x direction
 * \param group_count_y The number of groups to dispatch in the y direction
 * \param group_count_z The number of groups to dispatch in the z direction
 */
void hg_compute_dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z);

#endif // HG_GRAPHICS_H
