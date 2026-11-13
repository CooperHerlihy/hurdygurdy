#include "hg/audio.hpp"

#include "hg/memory.hpp"

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
    std::optional<std::vector<u8>> bin = loadFile(data->path);
    if (!bin.has_value())
    {
        setError("Could not load audio asset file: {}", data->path.c_str());
        return false;
    }

    if (isSerialData(*bin))
    {
        std::optional<Serializer> s = readSerialBinary(*bin);
        if (!s.has_value() || !serialize(&*s, &data->asset))
        {
            setError("Could not deserialize serial data from audio file: {}", data->path.c_str());
            return false;
        }

        return true;
    }

    if (bin->size() > 12 &&
        std::memcmp("RIFF", &(*bin)[0], 4) == 0 &&
        std::memcmp("WAVE", &(*bin)[8], 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drwav_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drwav_open_file_and_read_pcm_frames_f32(data->path.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read wav file: {}", data->path.c_str());
            return false;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        data->asset.channels = channels;
        data->asset.frequency = sampleRate;
        data->asset.data.resize(totalPCMFrameCount * channels);
        std::memcpy(data->asset.data.data(), pSampleData, data->asset.data.size() * sizeof(f32));
    }
    else if (bin->size() > 3 && memcmp("ID3", bin->data(), 3) == 0)
    {
        drmp3_config config{};
        drmp3_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drmp3_open_file_and_read_pcm_frames_f32(data->path.c_str(), &config, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read mp3 file: {}", data->path.c_str());
            return false;
        }
        HG_DEFER(drmp3_free(pSampleData, nullptr));

        data->asset.channels = config.channels;
        data->asset.frequency = config.sampleRate;
        data->asset.data.resize(totalPCMFrameCount * config.channels);
        std::memcpy(data->asset.data.data(), pSampleData, data->asset.data.size() * sizeof(f32));
    }
    else if (bin->size() > 4 && memcmp("fLaC", bin->data(), 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drflac_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drflac_open_file_and_read_pcm_frames_f32(data->path.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read flac file: {}", data->path.c_str());
            return false;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        data->asset.channels = channels;
        data->asset.frequency = sampleRate;
        data->asset.data.resize(totalPCMFrameCount * channels);
        std::memcpy(data->asset.data.data(), pSampleData, totalPCMFrameCount * sizeof(f32) * channels);
    }
    else
    {
        setError("Unknown audio file type: {}", data->path.c_str());
        return false;
    }

    return true;
}

bool storeSoundWav(const Sound& sound, std::string_view path)
{
    if (sound.channels == 0)
    {
        setError("Could not store wav file, sound has no channels");
        return false;
    }

    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};
    const char* cPath = p.c_str();

    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = sound.channels;
    format.sampleRate = sound.frequency;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write_sequential(&wav, cPath, &format, sound.data.size(), nullptr))
    {
        setError("Could not open wav file for writing: {}", cPath);
        return false;
    }

    drwav_uint64 frameCount = sound.data.size() / sound.channels;
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, frameCount, sound.data.data());
    if (framesWritten != frameCount || drwav_uninit(&wav) != DRWAV_SUCCESS)
    {
        setError("Could not write wav file: {}", cPath);
        return false;
    }

    return true;
}

void AudioPlayer::update(std::span<f32> buf, AudioConfig config)
{
    SpinLockScope scope{lock};

    (void)config;

    for (u32 i = 0; i < sounds.size(); ++i)
    {
        for (f32& f : buf)
        {
            f += sounds[i].asset->data[sounds[i].pos++] * sounds[i].gain;
            if (sounds[i].pos == sounds[i].asset->data.size())
            {
                sounds[i--] = std::move(sounds.back());
                sounds.pop_back();
                break;
            }
        }
    }

    for (AudioPlayerSound& m : music)
    {
        if (!m.playing)
            continue;

        for (f32& f : buf)
        {
            f += m.asset->data[m.pos++] * m.gain;
            if (m.pos == m.asset->data.size())
            {
                m.pos -= m.asset->data.size();
            }
        }
    }
}

void AudioPlayer::playSound(const Asset<Sound>& soundSrc, f32 gain)
{
    SpinLockScope scope{lock};

    sounds.emplace_back(soundSrc.clone(), 0, gain, true);
}

void AudioPlayer::playMusic(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == musicSrc)
        {
            m.playing = true;
            return;
        }
    }

    music.emplace_back(musicSrc.clone(), 0, gain, true);
}

void AudioPlayer::killMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < sounds.size(); ++i)
    {
        if (music[i].asset == musicSrc)
        {
            music[i--] = std::move(music.back());
            music.pop_back();
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Asset<Sound>& musicSrc)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == musicSrc)
        {
            m.playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Asset<Sound>& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == musicSrc)
        {
            m.gain = gain;
            return;
        }
    }
}

} // namespace hg
