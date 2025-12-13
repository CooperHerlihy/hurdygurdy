#include "hurdygurdy.hpp"

#ifdef _WIN32
#include <malloc.h>
#endif
#ifdef __unix__
#include <alloca.h>
#endif

f64 hg_clock_tick(HgClock *hclock) {
    assert(hclock != NULL);
    f64 prev = (f64)hclock->tv_sec + (f64)hclock->tv_nsec / 1.0e9;
    if (timespec_get(hclock, TIME_UTC) == 0)
        hg_warn("timespec_get failed\n");
    return ((f64)hclock->tv_sec + (f64)hclock->tv_nsec / 1.0e9) - prev;
}

HgMat4 hg_model_matrix_2d(HgVec3 position, HgVec2 scale, f32 rotation) {
    HgMat2 m2 = hg_smat2(1.0f);
    m2.x.x = scale.x;
    m2.y.y = scale.y;
    m2 = hg_mmul2({
        {cosf(rotation), sinf(rotation)},
        {-sinf(rotation), cosf(rotation)},
    }, m2);
    HgMat4 m4 = hg_mat2to4(m2);
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_model_matrix_3d(HgVec3 position, HgVec3 scale, HgQuat rotation) {
    HgMat3 m3 = hg_smat3(1.0f);
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hg_rotate_mat3(rotation, m3);
    HgMat4 m4 = hg_mat3to4(m3);
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hg_view_matrix(HgVec3 position, f32 zoom, HgQuat rotation) {
    HgMat4 rot = hg_mat3to4(hg_rotate_mat3(hg_qconj(rotation), hg_smat3(1.0f)));
    HgMat4 pos = hg_smat4(1.0f);
    pos.x.x = zoom;
    pos.y.y = zoom;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return hg_mmul4(rot, pos);
}

HgMat4 hg_projection_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return {
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hg_projection_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    assert(near > 0.0f);
    assert(far > near);
    f32 scale = 1 / tanf(fov / 2);
    return {
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

u32 hg_max_mipmaps(u32 width, u32 height, u32 depth) {
    return (u32)log2f((f32)hg_max(hg_max(width, height), depth)) + 1;
}

static const HgAllocator hg_internal_std_allocator = {NULL, hg_std_alloc, hg_std_realloc, hg_std_free};

const HgAllocator *hg_persistent_allocator(void) {
    return &hg_internal_std_allocator;
}

thread_local HgAllocator hg_internal_temp_allocator = {NULL, hg_std_alloc, hg_std_realloc, hg_std_free};

HgAllocator *hg_temp_allocator_get(void) {
    return &hg_internal_temp_allocator;
}

void hg_temp_allocator_set(HgAllocator allocator) {
    hg_internal_temp_allocator = allocator;
}

void *hg_std_alloc(void *dummy, usize size, usize alignment) {
    (void)dummy;
    (void)alignment;
    void *allocation = malloc(size);
    assert(allocation != NULL);
    return allocation;
}

void *hg_std_realloc(void *dummy, void *allocation, usize old_size, usize new_size, usize alignment) {
    (void)old_size;
    (void)dummy;
    (void)alignment;
    void *new_allocation = realloc(allocation, new_size);
    assert(new_allocation != NULL);
    return new_allocation;
}

void hg_std_free(void *dummy, void *allocation, usize size, usize alignment) {
    (void)size;
    (void)dummy;
    (void)alignment;
    free(allocation);
}

HgArena hg_arena_create(HgAllocator *allocator, usize capacity) {
    assert(allocator != NULL);

    HgArena arena;
    arena.allocator = allocator;
    arena.data = hg_alloc(allocator, capacity, 16);
    arena.capacity = (u8 *)arena.data + capacity;
    arena.head = arena.data;
    return arena;
}

void hg_arena_destroy(HgArena *arena) {
    assert(arena != NULL);
    hg_free(arena->allocator, arena->data, (usize)arena->capacity - (usize)arena->data, 16);
}

void hg_arena_reset(HgArena *arena) {
    assert(arena != NULL);
    arena->head = 0;
}

void *hg_arena_alloc(HgArena *arena, usize size, usize alignment) {
    assert(arena != NULL);

    void *allocation = (void *)hg_align((usize)arena->head, alignment);

    void *new_head = (u8 *)allocation + size;
    if (new_head > arena->capacity)
        return NULL;
    arena->head = new_head;

    return allocation;
}

void *hg_arena_realloc(HgArena *arena, void *allocation, usize old_size, usize new_size, usize alignment) {
    assert(arena != NULL);

    if ((u8 *)allocation + old_size == arena->head) {
        void *new_head = (u8 *)allocation + new_size;
        if (new_head > arena->capacity)
            return NULL;
        arena->head = new_head;
        return allocation;
    }

    void *new_allocation = hg_arena_alloc(arena, new_size, alignment);
    memcpy(new_allocation, allocation, old_size);
    return new_allocation;
}

void hg_arena_free(HgArena *arena, void *allocation, usize size, usize alignment) {
    (void)arena;
    (void)allocation;
    (void)size;
    (void)alignment;
}

HgAllocator hg_arena_allocator(HgArena *arena) {
    assert(arena != NULL);

    HgAllocator allocator;
    allocator.user_data = arena;
    allocator.alloc = (void *(*)(void *, usize, usize))hg_arena_alloc;
    allocator.realloc = (void *(*)(void *, void *, usize, usize, usize))hg_arena_realloc;
    allocator.free = (void (*)(void *, void *, usize, usize))hg_arena_free;
    return allocator;
}

HgStack hg_stack_create(HgAllocator *allocator, usize capacity) {
    assert(allocator != NULL);

    HgStack stack;
    stack.allocator = allocator;
    stack.data = hg_alloc(allocator, capacity, 16);
    stack.capacity = capacity;
    stack.head = 0;
    return stack;
}

void hg_stack_destroy(HgStack *stack) {
    assert(stack != NULL);
    hg_std_free(stack->allocator, stack->data, stack->capacity, 16);
}

void hg_stack_reset(HgStack *stack) {
    assert(stack != NULL);
    stack->head = 0;
}

void *hg_stack_alloc(HgStack *stack, usize size, usize alignment) {
    (void)alignment;
    assert(stack != NULL);
    if (size == 0)
        return NULL;

    usize new_head = stack->head + hg_align(size, 16);
    if (new_head > stack->capacity)
        return NULL;

    void *allocation = (u8 *)stack->data + stack->head;
    stack->head = new_head;
    return allocation;
}

void *hg_stack_realloc(HgStack *stack, void *allocation, usize old_size, usize new_size, usize alignment) {
    (void)alignment;
    assert(stack != NULL);
    if (new_size == 0) {
        stack->head = (usize)allocation - (usize)stack->data;
        return NULL;
    }

    if ((usize)allocation + hg_align(old_size, 16) == stack->head) {
        usize new_head = (usize)allocation
                       - (usize)stack->data
                       + hg_align(new_size, 16);
        if (new_head > stack->capacity)
            return NULL;
        stack->head = new_head;
        return allocation;
    }

    void *new_allocation = hg_stack_alloc(stack, new_size, 16);
    memcpy(new_allocation, allocation, old_size);
    return new_allocation;
}

void hg_stack_free(HgStack *stack, void *allocation, usize size, usize alignment) {
    (void)alignment;
    assert(stack != NULL);
    if ((usize)allocation + hg_align(size, 16) == stack->head)
        stack->head = (usize)allocation - (usize)stack->data;
    else
        hg_warn("Attempt to free a stack allocation not at the head\n");
}

HgAllocator hg_stack_allocator(HgStack *stack) {
    assert(stack != NULL);

    HgAllocator allocator;
    allocator.user_data = stack;
    allocator.alloc = (void *(*)(void *, usize, usize))hg_stack_alloc;
    allocator.realloc = (void *(*)(void *, void *, usize, usize, usize))hg_stack_realloc;
    allocator.free = (void (*)(void *, void *, usize, usize))hg_stack_free;
    return allocator;
}

bool hg_file_load_binary(HgAllocator *allocator, u8** data, usize* size, const char *path) {
    assert(allocator != NULL);
    assert(data != NULL);
    assert(size != NULL);
    assert(path != NULL);
    *data = NULL;
    *size = 0;

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        hg_warn("Could not find file to read binary: %s\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = (usize)ftell(file);
    rewind(file);

    u8* file_data = (u8 *)hg_alloc(allocator, file_size, 16);
    if (fread(file_data, 1, file_size, file) != file_size) {
        fclose(file);
        hg_warn("Failed to read binary from file: %s\n", path);
        return false;
    }

    *data = file_data;
    *size = file_size;
    fclose(file);
    return true;
}

void hg_file_unload_binary(HgAllocator *allocator, u8* data, usize size) {
    assert(allocator != NULL);
    hg_free(allocator, data, size, 16);
}

bool hg_file_save_binary(const u8* data, usize size, const char *path) {
    if (size == 0)
        assert(data != NULL);
    assert(path != NULL);

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        hg_warn("Failed to create file to write binary: %s\n", path);
        return false;
    }

    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        hg_warn("Failed to write binary data to file: %s\n", path);
        return false;
    }

    fclose(file);
    return true;
}

const char *hg_vk_result_string(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

u32 hg_vk_format_to_size(VkFormat format) {
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_A8_UNORM:
        case VK_FORMAT_R8_BOOL_ARM:
            return 1;

        case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
            return 2;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;

        case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return 8;

        case VK_FORMAT_G8B8G8R8_422_UNORM:
        case VK_FORMAT_B8G8R8G8_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16B16G16R16_422_UNORM:
        case VK_FORMAT_B16G16R16G16_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return 0;

        default:
            hg_warn("Unrecognized Vulkan format value\n");
            return 0;
    }
}

static VkBool32 hg_internal_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        (void)fprintf(stderr, "Vulkan Error: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        (void)fprintf(stderr, "Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                        |  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
        (void)fprintf(stderr, "Vulkan Info: %s\n", callback_data->pMessage);
    } else {
        (void)fprintf(stderr, "Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT hg_internal_debug_utils_messenger_info = {
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    NULL, 0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    hg_internal_debug_callback, NULL,
};

VkInstance hg_vk_create_instance(const char *app_name) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name != NULL ? app_name : "Hurdy Gurdy Application",
    app_info.applicationVersion = 0;
    app_info.pEngineName = "Hurdy Gurdy Engine";
    app_info.engineVersion = 0;
    app_info.apiVersion = VK_API_VERSION_1_3;

#ifndef NDEBUG
    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
#endif

    const char *exts[] = {
#ifndef NDEBUG
        "VK_EXT_debug_utils",
#endif
        "VK_KHR_surface",
#if defined(__linux__)
        "VK_KHR_xlib_surface",
#elif defined(_WIN32)
        "VK_KHR_win32_surface",
#else
#error "unsupported platform"
#endif
    };

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifndef NDEBUG
    instance_info.pNext = &hg_internal_debug_utils_messenger_info;
#endif
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
#ifndef NDEBUG
    instance_info.enabledLayerCount = hg_countof(layers);
    instance_info.ppEnabledLayerNames = layers;
#endif
    instance_info.enabledExtensionCount = hg_countof(exts);
    instance_info.ppEnabledExtensionNames = exts;

    VkInstance instance = NULL;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if (instance == NULL)
        hg_error("Failed to create Vulkan instance: %s\n", hg_vk_result_string(result));

    hg_vk_load_instance(instance);
    return instance;
}

VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance) {
    assert(instance != NULL);

    VkDebugUtilsMessengerEXT messenger = NULL;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        instance, &hg_internal_debug_utils_messenger_info, NULL, &messenger);
    if (messenger == NULL)
        hg_error("Failed to create Vulkan debug messenger: %s\n", hg_vk_result_string(result));

    return messenger;
}

bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32 *queue_family, VkQueueFlags queue_flags) {
    assert(gpu != NULL);

    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, NULL);
    VkQueueFamilyProperties *families = (VkQueueFamilyProperties *)alloca(family_count * sizeof(*families));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families);

    for (u32 i = 0; i < family_count; ++i) {
        if (families[i].queueFlags & queue_flags) {
            if (queue_family != NULL)
                *queue_family = i;
            return true;
        }
    }
    return false;
}

static const char *const hg_internal_vk_device_extensions[] = {
    "VK_KHR_swapchain",
};

static VkPhysicalDevice hg_internal_find_single_queue_gpu(VkInstance instance, u32 *queue_family) {
    assert(instance != NULL);

    u32 gpu_count;
    vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
    VkPhysicalDevice *gpus = (VkPhysicalDevice *)alloca(gpu_count * sizeof(*gpus));
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus);

    u32 ext_prop_count = 0;
    VkExtensionProperties* ext_props = NULL;

    VkPhysicalDevice suitable_gpu = NULL;
    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];

        u32 new_prop_count = 0;
        vkEnumerateDeviceExtensionProperties(gpu, NULL, &new_prop_count, NULL);
        if (new_prop_count > ext_prop_count) {
            ext_prop_count = new_prop_count;
            ext_props = (VkExtensionProperties *)realloc(ext_props, ext_prop_count * sizeof(*ext_props));
        }
        vkEnumerateDeviceExtensionProperties(gpu, NULL, &new_prop_count, ext_props);

        for (usize j = 0; j < hg_countof(hg_internal_vk_device_extensions); j++) {
            for (usize k = 0; k < new_prop_count; k++) {
                if (strcmp(hg_internal_vk_device_extensions[j], ext_props[k].extensionName) == 0)
                    goto next_ext;
            }
            goto next_gpu;
next_ext:
            continue;
        }

        if (!hg_vk_find_queue_family(gpu, queue_family, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            goto next_gpu;

        suitable_gpu = gpu;
        break;

next_gpu:
        continue;
    }

    free(ext_props);
    if (suitable_gpu == NULL)
        hg_warn("Could not find a suitable gpu\n");
    return suitable_gpu;
}

static VkDevice hg_internal_create_single_queue_device(VkPhysicalDevice gpu, u32 queue_family) {
    assert(gpu != NULL);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{};
    dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_feature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2_feature{};
    synchronization2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2_feature.pNext = &dynamic_rendering_feature;
    synchronization2_feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family;
    queue_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &synchronization2_feature;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = hg_countof(hg_internal_vk_device_extensions);
    device_info.ppEnabledExtensionNames = hg_internal_vk_device_extensions;
    device_info.pEnabledFeatures = &features;

    VkDevice device = NULL;
    VkResult result = vkCreateDevice(gpu, &device_info, NULL, &device);

    if (device == NULL)
        hg_error("Could not create Vulkan device: %s\n", hg_vk_result_string(result));
    return device;
}

HgSingleQueueDeviceData hg_vk_create_single_queue_device(VkInstance instance) {
    assert(instance != NULL);

    HgSingleQueueDeviceData device{};
    device.gpu = hg_internal_find_single_queue_gpu(instance, &device.queue_family);
    device.handle = hg_internal_create_single_queue_device(device.gpu, device.queue_family);
    hg_vk_load_device(device.handle);
    vkGetDeviceQueue(device.handle, device.queue_family, 0, &device.queue);

    return device;
}

VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig *config) {
    assert(device != NULL);
    assert(config != NULL);
    if (config->color_attachment_count > 0)
        assert(config->color_attachment_formats != NULL);
    assert(config->shader_stages != NULL);
    assert(config->shader_count > 0);
    assert(config->layout != NULL);
    if (config->vertex_binding_count > 0)
        assert(config->vertex_bindings != NULL);

    VkPipelineVertexInputStateCreateInfo vertex_input_state{};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount = config->vertex_binding_count;
    vertex_input_state.pVertexBindingDescriptions = config->vertex_bindings;
    vertex_input_state.vertexAttributeDescriptionCount = config->vertex_attribute_count;
    vertex_input_state.pVertexAttributeDescriptions = config->vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = config->topology;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellation_state{};
    tessellation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_state.patchControlPoints = config->tesselation_patch_control_points;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state{};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = config->polygon_mode;
    rasterization_state.cullMode = config->cull_mode;
    rasterization_state.frontFace = config->front_face;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state{};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = config->multisample_count != 0
        ? config->multisample_count
        : VK_SAMPLE_COUNT_1_BIT,
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = NULL;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthWriteEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthCompareOp = config->enable_color_blend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable = config->depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.stencilTestEnable = config->stencil_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
    // depth_stencil_state.front = {}; : TODO
    // depth_stencil_state.back = {}; : TODO
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = config->enable_color_blend ? VK_TRUE : VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment;
    color_blend_state.blendConstants[0] = {1.0f};
    color_blend_state.blendConstants[1] = {1.0f};
    color_blend_state.blendConstants[2] = {1.0f};
    color_blend_state.blendConstants[3] = {1.0f};

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = hg_countof(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = config->color_attachment_count;
    rendering_info.pColorAttachmentFormats = config->color_attachment_formats;
    rendering_info.depthAttachmentFormat = config->depth_attachment_format;
    rendering_info.stencilAttachmentFormat = config->stencil_attachment_format;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = &rendering_info;
    pipeline_info.stageCount = config->shader_count;
    pipeline_info.pStages = config->shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.pTessellationState = &tessellation_state;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = config->layout;
    pipeline_info.basePipelineHandle = NULL;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = NULL;
    VkResult result = vkCreateGraphicsPipelines(device, NULL, 1, &pipeline_info, NULL, &pipeline);
    if (pipeline == NULL)
        hg_error("Failed to create Vulkan graphics pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig *config) {
    assert(device != NULL);
    assert(config != NULL);
    assert(config->color_attachment_count == 0);
    assert(config->color_attachment_formats == NULL);
    assert(config->depth_attachment_format == VK_FORMAT_UNDEFINED);
    assert(config->stencil_attachment_format == VK_FORMAT_UNDEFINED);
    assert(config->shader_stages != NULL);
    assert(config->shader_stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    assert(config->shader_count == 1);
    assert(config->layout != NULL);
    assert(config->vertex_binding_count == 0);
    assert(config->vertex_bindings == NULL);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = config->shader_stages[0];
    pipeline_info.layout = config->layout;
    pipeline_info.basePipelineHandle = NULL;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = NULL;
    VkResult result = vkCreateComputePipelines(device, NULL, 1, &pipeline_info, NULL, &pipeline);
    if (pipeline == NULL)
        hg_error("Failed to create Vulkan compute pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

u32 hg_vk_find_memory_type_index(
    VkPhysicalDevice gpu,
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags
) {
    assert(gpu != NULL);
    assert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & undesired_flags) != 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type without undesired flags\n");
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type with desired flags\n");
        return i;
    }
    hg_error("Could not find Vulkan memory type\n");
}

static VkFormat hg_internal_vk_find_swapchain_format(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    assert(gpu != NULL);
    assert(surface != NULL);

    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, NULL);
    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)alloca(format_count * sizeof(*formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats);

    for (usize i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hg_error("No supported swapchain formats\n");
}

static VkPresentModeKHR hg_internal_vk_find_swapchain_present_mode(
    VkPhysicalDevice gpu,
    VkSurfaceKHR surface,
    VkPresentModeKHR desired_mode
) {
    assert(gpu != NULL);
    assert(surface != NULL);

    if (desired_mode == VK_PRESENT_MODE_FIFO_KHR)
        return desired_mode;

    u32 mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, NULL);
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)alloca(mode_count * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, present_modes);

    for (usize i = 0; i < mode_count; ++i) {
        if (present_modes[i] == desired_mode)
            return desired_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

HgSwapchainData hg_vk_create_swapchain(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode
) {
    assert(device != NULL);
    assert(gpu != NULL);
    assert(surface != NULL);
    assert(image_usage != 0);

    HgSwapchainData swapchain{};

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities);

    if (surface_capabilities.currentExtent.width == 0 ||
        surface_capabilities.currentExtent.height == 0 ||
        surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width ||
        surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height ||
        surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width ||
        surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height
    ) {
        hg_warn("Could not create swapchain of the surface's size\n");
        return swapchain;
    }

    swapchain.width = surface_capabilities.currentExtent.width;
    swapchain.height = surface_capabilities.currentExtent.height;
    swapchain.format = hg_internal_vk_find_swapchain_format(gpu, surface);

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_info.imageFormat = swapchain.format;
    swapchain_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = image_usage;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = hg_internal_vk_find_swapchain_present_mode(gpu, surface, desired_mode);
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;

    VkResult result = vkCreateSwapchainKHR(device, &swapchain_info, NULL, &swapchain.handle);
    if (swapchain.handle == NULL)
        hg_error("Failed to create swapchain: %s\n", hg_vk_result_string(result));

    return swapchain;
}

HgSwapchainCommands hg_swapchain_commands_create(VkDevice device, VkSwapchainKHR swapchain, VkCommandPool cmd_pool) {
    assert(device != NULL);
    assert(cmd_pool != NULL);
    assert(swapchain != NULL);

    HgSwapchainCommands sync;
    sync.cmd_pool = cmd_pool;
    sync.swapchain = swapchain;

    vkGetSwapchainImagesKHR(device, swapchain, &sync.frame_count, NULL);

    void *allocation = hg_alloc(hg_persistent_allocator(),
        sync.frame_count * sizeof(*sync.cmds) +
        sync.frame_count * sizeof(*sync.frame_finished) +
        sync.frame_count * sizeof(*sync.image_available) +
        sync.frame_count * sizeof(*sync.ready_to_present),
        16);

    sync.cmds = (VkCommandBuffer *)allocation;
    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = NULL;
    cmd_alloc_info.commandPool = cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = sync.frame_count;

    vkAllocateCommandBuffers(device, &cmd_alloc_info, sync.cmds);

    sync.frame_finished = (VkFence *)(sync.cmds + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &info, NULL, &sync.frame_finished[i]);
    }

    sync.image_available = (VkSemaphore *)(sync.frame_finished + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &info, NULL, &sync.image_available[i]);
    }

    sync.ready_to_present = (VkSemaphore *)(sync.image_available + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &info, NULL, &sync.ready_to_present[i]);
    }

    return sync;
}

void hg_swapchain_commands_destroy(VkDevice device, HgSwapchainCommands *sync) {
    assert(sync != NULL);

    vkFreeCommandBuffers(device, sync->cmd_pool, sync->frame_count, sync->cmds);
    for (usize i = 0; i < sync->frame_count; ++i) {
        vkDestroyFence(device, sync->frame_finished[i], NULL);
    }
    for (usize i = 0; i < sync->frame_count; ++i) {
        vkDestroySemaphore(device, sync->image_available[i], NULL);
    }
    for (usize i = 0; i < sync->frame_count; ++i) {
        vkDestroySemaphore(device, sync->ready_to_present[i], NULL);
    }
    hg_std_free(NULL,
        sync->cmds,
        sync->frame_count * sizeof(*sync->cmds) +
        sync->frame_count * sizeof(*sync->frame_finished) +
        sync->frame_count * sizeof(*sync->image_available) +
        sync->frame_count * sizeof(*sync->ready_to_present),
        16);
    memset(sync, 0, sizeof(*sync));
}

VkCommandBuffer hg_swapchain_commands_record(VkDevice device, HgSwapchainCommands *sync) {
    assert(sync != NULL);

    sync->current_frame = (sync->current_frame + 1) % sync->frame_count;

    vkWaitForFences(device, 1, &sync->frame_finished[sync->current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &sync->frame_finished[sync->current_frame]);

    VkResult result = vkAcquireNextImageKHR(
        device,
        sync->swapchain,
        UINT64_MAX,
        sync->image_available[sync->current_frame],
        NULL,
        &sync->current_image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return NULL;


    VkCommandBuffer cmd = sync->cmds[sync->current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

void hg_swapchain_commands_present(VkQueue queue, HgSwapchainCommands *sync) {
    assert(queue != NULL);
    assert(sync != NULL);

    VkCommandBuffer cmd = sync->cmds[sync->current_frame];
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &sync->image_available[sync->current_frame];
    VkPipelineStageFlags stage_flags = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
    submit.pWaitDstStageMask = &stage_flags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &sync->ready_to_present[sync->current_image];

    vkQueueSubmit(queue, 1, &submit, sync->frame_finished[sync->current_frame]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &sync->ready_to_present[sync->current_image];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &sync->swapchain;
    present_info.pImageIndices = &sync->current_image;

    vkQueuePresentKHR(queue, &present_info);
}

void hg_vk_buffer_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    VkBuffer dst,
    usize offset,
    void *src,
    usize size
) {
    assert(device != NULL);
    assert(allocator != NULL);
    assert(cmd_pool != NULL);
    assert(transfer_queue != NULL);
    assert(dst != NULL);
    assert(src != NULL);
    assert(size > 0);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = NULL;
    VmaAllocation stage_alloc = NULL;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, NULL);
    vmaCopyMemoryToAllocation(allocator, src, stage_alloc, offset, size);

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = NULL;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, stage, dst, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = NULL;
    vkCreateFence(device, &fence_info, NULL, &fence);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, NULL);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(allocator, stage, stage_alloc);
}

void hg_vk_buffer_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    void *dst,
    VkBuffer src,
    usize offset,
    usize size
) {
    assert(device != NULL);
    assert(allocator != NULL);
    assert(cmd_pool != NULL);
    assert(transfer_queue != NULL);
    assert(dst != NULL);
    assert(src != NULL);
    assert(size > 0);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = NULL;
    VmaAllocation stage_alloc = NULL;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, NULL);

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = NULL;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, src, stage, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = NULL;
    vkCreateFence(device, &fence_info, NULL, &fence);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(allocator, stage_alloc, offset, dst, size);

    vkDestroyFence(device, fence, NULL);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(allocator, stage, stage_alloc);
}

void hg_vk_image_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgVkImageStagingWriteConfig *config
) {
    assert(device != NULL);
    assert(allocator != NULL);
    assert(cmd_pool != NULL);
    assert(transfer_queue != NULL);
    assert(config->dst_image != NULL);
    assert(config->src_data != NULL);
    assert(config->width > 0);
    assert(config->height > 0);
    assert(config->depth > 0);
    assert(config->format != VK_FORMAT_UNDEFINED);

    usize size = config->width
               * config->height
               * config->depth
               * hg_vk_format_to_size(config->format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = NULL;
    VmaAllocation stage_alloc = NULL;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, NULL);
    vmaCopyMemoryToAllocation(allocator, config->src_data, stage_alloc, 0, size);

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = NULL;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transfer_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transfer_barrier.image = config->dst_image;
    transfer_barrier.subresourceRange.aspectMask = config->subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config->subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config->subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config->subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config->subresource;
    region.imageExtent = {config->width, config->height, config->depth};

    vkCmdCopyBufferToImage(cmd, stage, config->dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (config->layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        VkImageMemoryBarrier2 end_barrier{};
        end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        end_barrier.newLayout = config->layout;
        end_barrier.srcQueueFamilyIndex = 0;
        end_barrier.dstQueueFamilyIndex = 0;
        end_barrier.image = config->dst_image;
        end_barrier.subresourceRange.aspectMask = config->subresource.aspectMask;
        end_barrier.subresourceRange.baseMipLevel = config->subresource.mipLevel;
        end_barrier.subresourceRange.levelCount = 1;
        end_barrier.subresourceRange.baseArrayLayer = config->subresource.baseArrayLayer;
        end_barrier.subresourceRange.layerCount = config->subresource.layerCount;

        VkDependencyInfo end_dep{};
        end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        end_dep.imageMemoryBarrierCount = 1;
        end_dep.pImageMemoryBarriers = &end_barrier;

        vkCmdPipelineBarrier2(cmd, &end_dep);
    }

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = NULL;
    vkCreateFence(device, &fence_info, NULL, &fence);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, NULL);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(allocator, stage, stage_alloc);
}

void hg_vk_image_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgVkImageStagingReadConfig *config
) {
    assert(device != NULL);
    assert(allocator != NULL);
    assert(cmd_pool != NULL);
    assert(transfer_queue != NULL);
    assert(config->src_image != NULL);
    assert(config->layout != VK_IMAGE_LAYOUT_UNDEFINED);
    assert(config->dst != NULL);
    assert(config->width > 0);
    assert(config->height > 0);
    assert(config->depth > 0);
    assert(config->format != VK_FORMAT_UNDEFINED);

    usize size = config->width
               * config->height
               * config->depth
               * hg_vk_format_to_size(config->format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = NULL;
    VmaAllocation stage_alloc = NULL;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, NULL);

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = NULL;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transfer_barrier.oldLayout = config->layout;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transfer_barrier.image = config->src_image;
    transfer_barrier.subresourceRange.aspectMask = config->subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config->subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config->subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config->subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config->subresource;
    region.imageExtent = {config->width, config->height, config->depth};

    vkCmdCopyImageToBuffer(cmd, config->src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage, 1, &region);

    VkImageMemoryBarrier2 end_barrier{};
    end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    end_barrier.newLayout = config->layout;
    end_barrier.image = config->src_image;
    end_barrier.subresourceRange.aspectMask = config->subresource.aspectMask;
    end_barrier.subresourceRange.baseMipLevel = config->subresource.mipLevel;
    end_barrier.subresourceRange.levelCount = 1;
    end_barrier.subresourceRange.baseArrayLayer = config->subresource.baseArrayLayer;
    end_barrier.subresourceRange.layerCount = config->subresource.layerCount;

    VkDependencyInfo end_dep{};
    end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    end_dep.imageMemoryBarrierCount = 1;
    end_dep.pImageMemoryBarriers = &end_barrier;

    vkCmdPipelineBarrier2(cmd, &end_dep);

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = NULL;
    vkCreateFence(device, &fence_info, NULL, &fence);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(allocator, stage_alloc, 0, config->dst, size);

    vkDestroyFence(device, fence, NULL);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    vmaDestroyBuffer(allocator, stage, stage_alloc);
}

HgECS hg_ecs_create(
    const HgAllocator *allocator,
    u32 max_entities,
    const HgSystemDescription *systems,
    u32 system_count
) {
    assert(allocator != NULL);
    assert(max_entities > 0);

    max_entities += 1;
    HgECS ecs{};
    ecs.allocator = allocator;
    ecs.entity_pool = (HgEntityID *)hg_alloc(allocator, max_entities * sizeof(HgEntityID), alignof(HgEntityID));
    ecs.entity_capacity = (u32)max_entities;
    ecs.systems = (HgSystem *)hg_alloc(allocator, max_entities * sizeof(HgSystem), alignof(HgSystem));
    ecs.system_count = (u32)system_count;

    for (u32 i = 0; i < ecs.entity_capacity; ++i) {
        ecs.entity_pool[i] = i + 1;
    }
    HgEntityID reserved_null_entity = hg_entity_create(&ecs);
    (void)reserved_null_entity;

    for (u32 i = 0; i < ecs.system_count; ++i) {
        u32 max_components = systems[i].max_components + 1;

        ecs.systems[i].data = hg_alloc(ecs.allocator, systems[i].data_size, 16);
        ecs.systems[i].data_size = systems[i].data_size;

        ecs.systems[i].entity_indices = (u32 *)hg_alloc(
            ecs.allocator,
            ecs.entity_capacity * sizeof(u32),
            alignof(u32));
        ecs.systems[i].component_entities = (HgEntityID *)hg_alloc(
            ecs.allocator,
            max_components * sizeof(HgEntityID),
            alignof(HgEntityID));
        ecs.systems[i].components = hg_alloc(
            ecs.allocator,
            max_components * systems[i].component_size,
            systems[i].component_alignment);

        ecs.systems[i].component_size = (u32)systems[i].component_size;
        ecs.systems[i].component_alignment = (u32)systems[i].component_alignment;
        ecs.systems[i].component_capacity = max_components;
        ecs.systems[i].component_count = 1;

        memset(ecs.systems[i].entity_indices, 0, max_components);
        memset(ecs.systems[i].component_entities, 0, max_components);
    }

    return ecs;
}

void hg_ecs_destroy(HgECS *ecs) {
    assert(ecs != NULL);

    for (u32 i = 0; i < ecs->system_count; ++i) {
        hg_free(
            ecs->allocator,
            ecs->systems[i].data,
            ecs->systems[i].data_size,
            16);
        hg_free(
            ecs->allocator,
            ecs->systems[i].entity_indices,
            ecs->entity_capacity * sizeof(u32),
            alignof(u32)),
        hg_free(
            ecs->allocator,
            ecs->systems[i].component_entities,
            ecs->systems[i].component_capacity * sizeof(HgEntityID),
            alignof(HgEntityID)),
        hg_free(
            ecs->allocator,
            ecs->systems[i].components,
            ecs->systems[i].component_capacity * ecs->systems[i].component_size,
            ecs->systems[i].component_alignment);
    }

    hg_free(
        ecs->allocator,
        ecs->entity_pool,
        ecs->entity_capacity * sizeof(HgEntityID),
        alignof(HgEntityID));
    hg_free(
        ecs->allocator,
        ecs->systems,
        ecs->system_count * sizeof(HgSystem),
        alignof(HgSystem));
}

void hg_ecs_reset(HgECS *ecs) {
    assert(ecs != NULL);

    for (u32 i = 0; i < ecs->entity_capacity; ++i) {
        ecs->entity_pool[i] = i + 1;
    }
    HgEntityID reserved_null_entity = hg_entity_create(ecs);
    (void)reserved_null_entity;

    for (u32 i = 0; i < ecs->system_count; ++i) {
        memset(ecs->systems[i].entity_indices, 0, ecs->systems[i].component_count);
        memset(ecs->systems[i].component_entities, 0, ecs->systems[i].component_count);
        ecs->systems[i].component_count = 1;
    }
}

void *hg_ecs_get_system(HgECS *ecs, u32 system_index) {
    return ecs->systems[system_index].data;
}

// add component removal queue : TODO
void hg_ecs_flush_system(HgECS *ecs, u32 system_index) {
    assert(ecs != NULL);
    assert(system_index < ecs->system_count);

    HgSystem *system = &ecs->systems[system_index];
    for (u32 i = 1; i < system->component_count; ++i) {
        if (system->component_entities[i] == 0) {
            --system->component_count;

            void *current = (u8 *)system->components + i * system->component_size;
            void *last = (u8 *)system->components + system->component_count * system->component_size;
            memcpy(current, last, system->component_size);

            HgEntityID entity = system->component_entities[i];
            HgEntityID last_entity = system->component_entities[system->component_count];

            system->component_entities[i] = last_entity;
            system->component_entities[system->component_count] = 0;

            system->entity_indices[entity] = 0;
            system->entity_indices[last_entity] = i;
        }
    }
}

bool hg_ecs_iterate_system(HgECS *ecs, u32 system_index, HgEntityID **iterator) {
    assert(ecs != NULL);
    assert(system_index < ecs->system_count);
    assert(iterator != NULL);

    HgSystem *system = &ecs->systems[system_index];
    if (*iterator == NULL) {
        *iterator = system->component_entities;
    } else {
        assert(*iterator > system->component_entities);
        assert(*iterator <= system->component_entities + system->component_count);
    }

    while (*iterator != system->component_entities + system->component_count) {
        *iterator += 1;
        if (**iterator != 0)
            return true;
    }
    return false;
}

HgEntityID hg_entity_create(HgECS *ecs) {
    assert(ecs != NULL);

    HgEntityID entity = ecs->entity_next;
    ecs->entity_next = ecs->entity_pool[entity & 0xffffffff];
    ecs->entity_pool[entity & 0xffffffff] = 0;
    return entity;
}

void hg_entity_destroy(HgECS *ecs, HgEntityID entity) {
    assert(ecs != NULL);
    assert(entity != 0 && ecs->entity_pool[entity & 0xffffffff] == 0);

    for (u32 i = 0; i < ecs->system_count; ++i) {
        HgSystem *system = &ecs->systems[i];
        system->component_entities[system->entity_indices[entity & 0xffffffff]] = 0;
        system->entity_indices[entity & 0xffffffff] = 0;
    }
    ecs->entity_pool[entity & 0xffffffff] = ecs->entity_next;
    ecs->entity_next = entity + ((u64)0x1 << 32);
}

bool hg_entity_is_alive(HgECS *ecs, HgEntityID entity) {
    assert(ecs != NULL);
    return entity != 0 && ecs->entity_pool[entity & 0xffffffff] == 0;
}

void *hg_entity_add_component(HgECS *ecs, HgEntityID entity, u32 system_index) {
    assert(ecs != NULL);
    assert(entity != 0);
    assert(system_index < ecs->system_count);

    HgSystem *system = &ecs->systems[system_index];
    assert(system->entity_indices[entity & 0xffffffff] == 0);

    u32 index = system->component_count;
    ++system->component_count;

    assert(system->entity_indices[entity & 0xffffff] == 0);
    system->entity_indices[entity & 0xffffffff] = index;

    assert(system->component_entities[index] == 0);
    system->component_entities[index] = entity;

    return (u8 *)system->components + index * system->component_size;
}

void hg_entity_remove_component(HgECS *ecs, HgEntityID entity, u32 system_index) {
    assert(ecs != NULL);
    assert(entity != 0);
    assert(system_index < ecs->system_count);

    HgSystem *system = &ecs->systems[system_index];

    u32 index = system->entity_indices[entity & 0xffffffff];

    assert(index != 0);
    system->entity_indices[entity & 0xffffffff] = 0;

    assert(system->component_entities[index] == entity);
    system->component_entities[index] = 0;
}

bool hg_entity_has_component(HgECS *ecs, HgEntityID entity, u32 system_index) {
    assert(ecs != NULL);
    assert(entity != 0);
    assert(system_index < ecs->system_count);
    return ecs->systems[system_index].entity_indices[entity & 0xffffffff] != 0;
}

void *hg_entity_get_component(HgECS *ecs, HgEntityID entity, u32 system_index) {
    assert(ecs != NULL);
    assert(entity != 0);
    assert(system_index < ecs->system_count);

    HgSystem *system = &ecs->systems[system_index];
    u32 index = system->entity_indices[entity & 0xffffffff];

    assert(index != 0);
    assert(system->component_entities[index] == entity);
    return (u8 *)system->components + index * system->component_size;
}

HgEntityID hg_entity_from_component(HgECS *ecs, void *component, u32 system_index) {
    assert(ecs != NULL);
    assert(component != NULL);
    assert(system_index < ecs->system_count);

    HgSystem *system = &ecs->systems[system_index];

    u32 index = (u32)(((usize)component - (usize)system->components) / system->component_size);
    HgEntityID entity = system->component_entities[index];

    assert(entity != 0);
    assert(system->entity_indices[entity & 0xffffffff] == index);
    return entity;
}

#include "sprite.frag.spv.h"
#include "sprite.vert.spv.h"

typedef struct HgPipelineSpriteVPUniform {
    HgMat4 proj;
    HgMat4 view;
} HgPipelineSpriteVPUniform;

HgPipelineSprite hg_pipeline_sprite_create(
    VkDevice device,
    VmaAllocator allocator,
    VkFormat color_format,
    VkFormat depth_format
) {
    assert(device != NULL);
    assert(color_format != VK_FORMAT_UNDEFINED);

    HgPipelineSprite pipeline;
    pipeline.device = device;
    pipeline.allocator = allocator;

    VkDescriptorSetLayoutBinding vp_bindings[1]{};
    vp_bindings[0].binding = 0;
    vp_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vp_bindings[0].descriptorCount = 1;
    vp_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo vp_layout_info{};
    vp_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vp_layout_info.bindingCount = hg_countof(vp_bindings);
    vp_layout_info.pBindings = vp_bindings;

    vkCreateDescriptorSetLayout(device, &vp_layout_info, NULL, &pipeline.vp_layout);

    VkDescriptorSetLayoutBinding image_bindings[1]{};
    image_bindings[0].binding = 0;
    image_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    image_bindings[0].descriptorCount = 1;
    image_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo image_layout_info{};
    image_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    image_layout_info.bindingCount = hg_countof(image_bindings);
    image_layout_info.pBindings = image_bindings;

    vkCreateDescriptorSetLayout(device, &image_layout_info, NULL, &pipeline.image_layout);

    VkDescriptorSetLayout set_layouts[] = {pipeline.vp_layout, pipeline.image_layout};
    VkPushConstantRange push_ranges[1]{};
    push_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_ranges[0].size = sizeof(HgPipelineSpritePush);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = hg_countof(set_layouts);
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = hg_countof(push_ranges);
    layout_info.pPushConstantRanges = push_ranges;

    vkCreatePipelineLayout(device, &layout_info, NULL, &pipeline.pipeline_layout);

    VkShaderModuleCreateInfo vertex_shader_info{};
    vertex_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_info.codeSize = sprite_vert_spv_size;
    vertex_shader_info.pCode = (u32 *)sprite_vert_spv;

    VkShaderModule vertex_shader;
    vkCreateShaderModule(device, &vertex_shader_info, NULL, &vertex_shader);

    VkShaderModuleCreateInfo fragment_shader_info{};
    fragment_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_info.codeSize = sprite_frag_spv_size;
    fragment_shader_info.pCode = (u32 *)sprite_frag_spv;

    VkShaderModule fragment_shader;
    vkCreateShaderModule(device, &fragment_shader_info, NULL, &fragment_shader);

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    HgVkPipelineConfig pipeline_config{};
    pipeline_config.color_attachment_formats = &color_format;
    pipeline_config.color_attachment_count = 1;
    pipeline_config.depth_attachment_format = depth_format;
    pipeline_config.stencil_attachment_format = VK_FORMAT_UNDEFINED;
    pipeline_config.shader_stages = shader_stages;
    pipeline_config.shader_count = hg_countof(shader_stages);
    pipeline_config.layout = pipeline.pipeline_layout;
    pipeline_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipeline_config.enable_color_blend = true;

    pipeline.pipeline = hg_vk_create_graphics_pipeline(device, &pipeline_config);

    vkDestroyShaderModule(device, fragment_shader, NULL);
    vkDestroyShaderModule(device, vertex_shader, NULL);

    VkDescriptorPoolSize desc_pool_sizes[2]{};
    desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_pool_sizes[0].descriptorCount = 1;
    desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_pool_sizes[1].descriptorCount = 255;

    VkDescriptorPoolCreateInfo desc_pool_info{};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc_pool_info.maxSets = 256;
    desc_pool_info.poolSizeCount = hg_countof(desc_pool_sizes);
    desc_pool_info.pPoolSizes = desc_pool_sizes;

    vkCreateDescriptorPool(device, &desc_pool_info, NULL, &pipeline.descriptor_pool);

    VkDescriptorSetAllocateInfo vp_set_alloc_info{};
    vp_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vp_set_alloc_info.descriptorPool = pipeline.descriptor_pool;
    vp_set_alloc_info.descriptorSetCount = 1;
    vp_set_alloc_info.pSetLayouts = &pipeline.vp_layout;

    vkAllocateDescriptorSets(device, &vp_set_alloc_info, &pipeline.vp_set);

    VkBufferCreateInfo vp_buffer_info{};
    vp_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vp_buffer_info.size = sizeof(HgPipelineSpriteVPUniform);
    vp_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo vp_alloc_info{};
    vp_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vp_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(
        allocator,
        &vp_buffer_info,
        &vp_alloc_info,
        &pipeline.vp_buffer,
        &pipeline.vp_buffer_allocation,
        NULL);

    HgPipelineSpriteVPUniform vp_data{};
    vp_data.proj = hg_smat4(1.0f);
    vp_data.view = hg_smat4(1.0f);

    vmaCopyMemoryToAllocation(allocator, &vp_data, pipeline.vp_buffer_allocation, 0, sizeof(vp_data));

    VkDescriptorBufferInfo desc_info{};
    desc_info.buffer = pipeline.vp_buffer;
    desc_info.offset = 0;
    desc_info.range = sizeof(HgPipelineSpriteVPUniform);

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = pipeline.vp_set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_write.pBufferInfo = &desc_info;

    vkUpdateDescriptorSets(device, 1, &desc_write, 0, NULL);

    return pipeline;
}

void hg_pipeline_sprite_destroy(HgPipelineSprite *pipeline) {
    if (pipeline == NULL)
        return;

    vmaDestroyBuffer(pipeline->allocator, pipeline->vp_buffer, pipeline->vp_buffer_allocation);
    vkFreeDescriptorSets(pipeline->device, pipeline->descriptor_pool, 1, &pipeline->vp_set);
    vkDestroyDescriptorPool(pipeline->device, pipeline->descriptor_pool, NULL);
    vkDestroyPipeline(pipeline->device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(pipeline->device, pipeline->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(pipeline->device, pipeline->image_layout, NULL);
    vkDestroyDescriptorSetLayout(pipeline->device, pipeline->vp_layout, NULL);
}

void hg_pipeline_sprite_update_projection(HgPipelineSprite *pipeline, HgMat4 *projection) {
    assert(pipeline != NULL);
    assert(projection != NULL);

    vmaCopyMemoryToAllocation(
        pipeline->allocator,
        projection,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, proj),
        sizeof(*projection));
}

void hg_pipeline_sprite_update_view(HgPipelineSprite *pipeline, HgMat4 *view) {
    assert(pipeline != NULL);
    assert(view != NULL);

    vmaCopyMemoryToAllocation(
        pipeline->allocator,
        view,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, view),
        sizeof(*view));
}

HgPipelineSpriteTexture hg_pipeline_sprite_create_texture(
    HgPipelineSprite *pipeline,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgPipelineSpriteTextureConfig *config
) {
    assert(pipeline != NULL);
    assert(config != NULL);
    assert(config->tex_data != NULL);
    assert(config->width > 0);
    assert(config->height > 0);
    assert(config->format != VK_FORMAT_UNDEFINED);

    HgPipelineSpriteTexture tex;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = config->format;
    image_info.extent = {config->width, config->height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(pipeline->allocator, &image_info, &alloc_info, &tex.image, &tex.allocation, NULL);

    HgVkImageStagingWriteConfig staging_config{};
    staging_config.dst_image = tex.image;
    staging_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    staging_config.subresource.layerCount = 1;
    staging_config.src_data = config->tex_data;
    staging_config.width = config->width;
    staging_config.height = config->height;
    staging_config.depth = 1;
    staging_config.format = config->format;
    staging_config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(pipeline->device, pipeline->allocator, cmd_pool, transfer_queue, &staging_config);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = tex.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = config->format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(pipeline->device, &view_info, NULL, &tex.view);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = config->filter;
    sampler_info.minFilter = config->filter;
    sampler_info.addressModeU = config->edge_mode;
    sampler_info.addressModeV = config->edge_mode;
    sampler_info.addressModeW = config->edge_mode;

    vkCreateSampler(pipeline->device, &sampler_info, NULL, &tex.sampler);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = pipeline->descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &pipeline->image_layout;

    vkAllocateDescriptorSets(pipeline->device, &set_info, &tex.set);

    VkDescriptorImageInfo desc_info{};
    desc_info.sampler = tex.sampler;
    desc_info.imageView = tex.view;
    desc_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = tex.set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_write.pImageInfo = &desc_info;

    vkUpdateDescriptorSets(pipeline->device, 1, &desc_write, 0, NULL);

    return tex;
}

void hg_pipeline_sprite_destroy_texture(HgPipelineSprite *pipeline, HgPipelineSpriteTexture *texture) {
    assert(pipeline != NULL);
    assert(texture != NULL);

    vkFreeDescriptorSets(pipeline->device, pipeline->descriptor_pool, 1, &texture->set);
    vkDestroySampler(pipeline->device, texture->sampler, NULL);
    vkDestroyImageView(pipeline->device, texture->view, NULL);
    vmaDestroyImage(pipeline->allocator, texture->image, texture->allocation);
}

void hg_pipeline_sprite_bind(HgPipelineSprite *pipeline, VkCommandBuffer cmd) {
    assert(cmd != NULL);
    assert(pipeline != NULL);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        0,
        1,
        &pipeline->vp_set,
        0,
        NULL);
}

void hg_pipeline_sprite_draw(
    HgPipelineSprite *pipeline,
    VkCommandBuffer cmd,
    HgPipelineSpriteTexture *texture,
    HgPipelineSpritePush *push_data
) {
    assert(cmd != NULL);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        1,
        1,
        &texture->set,
        0,
        NULL);

    vkCmdPushConstants(
        cmd,
        pipeline->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(*push_data),
        push_data);

    vkCmdDraw(cmd, 4, 1, 0, 0);
}

typedef struct HgWindowInput {
    u32 width;
    u32 height;
    f64 mouse_pos_x;
    f64 mouse_pos_y;
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    bool was_resized;
    bool was_closed;
    bool keys_down[HG_KEY_COUNT];
    bool keys_pressed[HG_KEY_COUNT];
    bool keys_released[HG_KEY_COUNT];
} HgWindowInput;

#if defined(__linux__)

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

typedef struct HgX11Funcs {
    Display *(*XOpenDisplay)(_Xconst char*);
    int (*XCloseDisplay)(Display*);
    Window (*XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int,
        unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    int (*XDestroyWindow)(Display*, Window);
    int (*XStoreName)(Display*, Window, _Xconst char*);
    Atom (*XInternAtom)(Display*, _Xconst char*, Bool);
    Status (*XSetWMProtocols)(Display*, Window, Atom*, int);
    int (*XMapWindow)(Display*, Window);
    Status (*XSendEvent)(Display*, Window, Bool, long, XEvent*);
    int (*XFlush)(Display*);
    int (*XNextEvent)(Display*, XEvent*);
    int (*XPending)(Display*);
    KeySym (*XLookupKeysym)(XKeyEvent*, int);
} HgX11Funcs;

static void *hg_internal_libx11 = NULL;
static HgX11Funcs hg_internal_x11_funcs{};

Display *XOpenDisplay(_Xconst char *name) {
    return hg_internal_x11_funcs.XOpenDisplay(name);
}

int XCloseDisplay(Display *dpy) {
    return hg_internal_x11_funcs.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display *dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int xclass,
    Visual *visual,
    unsigned long valuemask,
    XSetWindowAttributes *attributes
) {
    return hg_internal_x11_funcs.XCreateWindow(
        dpy, parent, x, y, width, height, border_width,
        depth, xclass, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display *dpy, Window w) {
    return hg_internal_x11_funcs.XDestroyWindow(dpy, w);
}

int XStoreName(Display *dpy, Window w, _Xconst char *name) {
    return hg_internal_x11_funcs.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display *dpy, _Xconst char *name, Bool only_if_exists) {
    return hg_internal_x11_funcs.XInternAtom(dpy, name, only_if_exists);
}

Status XSetWMProtocols(Display *dpy, Window w, Atom *protocols, int count) {
    return hg_internal_x11_funcs.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display *dpy, Window w) {
    return hg_internal_x11_funcs.XMapWindow(dpy, w);
}

Status XSendEvent(Display *dpy, Window w, Bool propagate, long event_mask, XEvent *event) {
    return hg_internal_x11_funcs.XSendEvent(dpy, w, propagate, event_mask, event);
}

int XFlush(Display *dpy) {
    return hg_internal_x11_funcs.XFlush(dpy);
}

int XNextEvent(Display *dpy, XEvent *event) {
    return hg_internal_x11_funcs.XNextEvent(dpy, event);
}

int XPending(Display *dpy) {
    return hg_internal_x11_funcs.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent *key_event, int index) {
    return hg_internal_x11_funcs.XLookupKeysym(key_event, index);
}

#define HG_LOAD_X11_FUNC(name) *(void **)&hg_internal_x11_funcs. name \
    = dlsym(hg_internal_libx11, #name); \
    if (hg_internal_x11_funcs. name == NULL) { hg_error("Could not load Xlib function: \n" #name); }

Display *hg_internal_x11_display = NULL;

static void hg_internal_platform_init(void) {
    if (hg_internal_libx11 == NULL) {
        hg_internal_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
        if (hg_internal_libx11 == NULL)
            hg_error("Could not open Xlib\n");

        HG_LOAD_X11_FUNC(XOpenDisplay);
        HG_LOAD_X11_FUNC(XCloseDisplay);
        HG_LOAD_X11_FUNC(XCreateWindow);
        HG_LOAD_X11_FUNC(XDestroyWindow);
        HG_LOAD_X11_FUNC(XStoreName);
        HG_LOAD_X11_FUNC(XInternAtom);
        HG_LOAD_X11_FUNC(XSetWMProtocols);
        HG_LOAD_X11_FUNC(XMapWindow);
        HG_LOAD_X11_FUNC(XSendEvent);
        HG_LOAD_X11_FUNC(XFlush);
        HG_LOAD_X11_FUNC(XPending);
        HG_LOAD_X11_FUNC(XNextEvent);
        HG_LOAD_X11_FUNC(XLookupKeysym);
    }

    hg_internal_x11_display = XOpenDisplay(NULL);
    if (hg_internal_x11_display == NULL)
        hg_error("Could not open X display\n");
}

static void hg_internal_platform_exit(void) {
    XCloseDisplay(hg_internal_x11_display);
}

static Window hg_internal_create_x11_window(
    Display *display,
    u32 width,
    u32 height,
    const char *title
) {
    XSetWindowAttributes window_attributes{};
    window_attributes.event_mask
        = KeyPressMask | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | StructureNotifyMask;
    Window window = XCreateWindow(
        display, RootWindow(display, DefaultScreen(display)),
        0, 0,
        width,
        height,
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &window_attributes
    );
    if (window == ~0U)
        hg_error("X11 could not create window\n");

    if (title != NULL) {
        int name_result = XStoreName(display, window, title);
        if (name_result == 0)
            hg_error("X11 could not set window title\n");
    }

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        hg_error("X11 could not map window\n");

    return window;
}

static Atom hg_internal_set_delete_behavior(
    Display* display,
    Window window
) {
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (delete_atom == None)
        hg_error("X11 could not get WM_DELETE_WINDOW atom\n");

    int set_protocols_result = XSetWMProtocols(
        display,
        window,
        &delete_atom,
        1
    );
    if (set_protocols_result == 0)
        hg_error("X11 could not set WM_DELETE_WINDOW protocol\n");

    return delete_atom;
}

static void hg_internal_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        hg_error("X11 failed to get state atom\n");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        hg_error("X11 failed to get fullscreen atom\n");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = state_atom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = (long)fullscreen_atom;

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event
    );
    if (fullscreen_result == 0)
        hg_error("X11 could not send fullscreen message\n");
}

struct HgWindow {
    HgWindowInput input;
    Window x11_window;
    Atom delete_atom;
};

HgWindow *hg_window_create(const HgWindowConfig *config) {
    assert(config != NULL);

    u32 width = config->windowed ? config->width
        : (u32)DisplayWidth(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));
    u32 height = config->windowed ? config->height
        : (u32)DisplayHeight(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));

    HgWindow *window = (HgWindow *)hg_alloc(hg_persistent_allocator(), sizeof(*window), 16);
    *window = {};
    window->input.width = width;
    window->input.height = height;

    window->x11_window = hg_internal_create_x11_window(hg_internal_x11_display, width, height, config->title);

    window->delete_atom = hg_internal_set_delete_behavior(hg_internal_x11_display, window->x11_window);

    if (!config->windowed)
        hg_internal_set_fullscreen(hg_internal_x11_display, window->x11_window);

    int flush_result = XFlush(hg_internal_x11_display);
    if (flush_result == 0)
        hg_error("X11 could not flush window\n");

    return window;
}

void hg_window_destroy(HgWindow *window) {
    if (window != NULL)
        XDestroyWindow(hg_internal_x11_display, window->x11_window);
    XFlush(hg_internal_x11_display);
    hg_std_free(NULL, window, sizeof(*window), 16);
}

void hg_window_set_icon(HgWindow *window, u32 *icon_data, u32 width, u32 height);

bool hg_window_get_fullscreen(HgWindow *window);

void hg_window_set_fullscreen(HgWindow *window, bool fullscreen);

void hg_window_set_cursor(HgWindow *window, HgCursor cursor);

void hg_window_set_cursor_image(HgWindow *window, u32 *data, u32 width, u32 height);

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, const HgWindow *window) {
    assert(instance != NULL);
    assert(window != NULL);

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == NULL)
        hg_error("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = hg_internal_x11_display;
    info.window = window->x11_window;

    VkSurfaceKHR surface = NULL;
    VkResult result = pfn_vkCreateXlibSurfaceKHR(instance, &info, NULL, &surface);
    if (surface == NULL)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    return surface;
}

void hg_window_process_events(HgWindow **windows, u32 window_count) {
    assert(windows != NULL);
    assert(window_count > 0);

    if (window_count > 1)
        hg_error("Multiple windows unsupported\n"); // : TODO
    HgWindow* window = windows[0];

    memset(window->input.keys_pressed, 0, sizeof(window->input.keys_pressed));
    memset(window->input.keys_released, 0, sizeof(window->input.keys_released));
    window->input.was_resized = false;

    u32 old_window_width = window->input.width;
    u32 old_window_height = window->input.height;
    f64 old_mouse_pos_x = window->input.mouse_pos_x;
    f64 old_mouse_pos_y = window->input.mouse_pos_y;

    while (XPending(hg_internal_x11_display)) {
        XEvent event;
        int event_result = XNextEvent(hg_internal_x11_display, &event);
        if (event_result != 0)
            hg_error("X11 could not get next event\n");

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window->delete_atom)
                    window->input.was_closed = true;
                break;
            case ConfigureNotify:
                window->input.width = (u32)event.xconfigure.width;
                window->input.height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HG_KEY_NONE;
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_0:
                        key = HG_KEY_0;
                        break;
                    case XK_1:
                        key = HG_KEY_1;
                        break;
                    case XK_2:
                        key = HG_KEY_2;
                        break;
                    case XK_3:
                        key = HG_KEY_3;
                        break;
                    case XK_4:
                        key = HG_KEY_4;
                        break;
                    case XK_5:
                        key = HG_KEY_5;
                        break;
                    case XK_6:
                        key = HG_KEY_6;
                        break;
                    case XK_7:
                        key = HG_KEY_7;
                        break;
                    case XK_8:
                        key = HG_KEY_8;
                        break;
                    case XK_9:
                        key = HG_KEY_9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HG_KEY_Q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HG_KEY_W;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HG_KEY_E;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HG_KEY_R;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HG_KEY_T;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HG_KEY_Y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HG_KEY_U;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HG_KEY_I;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HG_KEY_O;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HG_KEY_P;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HG_KEY_A;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HG_KEY_S;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HG_KEY_D;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HG_KEY_F;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HG_KEY_G;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HG_KEY_H;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HG_KEY_J;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HG_KEY_K;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HG_KEY_L;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HG_KEY_Z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HG_KEY_X;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HG_KEY_C;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HG_KEY_V;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HG_KEY_B;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HG_KEY_N;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HG_KEY_M;
                        break;

                    case XK_semicolon:
                        key = HG_KEY_SEMICOLON;
                        break;
                    case XK_colon:
                        key = HG_KEY_COLON;
                        break;
                    case XK_apostrophe:
                        key = HG_KEY_APOSTROPHE;
                        break;
                    case XK_quotedbl:
                        key = HG_KEY_QUOTATION;
                        break;
                    case XK_comma:
                        key = HG_KEY_COMMA;
                        break;
                    case XK_period:
                        key = HG_KEY_PERIOD;
                        break;
                    case XK_question:
                        key = HG_KEY_QUESTION;
                        break;
                    case XK_grave:
                        key = HG_KEY_GRAVE;
                        break;
                    case XK_asciitilde:
                        key = HG_KEY_TILDE;
                        break;
                    case XK_exclam:
                        key = HG_KEY_EXCLAMATION;
                        break;
                    case XK_at:
                        key = HG_KEY_AT;
                        break;
                    case XK_numbersign:
                        key = HG_KEY_HASH;
                        break;
                    case XK_dollar:
                        key = HG_KEY_DOLLAR;
                        break;
                    case XK_percent:
                        key = HG_KEY_PERCENT;
                        break;
                    case XK_asciicircum:
                        key = HG_KEY_CAROT;
                        break;
                    case XK_ampersand:
                        key = HG_KEY_AMPERSAND;
                        break;
                    case XK_asterisk:
                        key = HG_KEY_ASTERISK;
                        break;
                    case XK_parenleft:
                        key = HG_KEY_LPAREN;
                        break;
                    case XK_parenright:
                        key = HG_KEY_RPAREN;
                        break;
                    case XK_bracketleft:
                        key = HG_KEY_LBRACKET;
                        break;
                    case XK_bracketright:
                        key = HG_KEY_RBRACKET;
                        break;
                    case XK_braceleft:
                        key = HG_KEY_LBRACE;
                        break;
                    case XK_braceright:
                        key = HG_KEY_RBRACE;
                        break;
                    case XK_equal:
                        key = HG_KEY_EQUAL;
                        break;
                    case XK_less:
                        key = HG_KEY_LESS;
                        break;
                    case XK_greater:
                        key = HG_KEY_GREATER;
                        break;
                    case XK_plus:
                        key = HG_KEY_PLUS;
                        break;
                    case XK_minus:
                        key = HG_KEY_MINUS;
                        break;
                    case XK_slash:
                        key = HG_KEY_SLASH;
                        break;
                    case XK_backslash:
                        key = HG_KEY_BACKSLASH;
                        break;
                    case XK_underscore:
                        key = HG_KEY_UNDERSCORE;
                        break;
                    case XK_bar:
                        key = HG_KEY_BAR;
                        break;

                    case XK_Up:
                        key = HG_KEY_UP;
                        break;
                    case XK_Down:
                        key = HG_KEY_DOWN;
                        break;
                    case XK_Left:
                        key = HG_KEY_LEFT;
                        break;
                    case XK_Right:
                        key = HG_KEY_RIGHT;
                        break;
                    case XK_Escape:
                        key = HG_KEY_ESCAPE;
                        break;
                    case XK_space:
                        key = HG_KEY_SPACE;
                        break;
                    case XK_Return:
                        key = HG_KEY_ENTER;
                        break;
                    case XK_BackSpace:
                        key = HG_KEY_BACKSPACE;
                        break;
                    case XK_Delete:
                        key = HG_KEY_DELETE;
                        break;
                    case XK_Insert:
                        key = HG_KEY_INSERT;
                        break;
                    case XK_Tab:
                        key = HG_KEY_TAB;
                        break;
                    case XK_Home:
                        key = HG_KEY_HOME;
                        break;
                    case XK_End:
                        key = HG_KEY_END;
                        break;

                    case XK_F1:
                        key = HG_KEY_F1;
                        break;
                    case XK_F2:
                        key = HG_KEY_F2;
                        break;
                    case XK_F3:
                        key = HG_KEY_F3;
                        break;
                    case XK_F4:
                        key = HG_KEY_F4;
                        break;
                    case XK_F5:
                        key = HG_KEY_F5;
                        break;
                    case XK_F6:
                        key = HG_KEY_F6;
                        break;
                    case XK_F7:
                        key = HG_KEY_F7;
                        break;
                    case XK_F8:
                        key = HG_KEY_F8;
                        break;
                    case XK_F9:
                        key = HG_KEY_F9;
                        break;
                    case XK_F10:
                        key = HG_KEY_F10;
                        break;
                    case XK_F11:
                        key = HG_KEY_F11;
                        break;
                    case XK_F12:
                        key = HG_KEY_F12;
                        break;

                    case XK_Shift_L:
                        key = HG_KEY_LSHIFT;
                        break;
                    case XK_Shift_R:
                        key = HG_KEY_RSHIFT;
                        break;
                    case XK_Control_L:
                        key = HG_KEY_LCTRL;
                        break;
                    case XK_Control_R:
                        key = HG_KEY_RCTRL;
                        break;
                    case XK_Meta_L:
                        key = HG_KEY_LMETA;
                        break;
                    case XK_Meta_R:
                        key = HG_KEY_RMETA;
                        break;
                    case XK_Alt_L:
                        key = HG_KEY_LALT;
                        break;
                    case XK_Alt_R:
                        key = HG_KEY_RALT;
                        break;
                    case XK_Super_L:
                        key = HG_KEY_LSUPER;
                        break;
                    case XK_Super_R:
                        key = HG_KEY_RSUPER;
                        break;
                    case XK_Caps_Lock:
                        key = HG_KEY_CAPSLOCK;
                        break;
                }
                if (event.type == KeyPress) {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                } else if (event.type == KeyRelease) {
                    window->input.keys_released[key] = true;
                    window->input.keys_down[key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HG_KEY_NONE;
                switch (event.xbutton.button) {
                    case Button1:
                        key = HG_KEY_MOUSE1;
                        break;
                    case Button2:
                        key = HG_KEY_MOUSE2;
                        break;
                    case Button3:
                        key = HG_KEY_MOUSE3;
                        break;
                    case Button4:
                        key = HG_KEY_MOUSE4;
                        break;
                    case Button5:
                        key = HG_KEY_MOUSE5;
                        break;
                }
                if (event.type == ButtonPress) {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                } else if (event.type == ButtonRelease) {
                    window->input.keys_released[key] = true;
                    window->input.keys_down[key] = false;
                }
            } break;
            case MotionNotify:
                window->input.mouse_pos_x = (f64)event.xmotion.x / (f64)window->input.height;
                window->input.mouse_pos_y = (f64)event.xmotion.y / (f64)window->input.height;
                break;
            default:
                break;
        }
    }

    if (window->input.width != old_window_width || window->input.height != old_window_height) {
        window->input.was_resized = true;
    }

    window->input.mouse_delta_x = window->input.mouse_pos_x - old_mouse_pos_x;
    window->input.mouse_delta_y = window->input.mouse_pos_y - old_mouse_pos_y;
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

HINSTANCE hg_internal_win32_instance = NULL;

void hg_internal_platform_init(void) {
    hg_internal_win32_instance = GetModuleHandle(NULL);
}

void hg_internal_platform_exit(void) {
}

struct HgWindow {
    HgWindowInput input;
    HWND hwnd;
};

static LRESULT CALLBACK hg_internal_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HgWindow *window = (HgWindow *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA *)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->input.was_closed = true;
            break;
        case WM_SIZE:
            window->input.width = LOWORD(lparam);
            window->input.height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            memset(window->input.keys_down, 0, sizeof(window->input.keys_down));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HG_KEY_NONE;
            HgKey shift_key = HG_KEY_NONE;

            switch (wparam) {
                case '0':
                    key = HG_KEY_0;
                    shift_key = HG_KEY_RPAREN;
                    break;
                case '1':
                    key = HG_KEY_1;
                    shift_key = HG_KEY_EXCLAMATION;
                    break;
                case '2':
                    key = HG_KEY_2;
                    shift_key = HG_KEY_AT;
                    break;
                case '3':
                    key = HG_KEY_3;
                    shift_key = HG_KEY_HASH;
                    break;
                case '4':
                    key = HG_KEY_4;
                    shift_key = HG_KEY_DOLLAR;
                    break;
                case '5':
                    key = HG_KEY_5;
                    shift_key = HG_KEY_PERCENT;
                    break;
                case '6':
                    key = HG_KEY_6;
                    shift_key = HG_KEY_CAROT;
                    break;
                case '7':
                    key = HG_KEY_7;
                    shift_key = HG_KEY_AMPERSAND;
                    break;
                case '8':
                    key = HG_KEY_8;
                    shift_key = HG_KEY_ASTERISK;
                    break;
                case '9':
                    key = HG_KEY_9;
                    shift_key = HG_KEY_LPAREN;
                    break;

                case 'A':
                    key = HG_KEY_A;
                    break;
                case 'B':
                    key = HG_KEY_B;
                    break;
                case 'C':
                    key = HG_KEY_C;
                    break;
                case 'D':
                    key = HG_KEY_D;
                    break;
                case 'E':
                    key = HG_KEY_E;
                    break;
                case 'F':
                    key = HG_KEY_F;
                    break;
                case 'G':
                    key = HG_KEY_G;
                    break;
                case 'H':
                    key = HG_KEY_H;
                    break;
                case 'I':
                    key = HG_KEY_I;
                    break;
                case 'J':
                    key = HG_KEY_J;
                    break;
                case 'K':
                    key = HG_KEY_K;
                    break;
                case 'L':
                    key = HG_KEY_L;
                    break;
                case 'M':
                    key = HG_KEY_M;
                    break;
                case 'N':
                    key = HG_KEY_N;
                    break;
                case 'O':
                    key = HG_KEY_O;
                    break;
                case 'P':
                    key = HG_KEY_P;
                    break;
                case 'Q':
                    key = HG_KEY_Q;
                    break;
                case 'R':
                    key = HG_KEY_R;
                    break;
                case 'S':
                    key = HG_KEY_S;
                    break;
                case 'T':
                    key = HG_KEY_T;
                    break;
                case 'U':
                    key = HG_KEY_U;
                    break;
                case 'V':
                    key = HG_KEY_V;
                    break;
                case 'W':
                    key = HG_KEY_W;
                    break;
                case 'X':
                    key = HG_KEY_X;
                    break;
                case 'Y':
                    key = HG_KEY_Y;
                    break;
                case 'Z':
                    key = HG_KEY_Z;
                    break;

                case VK_OEM_1:
                    key = HG_KEY_SEMICOLON;
                    shift_key = HG_KEY_COLON;
                    break;
                case VK_OEM_7:
                    key = HG_KEY_APOSTROPHE;
                    shift_key = HG_KEY_QUOTATION;
                    break;
                case VK_OEM_COMMA:
                    key = HG_KEY_COMMA;
                    shift_key = HG_KEY_LESS;
                    break;
                case VK_OEM_PERIOD:
                    key = HG_KEY_PERIOD;
                    shift_key = HG_KEY_GREATER;
                    break;
                case VK_OEM_2:
                    key = HG_KEY_SLASH;
                    shift_key = HG_KEY_QUESTION;
                    break;
                case VK_OEM_3:
                    key = HG_KEY_GRAVE;
                    shift_key = HG_KEY_TILDE;
                    break;
                case VK_OEM_4:
                    key = HG_KEY_LBRACKET;
                    shift_key = HG_KEY_LBRACE;
                    break;
                case VK_OEM_6:
                    key = HG_KEY_RBRACKET;
                    shift_key = HG_KEY_RBRACE;
                    break;
                case VK_OEM_5:
                    key = HG_KEY_BACKSLASH;
                    shift_key = HG_KEY_BAR;
                    break;
                case VK_OEM_PLUS:
                    key = HG_KEY_EQUAL;
                    shift_key = HG_KEY_PLUS;
                    break;
                case VK_OEM_MINUS:
                    key = HG_KEY_MINUS;
                    shift_key = HG_KEY_UNDERSCORE;
                    break;

                case VK_UP:
                    key = HG_KEY_UP;
                    break;
                case VK_DOWN:
                    key = HG_KEY_DOWN;
                    break;
                case VK_LEFT:
                    key = HG_KEY_LEFT;
                    break;
                case VK_RIGHT:
                    key = HG_KEY_RIGHT;
                    break;
                case VK_ESCAPE:
                    key = HG_KEY_ESCAPE;
                    break;
                case VK_SPACE:
                    key = HG_KEY_SPACE;
                    break;
                case VK_RETURN:
                    key = HG_KEY_ENTER;
                    break;
                case VK_BACK:
                    key = HG_KEY_BACKSPACE;
                    break;
                case VK_DELETE:
                    key = HG_KEY_DELETE;
                    break;
                case VK_INSERT:
                    key = HG_KEY_INSERT;
                    break;
                case VK_TAB:
                    key = HG_KEY_TAB;
                    break;
                case VK_HOME:
                    key = HG_KEY_HOME;
                    break;
                case VK_END:
                    key = HG_KEY_END;
                    break;

                case VK_F1:
                    key = HG_KEY_F1;
                    break;
                case VK_F2:
                    key = HG_KEY_F2;
                    break;
                case VK_F3:
                    key = HG_KEY_F3;
                    break;
                case VK_F4:
                    key = HG_KEY_F4;
                    break;
                case VK_F5:
                    key = HG_KEY_F5;
                    break;
                case VK_F6:
                    key = HG_KEY_F6;
                    break;
                case VK_F7:
                    key = HG_KEY_F7;
                    break;
                case VK_F8:
                    key = HG_KEY_F8;
                    break;
                case VK_F9:
                    key = HG_KEY_F9;
                    break;
                case VK_F10:
                    key = HG_KEY_F10;
                    break;
                case VK_F11:
                    key = HG_KEY_F11;
                    break;
                case VK_F12:
                    key = HG_KEY_F12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HG_KEY_RSHIFT;
                    else if (scancode == 0x2A)
                        key = HG_KEY_LSHIFT;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RALT;
                    else
                        key = HG_KEY_LALT;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RCTRL;
                    else
                        key = HG_KEY_LCTRL;
                    break;
                case VK_LWIN:
                    key = HG_KEY_LSUPER;
                    break;
                case VK_RWIN:
                    key = HG_KEY_RSUPER;
                    break;
                case VK_CAPITAL:
                    key = HG_KEY_CAPSLOCK;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
                if (shift_key != HG_KEY_NONE &&
                   (window->input.keys_down[HG_KEY_LSHIFT] ||
                    window->input.keys_down[HG_KEY_RSHIFT])
                ) {
                    window->input.keys_pressed[shift_key] = true;
                    window->input.keys_down[shift_key] = true;
                } else {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                }
            } else {
                window->input.keys_released[shift_key] = window->input.keys_down[shift_key];
                window->input.keys_down[shift_key] = false;
                window->input.keys_released[key] = window->input.keys_down[key];
                window->input.keys_down[key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = true;
            break;
        case WM_RBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = true;
            break;
        case WM_MBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = true;
            break;
        case WM_LBUTTONUP:
            window->input.keys_released[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = false;
            break;
        case WM_RBUTTONUP:
            window->input.keys_released[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = false;
            break;
        case WM_MBUTTONUP:
            window->input.keys_released[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = false;
            break;
        case WM_MOUSEMOVE:
            window->input.mouse_pos_x = (f64)LOWORD(lparam) / (f64)window->input.height;
            window->input.mouse_pos_y = (f64)HIWORD(lparam) / (f64)window->input.height;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow *hg_window_create(const HgWindowConfig *config) {
    const char *title = config->title != NULL ? config->title : "Hurdy Gurdy";

    HgWindow *window = hg_alloc(hg_persistent_allocator(), sizeof(*window), 16);
    *window = {};
    window->input.width = width;
    window->input.height = height;

    WNDCLASSA window_class{};
    window_class.hInstance = hg_internal_win32_instance;
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = title;
    window_class.lpfnWndProc = hg_internal_window_callback;
    if (!RegisterClassA(&window_class))
        hg_error("Win32 failed to register window class for window: %s\n", config->title);

    if (config->windowed) {
        window->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window->input.width,
            window->input.height,
            NULL,
            NULL,
            hg_internal_win32_instance,
            window
        );
    } else {
        window->input.width = GetSystemMetrics(SM_CXSCREEN);
        window->input.height = GetSystemMetrics(SM_CYSCREEN);
        window->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window->input.width,
            window->input.height,
            NULL,
            NULL,
            hg_internal_win32_instance,
            window
        );
    }
    if (window->hwnd == NULL)
        hg_error("Win32 window creation failed\n");

    ShowWindow(window->hwnd, SW_SHOW);
    return window;
}

void hg_window_destroy(HgWindow *window) {
    if (window != NULL)
        DestroyWindow(window->hwnd);
}

void hg_window_set_icon(HgWindow *window, u32 *icon_data, u32 width, u32 height);

bool hg_window_get_fullscreen(HgWindow *window);

void hg_window_set_fullscreen(HgWindow *window, bool fullscreen);

void hg_window_set_cursor(HgWindow *window, HgCursor cursor);

void hg_window_set_cursor_image(HgWindow *window, u32 *data, u32 width, u32 height);

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, const HgWindow *window) {
    assert(instance != NULL);
    assert(window != NULL);

    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (pfn_vkCreateWin32SurfaceKHR == NULL)
        hg_error("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = hg_internal_win32_instance;
    info.hwnd = window->hwnd;

    VkSurfaceKHR surface = NULL;
    VkResult result = pfn_vkCreateWin32SurfaceKHR(instance, &info, NULL, &surface);
    if (surface == NULL)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    assert(surface != NULL);
    return surface;
}

void hg_window_process_events(HgWindow **windows, u32 window_count) {
    for (usize i = 0; i < window_count; ++i) {
        memset(windows[i]->input.keys_pressed, 0, sizeof(windows[i]->input.keys_pressed));
        memset(windows[i]->input.keys_released, 0, sizeof(windows[i]->input.keys_released));
        windows[i]->input.was_resized = false;

        u32 old_window_width = windows[i]->input.width;
        u32 old_window_height = windows[i]->input.height;
        f64 old_mouse_pos_x = windows[i]->input.mouse_pos_x;
        f64 old_mouse_pos_y = windows[i]->input.mouse_pos_y;

        MSG msg;
        while (PeekMessageA(&msg, windows[i]->hwnd, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (windows[i]->input.width != old_window_width || windows[i]->input.height != old_window_height) {
            windows[i]->input.was_resized = true;
        }

        windows[i]->input.mouse_delta_x = windows[i]->input.mouse_pos_x - old_mouse_pos_x;
        windows[i]->input.mouse_delta_y = windows[i]->input.mouse_pos_y - old_mouse_pos_y;

        if (windows[i]->input.keys_down[HG_KEY_LSHIFT] && windows[i]->input.keys_down[HG_KEY_RSHIFT]) {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift) {
                windows[i]->input.keys_released[HG_KEY_LSHIFT] = true;
                windows[i]->input.keys_down[HG_KEY_LSHIFT] = false;
            }
            if (!rshift) {
                windows[i]->input.keys_released[HG_KEY_RSHIFT] = true;
                windows[i]->input.keys_down[HG_KEY_RSHIFT] = false;
            }
        }
    }
}

#else

#error "unsupported platform"

#endif

bool hg_window_was_closed(const HgWindow *window) {
    assert(window != NULL);
    return window->input.was_closed;
}

bool hg_window_was_resized(const HgWindow *window) {
    assert(window != NULL);
    return window->input.was_resized;
}

void hg_window_get_size(const HgWindow *window, u32 *width, u32 *height) {
    assert(window != NULL);
    *width = window->input.width;
    *height = window->input.height;
}

void hg_window_get_mouse_pos(const HgWindow *window, f64 *x, f64 *y) {
    assert(window != NULL);
    assert(x != NULL);
    assert(y != NULL);
    *x = window->input.mouse_pos_x;
    *y = window->input.mouse_pos_y;
}

void hg_window_get_mouse_delta(const HgWindow *window, f64 *x, f64 *y) {
    assert(window != NULL);
    assert(x != NULL);
    assert(y != NULL);
    *x = window->input.mouse_delta_x;
    *y = window->input.mouse_delta_y;
}

bool hg_window_is_key_down(const HgWindow *window, HgKey key) {
    assert(window != NULL);
    assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_down[key];
}

bool hg_window_was_key_pressed(const HgWindow *window, HgKey key) {
    assert(window != NULL);
    assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_pressed[key];
}

bool hg_window_was_key_released(const HgWindow *window, HgKey key) {
    assert(window != NULL);
    assert(key >= 0 && key < HG_KEY_COUNT);
    return window->input.keys_released[key];
}

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name

typedef struct hgVulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);

    HG_MAKE_VULKAN_FUNC(vkCreateInstance);
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance);

    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);

    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);

    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateDevice);

    HG_MAKE_VULKAN_FUNC(vkDestroyDevice);
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);
    HG_MAKE_VULKAN_FUNC(vkCreateFence);
    HG_MAKE_VULKAN_FUNC(vkDestroyFence);
    HG_MAKE_VULKAN_FUNC(vkResetFences);
    HG_MAKE_VULKAN_FUNC(vkWaitForFences);

    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkFreeDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);

    HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateImage);
    HG_MAKE_VULKAN_FUNC(vkDestroyImage);
    HG_MAKE_VULKAN_FUNC(vkCreateImageView);
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
    HG_MAKE_VULKAN_FUNC(vkCreateSampler);
    HG_MAKE_VULKAN_FUNC(vkDestroySampler);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);

    HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
    HG_MAKE_VULKAN_FUNC(vkFreeMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
    HG_MAKE_VULKAN_FUNC(vkMapMemory);
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);

    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);

    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);

    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport);
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor);
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline);
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants);
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers);
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdDraw);
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed);
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch);
} hgVulkanFuncs;

#undef HG_MAKE_VULKAN_FUNC

static void *hg_internal_libvulkan = NULL;
static hgVulkanFuncs hg_internal_vulkan_funcs{};

#define HG_LOAD_VULKAN_FUNC(name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(NULL, #name); \
    if (hg_internal_vulkan_funcs. name == NULL) { \
        hg_error("Could not load " #name "\n"); \
    }

void hg_vk_load(void) {

#if defined(__unix__)

    hg_internal_libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (hg_internal_libvulkan == NULL)
        hg_error("Could not load vulkan dynamic lib: %s\n", dlerror());

    *(void **)&hg_internal_vulkan_funcs.vkGetInstanceProcAddr = dlsym(hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == NULL)
        hg_error("Could not load vkGetInstanceProcAddr: %s\n", dlerror());

#elif defined(_WIN32)

    hg_internal_libvulkan = (void *)LoadLibraryA("vulkan-1.dll");
    if (hg_internal_libvulkan == NULL)
        hg_error("Could not load vulkan dynamic lib\n");

    hg_internal_vulkan_funcs.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress(hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == NULL)
        hg_error("Could not load vkGetInstanceProcAddr\n");

#else

#error "unsupported platform"

#endif

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

#undef HG_LOAD_VULKAN_FUNC

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, #name); \
    if (hg_internal_vulkan_funcs. name == NULL) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_instance(VkInstance instance) {
    assert(instance != NULL);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyInstance);
    hg_debug_mode(HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDebugUtilsMessengerEXT));
    hg_debug_mode(HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyDebugUtilsMessengerEXT));
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroySurfaceKHR)
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDevice)
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, #name); \
    if (hg_internal_vulkan_funcs. name == NULL) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_device(VkDevice device) {
    assert(device != NULL);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDevice)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDeviceWaitIdle)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetSwapchainImagesKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAcquireNextImageKHR)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkWaitForFences);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceQueue);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueWaitIdle);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueSubmit);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueuePresentKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeCommandBuffers);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUpdateDescriptorSets);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreatePipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateGraphicsPipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateComputePipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipeline);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSampler);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySampler);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkMapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUnmapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFlushMappedMemoryRanges);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkInvalidateMappedMemoryRanges);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBeginCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkEndCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetCommandBuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBlitImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBufferToImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImageToBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPipelineBarrier2);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBeginRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdEndRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetViewport);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetScissor);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindPipeline);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPushConstants);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindVertexBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindIndexBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDraw);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDrawIndexed);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDispatch);
}

#undef HG_LOAD_VULKAN_DEVICE_FUNC

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    return hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    return hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    return hg_internal_vulkan_funcs.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyInstance(instance, pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) {
    return hg_internal_vulkan_funcs.vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pCount, VkPhysicalDevice *pDevices) {
    return hg_internal_vulkan_funcs.vkEnumeratePhysicalDevices(instance, pCount, pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProps) {
    return hg_internal_vulkan_funcs.vkEnumerateDeviceExtensionProperties(device, pLayerName, pCount, pProps);
}

void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, uint32_t *pCount, VkQueueFamilyProperties *pProps) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceQueueFamilyProperties(device, pCount, pProps);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult vkCreateDevice(VkPhysicalDevice device, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    return hg_internal_vulkan_funcs.vkCreateDevice(device, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDevice(device, pAllocator);
}

VkResult vkDeviceWaitIdle(VkDevice device) {
    return hg_internal_vulkan_funcs.vkDeviceWaitIdle(device);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkSurfaceFormatKHR *pFormats) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, pCount, pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkPresentModeKHR *pModes) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, pCount, pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pCaps) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, pCaps);
}

VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    return hg_internal_vulkan_funcs.vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount, VkImage *pImages) {
    return hg_internal_vulkan_funcs.vkGetSwapchainImagesKHR(device, swapchain, pCount, pImages);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t *pIndex) {
    return hg_internal_vulkan_funcs.vkAcquireNextImageKHR(device, swapchain, timeout, sem, fence, pIndex);
}

VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    return hg_internal_vulkan_funcs.vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySemaphore(device, sem, pAllocator);
}

VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    return hg_internal_vulkan_funcs.vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(VkDevice device, uint32_t count, const VkFence *pFences) {
    return hg_internal_vulkan_funcs.vkResetFences(device, count, pFences);
}

VkResult vkWaitForFences(VkDevice device, uint32_t count, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
    return hg_internal_vulkan_funcs.vkWaitForFences(device, count, pFences, waitAll, timeout);
}

void vkGetDeviceQueue(VkDevice device, uint32_t family, uint32_t index, VkQueue *pQueue) {
    hg_internal_vulkan_funcs.vkGetDeviceQueue(device, family, index, pQueue);
}

VkResult vkQueueWaitIdle(VkQueue queue) {
    return hg_internal_vulkan_funcs.vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo *pSubmits, VkFence fence) {
    return hg_internal_vulkan_funcs.vkQueueSubmit(queue, count, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pInfo) {
    return hg_internal_vulkan_funcs.vkQueuePresentKHR(queue, pInfo);
}

VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pPool) {
    return hg_internal_vulkan_funcs.vkCreateCommandPool(device, pCreateInfo, pAllocator, pPool);
}

void vkDestroyCommandPool(VkDevice device, VkCommandPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyCommandPool(device, pool, pAllocator);
}

VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pInfo, VkCommandBuffer *pBufs) {
    return hg_internal_vulkan_funcs.vkAllocateCommandBuffers(device, pInfo, pBufs);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer *pBufs) {
    hg_internal_vulkan_funcs.vkFreeCommandBuffers(device, pool, count, pBufs);
}

VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pPool) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorPool(device, pInfo, pAllocator, pPool);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorPool(device, pool, pAllocator);
}

VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool pool, uint32_t flags) {
    return hg_internal_vulkan_funcs.vkResetDescriptorPool(device, pool, flags);
}

VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pInfo, VkDescriptorSet *pSets) {
    return hg_internal_vulkan_funcs.vkAllocateDescriptorSets(device, pInfo, pSets);
}

VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    return hg_internal_vulkan_funcs.vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet *pWrites, uint32_t copyCount, const VkCopyDescriptorSet *pCopies) {
    hg_internal_vulkan_funcs.vkUpdateDescriptorSets(device, writeCount, pWrites, copyCount, pCopies);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pLayout) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorSetLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorSetLayout(device, layout, pAllocator);
}

VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pLayout) {
    return hg_internal_vulkan_funcs.vkCreatePipelineLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipelineLayout(device, layout, pAllocator);
}

VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pModule) {
    return hg_internal_vulkan_funcs.vkCreateShaderModule(device, pInfo, pAllocator, pModule);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyShaderModule(device, module, pAllocator);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateGraphicsPipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateComputePipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuf) {
    return hg_internal_vulkan_funcs.vkCreateBuffer(device, pInfo, pAllocator, pBuf);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buf, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyBuffer(device, buf, pAllocator);
}

VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    return hg_internal_vulkan_funcs.vkCreateImage(device, pInfo, pAllocator, pImage);
}

void vkDestroyImage(VkDevice device, VkImage img, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImage(device, img, pAllocator);
}

VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    return hg_internal_vulkan_funcs.vkCreateImageView(device, pInfo, pAllocator, pView);
}

void vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImageView(device, view, pAllocator);
}

VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    return hg_internal_vulkan_funcs.vkCreateSampler(device, pInfo, pAllocator, pSampler);
}

void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySampler(device, sampler, pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2*pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    return hg_internal_vulkan_funcs.vkAllocateMemory(device, pInfo, pAllocator, pMemory);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkFreeMemory(device, mem, pAllocator);
}

VkResult vkBindBufferMemory(VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory(device, buf, mem, offset);
}

VkResult vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkBindImageMemory(VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindImageMemory(device, img, mem, offset);
}

VkResult vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
    return hg_internal_vulkan_funcs.vkMapMemory(device, mem, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory mem) {
    hg_internal_vulkan_funcs.vkUnmapMemory(device, mem);
}

VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_internal_vulkan_funcs.vkFlushMappedMemoryRanges(device, count, pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_internal_vulkan_funcs.vkInvalidateMappedMemoryRanges(device, count, pRanges);
}

VkResult vkBeginCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferBeginInfo *pInfo) {
    return hg_internal_vulkan_funcs.vkBeginCommandBuffer(cmd, pInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer cmd) {
    return hg_internal_vulkan_funcs.vkEndCommandBuffer(cmd);
}

VkResult vkResetCommandBuffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags) {
    return hg_internal_vulkan_funcs.vkResetCommandBuffer(cmd, flags);
}

void vkCmdCopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBuffer(cmd, src, dst, count, pRegions);
}

void vkCmdCopyImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions);
}

void vkCmdBlitImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit *pRegions, VkFilter filter) {
    hg_internal_vulkan_funcs.vkCmdBlitImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter);
}

void vkCmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBufferToImage(cmd, src, dst, dstLayout, count, pRegions);
}

void vkCmdCopyImageToBuffer(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImageToBuffer(cmd, src, srcLayout, dst, count, pRegions);
}

void vkCmdPipelineBarrier2(VkCommandBuffer cmd, const VkDependencyInfo *pInfo) {
    hg_internal_vulkan_funcs.vkCmdPipelineBarrier2(cmd, pInfo);
}

void vkCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo *pInfo) {
    hg_internal_vulkan_funcs.vkCmdBeginRendering(cmd, pInfo);
}

void vkCmdEndRendering(VkCommandBuffer cmd) {
    hg_internal_vulkan_funcs.vkCmdEndRendering(cmd);
}

void vkCmdSetViewport(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport *pViewports) {
    hg_internal_vulkan_funcs.vkCmdSetViewport(cmd, first, count, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D *pScissors) {
    hg_internal_vulkan_funcs.vkCmdSetScissor(cmd, first, count, pScissors);
}

void vkCmdBindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    hg_internal_vulkan_funcs.vkCmdBindPipeline(cmd, bindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet *pSets, uint32_t dynCount, const uint32_t *pDyn) {
    hg_internal_vulkan_funcs.vkCmdBindDescriptorSets(cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn);
}

void vkCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *pData) {
    hg_internal_vulkan_funcs.vkCmdPushConstants(cmd, layout, stages, offset, size, pData);
}

void vkCmdBindVertexBuffers(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer *pBufs, const VkDeviceSize *pOffsets) {
    hg_internal_vulkan_funcs.vkCmdBindVertexBuffers(cmd, first, count, pBufs, pOffsets);
}

void vkCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type) {
    hg_internal_vulkan_funcs.vkCmdBindIndexBuffer(cmd, buf, offset, type);
}

void vkCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z) {
    hg_internal_vulkan_funcs.vkCmdDispatch(cmd, x, y, z);
}

void hg_init(void) {
    hg_vk_load();
    hg_internal_platform_init();
}

void hg_exit(void) {
    hg_internal_platform_exit();
}

