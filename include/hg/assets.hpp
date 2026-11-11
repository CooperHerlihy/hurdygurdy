#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/binary.hpp"
#include "hg/map.hpp"
#include "hg/pool.hpp"
#include "hg/serialization.hpp"
#include "hg/filesystem.hpp"

namespace hg {

/**
 * The data associated with assets
 */
template<typename T>
struct AssetData {
    /**
     * The asset data
     */
    T asset{};
    /**
     * The reference count
     */
    u32 refCount = 0;
    /**
     * The unique path for caching
     */
    String path{};
};

/**
 * An asset manager
 */
template<typename T>
struct AssetManager {
    /**
     * The asset lookup
     */
    static inline Map<StringView, AssetData<T>*> map{};
    /**
     * The asset pool
     */
    static inline Pool<AssetData<T>> pool{};
};

/**
 * An asset reference
 */
template<typename T>
struct Asset {
    /**
     * The asset data
     */
    AssetData<T>* data = nullptr;

    /**
     * Construct empty
     */
    Asset() noexcept = default;

    /**
     * Create a new asset reference
     */
    Asset(AssetData<T>* dataVal)
        : data{dataVal}
    {
        if (data != nullptr)
            ++data->refCount;
    }

    /**
     * Destroy the asset reference
     */
    ~Asset() noexcept
    {
        if (data != nullptr && --data->refCount == 0)
        {
            if (data->path != "")
                AssetManager<T>::map.remove(data->path);

            AssetManager<T>::pool.free(data);
        }
    }

    /**
     * Dereference to access asset
     */
    T& operator*() const
    {
        return data->asset;
    }

    /**
     * Dereference underlying
     */
    T* operator->() const
    {
        return &data->asset;
    }

    /**
     * Create a new asset reference
     */
    Asset clone() const
    {
        return Asset{data};
    }

    /**
     * Move construct
     */
    Asset(Asset&& other) noexcept
        : data{std::exchange(other.data, {})}
    {}

    /**
     * Move assign
     */
    Asset& operator=(Asset&& other) noexcept
    {
        if (this != &other)
        {
            this->~Asset();
            new (this) Asset{std::move(other)};
        }
        return *this;
    }

    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
};

/**
 * Compare assets
 */
template<typename T>
bool operator==(const Asset<T>& lhs, const Asset<T>& rhs)
{
    return lhs.data == rhs.data;
}

/**
 * Compare assets
 */
template<typename T>
bool operator!=(const Asset<T>& lhs, const Asset<T>& rhs)
{
    return !(lhs == rhs);
}

/**
 * Compare assets
 */
template<typename T>
bool operator==(const Asset<T>& lhs, std::nullptr_t)
{
    return lhs.data == nullptr;
}

/**
 * Compare assets
 */
template<typename T>
bool operator!=(const Asset<T>& lhs, std::nullptr_t)
{
    return !(lhs == nullptr);
}

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T> newAsset()
{
    return AssetManager<T>::pool.alloc();
}

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T> newAsset(const T& val)
{
    return AssetManager<T>::pool.alloc(val);
}

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T> newAsset(T&& val)
{
    return AssetManager<T>::pool.alloc(std::move(val));
}

/**
 * Default store an asset to a path, may be overridden
 */
template<typename T>
bool assetStoreImpl(const T& asset, StringView path)
{
    ArenaScope scratch = getScratch();

    Serializer s = serialWriter(scratch);
    serialize(&s, const_cast<T*>(&asset));

    BinaryView bin = writeSerialBinary(scratch, &s);
    return storeFile(bin, path);
}

/**
 * A convenience to follow load name convention
 */
template<typename T>
bool store(const T& asset, StringView path)
{
    return assetStoreImpl(asset, path);
}

/**
 * Default load an asset from a path, may be overridden to support other file types
 *
 * Note, this function should be blocking, not async
 */
template<typename T>
bool assetLoadImpl(AssetData<T>* data)
{
    Maybe<Binary> bin = loadFile(data->path);
    if (!bin.has)
    {
        setError("Could not load asset file: %.*s", (int)data->path.length, data->path.chars);
        return false;
    }

    if (isSerialData(*bin))
    {
        ArenaScope scratch = getScratch();

        Serializer s = readSerialBinary(scratch, *bin);
        if (!serialize(&s, &data->asset))
        {
            setError("Could not deserialize serial data from file: %.*s", (int)data->path.length, data->path.chars);
            return false;
        }

        return true;
    }

    setError("Asset file does not contain serial data: %.*s", (int)data->path.length, data->path.chars);
    return false;
}

/**
 * Load an asset (or create a new reference)
 */
template<typename T>
Asset<T> load(StringView path)
{
    AssetData<T>** asset = AssetManager<T>::map.get(path);
    if (asset != nullptr)
        return *asset;

    AssetData<T>* data = AssetManager<T>::pool.alloc();

    data->path = String{path};
    AssetManager<T>::map.add(data->path, data);

    if (!assetLoadImpl(data))
    {
        [[maybe_unused]]
        StringView e = getError();
        HG_WARN("Asset load impl encountered an error while loading file \"%.*s\": %.*s\n",
            (int)path.length, path.chars,
            (int)e.length, e.chars);
    }
    return data;
}

/**
 * Hot reload an asset
 */
template<typename T>
void reload(const Asset<T>& asset)
{
    if (asset.data != nullptr)
    {
        *asset = {};
        assetLoadImpl(asset.data);
    }
}

/**
 * Hot reload an asset from a path
 */
template<typename T>
bool reload(StringView path)
{
    AssetData<T>** asset = AssetManager<T>::map.get(path);
    if (asset != nullptr)
    {
        (*asset)->asset = {};
        if (!assetLoadImpl(*asset))
        {
            [[maybe_unused]]
            StringView e = getError();
            HG_WARN("Asset load impl encountered an error while loading file \"%.*s\": %.*s\n",
                (int)path.length, path.chars,
                (int)e.length, e.chars);
            return false;
        }
    }
    return true;
}

/**
 * Asset serialization
 */
template<typename T>
bool serialize(Serializer* s, Asset<T>* asset)
{
    if (s->writing)
    {
        serialize(s, &asset->data->path);
    }
    else
    {
        String path;
        if (!serialize(s, &path))
            return false;
        if (path != "")
            *asset = load<T>(path);
        else
            *asset = {};
    }
    return true;
}

/**
 * Binary asset load implementation
 */
template<>
bool assetLoadImpl(AssetData<Binary>* data);

} // namespace hg

