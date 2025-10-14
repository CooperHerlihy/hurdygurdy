#include "hurdy_gurdy.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

HgError hg_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    hg_init_graphics();

    return HG_SUCCESS;
}

void hg_shutdown(void) {
    hg_shutdown_graphics();
    SDL_Quit();
}

