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
#include "hg_math.hpp"
#include "hg_memory.hpp"
#include "hg_strings.hpp"
#include "hg_concurrency.hpp"
#include "hg_gpu.hpp"

/**
 * Initialize all default HurdyGurdy asset types
 */
void hgAssetInitDefaults(
    HgArena* arena,
    u32 maxBinaries,
    u32 maxTextures,
    u32 maxGpuTextures,
    u32 maxMeshes,
    u32 maxGpuMeshes);

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
    HgPool<HgAssetData<T>> pool;
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
    hgAssets<T>.pool = hgPoolCreate<HgAssetData<T>>(arena, maxAssets);
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
        ++hgPoolGet(&hgAssets<T>.pool, asset->handle)->refCount;
        hgMutexRelease(hgAssets<T>.mtx);
        return *asset;
    }

    HgHandle handle = hgPoolAlloc(&hgAssets<T>.pool);

    HgAssetData<T>* data = hgPoolGet(&hgAssets<T>.pool, handle);
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
                hgStringFree(&data->path);
            }
            hgFenceDestroy(data->isLoaded);
            hgPoolFree(&hgAssets<T>.pool, asset.handle);
        }
        hgMutexRelease(hgAssets<T>.mtx);
    }
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
 * A binary asset
 */
struct HgBinary {
    /**
     * The data in the file
     */
    void* data;
    /**
     * The size of the file in bytes
     */
    u64 size;
};

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
 * Resize the file
 *
 * Parameters
 * - arena The arena to allocate from
 * - newSize The new size of the file in bytes
 */
HgBinary hgBinaryResize(HgArena* arena, const HgBinary* bin, u64 newSize);

/**
 * Read data at index into a buffer
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 * - dst A pointer to store the read data
 * - size The size in bytes to read
 */
void hgBinaryRead(const HgBinary* bin, u64 idx, void* dst, u64 len);

/**
 * Read data of arbitrary type from the file
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 */
template<typename T>
T hgBinaryRead(const HgBinary* bin, u64 idx)
{
    T ret;
    hgBinaryRead(bin, idx, &ret, sizeof(T));
    return ret;
}

/**
 * Overwrite data at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 * - size The size of the data in bytes
 */
void hgBinaryOverwrite(HgBinary* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void hgBinaryOverwrite(HgBinary* bin, u64 idx, const T& src)
{
    hgBinaryOverwrite(bin, idx, &src, sizeof(T));
}

/**
 * A texture asset
 */
struct HgTexture {
    /**
     * The width of the texture in pixels
     */
    u32 width;
    /**
     * The height of the texture in pixels
     */
    u32 height;
    /**
     * The depth of the texture in pixels
     */
    u32 depth;
    /**
     * The format of each pixel
     */
    HgFormat format;
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels;
};

/**
 * A handle to a texture
 */
typedef HgAssetHandle<HgTexture> HgTextureHandle;

/**
 * HgTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgTexture>* data);

/**
 * HgTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgTexture>* data);

/**
 * Store an image to disc in the png format
 */
void hgTextureStorePng(HgTexture* texture, HgStringView path, HgFence fence);

/**
 * A texture asset stored on the gpu
 */
struct HgGpuTexture {
    /**
     * The image
     */
    HgGpuImage image;
    /**
     * The image view
     */
    HgGpuView view;
};

/**
 * A handle to a texture asset
 */
typedef HgAssetHandle<HgGpuTexture> HgGpuTextureHandle;

/**
 * HgGpuTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgGpuTexture>* data);

/**
 * HgGpuTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgGpuTexture>* data);

/**
 * A vertex in a mesh
 */
struct HgMeshVertex {
    /**
     * The vertex position
     */
    alignas(16) HgVec3 pos;
    /**
     * The vertex normal
     */
    alignas(16) HgVec3 norm;
    /**
     * The vertex tangent
     */
    alignas(16) HgVec4 tan;
    /**
     * The vertex uv coordinate
     */
    alignas(16) HgVec2 uv;
};

/**
 * A 3d mesh asset
 */
struct HgMesh {
    /**
     * The file index of the first vertex
     */
    HgMeshVertex* vertices;
    /**
     * The file index of the first geometry index
     */
    u32* indices;
    /**
     * The number of vertices
     */
    u32 vertexCount;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount;
    /**
     * How the vertices should be interpreted in sequence
     */
    HgGpuTopology topology;
};

/**
 * A handle to a 2d mesh asset
 */
typedef HgAssetHandle<HgMesh> HgMeshHandle;

/**
 * HgMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgMesh>* data);

/**
 * HgMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgMesh>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void hgMeshStoreGltf(HgMesh* data, HgStringView path, HgFence fence);

/**
 * A 3d mesh asset stored on the gpu
 */
struct HgGpuMesh {
    /**
     * The vertex buffer
     */
    HgGpuBuffer vertexBuffer;
    /**
     * The index buffer
     */
    HgGpuBuffer indexBuffer;
    /**
     * The number of vertices
     */
    u32 vertexCount;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount;
};

/**
 * A gpu mesh asset handle
 */
typedef HgAssetHandle<HgGpuMesh> HgGpuMeshHandle;

/**
 * HgGpuMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAssetData<HgGpuMesh>* data);

/**
 * HgGpuMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAssetData<HgGpuMesh>* data);

#endif // HG_ASSETS_HPP
