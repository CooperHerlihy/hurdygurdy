#pragma once

#include "hg/inttypes.hpp"
#include "hg/error.hpp"
#include "hg/serialization.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace hg {

/**
 * Load a file into memory
 */
std::optional<std::vector<u8>> loadFile(std::string_view path);

/**
 * Store a file to disc
 */
bool storeFile(std::span<u8> bin, std::string_view path);

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
    std::string path{};
};

/**
 * An asset manager
 */
template<typename T>
struct AssetManager {
    /**
     * The asset lookup
     */
    static inline HashMap<std::string_view, std::unique_ptr<AssetData<T>>> map{};
    /**
     * Unique assets
     */
    static inline HashMap<AssetData<T>*, std::unique_ptr<AssetData<T>>> unique{};
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
            if (data->path.empty())
                AssetManager<T>::unique.remove(data);
            else
                AssetManager<T>::map.remove(data->path);
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
    std::unique_ptr<AssetData<T>> data = std::make_unique<AssetData<T>>();
    Asset<T> asset = data.get();
    AssetManager<T>::unique.add(data.get(), std::move(data));
    return asset;
}

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T> newAsset(const T& val)
{
    std::unique_ptr<AssetData<T>> data = std::make_unique<AssetData<T>>(val);
    Asset<T> asset = data.get();
    AssetManager<T>::unique.add(data.get(), std::move(data));
    return asset;
}

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T> newAsset(T&& val)
{
    std::unique_ptr<AssetData<T>> data = std::make_unique<AssetData<T>>(std::move(val));
    Asset<T> asset = data.get();
    AssetManager<T>::unique.add(data.get(), std::move(data));
    return asset;
}

/**
 * Default store an asset to a path, may be overridden
 */
template<typename T>
bool assetStoreImpl(const T& asset, std::string_view path)
{
    Serializer s = serialWriter();
    serialize(&s, const_cast<T*>(&asset));

    std::vector<u8> bin = writeSerialBinary(&s);
    return storeFile(bin, path);
}

/**
 * A convenience to follow load name convention
 */
template<typename T>
bool store(const T& asset, std::string_view path)
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
    std::optional<std::vector<u8>> bin = loadFile(data->path);
    if (!bin.has_value())
    {
        setError("Could not load asset file: {}", data->path.c_str());
        return false;
    }

    if (isSerialData(*bin))
    {
        std::optional<Serializer> s = readSerialBinary(*bin);
        if (!s.has_value() || !serialize(&*s, &data->asset))
        {
            setError("Could not deserialize serial data from file: {}", data->path.c_str());
            return false;
        }
        return true;
    }

    setError("Asset file does not contain serial data: {}", data->path.c_str());
    return false;
}

/**
 * Load an asset (or create a new reference)
 */
template<typename T>
Asset<T> load(std::string_view path)
{
    {
        std::unique_ptr<AssetData<T>>* asset = AssetManager<T>::map.get(path);
        if (asset != nullptr)
            return asset->get();
    }

    std::unique_ptr<AssetData<T>> data = std::make_unique<AssetData<T>>();
    AssetData<T>* dataPtr = data.get();
    Asset<T> asset = dataPtr;

    data->path = path;
    AssetManager<T>::map.add(data->path, std::move(data));
    if (!assetLoadImpl(dataPtr))
    {
        [[maybe_unused]]
        std::string_view e = getError();
        HG_WARN("Asset load impl encountered an error while loading file \"{}\": {}\n", path, e);
    }

    return asset;
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
bool reload(std::string_view path)
{
    AssetData<T>** asset = AssetManager<T>::map.get(path);
    if (asset != nullptr)
    {
        (*asset)->asset = {};
        if (!assetLoadImpl(*asset))
        {
            [[maybe_unused]]
            std::string_view e = getError();
            HG_WARN("Asset load impl encountered an error while loading file \"{}\": {}\n", path, e);
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
        std::string path;
        if (!serialize(s, &path))
            return false;
        if (path != "")
            *asset = load<T>(path);
        else
            *asset = {};
    }
    return true;
}

} // namespace hg
