#pragma once

#include "hg/error.hpp"
#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/binary.hpp"
#include "hg/memory.hpp"
#include "hg/math.hpp"
#include "hg/product.hpp"
#include "hg/sum.hpp"
#include "hg/maybe.hpp"
#include "hg/smart_ptr.hpp"
#include "hg/array.hpp"
#include "hg/set.hpp"
#include "hg/map.hpp"

namespace hg {

/**
 * A serialized data node
 */
struct SerialNode;

/**
 * A serialized data node's children
 */
struct SerialObject {
    /**
     * The number of children
     */
    u32 childCount = 0;
    /**
     * The first child in the list
     */
    SerialNode* firstChild = nullptr;
};

/**
 * The data in a serial node
 */
using SerialData = Sum<SerialObject, StringView, i64, f64, bool>;

/**
 * A serialized data node
 */
struct SerialNode {
    /**
     * The parent object
     */
    SerialNode* parent = nullptr;
    /**
     * The next node in the parent object
     */
    SerialNode* next = nullptr;
    /**
     * The serialized data
     */
    SerialData data{};
};

/**
 * The data for serialization
 */
struct Serializer {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The root node
     */
    SerialNode* root = nullptr;
    /**
     * The current object
     */
    SerialNode* parent = nullptr;
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
Serializer serialWriter(Arena* arena);

/**
 * Begin a serial reader
 */
Serializer serialReader(Arena* arena, SerialNode* begin);

/**
 * The preamble to serializing a primitive node, generally only used internally
 */
void serializeNodeStart(Serializer* s);

/**
 * Begin serializing an object or array
 */
void serializeBegin(Serializer* s, u32* size = nullptr);

/**
 * Begin serializing an object or array
 */
void serializeEnd(Serializer* s);

/**
 * Serialize a value of unknown type
 */
bool serializeVoid(Serializer* s, Span<void> data);

/**
 * Serialize a value, should be overridden
 */
template<typename T>
bool serialize(Serializer* s, T* val)
{
    return serializeVoid(s, {val, sizeof(*val)});
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
 * Integer serialization
 */
template<std::integral T>
bool serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = static_cast<i64>(*val);
    }
    else
    {
        if (!s->current->data.is<i64>())
        {
            setError("Unexpected type while deserializing integer");
            return false;
        }
        *val = static_cast<T>(s->current->data.get<i64>());
    }
    return true;
}

/**
 * Float serialization
 */
template<std::floating_point T>
bool serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = f64{*val};
    }
    else
    {
        if (!s->current->data.is<f64>())
        {
            setError("Unexpected type while deserializing float");
            return false;
        }
        *val = static_cast<T>(s->current->data.get<f64>());
    }
    return true;
}

/**
 * bool serialization
 */
template<>
bool serialize(Serializer* s, bool* val);

/**
 * Vec2 serialization
 */
template<>
bool serialize(Serializer* s, Vec2* val);

/**
 * Vec3 serialization
 */
template<>
bool serialize(Serializer* s, Vec3* val);

/**
 * Vec4 serialization
 */
template<>
bool serialize(Serializer* s, Vec4* val);

/**
 * Mat2 serialization
 */
template<>
bool serialize(Serializer* s, Mat2* val);

/**
 * Mat3 serialization
 */
template<>
bool serialize(Serializer* s, Mat3* val);

/**
 * Mat4 serialization
 */
template<>
bool serialize(Serializer* s, Mat4* val);

/**
 * Complex serialization
 */
template<>
bool serialize(Serializer* s, Complex* val);

/**
 * Quat serialization
 */
template<>
bool serialize(Serializer* s, Quat* val);

/**
 * Product serialization
 */
template<typename... Ts>
bool serialize(Serializer* s, Product<Ts...>* product)
{
    if constexpr (sizeof...(Ts) > 0)
    {
        return [&]<u64... Is>(std::integer_sequence<u64, Is...>)
        {
            return serializeObject(s, &product->template get<Is>()...);
        }(std::make_integer_sequence<u64, sizeof...(Ts)>{});
    }
    else
    {
        return true;
    }
}

/**
 * Sum serialization
 */
template<typename... Ts>
bool serialize(Serializer* s, Sum<Ts...>* sum)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    u32 tag = sum->tag;
    if (!serialize(s, &tag))
        return false;

    if (tag < sum->count)
    {
        if (!s->writing && tag != sum->tag)
        {
            sum->tag = tag;
            sum->call([&](auto& val) { new (&val) std::remove_cvref_t<decltype(val)>{}; });
        }
        return sum->call([&](auto& val) { return serialize(s, &val); });
    }
    else
    {
        if (!s->writing)
            *sum = {};
        return true;
    }
}

/**
 * Maybe serialization
 */
template<typename T>
bool serialize(Serializer* s, Maybe<T>* maybe)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    bool has = maybe->has;
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (!s->writing && !maybe->has)
            *maybe = some<T>();
        return serialize(s, &maybe->val);
    }
    else
    {
        if (!s->writing)
            *maybe = {};
        return true;
    }
}

/**
 * String serialization
 */
template<>
bool serialize(Serializer* s, String* val);

/**
 * Binary serialization
 */
template<>
bool serialize(Serializer* s, Binary* val);

/**
 * UniquePtr serialization
 */
template<typename T>
bool serialize(Serializer* s, UniquePtr<T>* ptr)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    bool has = *ptr != nullptr;
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (!s->writing)
            *ptr = makeUnique<T>();
        return serialize(s, ptr->ptr);
    }
    else
    {
        return true;
    }
}

/**
 * Array serialization
 */
template<typename T>
bool serialize(Serializer* s, Array<T>* arr)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    if (s->writing)
    {
        serialize(s, &arr->count);
        serialize(s, &arr->capacity);
    }
    else
    {
        u32 count;
        if (!serialize(s, &count))
            return false;

        u32 capacity;
        if (!serialize(s, &capacity))
            return false;

        *arr = Array<T>{count, capacity};
    }

    for (u32 i = 0; i < arr->count; ++i)
    {
        if (!serialize(s, arr->vals + i))
            return false;
    }
    return true;
}

/**
 * Set serialization
 */
template<typename V>
bool serialize(Serializer* s, Set<V>* set)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    if (s->writing)
    {
        serialize(s, &set->capacity);
        serialize(s, &set->count);

        set->forEach([&](const V& val)
        {
            serialize(s, const_cast<V*>(&val));
        });
    }
    else
    {
        u32 capacity;
        if (!serialize(s, &capacity))
            return false;

        u32 count;
        if (!serialize(s, &count))
            return false;

        *set = Set<V>{capacity};
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
 * Map serialization
 */
template<typename K, typename V>
bool serialize(Serializer* s, Map<K, V>* map)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    if (s->writing)
    {
        serialize(s, &map->capacity);
        serialize(s, &map->count);

        map->forEach([&](const K& key, V& val)
        {
            serializeObject(s, const_cast<K*>(&key), &val);
        });
    }
    else
    {
        u32 capacity;
        if (!serialize(s, &capacity))
            return false;

        u32 count;
        if (!serialize(s, &count))
            return false;

        *map = Map<K, V>{capacity};
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
bool isSerialData(BinaryView bin);

/**
 * Write serialized data in a binary format
 */
BinaryView writeSerialBinary(Arena* arena, Serializer* data);

/**
 * Read binary data to be deserialized
 */
Serializer readSerialBinary(Arena* arena, BinaryView bin);

} // namespace hg
