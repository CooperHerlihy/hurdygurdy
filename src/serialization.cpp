#include "hg/serialization.hpp"

#include "hg/utility.hpp"

#include <string_view>
#include <cstring>

namespace hg {

static constexpr char serialBinTag[] = "HGSD"; // Hurdy Gurdy Serial Data
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

bool isSerialData(std::span<const byte> bin)
{
    return bin.size() > sizeof(serialBinTag) && std::memcmp(bin.data(), serialBinTag, sizeof(serialBinTag)) == 0;
}

Serializer Serializer::writer()
{
    Serializer s{};
    s.root = std::make_unique<SerialNode>();
    s.current = nullptr;
    s.writing = true;
    return s;
}

Serializer Serializer::reader(std::unique_ptr<SerialNode> begin)
{
    Serializer s{};
    s.root = std::move(begin);
    s.current = nullptr;
    s.writing = false;
    return s;
}

std::unique_ptr<SerialNode> Serializer::takeNode()
{
    writing = false;
    current = nullptr;
    return std::move(root);
}

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)] = {};
    u32 versionMajor = 0;
    u32 versionMinor = 0;
    u32 versionPatch = 0;

    u32 nodeBegin = 0;
};

struct SerialBinNode {
    /**
     * The node is an object if 1, a leaf if 0
     */
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

static void serialBinWriteNode(std::vector<byte>* bin, u32 idx, SerialNode* node)
{
    SerialBinNode out{};
    out.isObject = node->children.empty() ? 0 : 1;
    out.count = out.isObject
        ? assertCast<u32>(node->children.size())
        : assertCast<u32>(node->data.size());
    out.begin = assertCast<u32>(bin->size());
    bin->resize(bin->size() + (out.isObject
        ? out.count * sizeof(SerialBinNode)
        : out.count));
    std::memcpy(bin->data() + idx, &out, sizeof(out));

    if (out.isObject)
    {
        for (u32 i = 0; i < out.count; ++i)
        {
            serialBinWriteNode(
                bin, out.begin + i * assertCast<u32>(sizeof(SerialBinNode)), node->children[i].get());
        }
    }
    else if (out.count > 0)
    {
        std::memcpy(bin->data() + out.begin, node->data.data(), out.count);
    }
}

std::vector<byte> Serializer::writeBinary()
{
    std::vector<byte> bin(sizeof(SerialBinHeader) + sizeof(SerialBinNode));

    SerialBinHeader header{};
    std::memcpy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;
    header.nodeBegin = sizeof(SerialBinHeader);
    std::memcpy(bin.data(), &header, sizeof(header));

    if (root != nullptr)
        serialBinWriteNode(&bin, header.nodeBegin, root.get());
    return bin;
}

static bool serialBinReadNode(std::span<const byte> bin, u64 idx, Serializer* s)
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
        s->beginObject();
        for (u32 i = 0; i < node.count; ++i)
        {
            u64 childIdx = node.begin + assertCast<u64>(i) * sizeof(SerialBinNode);
            if (!serialBinReadNode(bin, childIdx, s))
            {
                s->endObject();
                return false;
            }
        }
        s->endObject();
    }
    else
    {
        if (bin.size() < node.begin + assertCast<u64>(node.count))
        {
            setError("Serial binary is truncated");
            return false;
        }
        s->serializeData({const_cast<byte*>(bin.data()) + node.begin, node.count});
    }
    return true;
}

/**
 * Read binary data to be deserialized, nullopt and an error set on failure
 */
std::optional<Serializer> Serializer::readBinary(std::span<const byte> bin)
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

    if (bin.size() < assertCast<u64>(header.nodeBegin) + sizeof(SerialBinNode))
    {
        setError("Serial binary is truncated");
        return std::nullopt;
    }

    SerialBinNode rootNode{};
    std::memcpy(&rootNode, bin.data() + header.nodeBegin, sizeof(rootNode));

    Serializer writer = Serializer::writer();
    if (rootNode.isObject)
    {
        for (u32 i = 0; i < rootNode.count; ++i)
        {
            u64 childIdx = rootNode.begin + assertCast<u64>(i) * sizeof(SerialBinNode);
            if (!serialBinReadNode(bin, childIdx, &writer))
                return std::nullopt;
        }
    }
    else
    {
        if (bin.size() < rootNode.begin + assertCast<u64>(rootNode.count))
        {
            setError("Serial binary is truncated");
            return std::nullopt;
        }
        writer.serializeData({const_cast<byte*>(bin.data()) + rootNode.begin, rootNode.count});
    }

    writer.current = nullptr;
    writer.writing = false;
    return writer;
}

bool Serializer::beginObject(u32* size)
{
    if (writing)
    {
        if (current == nullptr)
            current = root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = current;

        current = current->children.emplace_back(std::move(next)).get();
        if (size != nullptr)
            current->children.reserve(*size);
    }
    else
    {
        if (current == nullptr)
            current = root.get();

        if (current->currentChild >= current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        current = current->children[current->currentChild++].get();

        if (size != nullptr)
            *size = assertCast<u32>(current->children.size());
    }
    return true;
}

void Serializer::endObject()
{
    HG_ASSERT(current != nullptr);
    current = current->parent;
}

bool Serializer::serializeData(std::span<byte> data)
{
    if (writing)
    {
        if (current == nullptr)
            current = root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = current;
        next->data = std::vector<byte>(data.begin(), data.end());

        current->children.emplace_back(std::move(next));
    }
    else
    {
        if (current == nullptr)
            current = root.get();

        if (current->currentChild >= current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        SerialNode* leaf = current->children[current->currentChild++].get();
        if (leaf->data.size() != data.size())
        {
            setError("Unexpected data length while deserializing");
            return false;
        }
        if (!data.empty())
            std::memcpy(data.data(), leaf->data.data(), data.size());
    }
    return true;
}

bool Serializer::serializeVoid(std::span<const byte>* data)
{
    if (writing)
    {
        if (current == nullptr)
            current = root.get();

        std::unique_ptr next = std::make_unique<SerialNode>();
        next->parent = current;
        next->data = std::vector<byte>(data->begin(), data->end());

        current->children.emplace_back(std::move(next));
    }
    else
    {
        if (current == nullptr)
            current = root.get();

        if (current->currentChild >= current->children.size())
        {
            setError("Unexpected end of serial data");
            return false;
        }

        SerialNode* leaf = current->children[current->currentChild++].get();
        *data = leaf->data;
    }
    return true;
}

} // namespace hg
