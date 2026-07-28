#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"
#include "hg/math.hpp"
#include "hg/product.hpp"
#include "hg/sum.hpp"
#include "hg/maybe.hpp"
#include "hg/smart_ptr.hpp"
#include "hg/array.hpp"
#include "hg/set.hpp"
#include "hg/map.hpp"
#include "hg/assets.hpp"

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
void serializeVoid(Serializer* s, Span<void> data);

/**
 * Serialize a value, should be overridden
 */
template<typename T>
void serialize(Serializer* s, T* val)
{
    serializeVoid(s, {val, sizeof(*val)});
}

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals)
{
    serializeBegin(s);
    (serialize(s, vals), ...);
    serializeEnd(s);
}

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N])
{
    serializeBegin(s);
    for (u64 i = 0; i < N; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

/**
 * Integer serialization
 */
template<std::integral T>
void serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = static_cast<i64>(*val);
    }
    else
    {
        HG_ASSERT(s->current->data.is<i64>());
        *val = static_cast<T>(s->current->data.get<i64>());
    }
}

/**
 * Float serialization
 */
template<std::floating_point T>
void serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = f64{*val};
    }
    else
    {
        HG_ASSERT(s->current->data.is<f64>());
        *val = static_cast<T>(s->current->data.get<f64>());
    }
}

/**
 * bool serialization
 */
template<>
void serialize(Serializer* s, bool* val);

/**
 * Vec2 serialization
 */
template<>
void serialize(Serializer* s, Vec2* val);

/**
 * Vec3 serialization
 */
template<>
void serialize(Serializer* s, Vec3* val);

/**
 * Vec4 serialization
 */
template<>
void serialize(Serializer* s, Vec4* val);

/**
 * Mat2 serialization
 */
template<>
void serialize(Serializer* s, Mat2* val);

/**
 * Mat3 serialization
 */
template<>
void serialize(Serializer* s, Mat3* val);

/**
 * Mat4 serialization
 */
template<>
void serialize(Serializer* s, Mat4* val);

/**
 * Complex serialization
 */
template<>
void serialize(Serializer* s, Complex* val);

/**
 * Quat serialization
 */
template<>
void serialize(Serializer* s, Quat* val);

/**
 * Product serialization
 */
template<typename... Ts>
void serialize(Serializer* s, Product<Ts...>* product)
{
    if constexpr (sizeof...(Ts) > 0)
    {
        [&]<u64... Is>(std::index_sequence<Is...>)
        {
            serializeObject(s, &product->template get<Is>()...);
        }(std::index_sequence_for<Ts...>{});
    }
}

/**
 * Sum serialization
 */
template<typename... Ts>
void serialize(Serializer* s, Sum<Ts...>* sum)
{
    serializeBegin(s);
    u32 tag = sum->tag;
    serialize(s, &tag);
    if (tag < sum->count)
    {
        if (!s->writing && tag != sum->tag)
        {
            sum->tag = tag;
            sum->call([&](auto& val) { new (&val) std::remove_cvref_t<decltype(val)>{}; });
        }
        sum->call([&](auto& val) { serialize(s, &val); });
    }
    else
    {
        if (!s->writing)
            *sum = {};
    }
    serializeEnd(s);
}

/**
 * Maybe serialization
 */
template<typename T>
void serialize(Serializer* s, Maybe<T>* maybe)
{
    serializeBegin(s);
    bool has = maybe->has;
    serialize(s, &has);
    if (has)
    {
        if (!s->writing && !maybe->has)
            *maybe = some<T>();
        serialize(s, &maybe->val);
    }
    else
    {
        if (!s->writing)
            *maybe = {};
    }
    serializeEnd(s);
}

/**
 * String serialization
 */
template<>
void serialize(Serializer* s, String* val);

/**
 * Binary serialization
 */
template<>
void serialize(Serializer* s, Binary* val);

/**
 * UniquePtr serialization
 */
template<typename T>
void serialize(Serializer* s, UniquePtr<T>* ptr)
{
    serializeBegin(s);
    bool has = *ptr != nullptr;
    serialize(s, &has);
    if (has)
    {
        if (!s->writing)
            *ptr = makeUnique<T>();
        serialize(s, ptr->ptr);
    }
    serializeEnd(s);
}

/**
 * Array serialization
 */
template<typename T>
void serialize(Serializer* s, Array<T>* arr)
{
    serializeBegin(s);
    if (s->writing)
    {
        serialize(s, &arr->count);
        serialize(s, &arr->capacity);
    }
    else
    {
        u32 count;
        u32 capacity;
        serialize(s, &count);
        serialize(s, &capacity);
        *arr = Array<T>{count, capacity};
    }
    for (u32 i = 0; i < arr->count; ++i)
    {
        serialize(s, arr->vals + i);
    }
    serializeEnd(s);
}

/**
 * Set serialization
 */
template<typename V>
void serialize(Serializer* s, Set<V>* set)
{
    serializeBegin(s);

    if (s->writing)
    {
        serialize(s, &set->capacity);
        serialize(s, &set->count);

        set->forEach([&](V* val)
        {
            serialize(s, val);
        });
    }
    else
    {
        u32 capacity;
        u32 count;
        serialize(s, &capacity);
        serialize(s, &count);

        *set = Set<V>{capacity};
        for (u32 i = 0; i < count; ++i)
        {
            V val;
            serialize(s, &val);
            set->add(val);
        }
    }

    serializeEnd(s);
}

/**
 * Map serialization
 */
template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* map)
{
    serializeBegin(s);

    if (s->writing)
    {
        serialize(s, &map->capacity);
        serialize(s, &map->count);

        map->forEach([&](K* key, V* val)
        {
            serializeObject(s, key, val);
        });
    }
    else
    {
        u32 capacity;
        u32 count;
        serialize(s, &capacity);
        serialize(s, &count);

        *map = Map<K, V>{capacity};
        for (u32 i = 0; i < count; ++i)
        {
            K key;
            V val;
            serializeObject(s, &key, &val);
            map->add(std::move(key), std::move(val));
        }
    }

    serializeEnd(s);
}

/**
 * Asset serialization
 */
template<typename T>
void serialize(Serializer* s, Asset<T>* asset)
{
    if (s->writing)
    {
        serialize(s, &asset->data->path);
    }
    else
    {
        String path;
        serialize(s, &path);
        if (path != "")
            *asset = load<T>(path);
        else
            *asset = {};
    }
}

/**
 * Write serialized data in a binary format
 */
BinaryView writeSerialBinary(Arena* arena, Serializer* data);

/**
 * Read binary data to be deserialized
 */
Serializer readSerialBinary(Arena* arena, BinaryView bin);

// /**
//  * Write serialized data as json
//  */
// StringView jsonWriteSerial(Arena* arena, Serializer* data);

// /**
//  * Read json data to be deserialized : TODO
//  */
// Serializer jsonReadSerial(Arena* arena, StringView json);

// /**
//  * An error contained in the json
//  */
// struct JsonError {
//     /**
//      * The next error
//      */
//     JsonError* next = nullptr;
//     /**
//      * The error message
//      */
//     StringView msg = {};
// };
//
// /**
//  * A node in the json file
//  */
// struct JsonNode;
//
// /**
//  * The types contained in nodes
//  */
// enum JsonType : u32 {
//     JsonType_none = 0,
//     JsonType_struct,
//     JsonType_field,
//     JsonType_array,
//     JsonType_string,
//     JsonType_float,
//     JsonType_integer,
//     JsonType_bool,
// };
//
// /**
//  * A field in a struct
//  */
// struct JsonField {
//     /**
//      * The next field
//      */
//     JsonField* next = nullptr;
//     /**
//      * The name of the field
//      */
//     StringView name = {};
//     /**
//      * The value stored in the field
//      */
//     JsonNode* value = nullptr;
// };
//
// /**
//  * A struct contained in the json
//  */
// struct JsonStruct {
//     /**
//      * The first field
//      */
//     JsonField* fields = nullptr;
// };
//
// /**
//  * An element in an array
//  */
// struct JsonElem {
//     /**
//      * The next element
//      */
//     JsonElem* next = nullptr;
//     /**
//      * The value stored in the element
//      */
//     JsonNode* value = nullptr;
// };
//
// /**
//  * An array contained in the json
//  */
// struct JsonArray {
//     /**
//      * The first element
//      */
//     JsonElem* elems = nullptr;
// };
//
// /**
//  * A node in the json file
//  */
// struct JsonNode {
//     /**
//      * The node's type
//      */
//     JsonType type = {};
//     /**
//      * The value in the node
//      */
//     union {
//         JsonStruct jstruct;
//         JsonField field;
//         JsonArray array;
//         StringView string;
//         f64 floating;
//         i64 integer;
//         bool boolean;
//     };
// };
//
// /**
//  * A parsed Json file
//  */
// struct Json {
//     /**
//      * The successfully parsed nodes
//      */
//     JsonNode* file = nullptr;
//     /**
//      * The errors found
//      */
//     JsonError* errors = nullptr;
// };
//
// /**
//  * Parses json text into a tree
//  *
//  * Parameters
//  * - arena The arena to allocate from
//  * - text The json text to parse
//  *
//  * Returns
//  * - The parsed json, errors contained inside
//  */
// Json parseJson(Arena* arena, StringView text);

// JSON

struct JsonValue;

using JsonObject = MapTemp<StringView, JsonValue>;

using JsonArray = ArrayTemp<JsonValue>;

struct JsonValue : Sum<JsonObject, JsonArray, StringView, f64, bool> {
    using Sum::Sum;
};

StringView writeJson(Arena* arena, const JsonObject& json);

JsonObject readJson(Arena* arena, StringView json);

} // namespace hg

