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

#ifndef HG_SERIALIZATION_HPP
#define HG_SERIALIZATION_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_strings.hpp"
#include "hg_math.hpp"

namespace hg {

/**
 * The primitive serializable types
 */
enum SerialType : u32 {
    /**
     * An object with fields
     */
    SerialType_object,
    /**
     * String data
     */
    SerialType_string,
    /**
     * An integer value
     */
    SerialType_integer,
    /**
     * A floating point value
     */
    SerialType_floating,
    /**
     * A boolean value
     */
    SerialType_boolean,
};

/**
 * Stringify SerialType
 */
const char* serialTypeToString(SerialType s);

/**
 * A serialized data node
 */
struct SerialNode {
    /**
     * The parent object or array
     */
    SerialNode* parent;
    /**
     * The next node in the object or array
     */
    SerialNode* next;
    /**
     * The type of data
     */
    SerialType type;
    /**
     * The number of fields, if an object
     */
    u32 count;
    /**
     * The data
     */
    union {
        /**
         * The fields of an object
         */
        SerialNode* children;
        /**
         * String data
         */
        String string;
        /**
         * Integer value
         */
        i64 integer;
        /**
         * Floating point value
         */
        f64 floating;
        /**
         * Boolean value
         */
        bool boolean;
    };
};

/**
 * The data for serialization
 */
struct Serializer {
    /**
     * The arena to allocate from
     */
    Arena* arena;
    /**
     * The root node
     */
    SerialNode* root;
    /**
     * The current object
     */
    SerialNode* parent;
    /**
     * The current data node
     */
    SerialNode* current;
    /**
     * Whether the serializer is reading or writing
     */
    bool writing;
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
 * The preamble to serializing a node, generally not needed
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
void serializeVoid(Serializer* s, void* val, u32 size);

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
 * Binary serialization
 */
template<>
void serialize(Serializer* s, Binary* val);

/**
 * String serialization
 */
template<>
void serialize(Serializer* s, String* val);

/**
 * StringBuilder serialization
 */
template<>
void serialize(Serializer* s, StringBuilder* val);

/**
 * u8 serialization
 */
template<>
void serialize(Serializer* s, u8* val);

/**
 * u16 serialization
 */
template<>
void serialize(Serializer* s, u16* val);

/**
 * u32 serialization
 */
template<>
void serialize(Serializer* s, u32* val);

/**
 * u64 serialization
 */
template<>
void serialize(Serializer* s, u64* val);

/**
 * i8 serialization
 */
template<>
void serialize(Serializer* s, i8* val);

/**
 * i16 serialization
 */
template<>
void serialize(Serializer* s, i16* val);

/**
 * i32 serialization
 */
template<>
void serialize(Serializer* s, i32* val);

/**
 * i64 serialization
 */
template<>
void serialize(Serializer* s, i64* val);

/**
 * f32 serialization
 */
template<>
void serialize(Serializer* s, f32* val);

/**
 * f64 serialization
 */
template<>
void serialize(Serializer* s, f64* val);

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
 * Write serialized data in a binary format
 */
Binary binaryWriteSerial(Arena* arena, Serializer* data);

/**
 * Read binary data to be deserialized
 */
Serializer binaryReadSerial(Arena* arena, Binary bin);

/**
 * Write serialized data as json
 */
String jsonWriteSerial(Arena* arena, Serializer* data);

// /**
//  * Read json data to be deserialized : TODO
//  */
// Serializer jsonReadSerial(Arena* arena, StringView json);

/**
 * An error contained in the json
 */
struct JsonError {
    /**
     * The next error
     */
    JsonError* next;
    /**
     * The error message
     */
    String msg;
};

/**
 * A node in the json file
 */
struct JsonNode;

/**
 * The types contained in nodes
 */
enum JsonType : u32 {
    JsonType_none = 0,
    JsonType_struct,
    JsonType_field,
    JsonType_array,
    JsonType_string,
    JsonType_float,
    JsonType_integer,
    JsonType_bool,
};

/**
 * A field in a struct
 */
struct JsonField {
    /**
     * The next field
     */
    JsonField* next;
    /**
     * The name of the field
     */
    String name;
    /**
     * The value stored in the field
     */
    JsonNode* value;
};

/**
 * A struct contained in the json
 */
struct JsonStruct {
    /**
     * The first field
     */
    JsonField* fields;
};

/**
 * An element in an array
 */
struct JsonElem {
    /**
     * The next element
     */
    JsonElem* next;
    /**
     * The value stored in the element
     */
    JsonNode* value;
};

/**
 * An array contained in the json
 */
struct JsonArray {
    /**
     * The first element
     */
    JsonElem* elems;
};

/**
 * A node in the json file
 */
struct JsonNode {
    /**
     * The node's type
     */
    JsonType type;
    /**
     * The value in the node
     */
    union {
        JsonStruct jstruct;
        JsonField field;
        JsonArray array;
        String string;
        f64 floating;
        i64 integer;
        bool boolean;
    };
};

/**
 * A parsed Json file
 */
struct Json {
    /**
     * The successfully parsed nodes
     */
    JsonNode* file;
    /**
     * The errors found
     */
    JsonError* errors;
};

// /**
//  * A binary file asset handle
//  */
// typedef AssetHandle<Json> JsonHandle;
//
// /**
//  * Json asset load implementation
//  */
// template<>
// void assetLoadImpl(AssetData<Json>* data);
//
// /**
//  * Json asset unload implementation
//  */
// template<>
// void assetUnloadImpl(AssetData<Json>* data);

/**
 * Parses json text into a tree
 *
 * Parameters
 * - arena The arena to allocate from
 * - text The json text to parse
 *
 * Returns
 * - The parsed json, errors contained inside
 */
Json parseJson(Arena* arena, String text);

} // namespace hg

#endif // SERIALIZATION_HPP
