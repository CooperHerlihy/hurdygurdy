/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
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
#include "hg_memory.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"

/**
 * Initialize the audio subsystem
 */
void hgAudioInit();

/**
 * Deinitialize the audio subsystem
 */
void hgAudioDeinit();

/**
 * An audio player
 */
struct HgAudioPlayer;

/**
 * Audio formats
 */
enum HgAudioFormat : u32 {
    /**
     * Unsigned 8 bit int
     */
    HgAudioFormat_u8,
    /**
     * Signed 8 bit int
     */
    HgAudioFormat_s8,
    /**
     * Signed 16 bit int
     */
    HgAudioFormat_s16,
    /**
     * Signed 32 bit int
     */
    HgAudioFormat_s32,
    /**
     * 32 bit float
     */
    HgAudioFormat_f32,
};

/**
 * Returns the size in bytes of the format
 */
u32 hgAudioFormatSize(HgAudioFormat format);

/**
 * Create a new audio player
 *
 * Parameters
 * - format The format that will be pushed to the player
 * - frequency The segments per second to play
 * - channels The number of channels (mono, stereo, etc.)
 */
HgAudioPlayer* hgAudioPlayerCreate(HgAudioFormat format, u32 frequency, u32 channels);

/**
 * Destroy an audio player
 */
void hgAudioPlayerDestroy(HgAudioPlayer* player);

/**
 * Push data to the audio player
 */
void hgAudioPlayerPush(HgAudioPlayer* player, const void* data, u64 size);

/**
 * Returns the amount of audio still queued in bytes
 */
u32 hgAudioPlayerQueuedSize(HgAudioPlayer* player);

/**
 * Clear data from the audio player
 */
void hgAudioPlayerClear(HgAudioPlayer* player);

/**
 * Audio data asset
 */
struct HgAudio {
    void* data;
    u64 size;
    HgAudioFormat format;
    u32 frequency;
    u32 channels;
};

/**
 * A handle to an audio asset
 */
typedef HgAsset<HgAudio> HgAudioAsset;

/**
 * HgAudioData asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgAudio>* data);

/**
 * HgAudioData asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgAudio>* data);

/**
 * An audio source component
 */
struct HgAudioSource {
    /**
     * The audio player for this source, should not be modified
     */
    HgAudioPlayer* player;
    /**
     * The audio to play from
     */
    HgAudioAsset* audio;
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
void hgSerializeImpl(HgSerializer* s, HgAudioSource* src);

/**
 * Add an audio source to an entity
 */
HgAudioSource* hgAudioSourceAdd(HgEcs* ecs, HgEntity e, HgAudioAsset* audio, bool repeat);

/**
 * HgAudioSource ecs destructor
 */
template<>
void hgEcsDtor(HgAudioSource* src);

/**
 * Update all audio sources, playing sound if needed
 */
void hgAudioUpdate(HgEcs* ecs, HgEntity listener);

// mixing : TODO

#endif // HG_AUDIO_HPP
