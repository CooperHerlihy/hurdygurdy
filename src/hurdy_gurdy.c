#include "hurdy_gurdy.h"

HgError hg_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    hg_init_graphics();

    return HG_SUCCESS;
}

void hg_shutdown(void) {
    hg_shutdown_graphics();
    SDL_Quit();
}

