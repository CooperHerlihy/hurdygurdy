/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
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

/**
 * The primitive serializable types
 */
enum HgSerialType : u32 {
    /**
     * An object with fields
     */
    HgSerialType_object,
    /**
     * String data
     */
    HgSerialType_string,
    /**
     * An integer value
     */
    HgSerialType_integer,
    /**
     * A floating point value
     */
    HgSerialType_floating,
    /**
     * A boolean value
     */
    HgSerialType_boolean,
};

/**
 * Stringify HgSerialType
 */
const char* hgSerialTypeToString(HgSerialType s);

/**
 * A serialized data node
 */
struct HgSerialNode {
    /**
     * The parent object or array
     */
    HgSerialNode* parent;
    /**
     * The next node in the object or array
     */
    HgSerialNode* next;
    /**
     * The type of data
     */
    HgSerialType type;
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
        HgSerialNode* children;
        /**
         * String data
         */
        HgStringView string;
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
struct HgSerializer {
    /**
     * The arena to allocate from
     */
    HgArena* arena;
    /**
     * The root node
     */
    HgSerialNode* root;
    /**
     * The current object
     */
    HgSerialNode* parent;
    /**
     * The current data node
     */
    HgSerialNode* current;
    /**
     * Whether the serializer is reading or writing
     */
    bool writing;
};

/**
 * Begin a serial writer
 */
HgSerializer hgSerialWriter(HgArena* arena);

/**
 * Begin a serial reader
 */
HgSerializer hgSerialReader(HgArena* arena, HgSerialNode* begin);

/**
 * Begin serializing an object
 */
void hgSerializeBegin(HgSerializer* s, u32* size = nullptr);

/**
 * Begin serializing an object
 */
void hgSerializeEnd(HgSerializer* s);

/**
 * Serialize a value, should be overridden
 */
template<typename T>
void hgSerialize(HgSerializer* s, T* val);

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
void hgSerializeObject(HgSerializer* s, Ts*... vals)
{
    hgSerializeBegin(s);
    (hgSerialize(s, vals), ...);
    hgSerializeEnd(s);
}

/**
 * HgBinary serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgBinary* val);

/**
 * HgStringView serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgStringView* val);

/**
 * HgStringOwner serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgStringOwner* val);

/**
 * HgStringBuilder serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgStringBuilder* val);

/**
 * u8 serialization
 */
template<>
void hgSerialize(HgSerializer* s, u8* val);

/**
 * u16 serialization
 */
template<>
void hgSerialize(HgSerializer* s, u16* val);

/**
 * u32 serialization
 */
template<>
void hgSerialize(HgSerializer* s, u32* val);

/**
 * u64 serialization
 */
template<>
void hgSerialize(HgSerializer* s, u64* val);

/**
 * i8 serialization
 */
template<>
void hgSerialize(HgSerializer* s, i8* val);

/**
 * i16 serialization
 */
template<>
void hgSerialize(HgSerializer* s, i16* val);

/**
 * i32 serialization
 */
template<>
void hgSerialize(HgSerializer* s, i32* val);

/**
 * i64 serialization
 */
template<>
void hgSerialize(HgSerializer* s, i64* val);

/**
 * f32 serialization
 */
template<>
void hgSerialize(HgSerializer* s, f32* val);

/**
 * f64 serialization
 */
template<>
void hgSerialize(HgSerializer* s, f64* val);

/**
 * bool serialization
 */
template<>
void hgSerialize(HgSerializer* s, bool* val);

/**
 * HgVec2 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgVec2* val);

/**
 * HgVec3 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgVec3* val);

/**
 * HgVec4 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgVec4* val);

/**
 * HgMat2 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgMat2* val);

/**
 * HgMat3 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgMat3* val);

/**
 * HgMat4 serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgMat4* val);

/**
 * HgComplex serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgComplex* val);

/**
 * HgQuat serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgQuat* val);

/**
 * Write serialized data in a binary format
 */
HgBinary hgBinaryWriteSerial(HgArena* arena, HgSerializer* data);

/**
 * Read binary data to be deserialized
 */
HgSerializer hgBinaryReadSerial(HgArena* arena, HgBinary* bin);

/**
 * Write serialized data as json
 */
HgStringView hgJsonWriteSerial(HgArena* arena, HgSerializer* data);

// /**
//  * Read json data to be deserialized : TODO
//  */
// HgSerializer hgJsonReadSerial(HgArena* arena, HgStringView json);

/**
 * An error contained in the json
 */
struct HgJsonError {
    /**
     * The next error
     */
    HgJsonError* next;
    /**
     * The error message
     */
    HgStringView msg;
};

/**
 * A node in the json file
 */
struct HgJsonNode;

/**
 * The types contained in nodes
 */
enum HgJsonType : u32 {
    HgJsonType_none = 0,
    HgJsonType_struct,
    HgJsonType_field,
    HgJsonType_array,
    HgJsonType_string,
    HgJsonType_float,
    HgJsonType_integer,
    HgJsonType_bool,
};

/**
 * A field in a struct
 */
struct HgJsonField {
    /**
     * The next field
     */
    HgJsonField* next;
    /**
     * The name of the field
     */
    HgStringView name;
    /**
     * The value stored in the field
     */
    HgJsonNode* value;
};

/**
 * A struct contained in the json
 */
struct HgJsonStruct {
    /**
     * The first field
     */
    HgJsonField* fields;
};

/**
 * An element in an array
 */
struct HgJsonElem {
    /**
     * The next element
     */
    HgJsonElem* next;
    /**
     * The value stored in the element
     */
    HgJsonNode* value;
};

/**
 * An array contained in the json
 */
struct HgJsonArray {
    /**
     * The first element
     */
    HgJsonElem* elems;
};

/**
 * A node in the json file
 */
struct HgJsonNode {
    /**
     * The node's type
     */
    HgJsonType type;
    /**
     * The value in the node
     */
    union {
        HgJsonStruct jstruct;
        HgJsonField field;
        HgJsonArray array;
        HgStringView string;
        f64 floating;
        i64 integer;
        bool boolean;
    };
};

/**
 * A parsed Json file
 */
struct HgJson {
    /**
     * The successfully parsed nodes
     */
    HgJsonNode* file;
    /**
     * The errors found
     */
    HgJsonError* errors;
};

// /**
//  * A binary file asset handle
//  */
// typedef HgAssetHandle<HgJson> HgJsonHandle;
//
// /**
//  * HgJson asset load implementation
//  */
// template<>
// void hgAssetLoadImpl(HgAssetData<HgJson>* data);
//
// /**
//  * HgJson asset unload implementation
//  */
// template<>
// void hgAssetUnloadImpl(HgAssetData<HgJson>* data);

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
HgJson hgParseJson(HgArena* arena, HgStringView text);

#endif // HG_SERIALIZATION_HPP
