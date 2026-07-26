#pragma once

#include "hg_types.hpp"
#include "hg_strings.hpp"
#include "hg_containers.hpp"

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
    Map<StringView, AssetData<T>*> map{};
    /**
     * The asset pool
     */
    Pool<AssetData<T>> pool{};
};

/**
 * Global per type asset managers
 */
template<typename T>
inline AssetManager<T> assets{};

/**
 * Load an asset, implemented per asset type, should be blocking
 */
template<typename T>
void assetLoadImpl(AssetData<T>* data)
{
    static_cast<void>(data);
    static_assert(false, "Asset type cannot be loaded without template specialization");
}

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
                assets<T>.map.remove(data->path);

            assets<T>.pool.free(data);
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
    return assets<T>.pool.alloc();
}

/**
 * Load an asset (or create a new reference)
 */
template<typename T>
Asset<T> load(StringView path)
{
    AssetData<T>** asset = assets<T>.map.get(path);
    if (asset != nullptr)
        return *asset;

    AssetData<T>* data = assets<T>.pool.alloc();

    data->path = String::create(path);
    assets<T>.map.add(data->path, data);

    assetLoadImpl(data);
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
 * Binary asset load implementation
 */
template<>
void assetLoadImpl(AssetData<Binary>* data);

/**
 * Store a binary file to disc
 *
 * Returns
 * - Whether the write succeeded
 */
bool binaryStore(BinaryView bin, StringView path);

} // namespace hg

