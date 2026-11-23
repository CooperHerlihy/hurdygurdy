#include "hg/audio.hpp"

#include "hg/utility.hpp"
#include "hg/memory.hpp"
#include "hg/serialization.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "pipewire/pipewire_audio.hpp"
#endif

#if defined(HG_PLATFORM_WINDOWS)
#include "win32/win32_audio.hpp"
#endif

#if defined(HG_USE_SDL)
#include "sdl/sdl_audio.hpp"
#endif

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

namespace hg {

#if defined(HG_PLATFORM_LINUX)
static AudioBackend preferredBackend = AudioBackend::pipewire;
#elif defined(HG_PLATFORM_WINDOWS)
static AudioBackend preferredBackend = AudioBackend::wasapi;
#else
static AudioBackend preferredBackend = AudioBackend::sdl3;
#endif

void AudioDevice::hintBackend(AudioBackend backend)
{
    preferredBackend = backend;
}

static std::unique_ptr<AudioDevice> createBackend(AudioBackend backend)
{
    switch (backend)
    {
#if defined(HG_USE_SDL)
    case AudioBackend::sdl3:
        return AudioDeviceSdl3::create();
#endif
#if defined(HG_PLATFORM_LINUX)
    case AudioBackend::pipewire:
        return AudioDevicePipewire::create();
#endif
#if defined(HG_PLATFORM_WINDOWS)
    case AudioBackend::wasapi:
        return AudioDeviceWasapi::create();
#endif
    case AudioBackend::none:
        return AudioDeviceDummy::create();
    default:
        break;
    }

    setError("Could not create audio backend: {}", static_cast<u32>(backend));
    return nullptr;
}

std::unique_ptr<AudioDevice> AudioDevice::create()
{

    std::unique_ptr<AudioDevice> d = nullptr;

    d = createBackend(preferredBackend);
    if (d != nullptr)
        return d;

#if defined(HG_PLATFORM_LINUX)
    d = createBackend(AudioBackend::pipewire);
    if (d != nullptr)
        return d;
#endif

#if defined(HG_PLATFORM_WINDOWS)
    d = createBackend(AudioBackend::wasapi);
    if (d != nullptr)
        return d;
#endif

#if defined(HG_USE_SDL)
    d = createBackend(AudioBackend::sdl3);
    if (d != nullptr)
        return d;
#endif

    return AudioDeviceDummy::create();
}

template<>
bool serialize(Serializer& s, Sound* sound)
{
    return serializeObject(s,
        &sound->data,
        &sound->frequency,
        &sound->channels);
}

std::optional<Sound> soundLoad(std::string_view path)
{
    std::optional<std::vector<byte>> bin = binaryLoad(path);
    if (!bin.has_value())
        return std::nullopt;

    std::optional ret = serialLoadFromBinary<Sound>(*bin);
    if (ret.has_value())
        return ret;

    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};

    if (bin->size() > 12 &&
        std::memcmp("RIFF", &(*bin)[0], 4) == 0 &&
        std::memcmp("WAVE", &(*bin)[8], 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drwav_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drwav_open_file_and_read_pcm_frames_f32(p.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read wav file: {}", path);
            return std::nullopt;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        std::optional ret = std::make_optional<Sound>();
        ret->channels = channels;
        ret->frequency = sampleRate;
        ret->data.resize(totalPCMFrameCount * channels);
        std::memcpy(ret->data.data(), pSampleData, ret->data.size() * sizeof(f32));
        return ret;
    }
    else if (bin->size() > 3 && memcmp("ID3", bin->data(), 3) == 0)
    {
        drmp3_config config{};
        drmp3_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drmp3_open_file_and_read_pcm_frames_f32(p.c_str(), &config, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read mp3 file: {}", path);
            return std::nullopt;
        }
        HG_DEFER(drmp3_free(pSampleData, nullptr));

        std::optional ret = std::make_optional<Sound>();
        ret->channels = config.channels;
        ret->frequency = config.sampleRate;
        ret->data.resize(totalPCMFrameCount * config.channels);
        std::memcpy(ret->data.data(), pSampleData, ret->data.size() * sizeof(f32));
        return ret;
    }
    else if (bin->size() > 4 && memcmp("fLaC", bin->data(), 4) == 0)
    {
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        drflac_uint64 totalPCMFrameCount = 0;
        f32* pSampleData = drflac_open_file_and_read_pcm_frames_f32(p.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr) {
            setError("Could not read flac file: {}", path);
            return std::nullopt;
        }
        HG_DEFER(drwav_free(pSampleData, NULL));

        std::optional ret = std::make_optional<Sound>();
        ret->channels = channels;
        ret->frequency = sampleRate;
        ret->data.resize(totalPCMFrameCount * channels);
        std::memcpy(ret->data.data(), pSampleData, ret->data.size() * sizeof(f32));
        return ret;
    }

    setError("Unknown audio file type: {}", path);
    return std::nullopt;
}

bool soundStoreWav(const Sound& sound, std::string_view path)
{
    if (sound.channels == 0)
    {
        setError("Could not store wav file, sound has no channels");
        return false;
    }

    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};
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

AudioPlayer::AudioPlayer(AudioDevice* device)
    : device{device}
{
    device->setCallback([this](std::span<f32> buf, AudioConfig config) { update(buf, config); });
}

AudioPlayer::~AudioPlayer() noexcept
{
    SpinLockScope scope{lock};
    if (device != nullptr)
        device->unsetCallback();
}

void AudioPlayer::update(std::span<f32> buf, AudioConfig config)
{
    (void)config;

    SpinLockScope scope{lock};

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

void AudioPlayer::playSound(const Sound& soundSrc, f32 gain)
{
    SpinLockScope scope{lock};
    sounds.emplace_back(&soundSrc, 0, gain, true);
}

void AudioPlayer::playMusic(const Sound& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == &musicSrc)
        {
            m.playing = true;
            return;
        }
    }

    music.emplace_back(&musicSrc, 0, gain, true);
}

void AudioPlayer::killMusic(const Sound& musicSrc)
{
    SpinLockScope scope{lock};

    for (u32 i = 0; i < music.size(); ++i)
    {
        if (music[i].asset == &musicSrc)
        {
            music[i--] = std::move(music.back());
            music.pop_back();
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Sound& musicSrc)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == &musicSrc)
        {
            m.playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Sound& musicSrc, f32 gain)
{
    SpinLockScope scope{lock};

    for (AudioPlayerSound& m : music)
    {
        if (m.asset == &musicSrc)
        {
            m.gain = gain;
            return;
        }
    }
}

} // namespace hg
