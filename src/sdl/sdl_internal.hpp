#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace hg::sdl {

bool loadSdl();

bool windowInit();
void windowDeinit();

bool initAudio();
void deinitAudio();

} // namespace hg::sdl
