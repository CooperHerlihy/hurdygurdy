#pragma once

#include "hg/inttypes.hpp"
#include "hg/utility.hpp"
#include "hg/format.hpp"
#include "hg/hash.hpp"

#include <span>
#include <optional>
#include <tuple>
#include <variant>
#include <limits>
#include <memory>
#include <string>
#include <array>
#include <vector>

namespace hg {

/**
 * The first two bytes of any Hurdy Gurdy serialized data file
 */
static constexpr char serialDataSignature[] = {'H', 'G'};

/**
 * Returns whether a binary buffer contains serial data in Hurdy Gurdy format
 */
constexpr bool isSerialData(std::span<const byte> bin)
{
    return bin.size() >= sizeof(serialDataSignature)
        && std::memcmp(bin.data(), serialDataSignature, sizeof(serialDataSignature)) == 0;
}

/**
 * The data for serialization
 */
class Serializer {
private:
    /**
     * A reader
     */
    struct Reader {
        /**
         * The binary being read
         */
        std::span<const byte> bin;
        /**
         * The current position in the binary
         */
        usize pos;
    };

    /**
     * A writer
     */
    struct Writer {
        /**
         * The binary being written
         */
        std::vector<byte> bin;
    };

    /**
     * The reader or writer
     */
    std::variant<Reader, Writer> s;

public:
    /**
     * Construct empty
     */
    Serializer() noexcept = default;

    /**
     * Begin a serial reader
     */
    static Serializer reader(std::span<const byte> data);

    /**
     * Begin a serial writer
     */
    static Serializer writer();

    /**
     * Get the binary data from the serializer
     */
    std::span<const byte> get() const;

    /**
     * The the binary data from the writer, leaving an empty reader
     */
    std::vector<byte> takeFromWriter();

    /**
     * The number of bytes left to read, or the max u64 when writing
     */
    u64 remaining() const;

    /**
     * Returns true if the serializer is reading from the data
     */
    bool isReading() const;

    /**
     * Returns true if the serializer is writing to the data
     */
    bool isWriting() const;

    /**
     * Serialize data directly
     */
    bool serializeData(std::span<byte> data);
};

/**
 * Serialize a value, should be overridden
 */
template<typename T>
bool serialize(Serializer& s, T* val)
{
    return s.serializeData(std::as_writable_bytes(std::span{val, 1}));
}

/**
 * Deserialize an object from a binary
 */
template<typename T>
std::optional<T> serialLoadFromBinary(std::span<const byte> bin)
{
    if (!isSerialData(bin))
        return std::nullopt;

    Serializer s = Serializer::reader(bin);
    std::optional ret = std::make_optional<T>();
    if (!serialize(s, &*ret))
        return std::nullopt;

    return ret;
}

/**
 * Deserialize an object from a binary
 */
template<typename T>
std::vector<byte> serialStoreToBinary(const T& val)
{
    Serializer s = Serializer::writer();
    serialize(s, const_cast<T*>(&val));
    return s.takeFromWriter();
}

/**
 * Deserialize an object from a file
 */
template<typename T>
std::optional<T> serialLoad(std::string_view path)
{
    std::optional<std::vector<byte>> file = binaryLoad(path);
    if (!file.has_value())
        return std::nullopt;

    std::optional<T> ret = serialLoadFromBinary<T>(*file);
    if (!ret.has_value())
    {
        setError("Could not deserialize file \"{}\": {}", path, getError());
        return std::nullopt;
    }

    return ret;
}

/**
 * Serialize an object to a file
 */
template<typename T>
bool serialStore(const T& val, std::string_view path)
{
    std::vector<byte> bin = serialStoreToBinary(val);
    return binaryStore(bin, path);
}

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
bool serializeObject(Serializer& s, Ts*... vals)
{
    return (serialize(s, vals) && ...);
}

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
bool serialize(Serializer& s, T (*arr)[N])
{
    for (T& val : *arr)
    {
        if (!serialize(s, &val))
            return false;
    }
    return true;
}

/**
 * optional serialization
 */
template<typename T>
bool serialize(Serializer& s, std::optional<T>* maybe)
{
    bool has = maybe->has_value();
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (s.isReading() && !maybe->has_value())
            *maybe = std::make_optional<T>();
        return serialize(s, &maybe->value());
    }
    else
    {
        if (s.isReading())
            *maybe = std::nullopt;
        return true;
    }
}

/**
 * tuple serialization
 */
template<typename... Ts>
bool serialize(Serializer& s, std::tuple<Ts...>* tuple)
{
    if constexpr (sizeof...(Ts) > 0)
    {
        return [&]<u64... Is>(std::integer_sequence<u64, Is...>)
        {
            return serializeObject(s, &std::get<Is>(*tuple)...);
        }(std::make_integer_sequence<u64, sizeof...(Ts)>{});
    }
    else
    {
        return true;
    }
}

namespace internal {
    template<usize I = 0, typename... Ts>
    static void defaultInitVariant(std::variant<Ts...>& v, usize idx)
    {
        if constexpr (I < sizeof...(Ts))
        {
            HG_ASSERT(idx < sizeof...(Ts));
            if (idx == I)
                v.template emplace<I>();
            else
                defaultInitVariant<I + 1>(v, idx);
        }
    }
}

/**
 * variant serialization
 */
template<typename... Ts>
bool serialize(Serializer& s, std::variant<Ts...>* variant)
{
    usize tag = variant->index();
    if (!serialize(s, &tag))
        return false;

    if (s.isReading() && tag != variant->index())
    {
        internal::defaultInitVariant(*variant, tag);
    }
    return std::visit([&](auto& val)
    {
        return serialize(s, &val);
    }, *variant);
}

/**
 * unique ptr serialization
 */
template<typename T>
bool serialize(Serializer& s, std::unique_ptr<T>* ptr)
{
    bool has = *ptr != nullptr;
    if (!serialize(s, &has))
        return false;

    if (has)
    {
        if (s.isReading())
            *ptr = std::make_unique<T>();
        return serialize(s, ptr->get());
    }
    else
    {
        *ptr = nullptr;
        return true;
    }
}

/**
 * string serialization
 */
template<>
inline bool serialize(Serializer& s, std::string* val)
{
    u64 length = val->length();
    if (!serialize(s, &length))
        return false;

    if (s.isReading())
    {
        if (length > s.remaining())
        {
            setError("Could not deserialize string, length exceeds remaining data");
            return false;
        }
        val->resize(length);
    }
    return s.serializeData(std::as_writable_bytes(std::span{*val}));
}

/**
 * array serialization
 */
template<typename T, usize I>
bool serialize(Serializer& s, std::array<T, I>* arr)
{
    for (T& val : *arr)
    {
        if (!serialize(s, &val))
            return false;
    }
    return true;
}

/**
 * vector serialization
 */
template<typename T>
bool serialize(Serializer& s, std::vector<T>* vec)
{
    u64 size = vec->size();
    if (!serialize(s, &size))
        return false;

    if (s.isReading())
    {
        // each element takes at least one byte, so reject lengths past the input
        if (size > s.remaining())
        {
            setError("Could not deserialize vector, size exceeds remaining data");
            return false;
        }
        vec->resize(size);
    }

    for (T& val : *vec)
    {
        if (!serialize(s, &val))
            return false;
    }
    return true;
}

/**
 * hash set serialization
 */
template<typename V>
bool serialize(Serializer& s, HashSet<V>* set)
{
    u64 size = set->size();
    if (!serialize(s, &size))
        return false;

    if (s.isWriting())
    {
        set->forEach([&](const V& val)
        {
            serialize(s, const_cast<V*>(&val));
        });
    }
    else
    {
        if (size > s.remaining())
        {
            setError("Could not deserialize hash set, size exceeds remaining data");
            return false;
        }
        set->clear();
        for (u64 i = 0; i < size; ++i)
        {
            V val;
            if (!serialize(s, &val))
                return false;
            set->add(val);
        }
    }
    return true;
}

/**
 * hash map serialization
 */
template<typename K, typename V>
bool serialize(Serializer& s, HashMap<K, V>* map)
{
    u64 size = map->size();
    if (!serialize(s, &size))
        return false;

    if (s.isWriting())
    {
        map->forEach([&](const K& key, V& val)
        {
            serializeObject(s, const_cast<K*>(&key), &val);
        });
    }
    else
    {
        if (size > s.remaining())
        {
            setError("Could not deserialize hash map, size exceeds remaining data");
            return false;
        }
        map->clear();
        for (u64 i = 0; i < size; ++i)
        {
            K key;
            V val;
            if (!serializeObject(s, &key, &val))
                return false;
            map->add(std::move(key), std::move(val));
        }
    }
    return true;
}

} // namespace hg
