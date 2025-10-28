#ifndef HG_WINDOW_INTERNAL_H
#define HG_WINDOW_INTERNAL_H

#include "hg_utils.h"

#include <vulkan/vulkan.h>

void hg_platform_init(void);
void hg_platform_shutdown(void);

void hg_platform_open_window(const char* title, u32 width, u32 height, bool windowed);
void hg_platform_close_window(void);

void hg_platform_get_vulkan_instance_extensions(u32 buffer_count, const char** extensions, u32* extension_count);
VkSurfaceKHR hg_platform_create_vulkan_surface(VkInstance instance);

#endif // HG_WINDOW_INTERNAL_H
