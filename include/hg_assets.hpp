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
struct HgAssetData {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset
     */
    T asset;
    /**
     * The reference count
     */
    u32 refCount;
    /**
     * Whether the asset has finished loading
     */
    HgFence isLoaded;
    /**
     * The unique path for caching
     */
    HgStringOwner path;
};

/**
 * A handle to an asset
 */
template<typename T>
struct HgAssetHandle {
    HgHandle handle;
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
    HgMap<HgStringView, HgAssetHandle<T>> map;
    /**
     * The asset pool
     */
    HgHandlePool<HgAssetData<T>> pool;
    /**
     * A mutex for concurrent load/unload
     */
    HgMutex mtx;
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
    hgAssets<T>.map = hgMapCreate<HgStringView, HgAssetHandle<T>>();
    hgAssets<T>.pool = hgPoolCreate<HgAssetData<T>>();
    hgAssets<T>.mtx = hgMutexCreate();
}

/**
 * Detsroy an asset manager
 */
template<typename T>
void hgAssetDeinit()
{
    hgMutexDestroy(hgAssets<T>.mtx);
    hgPoolDestroy(&hgAssets<T>.pool);
    hgMapDestroy(&hgAssets<T>.map);
}

/**
 * Load an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetLoadImpl(HgAssetData<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be loaded without implementation");
}

/**
 * Unload an asset, implemented per asset type, blocking
 */
template<typename T>
void hgAssetUnloadImpl(HgAssetData<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be unloaded without implementation");
}

/**
 * Create a unique empty asset
 */
template<typename T>
HgAssetHandle<T> hgAssetCreate()
{
    hgMutexAcquire(hgAssets<T>.mtx);
    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);
    hgMutexRelease(hgAssets<T>.mtx);

    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, handle);
    data->path = {};
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

    return {handle};
}

/**
 * Load an asset (or increment the ref count)
 */
template<typename T>
HgAssetHandle<T> hgAssetLoad(HgStringView path)
{
    hgMutexAcquire(hgAssets<T>.mtx);
    HgAssetHandle<T>* asset = hgMapGet(&hgAssets<T>.map, path);
    if (asset != nullptr)
    {
        ++hgPoolGet(&hgAssets<T>.pool, asset->handle)->refCount;
        hgMutexRelease(hgAssets<T>.mtx);
        return *asset;
    }

    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);
    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, handle);
    data->path = hgStringCreate(path);
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

    if (hgAssets<T>.map.count * 2 > hgAssets<T>.map.capacity)
        hgMapResize(&hgAssets<T>.map, hgAssets<T>.map.capacity * 2);
    hgMapAdd(&hgAssets<T>.map, data->path, {handle});

    hgFenceAttach(data->isLoaded, 1);
    hgMutexRelease(hgAssets<T>.mtx);
    hgAssetLoadImpl(data);
    hgFenceSignal(data->isLoaded, 1);

    return {handle};
}

/**
 * Load an asset asynchronously
 */
template<typename T>
HgAssetHandle<T> hgAssetLoadAsync(HgStringView path)
{
    hgMutexAcquire(hgAssets<T>.mtx);
    HgAssetHandle<T>* asset = hgMapGet(&hgAssets<T>.map, path);
    if (asset != nullptr)
    {
        hgAssert(hgPoolAlive(&hgAssets<T>.pool, asset->handle));
        ++hgPoolGet(&hgAssets<T>.pool, asset->handle)->refCount;
        hgMutexRelease(hgAssets<T>.mtx);
        return *asset;
    }

    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);
    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, handle);
    data->path = hgStringCreate(path);
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

    if (hgAssets<T>.map.count * 2 > hgAssets<T>.map.capacity)
        hgMapResize(&hgAssets<T>.map, hgAssets<T>.map.capacity * 2);
    hgMapAdd(&hgAssets<T>.map, data->path, {handle});

    hgThreadsCall(data->isLoaded, data, [](void* pdata)
    {
        hgAssetLoadImpl((HgAssetData<T>*)pdata);
    });
    hgMutexRelease(hgAssets<T>.mtx);

    return {handle};
}

/**
 * Hot reload an asset
 */
template<typename T>
void hgAssetReload(HgAssetHandle<T> asset)
{
    hgAssert(hgPoolAlive(&hgAssets<T>.pool, asset.handle));
    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, asset.handle);
    hgFenceAttach(data->isLoaded);
    hgAssetUnloadImpl(data);
    hgAssetLoadImpl(data);
    hgFenceSignal(data->isLoaded);
}

/**
 * Hot reload an asset asynchronously
 */
template<typename T>
void hgAssetReloadAsync(HgAssetHandle<T> asset)
{
    hgAssert(hgPoolAlive(&hgAssets<T>.pool, asset.handle));
    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, asset.handle);
    hgThreadsCall(data->isLoaded, data, [](void* pdata)
    {
        HgAssetData<T>* data = (HgAssetData<T>*)pdata;
        hgAssetUnloadImpl(data);
        hgAssetLoadImpl(data);
    });
}

/**
 * Destroy an asset and unload it (or decrement the ref count)
 */
template<typename T>
void hgAssetUnload(HgAssetHandle<T> asset)
{
    if (hgPoolAlive(&hgAssets<T>.pool, asset.handle))
    {
        HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, asset.handle);
        hgMutexAcquire(hgAssets<T>.mtx);
        if (--data->refCount == 0)
        {
            hgAssetUnloadImpl(data);

            if (data->path != nullptr)
            {
                hgMapRemove(&hgAssets<T>.map, data->path);
                hgStringDestroy(&data->path);
            }
            hgFenceDestroy(data->isLoaded);
            hgPoolFree(&hgAssets<T>.pool, asset.handle);
        }
        hgMutexRelease(hgAssets<T>.mtx);
    }
}

/**
 * Increment the asset's reference count
 */
template<typename T>
HgAssetHandle<T> hgAssetCopy(HgAssetHandle<T> asset)
{
    hgAssert(hgPoolAlive(&hgAssets<T>.pool, asset.handle));
    ++hgPoolGet(&hgAssets<T>.pool, asset.handle)->refCount;
    return asset;
}

/**
 * Returns the asset's data
 */
template<typename T>
T* hgAssetGet(HgAssetHandle<T> asset)
{
    if (!hgPoolAlive(&hgAssets<T>.pool, asset.handle))
        return nullptr;

    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, asset.handle);
    hgAssert(data->refCount > 0);
    hgFenceWaitIndefinite(data->isLoaded);
    return &data->asset;
}

/**
 * Returns the path associated with an asset
 */
template<typename T>
HgStringView hgAssetPath(HgAssetHandle<T> asset)
{
    return hgPoolGet(&hgAssets<T>.pool, asset.handle)->path;
}

/**
 * HgAssetHandle serialization
 */
template<typename T>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgAssetHandle<T>* asset)
{
    if (s->writing)
    {
        HgStringView path = hgAssetPath(*asset);
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
typedef HgAssetHandle<HgBinary> HgBinaryHandle;

/**
 * HgBinary asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgBinary>* data);

/**
 * HgBinary asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgBinary>* data);

/**
 * Store a binary file to disc
 *
 * Parameters
 * - bin The binary to store
 * - path The file path to store at
 * - fence The fence to signal on completion
 */
void hgBinaryStore(HgBinary* bin, HgStringView path, HgFence fence);

#endif // HG_ASSETS_HPP
