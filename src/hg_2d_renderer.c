#include "hg_2d_renderer.h"

typedef struct HgVPUniform {
    HgMat4 view;
    HgMat4 proj;
} HgVPUniform;

typedef struct HgSpriteVertex {
    HgVec2 pos;
} HgSpriteVertex;

typedef struct HgSpritePush {
    HgMat4 model;
    HgVec2 offset;
    HgVec2 size;
} HgSpritePush;

static HgShader* s_sprite_shader;

static HgBuffer* s_vp_buffer;

static const HgSpriteVertex s_quad_vertices[] = {
    {{0.0f, 0.0f}}, {{0.0f, 1.0f}}, {{1.0f, 1.0f}}, {{1.0f, 0.0f}},
};

static const u32 s_quad_indices[] = {0, 1, 2, 3};

static HgBuffer* s_quad_vertex_buffer;
static HgBuffer* s_quad_index_buffer;

typedef struct HgSpriteTicket {
    HgTexture* texture;
    HgVec2 offset;
    HgVec2 size;
    HgTransform2D transform;
} HgSpriteTicket;

static HgSpriteTicket* s_sprite_tickets;
static u32 s_sprite_ticket_capacity;
static u32 s_sprite_ticket_count;

void hg_2d_renderer_init(void) {
    HgVertexAttribute vertex_attributes[] = {{
        .format = HG_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(HgSpriteVertex, pos),
    }};
    HgVertexBinding vertex_bindings[] = {{
        .attributes = vertex_attributes,
        .attribute_count = HG_ARRAY_SIZE(vertex_attributes),
        .stride = sizeof(HgSpriteVertex),
    }};

    HgDescriptorSetBinding vp_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptor_count = 1,
    }};
    HgDescriptorSetBinding texture_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
        .descriptor_count = 1,
    }};
    HgDescriptorSet descriptor_sets[] = {{
        .bindings = vp_set_bindings,
        .binding_count = HG_ARRAY_SIZE(vp_set_bindings),
    }, {
        .bindings = texture_set_bindings,
        .binding_count = HG_ARRAY_SIZE(texture_set_bindings),
    }};

    byte* vertex_shader;
    usize vertex_shader_size;
    HgError vertex_shader_error = hg_file_load_binary(
        "build/hg_sprite.vert.spv", &vertex_shader, &vertex_shader_size
    );
    switch (vertex_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("2D Renderer sprite vertex shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("2D Renderer sprite vertex shader not readable");
        default: HG_ERROR("unknown error");
    }

    byte* fragment_shader;
    usize fragment_shader_size;
    HgError fragment_shader_error = hg_file_load_binary(
        "build/hg_sprite.frag.spv", &fragment_shader, &fragment_shader_size
    );
    switch (fragment_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("2D Renderer sprite fragment shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("2D Renderer sprite fragment shader not readable");
        default: HG_ERROR("unknown error");
    }

    s_sprite_shader = hg_shader_create(&(HgShaderConfig){
        .color_format = HG_FORMAT_R8G8B8A8_UNORM,
        .depth_format = HG_FORMAT_D32_SFLOAT,
        .spirv_vertex_shader = vertex_shader,
        .vertex_shader_size = (u32)vertex_shader_size,
        .spirv_fragment_shader = fragment_shader,
        .fragment_shader_size = (u32)fragment_shader_size,
        .vertex_bindings = vertex_bindings,
        .vertex_binding_count = HG_ARRAY_SIZE(vertex_bindings),
        .descriptor_sets = descriptor_sets,
        .descriptor_set_count = HG_ARRAY_SIZE(descriptor_sets),
        .push_constant_size = sizeof(HgSpritePush),
        .topology = HG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        .cull_mode = HG_CULL_MODE_NONE,
        .enable_depth_buffer = true,
        .enable_color_blend = true,
    });

    hg_file_unload_binary(vertex_shader, vertex_shader_size);
    hg_file_unload_binary(fragment_shader, fragment_shader_size);

    s_vp_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgVPUniform),
        .usage = HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    HgVPUniform vp_uniform = {
        .view = hg_smat4(1.0f),
        .proj = hg_smat4(1.0f),
    };
    hg_buffer_write(s_vp_buffer, 0, &vp_uniform, sizeof(vp_uniform));

    s_quad_vertex_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(s_quad_vertices),
        .usage = HG_BUFFER_USAGE_VERTEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(s_quad_vertex_buffer, 0, s_quad_vertices, sizeof(s_quad_vertices));

    s_quad_index_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(s_quad_indices),
        .usage = HG_BUFFER_USAGE_INDEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(s_quad_index_buffer, 0, s_quad_indices, sizeof(s_quad_indices));

    s_sprite_ticket_capacity = 1024;
    s_sprite_ticket_count = 0;
    s_sprite_tickets = hg_heap_alloc(sizeof(HgSpriteTicket) * s_sprite_ticket_capacity);
}

void hg_2d_renderer_shutdown(void) {
    hg_buffer_destroy(s_quad_vertex_buffer);
    hg_buffer_destroy(s_quad_index_buffer);
    hg_buffer_destroy(s_vp_buffer);
    hg_shader_destroy(s_sprite_shader);
}

void hg_2d_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer) {
    HG_ASSERT(target != NULL);
    HG_ASSERT(depth_buffer != NULL);
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);

    *target = hg_texture_create(&(HgTextureConfig){
        .width = width,
        .height = height,
        .depth = 1,
        .array_layers = 1,
        .mip_levels = 1,
        .format = HG_FORMAT_R8G8B8A8_UNORM,
        .aspect = HG_TEXTURE_ASPECT_COLOR_BIT,
        .usage = HG_TEXTURE_USAGE_RENDER_TARGET_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
    });

    *depth_buffer = hg_texture_create(&(HgTextureConfig){
        .width = width,
        .height = height,
        .depth = 1,
        .array_layers = 1,
        .mip_levels = 1,
        .format = HG_FORMAT_D32_SFLOAT,
        .aspect = HG_TEXTURE_ASPECT_DEPTH_BIT,
        .usage = HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
    });
}

HgTexture* hg_2d_sprite_create(const u32* data, u32 width, u32 height, HgFormat format, bool filter) {
    HG_ASSERT(data != NULL);
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);
    HG_ASSERT(format != HG_FORMAT_UNDEFINED);

    HgTexture* texture = hg_texture_create(&(HgTextureConfig){
        .width = width,
        .height = height,
        .depth = 1,
        .array_layers = 1,
        .mip_levels = 1,
        .format = format,
        .aspect = HG_TEXTURE_ASPECT_COLOR_BIT,
        .usage = HG_TEXTURE_USAGE_SAMPLED_BIT | HG_TEXTURE_USAGE_TRANSFER_DST_BIT,
        .edge_mode = HG_SAMPLER_EDGE_MODE_CLAMP_TO_EDGE,
        .bilinear_filter = filter,
    });
    hg_texture_write(texture, data, HG_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return texture;
}

void hg_2d_renderer_update_projection(f32 aspect, f32 near, f32 far) {
    HgMat4 proj = hg_projection_matrix_orthographic(-aspect, aspect, -1.0f, 1.0f, near, far);
    hg_buffer_write(s_vp_buffer, offsetof(HgVPUniform, proj), &proj, sizeof(proj));
}

void hg_2d_renderer_update_view(HgVec3 position, f32 zoom, HgQuat rotation) {
    HgMat4 view = hg_view_matrix(position, zoom, rotation);
    hg_buffer_write(s_vp_buffer, offsetof(HgVPUniform, view), &view, sizeof(view));
}

void hg_2d_renderer_queue_sprite(HgTexture* texture, HgVec2 offset, HgVec2 extent, HgTransform2D* transform) {
    HG_ASSERT(texture != NULL);

    if (s_sprite_ticket_count >= s_sprite_ticket_capacity) {
        s_sprite_ticket_capacity *= 2;
        s_sprite_tickets = hg_heap_realloc(s_sprite_tickets, sizeof(HgSpriteTicket) * s_sprite_ticket_capacity);
    }

    s_sprite_tickets[s_sprite_ticket_count] = (HgSpriteTicket){
        .texture = texture,
        .offset = offset,
        .size = extent,
        .transform = *transform,
    };
    ++s_sprite_ticket_count;
}

void hg_2d_renderer_draw(HgTexture* target, HgTexture* depth_buffer) {
    hg_renderpass_begin(target, depth_buffer);

    hg_shader_bind(s_sprite_shader);

    HgDescriptor vp_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count = 1,
        .buffers = &s_vp_buffer,
    }};
    hg_bind_descriptor_set(0, vp_descriptor_set, HG_ARRAY_SIZE(vp_descriptor_set));

    for (u32 i = 0; i < s_sprite_ticket_count; ++i) {
        HgSpriteTicket* ticket = &s_sprite_tickets[i];

        HgDescriptor texture_descriptor_set[] = {{
            .type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
            .count = 1,
            .textures = &ticket->texture,
        }};
        hg_bind_descriptor_set(1, texture_descriptor_set, HG_ARRAY_SIZE(texture_descriptor_set));

        HgSpritePush push_data = {
            .model = hg_model_matrix_2d(
                ticket->transform.position, ticket->transform.scale, ticket->transform.rotation
            ),
            .offset = ticket->offset,
            .size = ticket->size,
        };
        hg_draw_indexed(s_quad_vertex_buffer, s_quad_index_buffer, &push_data, sizeof(push_data));
    }

    hg_shader_unbind();

    hg_renderpass_end();

    s_sprite_ticket_count = 0;
}

