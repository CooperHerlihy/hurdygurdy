#include "hg_utils.h"

#include "stb_image.h"
#include "stb_image_write.h"

// static void* hg_align_ptr(const void* ptr, usize alignment) {
//     return (void*)(((uintptr_t)ptr + (alignment - 1)) & ~(alignment - 1));
// }

void* hg_heap_alloc(usize size) {
    void* ptr = malloc(size);
    if (ptr == NULL)
        HG_ERRORF("Failed to allocate %zu bytes", size);
    return ptr;
}

void* hg_heap_realloc(void* ptr, usize size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
        HG_ERRORF("Failed to reallocate %zu bytes", size);
    return new_ptr;
}

void hg_heap_free(void* ptr, usize size) {
    if (ptr == NULL)
        HG_ASSERT(size == 0);
    (void)size;
    free(ptr);
}

HgError hg_file_load_binary(const char* path, byte** data, usize* size) {
    HG_ASSERT(path != NULL);
    HG_ASSERT(data != NULL);
    HG_ASSERT(size != NULL);
    *data = NULL;
    *size = 0;

    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return HG_ERROR_FILE_NOT_FOUND;

    fseek(file, 0, SEEK_END);
    usize file_size = (usize)ftell(file);
    rewind(file);

    byte* file_data = hg_heap_alloc(file_size);
    if (fread(file_data, 1, file_size, file) != file_size) {
        fclose(file);
        hg_heap_free(*data, file_size);
        return HG_ERROR_FILE_READ_FAILURE;
    }

    *data = file_data;
    *size = file_size;
    fclose(file);
    return HG_SUCCESS;
}

HgError hg_file_save_binary(const char* path, const byte* data, usize size) {
    HG_ASSERT(path != NULL);
    HG_ASSERT(data != NULL);

    FILE* file = fopen(path, "wb");
    if (file == NULL)
        return HG_ERROR_FILE_NOT_FOUND;

    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        return HG_ERROR_FILE_WRITE_FAILURE;
    }

    fclose(file);
    return HG_SUCCESS;
}

void hg_file_unload_binary(byte* file_data, usize file_size) {
    if (file_data == NULL)
        HG_ASSERT(file_size == 0);
    hg_heap_free(file_data, file_size);
}

HgError hg_file_load_image(const char* path, u32** data, u32* width, u32* height) {
    HG_ASSERT(path != NULL);
    HG_ASSERT(data != NULL);
    HG_ASSERT(width != NULL);
    HG_ASSERT(height != NULL);

    *data = NULL;
    *width = 0;
    *height = 0;

    int file_width, file_height, file_channels;
    stbi_uc* file_data = stbi_load(path, &file_width, &file_height, &file_channels, STBI_rgb_alpha);
    if (file_data == NULL)
        return HG_ERROR_FILE_NOT_FOUND;
    if (file_width <= 0 || file_height <= 0 || file_channels <= 0) {
        stbi_image_free(file_data);
        return HG_ERROR_FILE_READ_FAILURE;
    }

    *data = (u32*)file_data;
    *width = (u32)file_width;
    *height = (u32)file_height;
    return HG_SUCCESS;
}

HgError hg_file_save_image(const char* path, const u32* data, u32 width, u32 height) {
    HG_ASSERT(path != NULL);
    HG_ASSERT(data != NULL);
    HG_ASSERT(width > 0);
    HG_ASSERT(height > 0);

    int result = stbi_write_png(path, (int)width, (int)height, 4, (void*)data, (int)(width * sizeof(u32)));
    if (result == 0)
        return HG_ERROR_FILE_WRITE_FAILURE;

    return HG_SUCCESS;
}

void hg_file_unload_image(u32* data, u32 width, u32 height) {
    stbi_image_free(data);
    (void)width;
    (void)height;
}

f64 hg_clock_tick(HgClock* clock) {
    struct timespec time;
    int result = timespec_get(&time, TIME_UTC);
    if (result == 0)
        HG_ERROR("Failed to get time");

    f64 delta = (f64)(time.tv_sec - clock->time.tv_sec);
    delta += (f64)(time.tv_nsec - clock->time.tv_nsec) / 1.0e9;

    clock->time = time;
    return delta;
}

