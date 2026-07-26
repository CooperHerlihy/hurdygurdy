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

} // namespace hg
