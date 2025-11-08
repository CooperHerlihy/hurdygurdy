#ifndef HG_PLATFORM_INTERNAL_H
#define HG_PLATFORM_INTERNAL_H

#include "hurdygurdy.h"

#include <vulkan/vulkan.h>

u32 hg_platform_get_vulkan_instance_extensions(
    const char** extension_buffer,
    u32 extension_buffer_capacity
);

typedef struct HgPlatformInternals HgPlatformInternals;

HgPlatformInternals* hg_platform_init(void);
void hg_platform_shutdown(HgPlatformInternals* platform);

typedef struct HgPlatformWindow HgPlatformWindow;

HgPlatformWindow* hg_platform_window_create(
    HgPlatformInternals* platform,
    const char* title,
    u32 width,
    u32 height,
    bool windowed
);
void hg_platform_window_destroy(
    HgPlatformInternals* platform,
    HgPlatformWindow* window
);

VkSurfaceKHR hg_platform_create_vulkan_surface(
    HgPlatformInternals* platform,
    HgPlatformWindow* window,
    VkInstance instance
);

#endif // HG_PLATFORM_INTERNAL_H
