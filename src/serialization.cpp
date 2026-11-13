#include "hg/serialization.hpp"

#include "hg/error.hpp"

#include <string_view>
#include <cstring>

namespace hg {

Serializer serialWriter()
{
    Serializer s{};
    s.root = std::make_unique<SerialNode>();
    s.current = nullptr;
    s.writing = true;
    return s;
}

Serializer serialReader(std::unique_ptr<SerialNode> begin)
{
    Serializer s{};
    s.root = std::move(begin);
    s.current = nullptr;
    s.writing = false;
    return s;
}

bool serializeBegin(Serializer* s, u32* size)
{
    if (s->writing)
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = s->current;

        s->current = s->current->children.emplace_back(std::move(next)).get();
        if (size != nullptr)
            s->current->children.reserve(*size);
    }
    else
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        if (s->current->currentChild >= s->current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        s->current = s->current->children[s->current->currentChild++].get();

        if (size != nullptr)
            *size = static_cast<u32>(s->current->children.size());
    }
    return true;
}

void serializeEnd(Serializer* s)
{
    HG_ASSERT(s->current != nullptr);
    s->current = s->current->parent;
}

bool serializeVoid(Serializer* s, std::span<u8> data)
{
    if (s->writing)
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = s->current;
        next->data = std::vector<u8>(data.begin(), data.end());

        s->current->children.emplace_back(std::move(next));
    }
    else
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        if (s->current->currentChild >= s->current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        SerialNode* leaf = s->current->children[s->current->currentChild++].get();
        if (leaf->data.size() != data.size())
        {
            setError("Unexpected data length while deserializing");
            return false;
        }
        std::memcpy(data.data(), leaf->data.data(), data.size());
    }
    return true;
}

template<>
bool serialize(Serializer* s, std::string* val)
{
    if (s->writing)
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = s->current;
        next->data = std::vector<u8>(val->begin(), val->end());

        s->current->children.emplace_back(std::move(next));
    }
    else
    {
        if (s->current == nullptr)
            s->current = s->root.get();

        if (s->current->currentChild >= s->current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        SerialNode* leaf = s->current->children[s->current->currentChild++].get();
        val->assign(leaf->data.begin(), leaf->data.end());
    }
    return true;
}

static constexpr char serialBinTag[] = "HGSD"; // Hurdy Gurdy Serial Data
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

bool isSerialData(std::span<const u8> bin)
{
    return bin.size() > sizeof(serialBinTag) && std::memcmp(bin.data(), serialBinTag, sizeof(serialBinTag)) == 0;
}

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)] = {};
    u32 versionMajor = 0;
    u32 versionMinor = 0;
    u32 versionPatch = 0;

    u32 nodeBegin = 0;
};

struct SerialBinNode {
    u32 isObject = 0;
    /**
     * The child count if object, the data length if leaf
     */
    u32 count = 0;
    /**
     * The child start index if object, the data start index if leaf
     */
    u32 begin = 0;
};

static void serialBinWriteNode(std::vector<u8>* bin, u32 idx, SerialNode* node)
{
    SerialBinNode out{};
    out.isObject = node->children.empty() ? 0 : 1;
    out.count = out.isObject
        ? static_cast<u32>(node->children.size())
        : static_cast<u32>(node->data.size());
    out.begin = static_cast<u32>(bin->size());
    bin->resize(bin->size() + (out.isObject
        ? out.count * sizeof(SerialBinNode)
        : out.count));
    std::memcpy(bin->data() + idx, &out, sizeof(out));

    if (out.isObject)
    {
        for (u32 i = 0; i < out.count; ++i)
        {
            serialBinWriteNode(
                bin, out.begin + i * static_cast<u32>(sizeof(SerialBinNode)), node->children[i].get());
        }
    }
    else if (out.count > 0)
    {
        std::memcpy(bin->data() + out.begin, node->data.data(), out.count);
    }
}

std::vector<u8> writeSerialBinary(Serializer* serial)
{
    SerialNode* root = serial->current != nullptr ? serial->current : serial->root.get();

    std::vector<u8> bin(sizeof(SerialBinHeader) + sizeof(SerialBinNode));

    SerialBinHeader header{};
    std::memcpy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;
    header.nodeBegin = sizeof(SerialBinHeader);
    std::memcpy(bin.data(), &header, sizeof(header));

    serialBinWriteNode(&bin, header.nodeBegin, root);
    return bin;
}

static bool serialBinReadNode(std::span<const u8> bin, u64 idx, Serializer* s)
{
    if (bin.size() < idx + sizeof(SerialBinNode))
    {
        setError("Serial binary is truncated");
        return false;
    }

    SerialBinNode node{};
    std::memcpy(&node, bin.data() + idx, sizeof(node));

    if (node.isObject)
    {
        serializeBegin(s);
        for (u32 i = 0; i < node.count; ++i)
        {
            u64 childIdx = node.begin + static_cast<u64>(i) * sizeof(SerialBinNode);
            if (!serialBinReadNode(bin, childIdx, s))
            {
                serializeEnd(s);
                return false;
            }
        }
        serializeEnd(s);
    }
    else
    {
        if (bin.size() < node.begin + static_cast<u64>(node.count))
        {
            setError("Serial binary is truncated");
            return false;
        }
        serializeVoid(s, {const_cast<u8*>(bin.data()) + node.begin, node.count});
    }
    return true;
}

std::optional<Serializer> readSerialBinary(std::span<const u8> bin)
{
    if (bin.size() < sizeof(SerialBinHeader))
    {
        setError("Serial binary is too small");
        return std::nullopt;
    }

    SerialBinHeader header{};
    std::memcpy(&header, bin.data(), sizeof(header));

    if (std::memcmp(header.tag, serialBinTag, sizeof(serialBinTag)) != 0)
    {
        setError("Serial binary could not be read, does not have a header");
        return std::nullopt;
    }

    if (header.versionMajor != serialBinVersionMajor)
    {
        setError("Serial binary has wrong major version: {} instead of {}", header.versionMajor, serialBinVersionMajor);
        return std::nullopt;
    }

    if (header.versionMinor != serialBinVersionMinor)
    {
        setError("Serial binary has wrong minor version: {} instead of {}", header.versionMinor, serialBinVersionMinor);
        return std::nullopt;
    }

    if (header.versionPatch != serialBinVersionPatch)
    {
        setError("Serial binary has wrong patch version: {} instead of {}", header.versionPatch, serialBinVersionPatch);
        return std::nullopt;
    }

    if (bin.size() < static_cast<u64>(header.nodeBegin) + sizeof(SerialBinNode))
    {
        setError("Serial binary is truncated");
        return std::nullopt;
    }

    SerialBinNode rootNode{};
    std::memcpy(&rootNode, bin.data() + header.nodeBegin, sizeof(rootNode));

    Serializer writer = serialWriter();
    if (rootNode.isObject)
    {
        for (u32 i = 0; i < rootNode.count; ++i)
        {
            u64 childIdx = rootNode.begin + static_cast<u64>(i) * sizeof(SerialBinNode);
            if (!serialBinReadNode(bin, childIdx, &writer))
                return std::nullopt;
        }
    }
    else
    {
        if (bin.size() < rootNode.begin + static_cast<u64>(rootNode.count))
        {
            setError("Serial binary is truncated");
            return std::nullopt;
        }
        serializeVoid(&writer, {const_cast<u8*>(bin.data()) + rootNode.begin, rootNode.count});
    }
    return serialReader(std::move(writer.root));
}

} // namespace hg
