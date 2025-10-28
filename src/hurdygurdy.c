#include "hurdygurdy.h"

#include "hg_platform.h"

HgError hg_init(void) {
    hg_platform_init();
    hg_graphics_init();

    return HG_SUCCESS;
}

void hg_shutdown(void) {
    hg_graphics_shutdown();
    hg_platform_shutdown();
}

