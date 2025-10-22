#include "hg_graphics_enums.h"
#include <hurdy_gurdy.h>

int main(void) {
    hg_init();

    byte* compute_shader;
    usize compute_shader_size;
    HgError shader_result = hg_file_load_binary("build/test.comp.spv", &compute_shader, &compute_shader_size);
    switch (shader_result) {
        case HG_SUCCESS: break;
        case HG_ERROR_UNKNOWN: HG_ERROR("Unknown error");
        case HG_ERROR_FILE_NOT_FOUND: HG_ERROR("Could not find shader file");
        default: HG_ERROR("Unexpected error");
    }

    HgDescriptorSetBinding binding = {
        .descriptor_type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptor_count = 1,
    };
    HgDescriptorSet descriptor_set = {
        .bindings = &binding,
        .binding_count = 1,
    };
    HgShader* shader = hg_compute_shader_create(&(HgComputeShaderConfig){
        .spirv_shader = compute_shader,
        .shader_size = (u32)compute_shader_size,
        .descriptor_sets = &descriptor_set,
        .descriptor_set_count = 1,
        .push_constant_size = 0,
    });

    hg_file_unload_binary(compute_shader, compute_shader_size);

    HgBuffer* pixels = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(u32) * 256 * 256,
        .usage = HG_BUFFER_USAGE_STORAGE_BUFFER_BIT | HG_BUFFER_USAGE_READ_WRITE_SRC_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_DEVICE_LOCAL,
    });

    hg_compute_begin();

    hg_shader_bind(shader);
    hg_bind_descriptor_set(0, &(HgDescriptor){
        .type = HG_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .count = 1,
        .buffers = &pixels,
    }, 1);
    hg_compute_dispatch(8, 8, 1);

    hg_compute_end();

    u32* dst = hg_heap_alloc(256 * 256 * 4);
    hg_buffer_read(pixels, 0, sizeof(u32) * 256 * 256, dst);
    hg_file_save_image("build/test.png", dst, 256, 256);
    hg_heap_free(dst, 256 * 256 * 4);

    hg_buffer_destroy(pixels);
    hg_shader_destroy(shader);

    hg_shutdown();
}

