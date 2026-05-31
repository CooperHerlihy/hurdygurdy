#include "hurdygurdy.hpp"

#include "SDL3/SDL.h"

struct HgAudioPlayerData {
    SDL_AudioStream* stream;
};

struct AudioState {
    SDL_AudioDeviceID device;
    HgPool<HgAudioPlayerData> players;
};

static AudioState audio{};

void hgAudioInit(HgArena* arena, u32 maxPlayers)
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
        hgError("SDL could not open audio device: %s\n", SDL_GetError());

    audio.players = hgPoolCreate<HgAudioPlayerData>(arena, maxPlayers);
    for (u32 i = 0; i < maxPlayers; ++i)
    {
        audio.players.vals[i].stream = nullptr;
    }
}

void hgAudioDeinit()
{
    for (u32 i = 0; i < audio.players.capacity; ++i)
    {
        if (audio.players.vals[i].stream != nullptr)
            SDL_DestroyAudioStream(audio.players.vals[i].stream);
    }
    SDL_CloseAudioDevice(audio.device);
}

static SDL_AudioFormat hgAudioFormatToSDL(HgAudioFormat format)
{
    switch (format)
    {
        case HgAudioFormat_u8:
            return SDL_AUDIO_U8;
        case HgAudioFormat_s8:
            return SDL_AUDIO_S8;
        case HgAudioFormat_s16:
            return SDL_AUDIO_S16;
        case HgAudioFormat_s32:
            return SDL_AUDIO_S32;
        case HgAudioFormat_f32:
            return SDL_AUDIO_F32;
        default:
            hgError("Invalid audio format enum: %u\n", format);
    }
}

HgAudioPlayer hgAudioPlayerCreate(HgAudioFormat format, u32 frequency, u32 channels)
{
    HgHandle handle = hgPoolAlloc(&audio.players);
    HgAudioPlayerData* data = hgPoolGet(&audio.players, handle);

    SDL_AudioSpec audioSpec{};
    audioSpec.format = hgAudioFormatToSDL(format);
    audioSpec.freq = frequency;
    audioSpec.channels = channels;
    if (data->stream == nullptr)
    {
        data->stream = SDL_CreateAudioStream(&audioSpec, nullptr);
        if (data->stream == nullptr)
            hgError("SDL could not create audio stream: %s\n", SDL_GetError());
    }
    else
    {
        if (!SDL_SetAudioStreamFormat(data->stream, &audioSpec, nullptr))
            hgError("SDL could not set audio stream format: %s\n", SDL_GetError());
    }

    if (!SDL_BindAudioStream(audio.device, data->stream))
        hgError("SDL could not set audio stream format: %s\n", SDL_GetError());

    return {handle};
}

void hgAudioPlayerDestroy(HgAudioPlayer player)
{
    HgAudioPlayerData* data = hgPoolGet(&audio.players, player.handle);

    SDL_UnbindAudioStream(data->stream);
    if (!SDL_ClearAudioStream(data->stream))
        hgError("SDL could not clear audio stream: %s\n", SDL_GetError());

    hgPoolFree(&audio.players, player.handle);
}

void hgAudioPlayerPush(HgAudioPlayer player, const void* data, u64 size)
{
    HgAudioPlayerData* playerData = hgPoolGet(&audio.players, player.handle);
    if (!SDL_PutAudioStreamData(playerData->stream, data, size))
        hgError("SDL could not push audio data: %s\n", SDL_GetError());
}

u64 hgAudioPlayerQueuedSize(HgAudioPlayer player)
{
    HgAudioPlayerData* data = hgPoolGet(&audio.players, player.handle);

    int size = SDL_GetAudioStreamQueued(data->stream);
    if (size == -1)
        hgError("SDL could not read audio data: %s\n", SDL_GetError());

    return (u64)size;
}

void hgAudioPlayerClear(HgAudioPlayer player)
{
    HgAudioPlayerData* data = hgPoolGet(&audio.players, player.handle);
    if (!SDL_ClearAudioStream(data->stream))
        hgError("SDL could not clear audio stream: %s\n", SDL_GetError());
}

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)dlopen(cstr, RTLD_LAZY);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\": %s\n", cstr, dlerror());

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        dlclose(lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = dlsym(lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\": %s\n", cstr, dlerror());

    return fn;
}

#elif defined(HG_PLATFORM_WINDOWS)

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)LoadLibraryA(cstr);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\"\n", cstr);

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        FreeLibrary((HMODULE)lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = GetProcAddress((HMODULE)lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\": %s\n", cstr, dlerror());

    return fn;
}

#endif

