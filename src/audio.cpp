#include "internal.hpp"
#include "hg_audio.hpp"
#include "hg_error.hpp"

#include "SDL3/SDL.h"

#include <cstdio>

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

template<>
void assetLoadImpl(AssetData<Sound>* data)
{
    static_cast<void>(data);
    HG_PANIC("Load audio file impl : TODO\n");
}

void AudioPlayer::update()
{
    for (u64 i = sounds.count - 1; i < sounds.count; --i)
    {
        if (sounds[i].queuedSize() == 0)
        {
            sounds.removeShift(i);
        }
    }

    for (u32 i = 0; i < music.count; ++i)
    {
        AudioPlayerMusic& m = music[i];
        if (!m.playing)
            continue;

        u32 total = m.sound->frequency / 16;
        u32 queued = m.stream.queuedSize();
        if (queued >= total)
            continue;
        u32 toPush = total - queued;

        ArenaScope scratch = getScratch();
        ArrayTemp<f32> queue{scratch, 0, toPush};

        while (queue.count < toPush)
        {
            if (m.pos == m.sound->data.count)
                m.pos = 0;

            u64 toQueue = std::min(toPush - queue.count, m.sound->data.count - m.pos);
            HG_ASSERT(queue.count + toQueue <= toPush);
            u64 end = queue.count;
            queue.count += toQueue;
            memcpy(&queue[end], &m.sound->data[m.pos], toQueue * sizeof(f32));
            m.pos += toQueue;
        }

        m.stream.push(queue);
    }
}

void AudioPlayer::playMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].playing = true;
            return;
        }
    }

    AudioPlayerMusic* track = music.push();
    track->stream = {musicSrc->frequency, musicSrc->channels};
    track->sound = musicSrc.clone();
    track->pos = 0;
    track->playing = true;
}

void AudioPlayer::killMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music.removeShift(i);
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Asset<Sound>& musicSrc, f32 gain)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].stream.setGain(gain);
            return;
        }
    }
}

void AudioPlayer::playSound(const Asset<Sound>& sound, f32 gain)
{
    AudioStream* stream = sounds.push({sound->frequency, sound->channels});
    stream->setGain(gain);
    stream->push(sound->data);
}

} // namespace hg
