#include "hg/audio.hpp"

#include "sdl_internal.hpp"
#include "hg/error.hpp"

namespace hg::sdl {

struct AudioState {
    SDL_AudioDeviceID device = 0;
    SDL_AudioStream* stream = nullptr;

    AudioCallback callback = nullptr;
    void* callbackData = nullptr;
    AudioConfig callbackConfig{};
};

static AudioState audio{};

bool initAudio()
{
    audio.device = sdlFuncs.SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
    {
        setError("SDL could not open audio device: %s", sdlFuncs.SDL_GetError());
        return false;
    }

    audio.stream = sdlFuncs.SDL_CreateAudioStream(nullptr, nullptr);
    if (audio.stream == nullptr)
    {
        setError("SDL could not open audio stream: %s", sdlFuncs.SDL_GetError());
        sdlFuncs.SDL_CloseAudioDevice(audio.device);
        return false;
    }

    if (!sdlFuncs.SDL_BindAudioStream(audio.device, audio.stream))
    {
        setError("SDL could not bind audio stream to device: %s", sdlFuncs.SDL_GetError());
        sdlFuncs.SDL_DestroyAudioStream(audio.stream);
        sdlFuncs.SDL_CloseAudioDevice(audio.device);
        return false;
    }

    return true;
}

void deinitAudio()
{
    sdlFuncs.SDL_DestroyAudioStream(audio.stream);
    sdlFuncs.SDL_CloseAudioDevice(audio.device);
}

static void sdlCallback(
    void* userData,
    SDL_AudioStream* stream,
    int additionalAmount,
    int totalAmount)
{
    (void)userData;
    (void)totalAmount;

    ArenaScope scratch = getScratch();

    void* buf = scratch.alloc(static_cast<u64>(additionalAmount), alignof(f32));
    memset(buf, 0, static_cast<u64>(additionalAmount));

    if (audio.callback != nullptr)
    {
        audio.callback(
            audio.callbackData,
            {static_cast<f32*>(buf), static_cast<u64>(additionalAmount) / sizeof(f32)},
            audio.callbackConfig);
    }

    if (!sdlFuncs.SDL_PutAudioStreamData(stream, buf, additionalAmount))
        HG_PANIC("SDL could not push audio stream data: %s\n", sdlFuncs.SDL_GetError());
}

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = static_cast<int>(preferredConfig.sampleRate);
    audioSpec.channels = static_cast<int>(preferredConfig.channels);

    if (!sdlFuncs.SDL_SetAudioStreamFormat(audio.stream, &audioSpec, nullptr))
        HG_PANIC("SDL could not set audio stream format: %s\n", sdlFuncs.SDL_GetError());

    audio.callback = callback;
    audio.callbackData = userData;
    audio.callbackConfig = preferredConfig;

    if (!sdlFuncs.SDL_SetAudioStreamGetCallback(audio.stream, sdlCallback, nullptr))
        HG_PANIC("SDL could not set audio stream callback: %s\n", sdlFuncs.SDL_GetError());
}

void unsetAudioCallback()
{
    audio.callback = nullptr;
    audio.callbackData = nullptr;
    audio.callbackConfig = {};
}

} // namespace hg::sdl
