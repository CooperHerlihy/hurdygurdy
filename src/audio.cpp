#include "hg/audio.hpp"

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

namespace hg {

template<>
bool serialize(Serializer* s, Sound* sound)
{
    return serializeObject(s,
        &sound->data,
        &sound->frequency,
        &sound->channels);
}

template<>
bool assetLoadImpl(AssetData<Sound>* data)
{
    Maybe<Binary> bin = loadFile(data->path);
    if (!bin.has)
    {
        setError("Could not load audio asset file: %.*s", (int)data->path.length, data->path.chars);
        return false;
    }

    ArenaScope scratch = getScratch();

    if (isSerialData(*bin))
    {
        Serializer s = readSerialBinary(scratch, *bin);
        if (!serialize(&s, &data->asset))
        {
            setError("Could not deserialize serial data from audio file: %.*s", (int)data->path.length, data->path.chars);
            return false;
        }

        return true;
    }

    char* cpath = cString(scratch, data->path);

    if (bin->size > 12 &&
        memcmp("RIFF", (u8*)bin->data + 0, 4) == 0 &&
        memcmp("WAVE", (u8*)bin->data + 8, 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drwav_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drwav_open_file_and_read_pcm_frames_f32(cpath, &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read wav file: %s", cpath);
            return false;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        data->asset.channels = channels;
        data->asset.frequency = sampleRate;
        data->asset.data = Array<f32>{totalPCMFrameCount * channels};
        memcpy(data->asset.data.vals, pSampleData, totalPCMFrameCount * sizeof(f32) * channels);
    }
    else if (bin->size > 3 && memcmp("ID3", bin->data, 3) == 0)
    {
        drmp3_config config{};
        drmp3_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drmp3_open_file_and_read_pcm_frames_f32(cpath, &config, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read mp3 file: %s", cpath);
            return false;
        }
        HG_DEFER(drmp3_free(pSampleData, nullptr));

        data->asset.channels = config.channels;
        data->asset.frequency = config.sampleRate;
        data->asset.data = Array<f32>{totalPCMFrameCount * config.channels};
        memcpy(data->asset.data.vals, pSampleData, totalPCMFrameCount * sizeof(f32) * config.channels);
    }
    else if (bin->size > 4 && memcmp("fLaC", bin->data, 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drflac_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drflac_open_file_and_read_pcm_frames_f32(cpath, &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read flac file: %s", cpath);
            return false;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        data->asset.channels = channels;
        data->asset.frequency = sampleRate;
        data->asset.data = Array<f32>{totalPCMFrameCount * channels};
        memcpy(data->asset.data.vals, pSampleData, totalPCMFrameCount * sizeof(f32) * channels);
    }
    else
    {
        setError("Unknown audio file type: %s", cpath);
        return false;
    }

    return true;
}

bool storeSoundWav(const Sound& sound, StringView path)
{
    if (sound.channels == 0)
    {
        setError("Could not store wav file, sound has no channels");
        return false;
    }

    ArenaScope scratch = getScratch();
    char* cpath = cString(scratch, path);

    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = sound.channels;
    format.sampleRate = sound.frequency;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write_sequential(&wav, cpath, &format, sound.data.count, nullptr))
    {
        setError("Could not open wav file for writing: %s", cpath);
        return false;
    }

    drwav_uint64 frameCount = sound.data.count / sound.channels;
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, frameCount, sound.data.vals);
    if (framesWritten != frameCount || drwav_uninit(&wav) != DRWAV_SUCCESS)
    {
        setError("Could not write wav file: %s", cpath);
        return false;
    }

    return true;
}

void AudioPlayer::update(Span<f32> buf, AudioConfig config)
{
    SpinLockScope scope{lock};

    (void)config;

    for (u32 i = 0; i < sounds.count; ++i)
    {
        for (f32& f : buf)
        {
            f += sounds[i].asset->data[sounds[i].pos++] * sounds[i].gain;
            if (sounds[i].pos == sounds[i].asset->data.count)
            {
                sounds.removeSwap(i--);
                break;
            }
        }
    }

    for (u32 i = 0; i < music.count; ++i)
    {
        if (!music[i].playing)
            continue;

        for (f32& f : buf)
        {
            f += music[i].asset->data[music[i].pos++] * music[i].gain;
            if (music[i].pos == music[i].asset->data.count)
            {
                music[i].pos -= music[i].asset->data.count;
            }
        }
    }
}

void AudioPlayer::playSound(const Asset<Sound>& soundSrc, f32 gain)
{
    SpinLockScope scope{lock};

    sounds.push({soundSrc.clone(), 0, gain, true});
}

void AudioPlayer::playMusic(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].playing = true;
            return;
        }
    }

    music.push({musicSrc.clone(), 0, gain, true});
}

void AudioPlayer::killMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music.removeShift(i);
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i].gain = gain;
            return;
        }
    }
}

} // namespace hg
