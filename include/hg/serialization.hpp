#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/hash.hpp"
#include "hg/error.hpp"

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
struct Serializer {
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
};

/**
 * Begin a serial writer
 */
Serializer serialWriter();

/**
 * Begin a serial reader
 */
Serializer serialReader(std::unique_ptr<SerialNode> begin);

/**
 * Begin serializing an object or array
 */
bool serializeBegin(Serializer* s, u32* size = nullptr);

/**
 * Begin serializing an object or array
 */
void serializeEnd(Serializer* s);

/**
 * Serialize data directly
 */
bool serializeVoid(Serializer* s, std::span<u8> data);

/**
 * Serialize a value, should be overridden
 */
template<typename T>
bool serialize(Serializer* s, T* val)
{
    return serializeVoid(s, {reinterpret_cast<u8*>(val), sizeof(*val)});
}

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
bool serializeObject(Serializer* s, Ts*... vals)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));
    return (serialize(s, vals) && ...);
}

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
bool serialize(Serializer* s, T (*arr)[N])
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));
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
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    bool has = maybe->has_value();
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (!s->writing && !maybe->has_value())
            *maybe = std::make_optional<T>();
        return serialize(s, &maybe->value());
    }
    else
    {
        if (!s->writing)
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
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    usize tag = variant->index();
    if (!serialize(s, &tag))
        return false;

    if (!s->writing && tag != variant->index())
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
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    bool has = *ptr != nullptr;
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (!s->writing)
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
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    return serializeVoid(s, {reinterpret_cast<u8*>(arr), sizeof(*arr)});
}

/**
 * array serialization
 */
template<typename T, usize I>
bool serialize(Serializer* s, std::array<T, I>* arr)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

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
    if (!s->writing)
    {
        if (s->current == nullptr)
            s->current = s->root.get();
        if (s->current->currentChild >= s->current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }
        arr->resize(s->current->children[s->current->currentChild]->data.size() / sizeof(T));
    }
    return serializeVoid(s, {reinterpret_cast<u8*>(arr->data()), arr->size() * sizeof(T)});
}

/**
 * vector serialization
 */
template<typename T>
bool serialize(Serializer* s, std::vector<T>* vec)
{
    u32 count = vec->size();
    serializeBegin(s, &count);
    HG_DEFER(serializeEnd(s));

    if (!s->writing)
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
    u32 count = set->count;
    serializeBegin(s, &count);
    HG_DEFER(serializeEnd(s));

    if (s->writing)
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
    u32 count = map->count;
    serializeBegin(s, &count);
    HG_DEFER(serializeEnd(s));

    if (s->writing)
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

/**
 * Returns whether the binary contains serial data
 */
bool isSerialData(std::span<const u8> bin);

/**
 * Write serialized data in a binary format
 */
std::vector<u8> writeSerialBinary(Serializer* data);

/**
 * Read binary data to be deserialized, nullopt and an error set on failure
 */
std::optional<Serializer> readSerialBinary(std::span<const u8> bin);

} // namespace hg
