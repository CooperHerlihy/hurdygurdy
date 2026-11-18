#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/hash.hpp"
#include "hg/utility.hpp"

#include <span>
#include <optional>
#include <tuple>
#include <variant>
#include <memory>
#include <string>
#include <array>
#include <vector>

namespace hg {

/**
 * A serialized data node
 */
struct SerialNode {
    /**
     * The parent object
     */
    SerialNode* parent = nullptr;
    /**
     * The current child, if reading
     */
    u32 currentChild = 0;
    /**
     * The children
     */
    std::vector<std::unique_ptr<SerialNode>> children{};
    /**
     * The serialized data
     */
    std::vector<u8> data{};
};

/**
 * The data for serialization
 */
class Serializer {
private:
    /**
     * The root node
     */
    std::unique_ptr<SerialNode> root = nullptr;
    /**
     * The current data node
     */
    SerialNode* current = nullptr;
    /**
     * Whether the serializer is reading or writing
     */
    bool writing = false;

public:
    /**
     * Construct empty
     */
    Serializer() noexcept = default;

    /**
     * Begin a serial writer
     */
    static Serializer writer();

    /**
     * Begin a serial reader
     */
    static Serializer reader(std::unique_ptr<SerialNode> begin);

    /**
     * Take the data structure from the serializer
     */
    std::unique_ptr<SerialNode> takeNode();

    /**
     * Write serialized data in a binary format
     */
    std::vector<u8> writeBinary();

    /**
     * Read binary data to be deserialized, nullopt and an error set on failure
     */
    static std::optional<Serializer> readBinary(std::span<const u8> bin);

    /**
     * Returns true if the serializer is writing to the data structure
     */
    bool isWriting() const
    {
        return writing;
    }

    /**
     * Returns true if the serializer is reading from the data structure
     */
    bool isReading() const
    {
        return !writing;
    }

    /**
     * Begin serializing an object or array
     */
    bool beginObject(u32* size = nullptr);

    /**
     * Begin serializing an object or array
     */
    void endObject();

    /**
     * Serialize data directly
     */
    bool serializeData(std::span<u8> data);

    /**
     * Serialize variable length data
     *
     * Note, if reading, the data pointed to should be copied
     */
    bool serializeVoid(std::span<const u8>* data);
};

/**
 * Serialize a value, should be overridden
 */
template<typename T>
bool serialize(Serializer* s, T* val)
{
    return s->serializeData({reinterpret_cast<u8*>(val), sizeof(*val)});
}

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
bool serializeObject(Serializer* s, Ts*... vals)
{
    s->beginObject();
    HG_DEFER(s->endObject());
    return (serialize(s, vals) && ...);
}

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
bool serialize(Serializer* s, T (*arr)[N])
{
    s->beginObject();
    HG_DEFER(s->endObject());
    for (T& val : *arr)
    {
        if (!serialize(s, &val))
            return false;
    }
    return true;
}

/**
 * optional serialization
 */
template<typename T>
bool serialize(Serializer* s, std::optional<T>* maybe)
{
    s->beginObject();
    HG_DEFER(s->endObject());

    bool has = maybe->has_value();
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (s->isReading() && !maybe->has_value())
            *maybe = std::make_optional<T>();
        return serialize(s, &maybe->value());
    }
    else
    {
        if (s->isReading())
            *maybe = std::nullopt;
        return true;
    }
}

/**
 * tuple serialization
 */
template<typename... Ts>
bool serialize(Serializer* s, std::tuple<Ts...>* tuple)
{
    if constexpr (sizeof...(Ts) > 0)
    {
        return [&]<u64... Is>(std::integer_sequence<u64, Is...>)
        {
            return serializeObject(s, &std::get<Is>(*tuple)...);
        }(std::make_integer_sequence<u64, sizeof...(Ts)>{});
    }
    else
    {
        return true;
    }
}

namespace internal {
    template<usize I = 0, typename... Ts>
    static void defaultInitVariant(std::variant<Ts...>& v, usize idx)
    {
        if constexpr (I < sizeof...(Ts))
        {
            HG_ASSERT(idx < sizeof...(Ts));
            if (idx == I)
                v.template emplace<I>();
            else
                defaultInitVariant<I + 1>(v, idx);
        }
    }
}

/**
 * variant serialization
 */
template<typename... Ts>
bool serialize(Serializer* s, std::variant<Ts...>* variant)
{
    s->beginObject();
    HG_DEFER(s->endObject());

    usize tag = variant->index();
    if (!serialize(s, &tag))
        return false;

    if (s->isReading() && tag != variant->index())
    {
        internal::defaultInitVariant(*variant, tag);
    }
    return std::visit([&](auto& val)
    {
        return serialize(s, &val);
    }, *variant);
}

/**
 * unique ptr serialization
 */
template<typename T>
bool serialize(Serializer* s, std::unique_ptr<T>* ptr)
{
    s->beginObject();
    HG_DEFER(s->endObject());

    bool has = *ptr != nullptr;
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (s->isReading())
            *ptr = std::make_unique<T>();
        return serialize(s, ptr->get());
    }
    else
    {
        return true;
    }
}

/**
 * string serialization
 */
template<>
bool serialize(Serializer* s, std::string* val);

/**
 * trivial array serialization
 */
template<typename T, usize I> requires std::is_trivially_copyable_v<T>
bool serialize(Serializer* s, std::array<T, I>* arr)
{
    s->beginObject();
    HG_DEFER(s->endObject());

    return s->serializeData({reinterpret_cast<u8*>(arr->data()), sizeof(T) * I});
}

/**
 * array serialization
 */
template<typename T, usize I>
bool serialize(Serializer* s, std::array<T, I>* arr)
{
    s->beginObject();
    HG_DEFER(s->endObject());

    for (T& val : *arr)
    {
        if (!serialize(s, &val))
            return false;
    }
    return true;
}

/**
 * trivial vector serialization
 */
template<typename T> requires std::is_trivially_copyable_v<T>
bool serialize(Serializer* s, std::vector<T>* arr)
{
    if (s->isWriting())
    {
        std::span<const u8> data{reinterpret_cast<u8*>(arr->data()), arr->size() * sizeof(T)};
        s->serializeVoid(&data);
    }
    else
    {
        std::span<const u8> data;
        if (!s->serializeVoid(&data))
            return false;
        *arr = std::vector<T>(reinterpret_cast<const T*>(data.data()), reinterpret_cast<const T*>(data.data()) + data.size() / sizeof(T));
    }
    return true;
}

/**
 * vector serialization
 */
template<typename T>
bool serialize(Serializer* s, std::vector<T>* vec)
{
    u32 count = assertCast<u32>(vec->size());
    s->beginObject(&count);
    HG_DEFER(s->endObject());

    if (s->isReading())
        vec->resize(count);

    for (u32 i = 0; i < vec->size(); ++i)
    {
        if (!serialize(s, vec->data() + i))
            return false;
    }
    return true;
}

/**
 * hash set serialization
 */
template<typename V>
bool serialize(Serializer* s, HashSet<V>* set)
{
    u32 count = assertCast<u32>(set->size());
    s->beginObject(&count);
    HG_DEFER(s->endObject());

    if (s->isWriting())
    {
        set->forEach([&](const V& val)
        {
            serialize(s, const_cast<V*>(&val));
        });
    }
    else
    {
        set->clear();
        for (u32 i = 0; i < count; ++i)
        {
            V val;
            if (!serialize(s, &val))
                return false;
            set->add(val);
        }
    }
    return true;
}

/**
 * hash map serialization
 */
template<typename K, typename V>
bool serialize(Serializer* s, HashMap<K, V>* map)
{
    u32 count = assertCast<u32>(map->size());
    s->beginObject(&count);
    HG_DEFER(s->endObject());

    if (s->isWriting())
    {
        map->forEach([&](const K& key, V& val)
        {
            serializeObject(s, const_cast<K*>(&key), &val);
        });
    }
    else
    {
        map->clear();
        for (u32 i = 0; i < count; ++i)
        {
            K key;
            V val;
            if (!serializeObject(s, &key, &val))
                return false;
            map->add(std::move(key), std::move(val));
        }
    }
    return true;
}

} // namespace hg
