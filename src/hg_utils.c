#include "hg_utils.h"

// static void* hg_align_ptr(const void* ptr, usize alignment) {
//     return (void*)(((uintptr_t)ptr + (alignment - 1)) & ~(alignment - 1));
// }

void* hg_heap_alloc(usize size) {
    void* ptr = malloc(size);
    if (ptr == NULL)
        HG_ERRORF("Failed to allocate %zu bytes", size);
    return ptr;
}

void hg_heap_free(void* ptr, usize size) {
    if (ptr == NULL)
        HG_ASSERT(size == 0);
    (void)size;
    free(ptr);
}

HgError hg_file_load(const char* path, byte** data, usize* size) {
    HG_ASSERT(path != NULL);
    HG_ASSERT(data != NULL);
    HG_ASSERT(size != NULL);
    *data = NULL;
    *size = 0;

    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return HG_FILE_NOT_FOUND;

    fseek(file, 0, SEEK_END);
    usize file_size = (usize)ftell(file);
    rewind(file);

    byte* file_data = hg_heap_alloc(file_size);
    if (fread(file_data, 1, file_size, file) != file_size) {
        fclose(file);
        hg_heap_free(*data, file_size);
        return HG_FILE_NOT_READABLE;
    }

    *data = file_data;
    *size = file_size;
    fclose(file);
    return HG_SUCCESS;
}

void hg_file_unload(byte* file_data, usize file_size) {
    if (file_data == NULL)
        HG_ASSERT(file_size == 0);
    hg_heap_free(file_data, file_size);
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

