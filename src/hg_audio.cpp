#include "hg_core.hpp"
#include "hg_audio.hpp"
#include "hg_templates.hpp"

#include "SDL3/SDL.h"

namespace hg {

struct AudioState {
    SDL_AudioDeviceID device;
    Array<SDL_AudioStream*> streams;
};

static AudioState audio{};

bool audioInit()
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
    {
        formatError("SDL could not open audio device: %s", SDL_GetError());
        return false;
    }

    audio.streams = arrayCreate<SDL_AudioStream*>();

    return true;
}

void audioDeinit()
{
    for (u32 i = 0; i < audio.streams.count; ++i)
    {
        SDL_DestroyAudioStream(audio.streams[i]);
    }
    arrayDestroy(&audio.streams);

    SDL_CloseAudioDevice(audio.device);
}

AudioStream* audioStreamCreate(u32 frequency, u32 channels)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = (int)frequency;
    audioSpec.channels = (int)channels;

    SDL_AudioStream* stream;
    if (audio.streams.count == 0)
    {
        stream = SDL_CreateAudioStream(&audioSpec, nullptr);
        if (stream == nullptr)
        {
            setError(SDL_GetError());
            return nullptr;
        }

        if (!SDL_BindAudioStream(audio.device, stream))
        {
            setError(SDL_GetError());
            SDL_DestroyAudioStream(stream);
            return nullptr;
        }
    }
    else
    {
        stream = arrayPop(&audio.streams);
        if (!SDL_SetAudioStreamFormat(stream, &audioSpec, nullptr))
            HG_PANIC("SDL could not set audio stream format: %s\n", SDL_GetError());
    }

    return (AudioStream*)stream;
}

void audioStreamDestroy(AudioStream* stream)
{
    if (stream != nullptr)
    {
        SDL_AudioStream* sdlStream = (SDL_AudioStream*)stream;

        if (!SDL_ClearAudioStream(sdlStream))
            HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());

        *arrayPush(&audio.streams) = sdlStream;
    }
}

void audioStreamPush(AudioStream* player, const f32* data, u64 size)
{
    SDL_AudioStream* stream = (SDL_AudioStream*)player;
    if (!SDL_PutAudioStreamData(stream, data, (int)size))
        HG_PANIC("SDL could not push audio data: %s\n", SDL_GetError());
}

u32 audioStreamQueuedSize(AudioStream* stream)
{
    int size = SDL_GetAudioStreamQueued((SDL_AudioStream*)stream);
    if (size == -1)
        HG_PANIC("SDL could not read audio data: %s\n", SDL_GetError());

    return (u32)size;
}

void audioStreamClear(AudioStream* stream)
{
    if (!SDL_ClearAudioStream((SDL_AudioStream*)stream))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

void audioStreamSetGain(AudioStream* stream, f32 gain)
{
    if (!SDL_SetAudioStreamGain((SDL_AudioStream*)stream, gain))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

template<>
void assetLoadImpl(Asset<Sound>* data)
{
    (void)data;
    HG_PANIC("Load audio file impl : TODO\n");
}

template<>
void assetUnloadImpl(Asset<Sound>* data)
{
    (void)data;
    HG_PANIC("Unload audio file impl : TODO\n");
}

template<>
void serialize(Serializer* s, AudioSource* src)
{
    serializeObject(s,
        &src->audio,
        &src->position,
        &src->repeat);
}

AudioPlayer audioPlayerCreate()
{
    AudioPlayer player{};
    player.music = arrayCreate<AudioPlayerMusic>();
    player.sounds = arrayCreate<AudioStream*>();
    return player;
}

void audioPlayerDestroy(AudioPlayer* player)
{
    HG_ASSERT(player != nullptr);

    for (u32 i = 0; i < player->sounds.count; ++i)
    {
        audioStreamDestroy(player->sounds[i]);
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        audioStreamDestroy(player->music[i].stream);
    }

    arrayDestroy(&player->sounds);
    arrayDestroy(&player->music);
}

void audioPlayerUpdate(AudioPlayer* player)
{
    for (u32 i = player->sounds.count - 1; i < player->sounds.count; --i)
    {
        if (audioStreamQueuedSize(player->sounds[i]) == 0)
        {
            AudioStream* stream = arrayRemove(&player->sounds, i);
            audioStreamDestroy(stream);
        }
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        AudioPlayerMusic* music = &player->music[i];
        if (!music->playing)
            continue;

        Sound* sound = &music->sound->asset;

        Arena* scratch = getScratch();
        HG_ARENA_SCOPE(scratch);

        u32 width = sizeof(f32);

        u32 total = sound->frequency * width / 16;
        u32 queued = audioStreamQueuedSize(music->stream);
        if (queued >= total)
            continue;
        u32 sizeToPush = total - queued;

        f32* queue = (f32*)arenaAlloc(scratch, sizeToPush, width);
        u32 queueSize = 0;

        while (queueSize < sizeToPush)
        {
            if (music->pos == sound->size)
                music->pos = 0;

            u32 sizeToQueue = min(sizeToPush - queueSize, (u32)(sound->size - music->pos));
            memCopy((u8*)queue + queueSize, (u8*)sound->data + music->pos, sizeToQueue);
            queueSize += sizeToQueue;
            music->pos += sizeToQueue;
        }

        HG_ASSERT(queueSize <= sizeToPush);
        audioStreamPush(music->stream, queue, queueSize);
    }
}

void audioPlayerMusic(AudioPlayer* player, SoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            player->music[i].playing = true;
            return;
        }
    }

    AudioPlayerMusic* track = arrayPush(&player->music);
    track->stream = audioStreamCreate(music->asset.frequency, music->asset.channels);
    track->sound = music;
    track->pos = 0;
    track->playing = true;
}

void audioPlayerMusicKill(AudioPlayer* player, SoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            AudioPlayerMusic track = arrayRemove(&player->music, i);
            audioStreamDestroy(track.stream);
            return;
        }
    }
}

void audioPlayerMusicPause(AudioPlayer* player, SoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            player->music[i].playing = false;
            return;
        }
    }
}

void audioPlayerSetMusicGain(AudioPlayer* player, SoundAsset* music, f32 gain)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            audioStreamSetGain(player->music[i].stream, gain);
            return;
        }
    }
}

void audioPlayerSound(AudioPlayer* player, SoundAsset* sound, f32 gain)
{
    AudioStream** stream = arrayPush(&player->sounds);
    *stream = audioStreamCreate(sound->asset.frequency, sound->asset.channels);
    audioStreamSetGain(*stream, gain);
    audioStreamPush(*stream, sound->asset.data, sound->asset.size);
}

AudioSource* audioSourceAdd(Ecs* ecs, Entity e, SoundAsset* audio, bool repeat)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    AudioSource* src = ecsAdd<AudioSource>(ecs, e);
    *src = {};
    src->player = audioStreamCreate(8000, 1);
    src->audio = audio;
    src->position = 0;
    src->repeat = repeat;

    return src;
}

template<>
void ecsDtor(AudioSource* src)
{
    assetUnload(src->audio);
    audioStreamDestroy(src->player);
}

void audioUpdate(Ecs* ecs, Entity listener)
{
    ecsForEach<AudioSource>(ecs, [&](Entity e, AudioSource* src)
    {
        Sound* audio = &src->audio->asset;
        if (src->position == audio->size && !src->repeat)
            return;

        Arena* scratch = getScratch();
        HG_ARENA_SCOPE(scratch);

        u32 width = sizeof(f32);

        u32 total = audio->frequency * width / 8;
        u32 queued = audioStreamQueuedSize(src->player);
        if (queued >= total)
            return;
        u32 sizeToPush = total - queued;

        f32* queue = (f32*)arenaAlloc(scratch, sizeToPush, width);
        u32 queueSize = 0;

        if (src->repeat)
        {
            while (queueSize < sizeToPush)
            {
                if (src->position == audio->size)
                    src->position = 0;

                u32 sizeToQueue = min(sizeToPush - queueSize, (u32)(audio->size - src->position));
                memCopy((u8*)queue + queueSize, (u8*)audio->data + src->position, sizeToQueue);
                queueSize += sizeToQueue;
                src->position += sizeToQueue;
            }
        }
        else
        {
            queueSize = min(sizeToPush, (u32)(audio->size - src->position));
            memCopy(queue, (u8*)audio->data + src->position, queueSize);
            src->position += queueSize;
        }

        if (ecsHas<Transform>(ecs, e))
        {
            HG_ASSERT(ecsHas<Transform>(ecs, listener));

            Vec3 relPos = transformWorldPos(*ecsGet<Transform>(ecs, listener))
                          - transformWorldPos(*ecsGet<Transform>(ecs, e));
            f32 factor = 1.0f / vecDot3(relPos, relPos);

            for (u64 i = 0; i < sizeToPush / sizeof(f32); ++i)
            {
                ((f32*)queue)[i] *= factor;
            }
        }

        HG_ASSERT(queueSize <= sizeToPush);
        audioStreamPush(src->player, queue, queueSize);
    });
}

} // namespace hg

