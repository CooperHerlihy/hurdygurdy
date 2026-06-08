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

#ifndef HG_ASSETS_HPP
#define HG_ASSETS_HPP

#include "hg_concurrency.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"

/**
 * Initialize all default asset types
 *
 * Asset types:
 * - HgBinary
 * - HgTexture
 * - HgGpuTexture
 * - HgMesh
 * - HgGpuMesh
 * - HgAudio
 */
void hgAssetInitDefaults();

/**
 * Deinitialize all default asset types
 */
void hgAssetDeinitDefaults();

/**
 * The data associated with assets
 */
template<typename T>
struct HgAsset {
    /**
     * The asset
     */
    T data;
    /**
     * The reference count
     */
    u32 refCount;
    /**
     * The unique path for caching
     */
    HgStringOwner path;
};

/**
 * An asset manager
 */
template<typename T>
struct HgAssetManager {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset lookup
     */
    HgMap<HgStringView, HgAsset<T>*> map;
    /**
     * The asset pool
     */
    HgPool<HgAsset<T>> pool;
};

/**
 * The global asset managers
 */
template<typename T>
inline HgAssetManager<T> hgAssets{};

/**
 * Initialize an asset manager
 */
template<typename T>
void hgAssetInit();

/**
 * Detsroy an asset manager
 */
template<typename T>
void hgAssetDeinit();

/**
 * Load an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetLoadImpl(HgAsset<T>* data);

/**
 * Unload an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetUnloadImpl(HgAsset<T>* data);

/**
 * Create a unique empty asset
 */
template<typename T>
HgAsset<T>* hgAssetCreate();

/**
 * Load an asset (or increment the ref count)
 */
template<typename T>
HgAsset<T>* hgAssetLoad(HgStringView path);

/**
 * Destroy an asset and unload it (or decrement the ref count)
 */
template<typename T>
void hgAssetUnload(HgAsset<T>* asset);

/**
 * Hot reload an asset
 */
template<typename T>
void hgAssetReload(HgAsset<T>* asset);

/**
 * Increment the asset's reference count
 */
template<typename T>
HgAsset<T>* hgAssetCopy(HgAsset<T>* asset);

/**
 * HgAssetHandle serialization
 */
template<typename T>
void hgSerialize(HgSerializer* s, HgAsset<T>** asset);

/**
 * A binary file asset handle
 */
typedef HgAsset<HgBinary> HgBinaryAsset;

/**
 * HgBinary asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgBinary>* data);

/**
 * HgBinary asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgBinary>* data);

/**
 * Store a binary file to disc
 *
 * Parameters
 * - bin The binary to store
 * - path The file path to store at
 * - fence The fence to signal on completion
 */
void hgBinaryStore(HgBinary* bin, HgStringView path, HgFence* fence);

#endif // HG_ASSETS_HPP
