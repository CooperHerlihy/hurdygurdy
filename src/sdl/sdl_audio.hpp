#pragma once

#include "hg/audio.hpp"

#include <SDL3/SDL.h>

namespace hg {

class AudioDeviceSdl3 : public AudioDevice {
private:
    SDL_AudioDeviceID device = 0;
    SDL_AudioStream* stream = nullptr;

    AudioCallback callback{};
    AudioConfig callbackConfig{};

    static void sdlCallback(
        void* userData,
        SDL_AudioStream* stream,
        int additionalAmount,
        int totalAmount);

public:
    static std::unique_ptr<AudioDeviceSdl3> create();
    ~AudioDeviceSdl3() override;

    virtual void setCallback(AudioCallback callback, const AudioConfig& preferredConfig = {}) override;
    virtual void unsetCallback() override;

    virtual AudioBackend getBackend() override
    {
        return AudioBackend::sdl3;
    }
};

} // namespace hg
