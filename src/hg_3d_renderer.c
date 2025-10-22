#include "hg_3d_renderer.h"
#include "hg_graphics_enums.h"

typedef struct HgWorldUniform {
    HgMat4 view;
    HgMat4 proj;
    u32 dir_light_count;
    u32 point_light_count;
} HgWorldUniform;

typedef struct HgModelPush {
    HgMat4 model;
} HgModelPush;

static HgShader* s_shader;
static HgBuffer* s_world_buffer;

typedef struct HgDirectionalLight {
    HgVec4 direction;
    HgVec4 color;
} HgDirectionalLight;
static HgBuffer* s_dir_light_buffer;

static u32 s_dir_light_capacity;
static u32 s_dir_light_count;
static HgDirectionalLight* s_dir_lights;

typedef struct HgPointLight {
    HgVec4 position;
    HgVec4 color;
} HgPointLight;
static HgBuffer* s_point_light_buffer;

static u32 s_point_light_capacity;
static u32 s_point_light_count;
static HgPointLight* s_point_lights;

typedef struct HgModelTicket {
    HgModel3D model;
    HgModelPush push;
} HgModelTicket;

static u32 s_model_ticket_capacity;
static u32 s_model_ticket_count;
static HgModelTicket* s_model_tickets;

typedef struct HgColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} HgColor;

static const HgColor s_default_color_data[] = {
    {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
    {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
};
static HgTexture* s_default_color_map;

static const HgVec4 s_default_normal_data[] = {
    {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f},
    {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f},
};
static HgTexture* s_default_normal_map;

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

    HgDescriptorSetBinding world_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptor_count = 1,
    }, {
        .descriptor_type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptor_count = 1,
    }, {
        .descriptor_type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptor_count = 1,
    }};
    HgDescriptorSetBinding object_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
        .descriptor_count = 2,
    }};
    HgDescriptorSet descriptor_sets[] = {{
        .bindings = world_set_bindings,
        .binding_count = HG_ARRAY_SIZE(world_set_bindings),
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
        case HG_ERROR_FILE_NOT_FOUND: HG_ERROR("vertex shader not found");
        case HG_ERROR_FILE_READ_FAILURE: HG_ERROR("vertex shader not readable");
        default: HG_ERROR("unknown error");
    }

    byte* fragment_shader;
    usize fragment_shader_size;
    HgError fragment_shader_error = hg_file_load_binary(
        "build/hg_model.frag.spv", &fragment_shader, &fragment_shader_size
    );
    switch (fragment_shader_error) {
        case HG_SUCCESS: break;
        case HG_ERROR_FILE_NOT_FOUND: HG_ERROR("fragment shader not found");
        case HG_ERROR_FILE_READ_FAILURE: HG_ERROR("fragment shader not readable");
        default: HG_ERROR("unknown error");
    }

    s_shader = hg_shader_create(&(HgShaderConfig){
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
        .cull_mode = HG_CULL_MODE_BACK_BIT,
        .enable_depth_buffer = true,
        .enable_color_blend = false,
    });

    hg_file_unload_binary(vertex_shader, vertex_shader_size);
    hg_file_unload_binary(fragment_shader, fragment_shader_size);

    s_world_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgWorldUniform),
        .usage = HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });

    s_dir_light_capacity = 32;
    s_dir_light_count = 0;
    s_dir_lights = hg_heap_alloc(s_dir_light_capacity * sizeof(HgDirectionalLight));

    s_dir_light_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgDirectionalLight) * s_dir_light_capacity,
        .usage = HG_BUFFER_USAGE_STORAGE_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });

    s_point_light_capacity = 128;
    s_point_light_count = 0;
    s_point_lights = hg_heap_alloc(s_point_light_capacity * sizeof(HgPointLight));

    s_point_light_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(HgPointLight) * s_point_light_capacity,
        .usage = HG_BUFFER_USAGE_STORAGE_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });

    s_model_ticket_capacity = 1024;
    s_model_ticket_count = 0;
    s_model_tickets = hg_heap_alloc(s_model_ticket_capacity * sizeof(HgModelTicket));

    s_default_color_map = hg_3d_texture_map_create(
        s_default_color_data,
        2, 2,
        HG_FORMAT_R8G8B8A8_UNORM,
        false
    );
    s_default_normal_map = hg_3d_texture_map_create(
        s_default_normal_data, 
        2, 2,
        HG_FORMAT_R32G32B32A32_SFLOAT,
        false
    );
}

void hg_3d_renderer_shutdown(void) {
    hg_texture_destroy(s_default_normal_map);
    hg_texture_destroy(s_default_color_map);
    hg_buffer_destroy(s_point_light_buffer);
    hg_buffer_destroy(s_dir_light_buffer);
    hg_buffer_destroy(s_world_buffer);
    hg_shader_destroy(s_shader);
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
        .edge_mode = HG_SAMPLER_EDGE_MODE_REPEAT,
        .bilinear_filter = filter,
    });
    hg_texture_write(texture, data, HG_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return texture;
}

void hg_3d_renderer_update_projection(f32 fov, f32 aspect, f32 near, f32 far) {
    HgMat4 proj = hg_projection_matrix_perspective(fov, aspect, near, far);
    hg_buffer_write(s_world_buffer, offsetof(HgWorldUniform, proj), &proj, sizeof(proj));
}

void hg_3d_renderer_update_view(HgVec3 position, f32 zoom, HgQuat rotation) {
    HgMat4 view = hg_view_matrix(position, zoom, rotation);
    hg_buffer_write(s_world_buffer, offsetof(HgWorldUniform, view), &view, sizeof(view));
}

void hg_3d_renderer_queue_directional_light(HgVec3 direction, HgVec3 color, f32 intensity) {
    if (s_dir_light_count >= s_dir_light_capacity) {
        s_dir_light_capacity *= 2;
        s_dir_lights = hg_heap_realloc(s_dir_lights, s_dir_light_capacity * sizeof(HgDirectionalLight));

        hg_buffer_destroy(s_dir_light_buffer);
        s_dir_light_buffer = hg_buffer_create(&(HgBufferConfig){
            .size = sizeof(HgDirectionalLight) * s_dir_light_capacity,
            .usage = HG_BUFFER_USAGE_STORAGE_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
        });
    }

    s_dir_lights[s_dir_light_count] = (HgDirectionalLight){
        .direction = {direction.x, direction.y, direction.z, 1.0f},
            .color = {color.x, color.y, color.z, intensity},
    };
    ++s_dir_light_count;
}

void hg_3d_renderer_queue_point_light(HgVec3 position, HgVec3 color, f32 intensity) {
    if (s_point_light_count >= s_point_light_capacity) {
        s_point_light_capacity *= 2;
        s_point_lights = hg_heap_realloc(s_point_lights, s_point_light_capacity * sizeof(HgPointLight));

        hg_buffer_destroy(s_point_light_buffer);
        s_point_light_buffer = hg_buffer_create(&(HgBufferConfig){
            .size = sizeof(HgPointLight) * s_point_light_capacity,
            .usage = HG_BUFFER_USAGE_STORAGE_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
        });
    }

    s_point_lights[s_point_light_count] = (HgPointLight){
        .position = {position.x, position.y, position.z, 1.0f},
        .color = {color.x, color.y, color.z, intensity},
    };
    ++s_point_light_count;
}

void hg_3d_renderer_queue_model(HgModel3D* model, HgTransform3D* transform) {
    HG_ASSERT(model != NULL);
    HG_ASSERT(transform != NULL);

    if (s_model_ticket_count >= s_model_ticket_capacity) {
        s_model_ticket_capacity *= 2;
        s_model_tickets = hg_heap_realloc(s_model_tickets, s_model_ticket_capacity * sizeof(HgModelTicket));
    }

    s_model_tickets[s_model_ticket_count] = (HgModelTicket){
        .model = *model,
        .push = (HgModelPush){
            .model = hg_model_matrix_3d(transform->position, transform->scale, transform->rotation),
        },
    };
    ++s_model_ticket_count;
}

void hg_3d_renderer_draw(HgTexture* target, HgTexture* depth_buffer) {
    HG_ASSERT(target != NULL);
    HG_ASSERT(depth_buffer != NULL);

    hg_buffer_write(
        s_world_buffer, offsetof(HgWorldUniform, dir_light_count), &s_dir_light_count, sizeof(s_dir_light_count)
    );
    hg_buffer_write(
        s_world_buffer, offsetof(HgWorldUniform, point_light_count), &s_point_light_count, sizeof(s_point_light_count)
    );

    if (s_dir_light_count > 0)
        hg_buffer_write(s_dir_light_buffer, 0, s_dir_lights, sizeof(HgDirectionalLight) * s_dir_light_count);
    if (s_point_light_count > 0) {
        hg_buffer_write(s_point_light_buffer, 0, s_point_lights, sizeof(HgPointLight) * s_point_light_count);
    }

    hg_renderpass_begin(target, depth_buffer);

    hg_shader_bind(s_shader);

    HgDescriptor world_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count = 1,
        .buffers = &s_world_buffer,
    }, {
        .type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .count = 1,
        .buffers = &s_dir_light_buffer,
    }, {
        .type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .count = 1,
        .buffers = &s_point_light_buffer,
    }};
    hg_bind_descriptor_set(0, world_descriptor_set, HG_ARRAY_SIZE(world_descriptor_set));

    for (u32 i = 0; i < s_model_ticket_count; ++i) {
        HgModel3D* model = &s_model_tickets[i].model;
        HgModelPush* push = &s_model_tickets[i].push;

        HgTexture* textures[] = {
            model->color_map != NULL ? model->color_map : s_default_color_map,
            model->normal_map != NULL ? model->normal_map : s_default_normal_map,
        };
        HgDescriptor object_descriptor_set[] = {{
            .type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
            .count = 2,
            .textures = textures,
        }};
        hg_bind_descriptor_set(1, object_descriptor_set, HG_ARRAY_SIZE(object_descriptor_set));

        hg_draw_indexed(model->vertex_buffer, model->index_buffer, push, sizeof(HgModelPush));
    }

    hg_renderpass_end();

    s_dir_light_count = 0;
    s_point_light_count = 0;
    s_model_ticket_count = 0;
}

