#include "hg_core.hpp"
#include "hg_serialization.hpp"
#include "hg_templates.hpp"

namespace hg {

const char* serialTypeToString(SerialType s)
{
    switch (s)
    {
        case SerialType_object:
            return "SerialType_object";
        case SerialType_string:
            return "SerialType_string";
        case SerialType_integer:
            return "SerialType_integer";
        case SerialType_floating:
            return "SerialType_floating";
        case SerialType_boolean:
            return "SerialType_boolean";
        default:
            return "invalid SerialType";
    }
}

Serializer serialWriter(Arena* arena)
{
    Serializer s{};
    s.arena = arena;
    s.root = arenaAlloc<SerialNode>(arena, 1);
    s.root->parent = nullptr;
    s.root->next = nullptr;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = true;
    return s;
}

Serializer serialReader(Arena* arena, SerialNode* begin)
{
    Serializer s{};
    s.arena = arena;
    s.root = begin;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = false;
    return s;
}

void serializeNodeStart(Serializer* s)
{
    if (s->writing)
    {
        if (s->current != nullptr)
        {
            s->current->next = arenaAlloc<SerialNode>(s->arena, 1);
            s->current = s->current->next;
            s->current->parent = s->parent;
            s->current->next = nullptr;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = arenaAlloc<SerialNode>(s->arena, 1);
                s->parent->children = s->current;
                s->current->parent = s->parent;
                s->current->next = nullptr;
            }
            else
            {
                HG_ASSERT(s->root != nullptr);
                s->current = s->root;
            }
        }

        if (s->parent != nullptr)
            ++s->parent->count;
    }
    else
    {
        if (s->current != nullptr)
        {
            s->current = s->current->next;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = s->parent->children;
            }
            else
            {
                HG_ASSERT(s->root != nullptr);
                s->current = s->root;
            }
        }
    }
}

void serializeBegin(Serializer* s, u32* size)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_object;
        s->current->count = 0;
        s->current->children = nullptr;
    }
    else
    {
        if (size != nullptr)
            *size = s->current->count;
    }

    s->parent = s->current;
    s->current = nullptr;
}

void serializeEnd(Serializer* s)
{
    HG_ASSERT(s->parent != nullptr);

    s->current = s->parent;
    s->parent = s->parent->parent;
}

void serializeVoid(Serializer* s, void* val, u32 size)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = stringCopy(s->arena, {(char*)val, size});
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        HG_ASSERT(s->current->string.length == size);
        memCopy(val, s->current->string.chars, size);
    }
}

template<>
void serialize(Serializer* s, Binary* val)
{
    serialize(s, (String*)val);
}

template<>
void serialize(Serializer* s, String* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = stringCopy(s->arena, *val);
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        *val = stringCreate(s->current->string);
    }
}

template<>
void serialize(Serializer* s, StringBuilder* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_string;
        s->current->string = stringCopy(s->arena, *val);
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_string);
        *val = stringCopy(s->arena, s->current->string);
    }
}

template<typename T>
static void serializeInt(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_integer;
        s->current->integer = (i64)*val;
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_integer);
        *val = (T)s->current->integer;
    }
}

template<>
void serialize(Serializer* s, u8* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u16* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u32* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, u64* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i8* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i16* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i32* val)
{
    serializeInt(s, val);
}

template<>
void serialize(Serializer* s, i64* val)
{
    serializeInt(s, val);
}

template<typename T>
static void serializeFloat(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_floating;
        s->current->floating = (f64)*val;
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_floating);
        *val = (T)s->current->floating;
    }
}

template<>
void serialize(Serializer* s, f32* val)
{
    serializeFloat(s, val);
}

template<>
void serialize(Serializer* s, f64* val)
{
    serializeFloat(s, val);
}

template<>
void serialize(Serializer* s, bool* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = SerialType_boolean;
        s->current->boolean = *val;
    }
    else
    {
        HG_ASSERT(s->current->type == SerialType_boolean);
        *val = s->current->boolean;
    }
}

template<>
void serialize(Serializer* s, Vec2* val)
{
    serializeObject(s,
        &val->x,
        &val->y);
}

template<>
void serialize(Serializer* s, Vec3* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void serialize(Serializer* s, Vec4* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void serialize(Serializer* s, Mat2* val)
{
    serializeObject(s,
        &val->x,
        &val->y);
}

template<>
void serialize(Serializer* s, Mat3* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void serialize(Serializer* s, Mat4* val)
{
    serializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void serialize(Serializer* s, Complex* val)
{
    serializeObject(s,
        &val->r,
        &val->i);
}

template<>
void serialize(Serializer* s, Quat* val)
{
    serializeObject(s,
        &val->r,
        &val->i,
        &val->j,
        &val->k);
}

static constexpr char serialBinTag[] = "Data";
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)];
    u32 versionMajor;
    u32 versionMinor;
    u32 versionPatch;

    u32 nodeBegin;
};

struct SerialBinObject {
    u32 fieldCount;
    u32 fieldsBegin;
};

struct SerialBinString {
    u32 begin;
    u32 length;
};

struct SerialBinNode {
    SerialType type;
    union {
        SerialBinObject object;
        SerialBinString string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

static void serialBinWriteNode(Arena* arena, BinaryBuilder* bin, u32 idx, SerialNode* node);

static void serialBinWriteString(Arena* arena, BinaryBuilder* bin, u32 idx, String string)
{
    SerialBinNode node{};
    node.type = SerialType_string;
    node.string.length = (u32)string.length;

    node.string.begin = (u32)bin->size;
    binaryResize(arena, bin, bin->size + string.length);

    binaryOverwrite(bin, idx, node);

    binaryOverwrite(bin, node.string.begin, string.chars, string.length);
}

static void serialBinWriteInteger(BinaryBuilder* bin, u32 idx, i64 integer)
{
    SerialBinNode node{};
    node.type = SerialType_integer;
    node.integer = integer;
    binaryOverwrite(bin, idx, node);
}

static void serialBinWriteFloating(BinaryBuilder* bin, u32 idx, f64 floating)
{
    SerialBinNode node{};
    node.type = SerialType_floating;
    node.floating = floating;
    binaryOverwrite(bin, idx, node);
}

static void serialBinWriteBoolean(BinaryBuilder* bin, u32 idx, bool boolean)
{
    SerialBinNode node{};
    node.type = SerialType_boolean;
    node.boolean = boolean;
    binaryOverwrite(bin, idx, node);
}

static void serialBinWriteObject(Arena* arena, BinaryBuilder* bin, u32 idx, SerialNode* object)
{
    SerialBinNode node{};
    node.type = SerialType_object;
    node.object.fieldCount = object->count;

    node.object.fieldsBegin = (u32)bin->size;
    binaryResize(arena, bin, bin->size + object->count * sizeof(SerialBinNode));

    binaryOverwrite(bin, idx, node);

    SerialNode* data = object->children;
    for (u32 i = 0; i < object->count; ++i)
    {
        serialBinWriteNode(arena, bin, node.object.fieldsBegin + i * (u32)sizeof(SerialBinNode), data);
        data = data->next;
    }
}

static void serialBinWriteNode(Arena* arena, BinaryBuilder* bin, u32 idx, SerialNode* node)
{
    switch (node->type)
    {
        case SerialType_object:
            serialBinWriteObject(arena, bin, idx, node);
            return;
        case SerialType_string:
            serialBinWriteString(arena, bin, idx, node->string);
            return;
        case SerialType_integer:
            serialBinWriteInteger(bin, idx, node->integer);
            return;
        case SerialType_floating:
            serialBinWriteFloating(bin, idx, node->floating);
            return;
        case SerialType_boolean:
            serialBinWriteBoolean(bin, idx, node->boolean);
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node->type));
    }
}

Binary binaryWriteSerial(Arena* arena, Serializer* serial)
{
    BinaryBuilder bin{};

    SerialBinHeader header{};
    binaryResize(arena, &bin, bin.size + sizeof(header));

    memCopy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;

    header.nodeBegin = (u32)bin.size;
    binaryResize(arena, &bin, bin.size + sizeof(SerialBinNode));

    binaryOverwrite(&bin, 0, header);

    serialBinWriteNode(arena, &bin, header.nodeBegin, serial->current);

    return bin;
}

static void serialBinReadNode(Binary bin, u32 idx, Serializer* s);

static void serialBinReadObject(Binary bin, SerialBinObject object, Serializer* s)
{
    serializeBegin(s);
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        serialBinReadNode(bin, object.fieldsBegin + i * (u32)sizeof(SerialBinNode), s);
    }
    serializeEnd(s);
}

static void serialBinReadString(Binary bin, SerialBinString string, Serializer* s)
{
    String val = {(char*)bin.data + string.begin, string.length};
    serialize(s, &val);
}

static void serialBinReadNode(Binary bin, u32 idx, Serializer* s)
{
    SerialBinNode node = binaryRead<SerialBinNode>(bin, idx);
    switch (node.type)
    {
        case SerialType_object:
            serialBinReadObject(bin, node.object, s);
            return;
        case SerialType_string:
            serialBinReadString(bin, node.string, s);
            return;
        case SerialType_integer:
            serialize(s, &node.integer);
            return;
        case SerialType_floating:
            serialize(s, &node.floating);
            return;
        case SerialType_boolean:
            serialize(s, &node.boolean);
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node.type));
    }
}

Serializer binaryReadSerial(Arena* arena, Binary bin)
{
    SerialBinHeader header = binaryRead<SerialBinHeader>(bin, 0);

    if (!memEqual(header.tag, serialBinTag, sizeof(serialBinTag)))
    {
        HG_WARN("Serial binary could not be read, does not have a header\n");
        return {};
    }
    else if (header.versionMajor != serialBinVersionMajor)
    {
        HG_WARN("Serial binary has wrong major version: %d instead of %d", header.versionMajor, serialBinVersionMajor);
    }
    else if (header.versionMinor != serialBinVersionMinor)
    {
        HG_WARN("Serial binary has wrong minor version: %d instead of %d", header.versionMinor, serialBinVersionMinor);
    }
    else if (header.versionPatch != serialBinVersionPatch)
    {
        HG_WARN("Serial binary has wrong patch version: %d instead of %d", header.versionPatch, serialBinVersionPatch);
    }

    Serializer s = serialWriter(arena);
    serialBinReadNode(bin, header.nodeBegin, &s);
    return serialReader(arena, s.current);
}

static void serialJsonWriteNode(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node);

static void serialJsonWriteString(Arena* arena, StringBuilder* str, String string)
{
    stringAppendC(arena, str, '"');
    for (u32 i = 0; i < string.length; ++i)
    {
        switch (string[i])
        {
        case '\\':
            stringAppend(arena, str, "\\\\");
            break;
        case '\"':
            stringAppend(arena, str, "\\\"");
            break;
        case '\n':
            stringAppend(arena, str, "\\n");
            break;
        case '\r':
            stringAppend(arena, str, "\\r");
            break;
        case '\f':
            stringAppend(arena, str, "\\f");
            break;
        case '\b':
            stringAppend(arena, str, "\\b");
            break;
        default:
            stringAppendC(arena, str, string[i]);
            break;
        }
    }
    stringAppendC(arena, str, '"');
}

static void serialJsonWriteArray(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node)
{
    if (node->count > 0)
    {
        stringAppend(arena, str, "[\n");

        SerialNode* elem = node->children;

        for (u32 i = 0; i < indentation + 1; ++i)
        {
            stringAppend(arena, str, "    ");
        }
        serialJsonWriteNode(arena, str, indentation + 1, elem);

        elem = elem->next;

        for (u32 i = 1; i < node->count; ++i)
        {
            stringAppend(arena, str, ",\n");
            for (u32 i = 0; i < indentation + 1; ++i)
            {
                stringAppend(arena, str, "    ");
            }
            serialJsonWriteNode(arena, str, indentation + 1, elem);

            elem = elem->next;
        }

        stringAppendC(arena, str, '\n');
        for (u32 i = 0; i < indentation; ++i)
        {
            stringAppend(arena, str, "    ");
        }
        stringAppendC(arena, str, ']');
    }
    else
    {
        stringAppend(arena, str, "[]");
    }
}

// static void serialJsonWriteObject(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node)
// {
//     if (node->count > 0)
//     {
//         stringAppend(arena, str, "{\n");
//
//         SerialNode* field = node->children;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         serialJsonWriteString(arena, str, field->name);
//         stringAppend(arena, str, " : ");
//         serialJsonWriteNode(arena, str, indentation + 1, field);
//
//         field = field->next;
//
//         for (u32 i = 1; i < node->count; ++i)
//         {
//             stringAppend(arena, str, ",\n");
//             for (u32 i = 0; i < indentation + 1; ++i)
//             {
//                 stringAppend(arena, str, "    ");
//             }
//             serialJsonWriteString(arena, str, field->name);
//             stringAppend(arena, str, " : ");
//             serialJsonWriteNode(arena, str, indentation + 1, field);
//
//             field = field->next;
//         }
//
//         stringAppendC(arena, str, '\n');
//         for (u32 i = 0; i < indentation; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         stringAppendC(arena, str, '}');
//     }
//     else
//     {
//         stringAppend(arena, str, "{}");
//     }
// }

static void serialJsonWriteNode(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node)
{
    switch (node->type)
    {
        case SerialType_object:
            serialJsonWriteArray(arena, str, indentation, node);
            return;
        case SerialType_string:
            serialJsonWriteString(arena, str, node->string);
            return;
        case SerialType_integer:
            stringAppend(arena, str, integerToString(scratch(), node->integer));
            return;
        case SerialType_floating:
            stringAppend(arena, str, floatToString(scratch(), node->floating, 6));
            return;
        case SerialType_boolean:
            if (node->boolean)
                stringAppend(arena, str, "true");
            else
                stringAppend(arena, str, "false");
            return;
        default:
            HG_PANIC("Invalid SerialType: %s\n", serialTypeToString(node->type));
    }
}

String jsonWriteSerial(Arena* arena, Serializer* serial)
{
    StringBuilder str{};
    serialJsonWriteNode(arena, &str, 0, serial->current);
    stringAppendC(arena, &str, '\n');
    return str;
}

// Serializer jsonReadSerial(Arena* arena, StringView json)
// {
// }

struct JsonParseState {
    String text;
    u64 head;
    u64 line;
};

static Json jsonParseNext(Arena* arena, JsonParseState* state);
static Json jsonParseStruct(Arena* arena, JsonParseState* state);
static Json jsonParseArray(Arena* arena, JsonParseState* state);
static Json jsonParseString(Arena* arena, JsonParseState* state);
static Json jsonParseNumber(Arena* arena, JsonParseState* state);
static Json jsonParseBoolean(Arena* arena, JsonParseState* state);

static Json jsonParseNext(Arena* arena, JsonParseState* state)
{
    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head >= state->text.length)
        return {};

    switch (state->text[state->head])
    {
        case '{':
            ++state->head;
            return jsonParseStruct(arena, state);
        case '[':
            ++state->head;
            return jsonParseArray(arena, state);
        case '\'': [[fallthrough]];
        case '"':
            ++state->head;
            return jsonParseString(arena, state);
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return jsonParseNumber(arena, state);
        case 't': [[fallthrough]];
        case 'f':
            return jsonParseBoolean(arena, state);
        case '}': {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", found unexpected token \"}\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
        case ']': {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", found unexpected token \"]\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
    }
    if (isNumeral(state->text[state->head]))
    {
        return jsonParseNumber(arena, state);
    }

    JsonError* error = arenaAlloc<JsonError>(arena, 1);
    error->next = nullptr;

    u64 begin = state->head;
    while (state->head < state->text.length && !isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    StringBuilder msg{};
    stringAppend(arena, &msg, "on line ");
    stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
    stringAppend(arena, &msg, ", found unexpected token \"");
    stringAppend(arena, &msg, {&state->text[begin], &state->text[state->head]});
    stringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    return {nullptr, error};
}

static Json jsonParseStruct(Arena* arena, JsonParseState* state)
{
    Json json{};
    json.file = arenaAlloc<JsonNode>(arena, 1);
    json.file->type = JsonType::JsonType_struct;
    json.file->jstruct.fields = nullptr;

    JsonField* lastField = nullptr;
    JsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == ']')
        {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", struct ends with \"]\" instead of \"}\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == '}')
        {
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        Json value = jsonParseNext(arena, state);

        if (value.file != nullptr)
        {
            if (value.file->type != JsonType::JsonType_field)
            {
                JsonError* error = arenaAlloc<JsonError>(arena, 1);
                error->next = nullptr;
                StringBuilder msg{};
                stringAppend(arena, &msg, "on line ");
                stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
                stringAppend(arena, &msg, ", struct has a literal instead of a field\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else if (value.file->field.value == nullptr)
            {
                JsonError* error = arenaAlloc<JsonError>(arena, 1);
                error->next = nullptr;
                StringBuilder msg{};
                stringAppend(arena, &msg, "on line ");
                stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
                stringAppend(arena, &msg, ", struct has a field named \"");
                stringAppend(arena, &msg, value.file->field.name);
                stringAppend(arena, &msg, "\" which has no value\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastField == nullptr)
                    json.file->jstruct.fields = &value.file->field;
                else
                    lastField->next = &value.file->field;
                lastField = &value.file->field;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static Json jsonParseArray(Arena* arena, JsonParseState* state)
{
    Json json{};
    json.file = arenaAlloc<JsonNode>(arena, 1);
    json.file->type = JsonType::JsonType_array;

    JsonType type = JsonType::JsonType_none;
    JsonElem* lastElem = nullptr;
    JsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == '}')
        {
            JsonError* error = arenaAlloc<JsonError>(arena, 1);
            error->next = nullptr;
            StringBuilder msg{};
            stringAppend(arena, &msg, "on line ");
            stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
            stringAppend(arena, &msg, ", array ends with \"}\" instead of \"]\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == ']')
        {
            ++state->head;
            while (state->head < state->text.length && isWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        JsonElem* elem = arenaAlloc<JsonElem>(arena, 1);
        elem->next = nullptr;

        Json value = jsonParseNext(arena, state);
        elem->value = value.file;

        if (value.file != nullptr)
        {
            if (type == JsonType::JsonType_none)
            {
                if (value.file->type != JsonType::JsonType_field)
                {
                    type = value.file->type;
                } else {
                    JsonError* error = arenaAlloc<JsonError>(arena, 1);
                    error->next = nullptr;
                    StringBuilder msg{};
                    stringAppend(arena, &msg, "on line ");
                    stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
                    stringAppend(arena, &msg, ", array has a field as an element\n");
                    error->msg = msg;
                    if (lastError == nullptr)
                        json.errors = lastError = error;
                    else
                        lastError->next = error;
                    lastError = error;
                }
            }
            if (value.file->type != type)
            {
                JsonError* error = arenaAlloc<JsonError>(arena, 1);
                error->next = nullptr;
                StringBuilder msg{};
                stringAppend(arena, &msg, "on line ");
                stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
                stringAppend(arena, &msg, ", array has element which is not the same type as the first valid element\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastElem == nullptr)
                    json.file->array.elems = elem;
                else
                    lastElem->next = elem;
                lastElem = elem;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static Json jsonParseString(Arena* arena, JsonParseState* state)
{
    u64 begin = state->head;
    while (state->head < state->text.length && state->text[state->head] != '"')
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    u64 end = state->head;
    if (state->head < state->text.length)
    {
        ++state->head;
        StringBuilder str{};
        for (u64 i = begin; i < end; ++i)
        {
            char c = state->text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            stringAppendC(arena, &str, c);
        }

        Json json{};
        json.file = arenaAlloc<JsonNode>(arena, 1);

        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ':')
        {
            ++state->head;
            json.file->type = JsonType::JsonType_field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            Json next = jsonParseNext(arena, state);
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = JsonType::JsonType_string;
            json.file->string = str;
        }
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;
        return json;
    }

    JsonError* error = arenaAlloc<JsonError>(arena, 1);
    StringBuilder msg{};
    stringAppend(arena, &msg, "on line ");
    stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
    stringAppend(arena, &msg, ", expected string to terminate\n");
    error->msg = msg;
    return {nullptr, error};
}

static Json jsonParseNumber(Arena* arena, JsonParseState* state)
{
    bool isNumFloat = false;
    u64 begin = state->head;
    while (state->head < state->text.length && (
        isNumeral(state->text[state->head]) ||
        state->text[state->head] == '-' ||
        state->text[state->head] == '+' ||
        state->text[state->head] == '.' ||
        state->text[state->head] == 'e'
    ))
    {
        if (state->text[state->head] == '.' || state->text[state->head] == 'e')
            isNumFloat = true;
        ++state->head;
    }
    String num{&state->text[begin], &state->text[state->head]};
    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head < state->text.length && state->text[state->head] == ',')
        ++state->head;

    if (isNumFloat)
    {
        if (isFloat(num))
        {
            JsonNode* node = arenaAlloc<JsonNode>(arena, 1);
            node->type = JsonType::JsonType_float;
            node->floating = stringToFloat(num);
            return {node, nullptr};
        }
    } else {
        if (isInteger(num))
        {
            JsonNode* node = arenaAlloc<JsonNode>(arena, 1);
            node->type = JsonType::JsonType_integer;
            node->integer = stringToInteger(num);
            return {node, nullptr};
        }
    }

    JsonError* error = arenaAlloc<JsonError>(arena, 1);

    StringBuilder msg{};
    stringAppend(arena, &msg, "on line ");
    stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
    stringAppend(arena, &msg, ", expected numeral value, found \"");
    stringAppend(arena, &msg, num);
    stringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        Json next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

static Json jsonParseBoolean(Arena* arena, JsonParseState* state)
{
    if (state->head + 4 <= state->text.length && String{&state->text[state->head], 4} == "true")
    {
        state->head += 4;
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        JsonNode* node = arenaAlloc<JsonNode>(arena, 1);
        node->type = JsonType::JsonType_bool;
        node->boolean = true;
        return {node, nullptr};
    }
    if (state->head + 5 <= state->text.length && String{&state->text[state->head], 5} == "false")
    {
        state->head += 5;
        while (state->head < state->text.length && isWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        JsonNode* node = arenaAlloc<JsonNode>(arena, 1);
        node->type = JsonType::JsonType_bool;
        node->boolean = false;
        return {node, nullptr};
    }

    JsonError* error = arenaAlloc<JsonError>(arena, 1);

    u64 begin = state->head;
    while (state->head < state->text.length && !isWhitespace(state->text[state->head])
        && state->text[state->head] != ','
        && state->text[state->head] != '}'
        && state->text[state->head] != ']'
    )
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    StringBuilder msg{};
    stringAppend(arena, &msg, "on line ");
    stringAppend(arena, &msg, integerToString(arena, (i64)state->line));
    stringAppend(arena, &msg, ", expected boolean value, found \"");
    stringAppend(arena, &msg, {&state->text[begin], &state->text[state->head]});
    stringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    if (state->text[state->head] == ',')
        ++state->head;

    while (state->head < state->text.length && isWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        Json next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

Json parseJson(Arena* arena, String text)
{
    HG_ASSERT(arena != nullptr);
    if (text.length > 0)
        HG_ASSERT(text.chars != nullptr);

    JsonParseState parseState{};
    parseState.text = text;
    parseState.head = 0;
    parseState.line = 1;
    return jsonParseNext(arena, &parseState);
}

} // namespace hg

