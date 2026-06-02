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

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_containers.hpp"
#include "hg_concurrency.hpp"

/**
 * Initialize all default HurdyGurdy asset types
 */
void hgAssetInitDefaults(
    HgArena* arena,
    u32 maxBinaries,
    u32 maxTextures,
    u32 maxGpuTextures,
    u32 maxMeshes,
    u32 maxGpuMeshes,
    u32 maxAudios);

/**
 * Deinitialize all default HurdyGurdy asset types
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
    HgPool pool;
    /**
     * The asset data
     */
    HgAssetData<T>* data;
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
 * Create an asset manager
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxAssets The max number of assets which can be created in the manager
 *
 * Returns
 * - The created asset manager
 */
template<typename T>
void hgAssetInit(HgArena* arena, u32 maxAssets)
{
    hgAssets<T>.map = hgMapCreate<HgStringView, HgAssetHandle<T>>(arena, maxAssets * 2);
    hgAssets<T>.pool = hgPoolCreate(arena, maxAssets);
    hgAssets<T>.data = hgAlloc<HgAssetData<T>>(arena, maxAssets);
    hgAssets<T>.mtx = hgMutexCreate();
}

/**
 * Detsroy an asset manager
 */
template<typename T>
void hgAssetDeinit()
{
    hgMutexDestroy(hgAssets<T>.mtx);
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

    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(handle)];
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
        ++hgAssets<T>.data[hgHandleIdx(asset->handle)].refCount;
        hgMutexRelease(hgAssets<T>.mtx);
        return *asset;
    }

    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);
    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(handle)];
    data->path = hgStringAlloc(path);
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

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
        ++hgAssets<T>.data[hgHandleIdx(asset->handle)].refCount;
        hgMutexRelease(hgAssets<T>.mtx);
        return *asset;
    }

    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);
    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(handle)];
    data->path = hgStringAlloc(path);
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

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
    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(asset.handle)];
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
    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(asset.handle)];
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
        HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(asset.handle)];
        hgMutexAcquire(hgAssets<T>.mtx);
        if (--data->refCount == 0)
        {
            hgAssetUnloadImpl(data);

            if (data->path != nullptr)
            {
                hgMapRemove(&hgAssets<T>.map, data->path);
                hgStringFree(&data->path);
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
    ++hgAssets<T>.data[hgHandleIdx(asset.handle)].ref_count;
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

    HgAssetData<T>* data = &hgAssets<T>.data[hgHandleIdx(asset.handle)];
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
    return hgAssets<T>.data[hgHandleIdx(asset.handle)].path;
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

/**
 * An error contained in the json
 */
struct HgJsonError {
    /**
     * The next error
     */
    HgJsonError* next;
    /**
     * The error message
     */
    HgStringView msg;
};

/**
 * A node in the json file
 */
struct HgJsonNode;

/**
 * The types contained in nodes
 */
enum HgJsonType : u32 {
    HgJsonType_none = 0,
    HgJsonType_struct,
    HgJsonType_field,
    HgJsonType_array,
    HgJsonType_string,
    HgJsonType_float,
    HgJsonType_integer,
    HgJsonType_bool,
};

/**
 * A field in a struct
 */
struct HgJsonField {
    /**
     * The next field
     */
    HgJsonField* next;
    /**
     * The name of the field
     */
    HgStringView name;
    /**
     * The value stored in the field
     */
    HgJsonNode* value;
};

/**
 * A struct contained in the json
 */
struct HgJsonStruct {
    /**
     * The first field
     */
    HgJsonField* fields;
};

/**
 * An element in an array
 */
struct HgJsonElem {
    /**
     * The next element
     */
    HgJsonElem* next;
    /**
     * The value stored in the element
     */
    HgJsonNode* value;
};

/**
 * An array contained in the json
 */
struct HgJsonArray {
    /**
     * The first element
     */
    HgJsonElem* elems;
};

/**
 * A node in the json file
 */
struct HgJsonNode {
    /**
     * The node's type
     */
    HgJsonType type;
    /**
     * The value in the node
     */
    union {
        HgJsonStruct jstruct;
        HgJsonField field;
        HgJsonArray array;
        HgStringView string;
        f64 floating;
        i64 integer;
        bool boolean;
    };
};

/**
 * A parsed Json file
 */
struct HgJson {
    /**
     * The successfully parsed nodes
     */
    HgJsonNode* file;
    /**
     * The errors found
     */
    HgJsonError* errors;
};

/**
 * A binary file asset handle
 */
typedef HgAssetHandle<HgJson> HgJsonHandle;

/**
 * HgJson asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgJson>* data);

/**
 * HgJson asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgJson>* data);

/**
 * Parses json text into a tree
 *
 * Parameters
 * - arena The arena to allocate from
 * - text The json text to parse
 *
 * Returns
 * - The parsed json, errors contained inside
 */
HgJson hgParseJson(HgArena* arena, HgStringView text);

#endif // HG_ASSETS_HPP
