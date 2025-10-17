#include "hg_3d_renderer.h"
#include "hg_graphics_enums.h"

typedef struct HgVPUniform {
    HgMat4 view;
    HgMat4 proj;
} HgVPUniform;

typedef struct HgModelPush {
    HgMat4 model;
} HgModelPush;

static HgShader* shader;
static HgBuffer* vp_buffer;

typedef struct HgModelTicket {
    HgModel3D model;
    HgModelPush push;
} HgModelTicket;

static u32 model_ticket_capacity;
static u32 model_ticket_count;
static HgModelTicket* model_tickets;

typedef struct HgColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} HgColor;

static const HgColor default_color_data[] = {
    {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
    {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
};
static HgTexture* default_color_map;

static const HgVec4 default_normal_data[] = {
    {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f},
    {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f},
};
static HgTexture* default_normal_map;

void hg_3d_renderer_init(void) {
    HgVertexAttribute vertex_attributes[] = {{
        .format = HG_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(HgVertex3D, position),
    }, {
        .format = HG_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(HgVertex3D, normal),
    }, {
        .format = HG_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(HgVertex3D, tangent),
    }, {
        .format = HG_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(HgVertex3D, uv),
    }};
    HgVertexBinding vertex_bindings[] = {{
        .attributes = vertex_attributes,
        .attribute_count = HG_ARRAY_SIZE(vertex_attributes),
        .stride = sizeof(HgVertex3D),
    }};

    HgDescriptorSetBinding vp_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptor_count = 1,
    }};
    HgDescriptorSetBinding object_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
        .descriptor_count = 2,
    }};
    HgDescriptorSet descriptor_sets[] = {{
        .bindings = vp_set_bindings,
        .binding_count = HG_ARRAY_SIZE(vp_set_bindings),
    }, {
        .bindings = object_set_bindings,
        .binding_count = HG_ARRAY_SIZE(object_set_bindings),
    }};

    byte* vertex_shader;
    usize vertex_shader_size;
    HgError vertex_shader_error = hg_file_load_binary(
        "build/hg_model.vert.spv", &vertex_shader, &vertex_shader_size
    );
    switch (vertex_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("vertex shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("vertex shader not readable");
        default: HG_ERROR("unknown error");
    }

    byte* fragment_shader;
    usize fragment_shader_size;
    HgError fragment_shader_error = hg_file_load_binary(
        "build/hg_model.frag.spv", &fragment_shader, &fragment_shader_size
    );
    switch (fragment_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("fragment shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("fragment shader not readable");
        default: HG_ERROR("unknown error");
    }

    shader = hg_shader_create(&(HgShaderConfig){
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
        .push_constant_size = sizeof(HgModelPush),
        .topology = HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .cull_mode = HG_CULL_MODE_NONE, // HG_CULL_MODE_BACK_BIT,
        .enable_depth_buffer = true,
        .enable_color_blend = true,
    });

    hg_file_unload_binary(vertex_shader, vertex_shader_size);
    hg_file_unload_binary(fragment_shader, fragment_shader_size);

    vp_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgVPUniform),
        .usage = HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });

    model_ticket_capacity = 1024;
    model_ticket_count = 0;
    model_tickets = hg_heap_alloc(model_ticket_capacity * sizeof(HgModelTicket));

    default_color_map = hg_3d_texture_map_create(
        default_color_data,
        3, 3,
        HG_FORMAT_R8G8B8A8_UNORM,
        false
    );
    default_normal_map = hg_3d_texture_map_create(
        default_normal_data, 
        2, 2,
        HG_FORMAT_R32G32B32A32_SFLOAT,
        false
    );
}

void hg_3d_renderer_shutdown(void) {
    hg_texture_destroy(default_normal_map);
    hg_texture_destroy(default_color_map);
    hg_buffer_destroy(vp_buffer);
    hg_shader_destroy(shader);
}

void hg_3d_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer) {
    HG_ASSERT(target != NULL);
    HG_ASSERT(depth_buffer != NULL);

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

HgBuffer* hg_3d_vertex_buffer_create(const HgVertex3D* vertices, u32 vertex_count) {
    HG_ASSERT(vertices != NULL);
    HG_ASSERT(vertex_count > 0);

    HgBuffer* buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgVertex3D) * vertex_count,
        .usage = HG_BUFFER_USAGE_VERTEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(buffer, 0, vertices, sizeof(HgVertex3D) * vertex_count);

    return buffer;
}

HgBuffer* hg_3d_index_buffer_create(const u32* indices, u32 index_count) {
    HG_ASSERT(indices != NULL);
    HG_ASSERT(index_count > 0);

    HgBuffer* buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(u32) * index_count,
        .usage = HG_BUFFER_USAGE_INDEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(buffer, 0, indices, sizeof(u32) * index_count);

    return buffer;
}

HgTexture* hg_3d_texture_map_create(const void* data, u32 width, u32 height, HgFormat format, bool filter) {
    HG_ASSERT(data != NULL);
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);

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

void hg_3d_renderer_update_projection(f32 fov, f32 aspect, f32 near, f32 far) {
    HgMat4 proj = hg_projection_matrix_perspective(fov, aspect, near, far);
    hg_buffer_write(vp_buffer, offsetof(HgVPUniform, proj), &proj, sizeof(proj));
}

void hg_3d_renderer_update_view(HgVec3 position, f32 zoom, HgQuat rotation) {
    HgMat4 view = hg_view_matrix(position, zoom, rotation);
    hg_buffer_write(vp_buffer, offsetof(HgVPUniform, view), &view, sizeof(view));
}

void hg_3d_renderer_set_directional_light(HgVec3 direction, HgVec3 color, f32 intensity) {
    (void)direction;
    (void)color;
    (void)intensity;
    HG_ERROR("3d renderer does not yet support directional lights");
}

void hg_3d_renderer_queue_point_light(HgVec3 position, HgVec3 color, f32 intensity) {
    (void)position;
    (void)color;
    (void)intensity;
    HG_ERROR("3d renderer does not yet support point lights");
}

void hg_3d_renderer_queue_model(HgModel3D* model, HgTransform3D* transform) {
    HG_ASSERT(model != NULL);
    HG_ASSERT(transform != NULL);

    if (model_ticket_count >= model_ticket_capacity) {
        model_ticket_capacity *= 2;
        model_tickets = hg_heap_realloc(model_tickets, model_ticket_capacity * sizeof(HgModelTicket));
    }

    model_tickets[model_ticket_count] = (HgModelTicket){
        .model = *model,
        .push = (HgModelPush){
            .model = hg_model_matrix_3d(transform->position, transform->scale, transform->rotation),
        },
    };
    ++model_ticket_count;
}

void hg_3d_renderer_draw(HgTexture* target, HgTexture* depth_buffer) {
    HG_ASSERT(target != NULL);
    HG_ASSERT(depth_buffer != NULL);

    hg_renderpass_begin(target, depth_buffer);

    hg_shader_bind(shader);

    HgDescriptor vp_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count = 1,
        .buffers = &vp_buffer,
    }};
    hg_bind_descriptor_set(0, vp_descriptor_set, HG_ARRAY_SIZE(vp_descriptor_set));

    for (u32 i = 0; i < model_ticket_count; ++i) {
        HgModel3D* model = &model_tickets[i].model;
        HgModelPush* push = &model_tickets[i].push;

        HgTexture* textures[] = {
            model->color_map != NULL ? model->color_map : default_color_map,
            model->normal_map != NULL ? model->normal_map : default_normal_map,
        };
        HgDescriptor object_descriptor_set[] = {{
            .type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
            .count = 2,
            .textures = textures,
        }};
        hg_bind_descriptor_set(1, object_descriptor_set, HG_ARRAY_SIZE(object_descriptor_set));

        hg_draw_indexed(model->vertex_buffer, model->index_buffer, push, sizeof(HgModelPush));
    }

    hg_shader_unbind();

    hg_renderpass_end();

    model_ticket_count = 0;
}

