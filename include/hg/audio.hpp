#pragma once

#include "hg/inttypes.hpp"
#include "hg/concurrency.hpp"
#include "hg/serialization.hpp"

#include <span>
#include <string_view>
#include <vector>
#include <functional>

namespace hg {

/**
 * The supported audio backends
 */
enum class AudioBackend {
    /**
     * A dummy backend for headless or audioless setups
     */
    none,
    /**
     * Pipewire for Unix systems
     */
    pipewire,
    /**
     * WASAPI for Windows
     */
    wasapi,
    /**
     * SDL3 for other platforms
     */
    sdl3,
};

/**
 * The config for the audio stream
 */
struct AudioConfig {
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels = 1;
    /**
     * The samples per second
     */
    u32 sampleRate = 48000;
};

/**
 * The callback used by the audio thread
 *
 * Note, the audio buffer is interleaved
 *
 * Parameters
 * - audioBuffer The buffer that needs to be filled by the implementation
 * - config The configuration for the audio stream
 */
using AudioCallback = std::function<void(std::span<f32> audioBuffer, AudioConfig config)>;

/**
 * The low level audio device
 */
class AudioDevice {
public:
    /**
     * Hint to create to use a specific backend
     *
     * Note, it is not guarunteed that this backend will be used
     */
    static void hintBackend(AudioBackend preferredBackend);

    /**
     * Create a new audio device
     *
     * Note, the backend should be checked to make sure it is not the dummy
     */
    static std::unique_ptr<AudioDevice> create();

    /**
     * Destroy the audio device
     */
    virtual ~AudioDevice() = default;

    /**
     * Set the current audio callback and start the device
     */
    virtual void setCallback(AudioCallback callback, const AudioConfig& preferredConfig = {}) = 0;

    /**
     * Remove the callback and stop the device
     */
    virtual void unsetCallback() = 0;

    // audio input : TODO

    /**
     * Get the device's audio backend
     */
    virtual AudioBackend getBackend() = 0;
};

/**
 * A dummy audio device for headless or audio-less setups
 */
class AudioDeviceDummy : public AudioDevice {
public:
    /**
     * Create a new audio device
     */
    static std::unique_ptr<AudioDeviceDummy> create()
    {
        return std::make_unique<AudioDeviceDummy>();
    }

    /**
     * Set the current audio callback and start the device
     */
    virtual void setCallback(AudioCallback, const AudioConfig&) override
    {
    }

    /**
     * Remove the callback and stop the device
     */
    virtual void unsetCallback() override
    {
    }

    /**
     * Get the device's audio backend
     */
    virtual AudioBackend getBackend() override
    {
        return AudioBackend::none;
    }
};

/**
 * Audio data asset
 */
struct Sound {
    /**
     * The sound data
     */
    std::vector<f32> data{};
    /**
     * The floats per second
     */
    u32 frequency = 0;
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels = 0;
};

/**
 * Sound serialization
 */
template<>
bool serialize(Serializer* s, Sound* sound);

/**
 * Load a sound from disc and try to decode with all supported formats
 */
std::optional<Sound> soundLoad(std::string_view path);

/**
 * Store a sound to disc in wav format
 */
bool soundStoreWav(const Sound& sound, std::string_view path);

/**
 * A music track in the audio player
 */
struct AudioPlayerSound {
    /**
     * The sound to play
     */
    const Sound* asset{};
    /**
     * The current position in the sound
     */
    u64 pos = 0;
    /**
     * The current gain
     */
    f32 gain = 0.0f;
    /**
     * Whether the music is currently playing or paused
     */
    bool playing = false;
};

/**
 * An audio player system
 */
class AudioPlayer {
private:
    /**
     * The audio device
     */
    AudioDevice* device = nullptr;
    /**
     * The mutex
     */
    SpinLock lock{};
    /**
     * The temporary sounds
     */
    std::vector<AudioPlayerSound> sounds{};
    /**
     * The repeating music
     */
    std::vector<AudioPlayerSound> music{};

public:
    /**
     * Construct empty
     */
    AudioPlayer() noexcept = default;

    /**
     * Construct with device and start callback
     */
    AudioPlayer(AudioDevice* device);

    /**
     * Destroy audio player and remove callback
     */
    ~AudioPlayer() noexcept;

    /**
     * Update the music and sounds, the callback
     */
    void update(std::span<f32> buf, AudioConfig config);

    /**
     * Play a sound once
     */
    void playSound(const Sound& sound, f32 gain = 1.0f);

    /**
     * Start a new music track, or resume an existing one
     */
    void playMusic(const Sound& music, f32 gain = 1.0f);

    /**
     * Remove a music track from the player
     */
    void killMusic(const Sound& music);

    /**
     * Pause a music track
     */
    void pauseMusic(const Sound& music);

    /**
     * Set the volume for a music track
     */
    void setMusicGain(const Sound& music, f32 gain);
};

} // namespace hg

