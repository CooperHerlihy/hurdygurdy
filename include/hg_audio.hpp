/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_AUDIO_HPP
#define HG_AUDIO_HPP

#include "hg_assets.hpp"
#include "hg_core.hpp"
#include "hg_ecs.hpp"
#include "hg_serialization.hpp"

namespace hg {

/**
 * Initialize the audio subsystem
 *
 * Returns
 * - Whether init succeeded
 */
bool audioInit();

/**
 * Deinitialize the audio subsystem
 */
void audioDeinit();

/**
 * An audio stream
 */
struct AudioStream;

/**
 * Create a new audio stream
 *
 * Parameters
 * - frequency The segments per second to play
 * - channels The number of channels (mono, stereo, etc.)
 *
 * Returns
 * - The created audio stream, or nullptr on failure
 */
AudioStream* audioStreamCreate(u32 frequency, u32 channels);

/**
 * Destroy an audio stream
 */
void audioStreamDestroy(AudioStream* player);

/**
 * Push data to the audio stream
 *
 * Parameters
 * - stream The stream to push to
 * - data The raw audio data in 32 bit float format
 * - size The size of the pushed data in bytes
 */
void audioStreamPush(AudioStream* player, const f32* data, u64 size);

/**
 * Returns the amount of audio still queued in bytes
 */
u32 audioStreamQueuedSize(AudioStream* player);

/**
 * Clear data from the audio player
 */
void audioStreamClear(AudioStream* player);

/**
 * The the gain for the stream
 */
void audioStreamSetGain(AudioStream* stream, f32 gain);

/**
 * Audio data asset
 */
struct Sound {
    /**
     * The sound data
     */
    f32* data;
    /**
     * The size of the data in bytes
     */
    u64 size;
    /**
     * The floats per second
     */
    u32 frequency;
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels;
};

/**
 * A handle to an audio asset
 */
typedef Asset<Sound> SoundAsset;

/**
 * AudioData asset load implementation
 */
template<>
void assetLoadImpl(Asset<Sound>* data);

/**
 * AudioData asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Sound>* data);

/**
 * A music track in the audio player
 */
struct AudioPlayerMusic {
    /**
     * The music's stream
     */
    AudioStream* stream;
    /**
     * The music sound to play
     */
    SoundAsset* sound;
    /**
     * The current position in the sound
     */
    u32 pos;
    /**
     * Whether the music is currently playing or paused
     */
    bool playing;
};

/**
 * An audio player system
 */
struct AudioPlayer {
    /**
     * The repeating music
     */
    Array<AudioPlayerMusic> music;
    /**
     * The temporary sounds
     */
    Array<AudioStream*> sounds;
};

/**
 * Create a new audio player
 */
AudioPlayer audioPlayerCreate();

/**
 * Destroy an audio player
 */
void audioPlayerDestroy(AudioPlayer* player);

/**
 * Update the audio player
 */
void audioPlayerUpdate(AudioPlayer* player);

/**
 * Start a new music track, or resume an existing one
 */
void audioPlayerMusic(AudioPlayer* player, SoundAsset* music);

/**
 * Remove a music track from the player
 */
void audioPlayerMusicKill(AudioPlayer* player, SoundAsset* music);

/**
 * Pause a music track
 */
void audioPlayerMusicPause(AudioPlayer* player, SoundAsset* music);

/**
 * Set the volume for a music track
 */
void audioPlayerSetMusicGain(AudioPlayer* player, SoundAsset* music, f32 gain = 1.0f);

/**
 * Play a sound once
 */
void audioPlayerSound(AudioPlayer* player, SoundAsset* sound, f32 gain);

/**
 * An audio source component
 */
struct AudioSource {
    /**
     * The audio player for this source, should not be modified
     */
    AudioStream* player;
    /**
     * The audio to play from
     */
    SoundAsset* audio;
    /**
     * The current position in the audio data
     */
    u64 position;
    /**
     * Whether the source should repeat playing
     */
    bool repeat;
};

/**
 * AudioSource serialization
 */
template<>
void serialize(Serializer* s, AudioSource* src);

/**
 * Add an audio source to an entity
 */
AudioSource* audioSourceAdd(Ecs* ecs, Entity e, SoundAsset* audio, bool repeat);

/**
 * AudioSource ecs destructor
 */
template<>
void ecsDtor(AudioSource* src);

/**
 * Update all audio sources, playing sound if needed
 */
void audioUpdate(Ecs* ecs, Entity listener);

} // namespace hg

#endif // AUDIO_HPP
