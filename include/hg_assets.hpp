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

#ifndef HG_ASSETS_HPP
#define HG_ASSETS_HPP

#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_serialization.hpp"

namespace hg {

/**
 * Initialize all default asset types
 *
 * Asset types:
 * - Binary
 * - Texture
 * - GpuTexture
 * - Mesh
 * - GpuMesh
 * - Audio
 */
void assetInitDefaults();

/**
 * Deinitialize all default asset types
 */
void assetDeinitDefaults();

/**
 * The data associated with assets
 */
template<typename T>
struct Asset {
    /**
     * The asset data
     */
    T asset;
    /**
     * The reference count
     */
    u32 refCount;
    /**
     * The unique path for caching
     */
    String path;
};

/**
 * An asset manager
 */
template<typename T>
struct AssetManager {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset lookup
     */
    Map<String, Asset<T>*> map;
    /**
     * The asset pool
     */
    Pool pool;
};

/**
 * Global per type asset managers
 */
template<typename T>
inline AssetManager<T> assets{};

/**
 * Initialize a type's asset manager
 */
template<typename T>
void assetInit();

/**
 * Deinitialize an type's asset manager
 */
template<typename T>
void assetDeinit();

/**
 * Load an asset, implemented per asset type, should be blocking
 */
template<typename T>
void assetLoadImpl(Asset<T>* data);

/**
 * Unload an asset, implemented per asset type, should be blocking
 */
template<typename T>
void assetUnloadImpl(Asset<T>* data);

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T>* assetCreate();

/**
 * Load an asset (or increment the ref count)
 */
template<typename T>
Asset<T>* assetLoad(String path);

/**
 * Destroy an asset and unload it (or decrement the ref count)
 */
template<typename T>
void assetUnload(Asset<T>* asset);

/**
 * Hot reload an asset
 */
template<typename T>
void assetReload(Asset<T>* asset);

/**
 * Increment the asset's reference count
 */
template<typename T>
Asset<T>* assetCopy(Asset<T>* asset);

/**
 * AssetHandle serialization
 */
template<typename T>
void serialize(Serializer* s, Asset<T>** asset);

/**
 * A binary file asset handle
 */
typedef Asset<Binary> BinaryAsset;

/**
 * Binary asset load implementation
 */
template<>
void assetLoadImpl(Asset<Binary>* data);

/**
 * Binary asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Binary>* data);

/**
 * Store a binary file to disc
 *
 * Returns
 * - Whether the write succeeded
 */
bool binaryStore(Binary bin, String path);

} // namespace hg

#endif // ASSETS_HPP
