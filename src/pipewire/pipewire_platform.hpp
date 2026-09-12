#pragma once

#include "hg/audio.hpp"

namespace hg::pipewire {

bool loadPipeWire();
bool initPipewire();
void deinitPipewire();

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig);
void unsetAudioCallback();

} // namespace hg::pipewire
