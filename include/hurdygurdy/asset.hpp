#pragma once

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
void assetLoadImpl(AssetData<T>* data);

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
    Asset(AssetData<T>* dataVal);

    /**
     * Destroy the asset reference
     */
    ~Asset() noexcept;

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
Asset<T> newAsset();

/**
 * Load an asset (or create a new reference)
 */
template<typename T>
Asset<T> load(StringView path);

/**
 * Hot reload an asset
 */
template<typename T>
void reload(const Asset<T>& asset);

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
