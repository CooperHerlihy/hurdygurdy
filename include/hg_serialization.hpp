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
     * No value
     */
    HgSerialType_null,
    /**
     * An array of values
     */
    HgSerialType_array,
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
struct HgSerialNode;

/**
 * A serialized data field in an object
 */
struct HgSerialField;

/**
 * A serialized array
 */
struct HgSerialArray {
    /**
     * The number of elements
     */
    u32 elemCount;
    /**
     * The elements
     */
    HgSerialNode* elems;
};

/**
 * A serialized object
 */
struct HgSerialObject {
    /**
     * The number of fields
     */
    u32 fieldCount;
    /**
     * The fields
     */
    HgSerialField* fields;
};

/**
 * A serialized data node
 */
struct HgSerialNode {
    /**
     * The type of data
     */
    HgSerialType type;
    /**
     * The data
     */
    union {
        HgSerialArray array;
        HgSerialObject object;
        HgStringView string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

/**
 * A serialized data field
 */
struct HgSerialField {
    /**
     * The name of the field
     */
    HgStringView name;
    /**
     * The field's data
     */
    HgSerialNode data;
};

/**
 * The data for serialization
 */
struct HgSerializer {
    /**
     * Whether the serializer is reading or writing
     */
    bool writing;
    /**
     * The current field/elem index
     */
    u32 idx;
    /**
     * The current node begin read from or written to
     */
    HgSerialNode* current;
};

/**
 * Begin a serial writer
 */
HgSerializer hgSerialWriter(HgArena* arena);

/**
 * Begin a serial reader
 */
HgSerializer hgSerialReader(HgSerialNode* begin);

/**
 * Check whether the current serial object is null
 */
bool hgSerializerIsNull(HgSerializer s);

/**
 * Begin serializing an array
 */
HgSerializer hgSerializeArray(HgArena* arena, HgSerializer* s, HgStringView name, u32* length);

/**
 * Begin serializing an object
 */
HgSerializer hgSerializeObject(HgArena* arena, HgSerializer* s, HgStringView name);

/**
 * Serialize a null object
 */
void hgSerializeNull(HgArena* arena, HgSerializer* s, HgStringView name);

/**
 * Serialize arbitrary data
 */
void hgSerializeBinary(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary* binary);

/**
 * Default serialization, should be overridden
 */
template<typename T>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, T* val)
{
    HgBinary bin = {val, sizeof(T)};
    if (s->writing)
    {
        hgSerializeBinary(arena, s, name, &bin);
    }
    else
    {
        hgSerializeBinary(hgScratch(), s, name, &bin);
        hgMemCopy(val, bin.data, sizeof(T));
    }
}

/**
 * HgBinary serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary* val);

/**
 * HgStringView serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringView* val);

/**
 * HgStringOwner serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringOwner* val);

/**
 * HgStringBuilder serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringBuilder* val);

/**
 * u8 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u8* val);

/**
 * u16 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u16* val);

/**
 * u32 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u32* val);

/**
 * u64 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u64* val);

/**
 * i8 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i8* val);

/**
 * i16 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i16* val);

/**
 * i32 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i32* val);

/**
 * i64 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i64* val);

/**
 * f32 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, f32* val);

/**
 * f64 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, f64* val);

/**
 * bool serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, bool* val);

/**
 * HgVec2 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec2* val);

/**
 * HgVec3 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec3* val);

/**
 * HgVec4 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec4* val);

/**
 * HgMat2 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat2* val);

/**
 * HgMat3 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat3* val);

/**
 * HgMat4 serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat4* val);

/**
 * HgComplex serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgComplex* val);

/**
 * HgQuat serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgQuat* val);

/**
 * Write serialized data in a binary format
 */
HgBinary hgBinaryWriteSerial(HgArena* arena, HgSerializer serial);

/**
 * Read binary data to be deserialized
 */
HgSerializer hgBinaryReadSerial(HgArena* arena, HgBinary bin);

/**
 * Write serialized data as json
 */
HgStringView hgJsonWriteSerial(HgArena* arena, HgSerializer serial);

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
