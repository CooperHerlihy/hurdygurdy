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
#include "hg_memory.hpp"
#include "hg_serialization.hpp"

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
void hgAssetInit()
{
    hgAssets<T>.map = hgMapCreate<HgStringView, HgAsset<T>*>();
    hgAssets<T>.pool = hgPoolCreate<HgAsset<T>>();
}

/**
 * Detsroy an asset manager
 */
template<typename T>
void hgAssetDeinit()
{
    hgPoolDestroy(&hgAssets<T>.pool);
    hgMapDestroy(&hgAssets<T>.map);
}

/**
 * Load an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetLoadImpl(HgAsset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be loaded without implementation");
}

/**
 * Unload an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetUnloadImpl(HgAsset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be unloaded without implementation");
}

/**
 * Create a unique empty asset
 */
template<typename T>
HgAsset<T>* hgAssetCreate()
{
    HgAsset<T>* asset = hgPoolAlloc(&hgAssets<T>.pool);
    asset->data = {};
    asset->refCount = 1;
    asset->path = {};

    return asset;
}

/**
 * Load an asset (or increment the ref count)
 */
template<typename T>
HgAsset<T>* hgAssetLoad(HgStringView path)
{
    if (HgAsset<T>** asset = hgMapGet(&hgAssets<T>.map, path);
        asset != nullptr)
    {
        ++(*asset)->refCount;
        return *asset;
    }

    HgAsset<T>* asset = hgPoolAlloc(&hgAssets<T>.pool);
    asset->data = {};
    asset->refCount = 1;
    asset->path = hgStringCreate(path);

    if (hgAssets<T>.map.count * 2 > hgAssets<T>.map.capacity)
        hgMapResize(&hgAssets<T>.map, hgAssets<T>.map.capacity * 2);
    hgMapAdd(&hgAssets<T>.map, asset->path, asset);

    hgAssetLoadImpl(asset);
    return asset;
}

/**
 * Hot reload an asset
 */
template<typename T>
void hgAssetReload(HgAsset<T>* asset)
{
    hgAssert(asset != nullptr);

    hgAssetUnloadImpl(asset);
    hgAssetLoadImpl(asset);
}

/**
 * Hot reload an asset asynchronously
 */
template<typename T>
void hgAssetReloadAsync(HgAsset<T>* asset, HgFence* fence)
{
    hgAssert(asset != nullptr);

    hgThreadsCall(fence, asset, [](void* passet)
    {
        HgAsset<T>* asset = (HgAsset<T>*)passet;
        hgAssetUnloadImpl(asset);
        hgAssetLoadImpl(asset);
    });
}

/**
 * Destroy an asset and unload it (or decrement the ref count)
 */
template<typename T>
void hgAssetUnload(HgAsset<T>* asset)
{
    if (asset != nullptr && --asset->refCount == 0)
    {
        hgAssetUnloadImpl(asset);

        if (asset->path != nullptr)
        {
            hgMapRemove(&hgAssets<T>.map, asset->path);
            hgStringDestroy(&asset->path);
        }
        hgPoolFree(&hgAssets<T>.pool, asset);
    }
}

/**
 * Increment the asset's reference count
 */
template<typename T>
HgAsset<T>* hgAssetCopy(HgAsset<T>* asset)
{
    hgAssert(asset != nullptr);
    ++asset->refCount;
    return asset;
}

/**
 * HgAssetHandle serialization
 */
template<typename T>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgAsset<T>* asset)
{
    if (s->writing)
    {
        HgStringView path = asset->path;
        hgSerialize(arena, s, name, &path);
    }
    else
    {
        HgStringView path;
        hgSerialize(hgScratch(), s, name, &path);
        if (path != "")
            *asset = hgAssetLoad<T>(path);
        else
            *asset = {};
    }
}

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
