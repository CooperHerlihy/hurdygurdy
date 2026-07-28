#pragma once

#include "hg/inttypes.hpp"
#include "hg/array.hpp"
#include "hg/assets.hpp"

namespace hg {

/**
 * AudioStream implementation data
 */
struct AudioStreamData;

/**
 * An audio stream
 */
struct AudioStream {
    /**
     * The implementation data
     */
    AudioStreamData* data = nullptr;

    /**
     * Construct empty
     */
    AudioStream() noexcept = default;

    /**
     * Create a new audio stream
     *
     * Parameters
     * - frequency The segments per second to play
     * - channels The number of channels (mono, stereo, etc.)
     */
    AudioStream(u32 frequency, u32 channels);

    /**
     * Destroy the audio stream
     */
    ~AudioStream() noexcept;

    /**
     * Push data to the audio stream
     */
    void push(Span<f32> samples);

    /**
     * Clear data from the audio player
     */
    void clear();

    /**
     * Returns the amount of audio still queued in floats
     */
    u32 queuedSize();

    /**
     * The the gain for the stream
     */
    void setGain(f32 gain);

    /**
     * Move construct
     */
    AudioStream(AudioStream&& other) noexcept
        : data{std::exchange(other.data, nullptr)}
    {}

    /**
     * Move assign
     */
    AudioStream& operator=(AudioStream&& other) noexcept
    {
        if (this != &other)
        {
            this->~AudioStream();
            new (this) AudioStream{std::move(other)};
        }
        return *this;
    }

    AudioStream(const AudioStream&) = delete;
    AudioStream& operator=(const AudioStream&) = delete;
};

/**
 * Audio data asset
 */
struct Sound {
    /**
     * The sound data
     */
    Span<f32> data{};
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
 * AudioData asset load implementation
 */
template<>
void assetLoadImpl(AssetData<Sound>* data);

/**
 * A music track in the audio player
 */
struct AudioPlayerMusic {
    /**
     * The music's stream
     */
    AudioStream stream{};
    /**
     * The music sound to play
     */
    Asset<Sound> sound{};
    /**
     * The current position in the sound
     */
    u64 pos = 0;
    /**
     * Whether the music is currently playing or paused
     */
    bool playing = false;
};

/**
 * An audio player system
 */
struct AudioPlayer {
    /**
     * The repeating music
     */
    Array<AudioPlayerMusic> music{};
    /**
     * The temporary sounds
     */
    Array<AudioStream> sounds{};

    /**
     * Update the music and sounds
     */
    void update();

    /**
     * Start a new music track, or resume an existing one
     */
    void playMusic(const Asset<Sound>& music);

    /**
     * Remove a music track from the player
     */
    void killMusic(const Asset<Sound>& music);

    /**
     * Pause a music track
     */
    void pauseMusic(const Asset<Sound>& music);

    /**
     * Set the volume for a music track
     */
    void setMusicGain(const Asset<Sound>& music, f32 gain);

    /**
     * Play a sound once
     */
    void playSound(const Asset<Sound>& sound, f32 gain = 1.0f);
};

} // namespace hg

