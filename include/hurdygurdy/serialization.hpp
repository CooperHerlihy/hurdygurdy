#pragma once

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
void serialize(Serializer* s, T* val);

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals);

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N]);

/**
 * Integer serialization
 */
template<std::integral T>
void serialize(Serializer* s, T* val);

/**
 * Float serialization
 */
template<std::floating_point T>
void serialize(Serializer* s, T* val);

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
void serialize(Serializer* s, Product<Ts...>* product);

/**
 * Sum serialization
 */
template<typename... Ts>
void serialize(Serializer* s, Sum<Ts...>* sum);

/**
 * Maybe serialization
 */
template<typename T>
void serialize(Serializer* s, Maybe<T>* maybe);

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
void serialize(Serializer* s, UniquePtr<T>* ptr);

/**
 * Array serialization
 */
template<typename T>
void serialize(Serializer* s, Array<T>* arr);

/**
 * Set serialization
 */
template<typename V>
void serialize(Serializer* s, Set<V>* set);

/**
 * Map serialization
 */
template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* set);

/**
 * Asset serialization
 */
template<typename T>
void serialize(Serializer* s, Asset<T>* asset);

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
