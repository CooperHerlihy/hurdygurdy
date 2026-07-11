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
bool hgAudioInit();

/**
 * Deinitialize the audio subsystem
 */
void hgAudioDeinit();

/**
 * An audio stream
 */
struct HgAudioStream;

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
HgAudioStream* hgAudioStreamCreate(u32 frequency, u32 channels);

/**
 * Destroy an audio stream
 */
void hgAudioStreamDestroy(HgAudioStream* player);

/**
 * Push data to the audio stream
 *
 * Parameters
 * - stream The stream to push to
 * - data The raw audio data in 32 bit float format
 * - size The size of the pushed data in bytes
 */
void hgAudioStreamPush(HgAudioStream* player, const f32* data, u64 size);

/**
 * Returns the amount of audio still queued in bytes
 */
u32 hgAudioStreamQueuedSize(HgAudioStream* player);

/**
 * Clear data from the audio player
 */
void hgAudioStreamClear(HgAudioStream* player);

/**
 * The the gain for the stream
 */
void hgAudioStreamSetGain(HgAudioStream* stream, f32 gain);

/**
 * Audio data asset
 */
struct HgSound {
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
typedef HgAsset<HgSound> HgSoundAsset;

/**
 * HgAudioData asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgSound>* data);

/**
 * HgAudioData asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgSound>* data);

/**
 * A music track in the audio player
 */
struct HgAudioPlayerMusic {
    /**
     * The music's stream
     */
    HgAudioStream* stream;
    /**
     * The music sound to play
     */
    HgSoundAsset* sound;
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
struct HgAudioPlayer {
    /**
     * The repeating music
     */
    HgArray<HgAudioPlayerMusic> music;
    /**
     * The temporary sounds
     */
    HgArray<HgAudioStream*> sounds;
};

/**
 * Create a new audio player
 */
HgAudioPlayer hgAudioPlayerCreate();

/**
 * Destroy an audio player
 */
void hgAudioPlayerDestroy(HgAudioPlayer* player);

/**
 * Update the audio player
 */
void hgAudioPlayerUpdate(HgAudioPlayer* player);

/**
 * Start a new music track, or resume an existing one
 */
void hgAudioPlayerMusic(HgAudioPlayer* player, HgSoundAsset* music);

/**
 * Remove a music track from the player
 */
void hgAudioPlayerMusicKill(HgAudioPlayer* player, HgSoundAsset* music);

/**
 * Pause a music track
 */
void hgAudioPlayerMusicPause(HgAudioPlayer* player, HgSoundAsset* music);

/**
 * Set the volume for a music track
 */
void hgAudioPlayerSetMusicGain(HgAudioPlayer* player, HgSoundAsset* music, f32 gain = 1.0f);

/**
 * Play a sound once
 */
void hgAudioPlayerSound(HgAudioPlayer* player, HgSoundAsset* sound, f32 gain);

/**
 * An audio source component
 */
struct HgAudioSource {
    /**
     * The audio player for this source, should not be modified
     */
    HgAudioStream* player;
    /**
     * The audio to play from
     */
    HgSoundAsset* audio;
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
 * HgAudioSource serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgAudioSource* src);

/**
 * Add an audio source to an entity
 */
HgAudioSource* hgAudioSourceAdd(HgEcs* ecs, HgEntity e, HgSoundAsset* audio, bool repeat);

/**
 * HgAudioSource ecs destructor
 */
template<>
void hgEcsDtor(HgAudioSource* src);

/**
 * Update all audio sources, playing sound if needed
 */
void hgAudioUpdate(HgEcs* ecs, HgEntity listener);

} // namespace hg

#endif // HG_AUDIO_HPP
