#include "hurdygurdy.hpp"
#include "internal.hpp"

namespace hg {

Serializer serialWriter(Arena* arena)
{
    Serializer s{};
    s.arena = arena;
    s.root = arena->alloc<SerialNode>(1);
    s.root->parent = nullptr;
    s.root->next = nullptr;
    new (&s.root->data) SerialData{};
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
            s->current->next = s->arena->alloc<SerialNode>(1);
            s->current = s->current->next;
            s->current->parent = s->parent;
            s->current->next = nullptr;
            new (&s->current->data) SerialData{};
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = s->arena->alloc<SerialNode>(1);
                s->current->parent = s->parent;
                s->current->next = nullptr;
                new (&s->current->data) SerialData{};
                s->parent->data.get<SerialObject>().firstChild = s->current;
            }
            else
            {
                HG_ASSERT(s->root != nullptr);
                s->current = s->root;
            }
        }

        if (s->parent != nullptr)
            ++s->parent->data.get<SerialObject>().childCount;
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
                s->current = s->parent->data.get<SerialObject>().firstChild;
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
        s->current->data = SerialObject{};
    }
    else
    {
        if (size != nullptr)
            *size = s->current->data.get<SerialObject>().childCount;
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

void serializeVoid(Serializer* s, Span<void> data)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = StringView{StringBuilder{s->arena, {static_cast<char*>(data.data), data.size}}};
    }
    else
    {
        HG_ASSERT(s->current->data.is<StringView>());
        HG_ASSERT(s->current->data.get<StringView>().length == data.size);
        memcpy(data.data, s->current->data.get<StringView>().chars, data.size);
    }
}

template<>
void serialize(Serializer* s, bool* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = bool{*val};
    }
    else
    {
        HG_ASSERT(s->current->data.is<bool>());
        *val = s->current->data.get<bool>();
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

template<>
void serialize(Serializer* s, String* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = StringView{StringBuilder{s->arena, *val}};
    }
    else
    {
        HG_ASSERT(s->current->data.is<StringView>());
        *val = String::create(s->current->data.get<StringView>());
    }
}

template<>
void serialize(Serializer* s, Binary* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = StringView{StringBuilder{s->arena, {static_cast<char*>(val->data), val->size}}};
    }
    else
    {
        HG_ASSERT(s->current->data.is<StringView>());
        StringView str = s->current->data.get<StringView>();
        *val = Binary::create({str.chars, str.length});
    }
}

static constexpr char serialBinTag[] = "Data";
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)] = {};
    u32 versionMajor = 0;
    u32 versionMinor = 0;
    u32 versionPatch = 0;

    u32 nodeBegin = 0;
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
    u32 type = {};
    union {
        SerialBinObject object;
        SerialBinString string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

static void serialBinWriteNode(BinaryBuilder* bin, u32 idx, SerialNode* node);

static void serialBinWriteString(BinaryBuilder* bin, u32 idx, StringView string)
{
    SerialBinNode node{};
    node.type = SerialData::typeIdx<StringView>;
    node.string.length = static_cast<u32>(string.length);
    node.string.begin = static_cast<u32>(bin->size);

    bin->overwrite(idx, node);
    bin->append(string.chars, string.length);
}

static void serialBinWriteInteger(BinaryBuilder* bin, u32 idx, i64 integer)
{
    SerialBinNode node{};
    node.type = SerialData::typeIdx<i64>;
    node.integer = integer;
    bin->overwrite(idx, node);
}

static void serialBinWriteFloating(BinaryBuilder* bin, u32 idx, f64 floating)
{
    SerialBinNode node{};
    node.type = SerialData::typeIdx<f64>;
    node.floating = floating;
    bin->overwrite(idx, node);
}

static void serialBinWriteBoolean(BinaryBuilder* bin, u32 idx, bool boolean)
{
    SerialBinNode node{};
    node.type = SerialData::typeIdx<bool>;
    node.boolean = boolean;
    bin->overwrite(idx, node);
}

static void serialBinWriteObject(BinaryBuilder* bin, u32 idx, SerialObject object)
{
    SerialBinNode node{};
    node.type = SerialData::typeIdx<SerialObject>;
    node.object.fieldCount = object.childCount;

    node.object.fieldsBegin = static_cast<u32>(bin->size);
    bin->resize(bin->size + object.childCount * sizeof(SerialBinNode));

    bin->overwrite(idx, node);

    SerialNode* data = object.firstChild;
    for (u32 i = 0; i < object.childCount; ++i)
    {
        serialBinWriteNode(bin, node.object.fieldsBegin + i * static_cast<u32>(sizeof(SerialBinNode)), data);
        data = data->next;
    }
}

static void serialBinWriteNode(BinaryBuilder* bin, u32 idx, SerialNode* node)
{
    node->data.match(
        [&](SerialObject& obj) { serialBinWriteObject(bin, idx, obj); },
        [&](StringView& str) { serialBinWriteString(bin, idx, str); },
        [&](i64& i) { serialBinWriteInteger(bin, idx, i); },
        [&](f64& f) { serialBinWriteFloating(bin, idx, f); },
        [&](bool& b) { serialBinWriteBoolean(bin, idx, b); }
    );
}

BinaryView writeSerialBinary(Arena* arena, Serializer* serial)
{
    BinaryBuilder bin{arena, sizeof(SerialBinHeader)};

    SerialBinHeader header{};
    memcpy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;
    header.nodeBegin = static_cast<u32>(bin.size);
    bin.overwrite(0, header);

    bin.resize(bin.size + sizeof(SerialBinNode));
    serialBinWriteNode(&bin, header.nodeBegin, serial->current);
    return bin;
}

static void serialBinReadNode(BinaryView bin, u32 idx, Serializer* s);

static void serialBinReadObject(BinaryView bin, SerialBinObject object, Serializer* s)
{
    serializeBegin(s);
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        serialBinReadNode(bin, object.fieldsBegin + i * static_cast<u32>(sizeof(SerialBinNode)), s);
    }
    serializeEnd(s);
}

static void serialBinReadString(BinaryView bin, SerialBinString string, Serializer* s)
{
    serializeVoid(s, {static_cast<u8*>(const_cast<void*>(bin.data)) + string.begin, string.length});
}

static void serialBinReadNode(BinaryView bin, u32 idx, Serializer* s)
{
    SerialBinNode node = bin.read<SerialBinNode>(idx);
    switch (node.type)
    {
        case SerialData::typeIdx<SerialObject>:
            serialBinReadObject(bin, node.object, s);
            return;
        case SerialData::typeIdx<StringView>:
            serialBinReadString(bin, node.string, s);
            return;
        case SerialData::typeIdx<i64>:
            serialize(s, &node.integer);
            return;
        case SerialData::typeIdx<f64>:
            serialize(s, &node.floating);
            return;
        case SerialData::typeIdx<bool>:
            serialize(s, &node.boolean);
            return;
        default:
            HG_PANIC("Invalid SerialType: %d\n", node.type);
    }
}

Serializer readSerialBinary(Arena* arena, BinaryView bin)
{
    SerialBinHeader header = bin.read<SerialBinHeader>(0);

    if (memcmp(header.tag, serialBinTag, sizeof(serialBinTag)) != 0)
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

// static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node);
//
// static void serialJsonWriteString(StringBuilder* str, StringView string)
// {
//     str->append('"');
//     for (u32 i = 0; i < string.length; ++i)
//     {
//         switch (string[i])
//         {
//         case '\\':
//             str->append("\\\\");
//             break;
//         case '\"':
//             str->append("\\\"");
//             break;
//         case '\n':
//             str->append("\\n");
//             break;
//         case '\r':
//             str->append("\\r");
//             break;
//         case '\f':
//             str->append("\\f");
//             break;
//         case '\b':
//             str->append("\\b");
//             break;
//         default:
//             str->append(string[i]);
//             break;
//         }
//     }
//     str->append('"');
// }
//
// static void serialJsonWriteArray(StringBuilder* str, u32 indentation, SerialObject object)
// {
//     if (object.childCount > 0)
//     {
//         str->append("[\n");
//
//         SerialNode* elem = object.firstChild;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             str->append("    ");
//         }
//         serialJsonWriteNode(str, indentation + 1, elem);
//
//         elem = elem->next;
//
//         for (u32 i = 1; i < object.childCount; ++i)
//         {
//             str->append(",\n");
//             for (u32 j = 0; j < indentation + 1; ++j)
//             {
//                 str->append("    ");
//             }
//             serialJsonWriteNode(str, indentation + 1, elem);
//
//             elem = elem->next;
//         }
//
//         str->append( '\n');
//         for (u32 k = 0; k < indentation; ++k)
//         {
//             str->append("    ");
//         }
//         str->append(']');
//     }
//     else
//     {
//         str->append("[]");
//     }
// }
//
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
//
// static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node)
// {
//     node->data.match(
//         [&](SerialObject& obj) { serialJsonWriteArray(str, indentation, obj); },
//         [&](StringView& strData) { serialJsonWriteString(str, strData); },
//         [&](i64& i) { str->append(integerToString(getScratch(), i)); },
//         [&](f64& f) { str->append(floatToString(getScratch(), f, 6)); },
//         [&](bool& b) { str->append(b ? "true" : "false"); }
//     );
// }
//
// StringView jsonWriteSerial(Arena* arena, Serializer* serial)
// {
//     StringBuilder str{arena};
//     serialJsonWriteNode(&str, 0, serial->current);
//     str.append('\n');
//     return str;
// }
//
// Serializer jsonReadSerial(Arena* arena, StringView json)
// {
// }
//
// struct JsonParseState {
//     StringView text = {};
//     u64 head = 0;
//     u64 line = 0;
// };
//
// static Json jsonParseNext(Arena* arena, JsonParseState* state);
// static Json jsonParseStruct(Arena* arena, JsonParseState* state);
// static Json jsonParseArray(Arena* arena, JsonParseState* state);
// static Json jsonParseString(Arena* arena, JsonParseState* state);
// static Json jsonParseNumber(Arena* arena, JsonParseState* state);
// static Json jsonParseBoolean(Arena* arena, JsonParseState* state);
//
// static Json jsonParseNext(Arena* arena, JsonParseState* state)
// {
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->head >= state->text.length)
//         return {};
//
//     switch (state->text[state->head])
//     {
//         case '{':
//             ++state->head;
//             return jsonParseStruct(arena, state);
//         case '[':
//             ++state->head;
//             return jsonParseArray(arena, state);
//         case '\'': [[fallthrough]];
//         case '"':
//             ++state->head;
//             return jsonParseString(arena, state);
//         case '.': [[fallthrough]];
//         case '+': [[fallthrough]];
//         case '-':
//             return jsonParseNumber(arena, state);
//         case 't': [[fallthrough]];
//         case 'f':
//             return jsonParseBoolean(arena, state);
//         case '}': {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(arena, static_cast<i64>(state->line)));
//             msg.append(", found unexpected token \"}\"\n");
//             error->msg = msg;
//             return {nullptr, error};
//         }
//         case ']': {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(arena, static_cast<i64>(state->line)));
//             msg.append(", found unexpected token \"]\"\n");
//             error->msg = msg;
//             return {nullptr, error};
//         }
//     }
//     if (isNumeral(state->text[state->head]))
//     {
//         return jsonParseNumber(arena, state);
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//     error->next = nullptr;
//
//     u64 begin = state->head;
//     while (state->head < state->text.length && !isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", found unexpected token \"");
//     msg.append({&state->text[begin], &state->text[state->head]});
//     msg.append("\"\n");
//     error->msg = msg;
//
//     return {nullptr, error};
// }
//
// static Json jsonParseStruct(Arena* arena, JsonParseState* state)
// {
//     Json json{};
//     json.file = arena->alloc<JsonNode>(1);
//     json.file->type = JsonType::JsonType_struct;
//     json.file->jstruct.fields = nullptr;
//
//     JsonField* lastField = nullptr;
//     JsonError* lastError = nullptr;
//
//     for (;;)
//     {
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head >= state->text.length)
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", expected struct to terminate\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             break;
//         }
//         if (state->text[state->head] == ']')
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", struct ends with \"]\" instead of \"}\"\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//         if (state->text[state->head] == '}')
//         {
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//
//         Json value = jsonParseNext(arena, state);
//
//         if (value.file != nullptr)
//         {
//             if (value.file->type != JsonType::JsonType_field)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", struct has a literal instead of a field\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else if (value.file->field.value == nullptr)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", struct has a field named \"");
//                 msg.append(value.file->field.name);
//                 msg.append("\" which has no value\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else {
//                 if (lastField == nullptr)
//                     json.file->jstruct.fields = &value.file->field;
//                 else
//                     lastField->next = &value.file->field;
//                 lastField = &value.file->field;
//             }
//         }
//         if (value.errors != nullptr)
//         {
//             if (lastError == nullptr)
//                 json.errors = lastError = value.errors;
//             else
//                 lastError->next = value.errors;
//             lastError = value.errors;
//         }
//     }
//
//     return json;
// }
//
// static Json jsonParseArray(Arena* arena, JsonParseState* state)
// {
//     Json json{};
//     json.file = arena->alloc<JsonNode>(1);
//     json.file->type = JsonType::JsonType_array;
//
//     JsonType type = JsonType::JsonType_none;
//     JsonElem* lastElem = nullptr;
//     JsonError* lastError = nullptr;
//
//     for (;;)
//     {
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head >= state->text.length)
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", expected struct to terminate\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             break;
//         }
//         if (state->text[state->head] == '}')
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", array ends with \"}\" instead of \"]\"\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//         if (state->text[state->head] == ']')
//         {
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//
//         JsonElem* elem = arena->alloc<JsonElem>(1);
//         elem->next = nullptr;
//
//         Json value = jsonParseNext(arena, state);
//         elem->value = value.file;
//
//         if (value.file != nullptr)
//         {
//             if (type == JsonType::JsonType_none)
//             {
//                 if (value.file->type != JsonType::JsonType_field)
//                 {
//                     type = value.file->type;
//                 } else {
//                     JsonError* error = arena->alloc<JsonError>(1);
//                     error->next = nullptr;
//                     StringBuilder msg{arena};
//                     msg.append("on line ");
//                     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                     msg.append(", array has a field as an element\n");
//                     error->msg = msg;
//                     if (lastError == nullptr)
//                         json.errors = lastError = error;
//                     else
//                         lastError->next = error;
//                     lastError = error;
//                 }
//             }
//             if (value.file->type != type)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", array has element which is not the same type as the first valid element\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else {
//                 if (lastElem == nullptr)
//                     json.file->array.elems = elem;
//                 else
//                     lastElem->next = elem;
//                 lastElem = elem;
//             }
//         }
//         if (value.errors != nullptr)
//         {
//             if (lastError == nullptr)
//                 json.errors = lastError = value.errors;
//             else
//                 lastError->next = value.errors;
//             lastError = value.errors;
//         }
//     }
//
//     return json;
// }
//
// static Json jsonParseString(Arena* arena, JsonParseState* state)
// {
//     u64 begin = state->head;
//     while (state->head < state->text.length && state->text[state->head] != '"')
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     u64 end = state->head;
//     if (state->head < state->text.length)
//     {
//         ++state->head;
//         StringBuilder str{};
//         for (u64 i = begin; i < end; ++i)
//         {
//             char c = state->text[i];
//             if (c == '\\')
//             {
//                 // escape sequences : TODO
//             }
//             str.append(c);
//         }
//
//         Json json{};
//         json.file = arena->alloc<JsonNode>(1);
//
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ':')
//         {
//             ++state->head;
//             json.file->type = JsonType::JsonType_field;
//             json.file->field.next = nullptr;
//             json.file->field.name = str;
//             Json next = jsonParseNext(arena, state);
//             json.file->field.value = next.file;
//             json.errors = next.errors;
//         } else {
//             json.file->type = JsonType::JsonType_string;
//             json.file->string = str;
//         }
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//         return json;
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected string to terminate\n");
//     error->msg = msg;
//     return {nullptr, error};
// }
//
// static Json jsonParseNumber(Arena* arena, JsonParseState* state)
// {
//     bool isNumFloat = false;
//     u64 begin = state->head;
//     while (state->head < state->text.length && (
//         isNumeral(state->text[state->head]) ||
//         state->text[state->head] == '-' ||
//         state->text[state->head] == '+' ||
//         state->text[state->head] == '.' ||
//         state->text[state->head] == 'e'
//     ))
//     {
//         if (state->text[state->head] == '.' || state->text[state->head] == 'e')
//             isNumFloat = true;
//         ++state->head;
//     }
//     StringView num{&state->text[begin], &state->text[state->head]};
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->head < state->text.length && state->text[state->head] == ',')
//         ++state->head;
//
//     if (isNumFloat)
//     {
//         if (isFloat(num))
//         {
//             JsonNode* node = arena->alloc<JsonNode>(1);
//             node->type = JsonType::JsonType_float;
//             node->floating = stringToFloat(num);
//             return {node, nullptr};
//         }
//     } else {
//         if (isInteger(num))
//         {
//             JsonNode* node = arena->alloc<JsonNode>(1);
//             node->type = JsonType::JsonType_integer;
//             node->integer = stringToInteger(num);
//             return {node, nullptr};
//         }
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected numeral value, found \"");
//     msg.append(num);
//     msg.append("\"\n");
//     error->msg = msg;
//
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->text[state->head] == '}' || state->text[state->head] == ']')
//     {
//         return {nullptr, error};
//     } else {
//         Json next = jsonParseNext(arena, state);
//         error->next = next.errors;
//         return {next.file, error};
//     }
// }
//
// static Json jsonParseBoolean(Arena* arena, JsonParseState* state)
// {
//     if (state->head + 4 <= state->text.length && StringView{&state->text[state->head], 4} == "true")
//     {
//         state->head += 4;
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//
//         JsonNode* node = arena->alloc<JsonNode>(1);
//         node->type = JsonType::JsonType_bool;
//         node->boolean = true;
//         return {node, nullptr};
//     }
//     if (state->head + 5 <= state->text.length && StringView{&state->text[state->head], 5} == "false")
//     {
//         state->head += 5;
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//
//         JsonNode* node = arena->alloc<JsonNode>(1);
//         node->type = JsonType::JsonType_bool;
//         node->boolean = false;
//         return {node, nullptr};
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//
//     u64 begin = state->head;
//     while (state->head < state->text.length && !isWhitespace(state->text[state->head])
//         && state->text[state->head] != ','
//         && state->text[state->head] != '}'
//         && state->text[state->head] != ']'
//     )
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected boolean value, found \"");
//     msg.append({&state->text[begin], &state->text[state->head]});
//     msg.append("\"\n");
//     error->msg = msg;
//
//     if (state->text[state->head] == ',')
//         ++state->head;
//
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->text[state->head] == '}' || state->text[state->head] == ']')
//     {
//         return {nullptr, error};
//     } else {
//         Json next = jsonParseNext(arena, state);
//         error->next = next.errors;
//         return {next.file, error};
//     }
// }
//
// Json parseJson(Arena* arena, StringView text)
// {
//     HG_ASSERT(arena != nullptr);
//     if (text.length > 0)
//         HG_ASSERT(text.chars != nullptr);
//
//     JsonParseState parseState{};
//     parseState.text = text;
//     parseState.head = 0;
//     parseState.line = 1;
//     return jsonParseNext(arena, &parseState);
// }

} // namespace hg
