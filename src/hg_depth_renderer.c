#include "hg_depth_renderer.h"
#include "hg_graphics_enums.h"

#include "hg_depth.vert.spv.h"
#include "hg_depth.frag.spv.h"

static HgShader* s_shader;

void hg_depth_renderer_init(void) {
    HgDescriptorSetBinding descriptor_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
        .descriptor_count = 1,
    }};
    HgDescriptorSet descriptor_sets[] = {{
        .bindings = descriptor_set_bindings,
        .binding_count = HG_ARRAY_SIZE(descriptor_set_bindings),
    }};

    s_shader = hg_shader_create(&(HgShaderConfig){
        .color_format = HG_FORMAT_R8G8B8A8_UNORM,
        .spirv_vertex_shader = hg_depth_vert_spv,
        .vertex_shader_size = (u32)hg_depth_vert_spv_size,
        .spirv_fragment_shader = hg_depth_frag_spv,
        .fragment_shader_size = (u32)hg_depth_frag_spv_size,
        .descriptor_sets = descriptor_sets,
        .descriptor_set_count = HG_ARRAY_SIZE(descriptor_sets),
        .topology = HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .cull_mode = HG_CULL_MODE_NONE,
    });
}

void hg_depth_renderer_shutdown(void) {
    HG_ASSERT(s_shader != NULL);
    hg_shader_destroy(s_shader);
}

void hg_depth_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer) {
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);
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
        .usage = HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT | HG_TEXTURE_USAGE_SAMPLED_BIT,
    });
}

void hg_depth_render_draw(HgTexture* target, HgTexture* depth_buffer) {
    HG_ASSERT(target != NULL);
    HG_ASSERT(depth_buffer != NULL);

    hg_renderpass_begin(target, NULL, false, false);

    hg_shader_bind(s_shader);

    HgDescriptor depth_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE,
        .count = 1,
        .textures = &depth_buffer,
    }};
    hg_bind_descriptor_set(0, depth_descriptor_set, HG_ARRAY_SIZE(depth_descriptor_set));

    hg_draw(NULL, NULL, 3);

    hg_renderpass_end();
}

