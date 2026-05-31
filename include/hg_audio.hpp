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

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_templates.hpp"

/**
 * Initialize the audio system
 */
void hgAudioInit(HgArena* arena, u32 maxPlayers);

/**
 * Deinitialize the audio system
 */
void hgAudioDeinit();

/**
 * An audio player
 */
struct HgAudioPlayer {
    HgHandle handle;
};

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
 * Create a new audio player
 *
 * Parameters
 * - format The format that will be pushed to the player
 * - frequency The segments per second to play
 * - channels The number of channels (mono, stereo, etc.)
 */
HgAudioPlayer hgAudioPlayerCreate(HgAudioFormat format, u32 frequency, u32 channels);

/**
 * Destroy an audio player
 */
void hgAudioPlayerDestroy(HgAudioPlayer);

/**
 * Push data to the audio player
 */
void hgAudioPlayerPush(HgAudioPlayer player, const void* data, u64 size);

/**
 * Returns the amount of audio still queued in bytes
 */
u64 hgAudioPlayerQueuedSize(HgAudioPlayer player);

/**
 * Clear data from the audio player
 */
void hgAudioPlayerClear(HgAudioPlayer player);

#endif // HG_AUDIO_HPP
