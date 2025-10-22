#include "hg_ray_marcher.h"

typedef struct HgVertex {
    HgVec2 pos;
} HgVertex;

typedef struct HgPush {
    HgMat4 view;
    f32 aspect;
} HgPush;

static HgShader* s_shader;

static HgVertex s_vertices[] = {
    {{-1.0f, -1.0f}}, {{3.0f, -1.0f}},
    {{-1.0f, 3.0f}},
};
static HgBuffer* s_vertex_buffer;

static u32 s_indices[] = {
    0, 1, 2,
};
static HgBuffer* s_index_buffer;

void hg_ray_marcher_init(void) {

    HgVertexAttribute vertex_attributes[] = {{
        .format = HG_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(HgVertex, pos),
    }};

    HgVertexBinding vertex_bindings[] = {{
        .attributes = vertex_attributes,
        .attribute_count = HG_ARRAY_SIZE(vertex_attributes),
        .stride = sizeof(HgVertex),
        .input_rate = HG_VERTEX_INPUT_RATE_VERTEX,
    }};

    usize vertex_shader_size = 0;
    byte* vertex_shader = NULL;
    HgError vertex_shader_error = hg_file_load_binary(
        "build/hg_ray_marcher.vert.spv", &vertex_shader, &vertex_shader_size
    );
    switch (vertex_shader_error) {
        case HG_SUCCESS: break;
        case HG_ERROR_FILE_NOT_FOUND: HG_ERROR("vertex shader not found");
        case HG_ERROR_FILE_READ_FAILURE: HG_ERROR("vertex shader not readable");
        default: HG_ERROR("unknown error");
    }

    usize fragment_shader_size = 0;
    byte* fragment_shader = NULL;
    HgError fragment_shader_error = hg_file_load_binary(
        "build/hg_ray_marcher.frag.spv", &fragment_shader, &fragment_shader_size
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
        .descriptor_sets = NULL,
        .descriptor_set_count = 0,
        .push_constant_size = sizeof(HgPush),
        .topology = HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .cull_mode = HG_CULL_MODE_NONE,
        .enable_color_blend = false,
        .enable_depth_buffer = false,
    });

    s_vertex_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(s_vertices),
        .usage = HG_BUFFER_USAGE_VERTEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(s_vertex_buffer, 0, s_vertices, sizeof(s_vertices));

    s_index_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(s_indices),
        .usage = HG_BUFFER_USAGE_INDEX_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
    });
    hg_buffer_write(s_index_buffer, 0, s_indices, sizeof(s_indices));
}

void hg_ray_marcher_shutdown(void) {
    hg_buffer_destroy(s_vertex_buffer);
    hg_buffer_destroy(s_index_buffer);
    hg_shader_destroy(s_shader);
}

void hg_ray_marcher_draw(HgTexture* target, HgMat4* view, f32 aspect) {
    hg_renderpass_begin(target, NULL, false, false);

    hg_shader_bind(s_shader);

    HgPush push_data = {
        .view = *view,
        .aspect = aspect,
    };
    hg_bind_push_constant(&push_data, sizeof(push_data));
    hg_draw(s_vertex_buffer, s_index_buffer, 0);

    hg_renderpass_end();
}

