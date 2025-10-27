#include "hg_platform.h"

#include <dlfcn.h>

static void* s_libx11 = NULL;

void hg_window_open(const char* title, u32 width, u32 height, bool windowed) {
    s_libx11 = dlopen("libX11.so", RTLD_LAZY);
    if (s_libx11 == NULL)
        HG_ERRORF("Could not load libX11.so: %s", dlerror());


}

void hg_window_close(void) {
}

void hg_get_vulkan_instance_extensions(u32 buffer_size, char** extensions, u32* extension_count) {
    extensions[0] = "VK_KHR_surface";
    // extensions[1] = "VK_KHR_xlib_surface";
    // extensions[2] = "VK_KHR_xcb_surface";

    *extension_count = 1;
}

VkSurfaceKHR hg_get_vulkan_surface(void) {
}

