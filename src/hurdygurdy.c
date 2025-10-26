#include "hurdygurdy.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

HgError hg_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    hg_graphics_init();

    return HG_SUCCESS;
}

void hg_shutdown(void) {
    hg_graphics_shutdown();
    SDL_Quit();
}

