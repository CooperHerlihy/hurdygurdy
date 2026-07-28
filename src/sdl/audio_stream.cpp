#include "internal.hpp"
#include "hg_audio.hpp"
#include "hg_error.hpp"

#include <SDL3/SDL_audio.h>

namespace hg {

struct AudioState {
    SDL_AudioDeviceID device = 0;
    Array<SDL_AudioStream*> streams = {};
};

static AudioState audio{};

bool internal::initAudio()
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
    {
        setError("SDL could not open audio device: %s", SDL_GetError());
        return false;
    }

    audio.streams = Array<SDL_AudioStream*>(0, 1024);

    return true;
}

void internal::deinitAudio()
{
    for (u32 i = 0; i < audio.streams.count; ++i)
    {
        SDL_DestroyAudioStream(audio.streams[i]);
    }
    audio.streams = {};

    SDL_CloseAudioDevice(audio.device);
}

AudioStream::AudioStream(u32 frequency, u32 channels)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = static_cast<int>(frequency);
    audioSpec.channels = static_cast<int>(channels);

    SDL_AudioStream* stream;
    if (audio.streams.count == 0)
    {
        stream = SDL_CreateAudioStream(&audioSpec, nullptr);
        if (stream == nullptr)
        {
            HG_PANIC("Could not create audio stream: %s\n", SDL_GetError());
        }

        if (!SDL_BindAudioStream(audio.device, stream))
        {
            SDL_DestroyAudioStream(stream);
            HG_PANIC("Could not create audio stream: %s\n", SDL_GetError());
        }
    }
    else
    {
        stream = audio.streams.pop();
        if (!SDL_SetAudioStreamFormat(stream, &audioSpec, nullptr))
            HG_PANIC("SDL could not set audio stream format: %s\n", SDL_GetError());
    }

    data = reinterpret_cast<AudioStreamData*>(stream);
}

AudioStream::~AudioStream() noexcept
{
    if (data != nullptr)
    {
        SDL_AudioStream* sdlStream = reinterpret_cast<SDL_AudioStream*>(data);

        if (!SDL_ClearAudioStream(sdlStream))
            HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());

        audio.streams.push(sdlStream);
    }
}

void AudioStream::push(Span<f32> samples)
{
    SDL_AudioStream* stream = reinterpret_cast<SDL_AudioStream*>(data);
    if (!SDL_PutAudioStreamData(stream, samples.data, static_cast<int>(samples.count * sizeof(f32))))
        HG_PANIC("SDL could not push audio data: %s\n", SDL_GetError());
}

void AudioStream::clear()
{
    if (!SDL_ClearAudioStream(reinterpret_cast<SDL_AudioStream*>(data)))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

u32 AudioStream::queuedSize()
{
    int size = SDL_GetAudioStreamQueued(reinterpret_cast<SDL_AudioStream*>(data));
    if (size == -1)
        HG_PANIC("SDL could not read audio data: %s\n", SDL_GetError());

    return static_cast<u32>(size) / sizeof(f32);
}

void AudioStream::setGain(f32 gain)
{
    if (!SDL_SetAudioStreamGain(reinterpret_cast<SDL_AudioStream*>(data), gain))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

} // namespace hg
