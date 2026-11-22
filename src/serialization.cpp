#include "hg/serialization.hpp"

#include "hg/utility.hpp"

#include <cstring>

namespace hg {

Serializer Serializer::reader(std::span<const byte> data)
{
    Reader r{};
    r.bin = data;
    r.pos = isSerialData(data) ? 2 : 0;

    Serializer s{};
    s.s = r;
    return s;
}

Serializer Serializer::writer()
{
    Writer w{};
    w.bin.resize(sizeof(serialDataSignature));
    std::memcpy(w.bin.data(), serialDataSignature, sizeof(serialDataSignature));

    Serializer s{};
    s.s = w;
    return s;
}

std::span<const byte> Serializer::get() const
{
    return std::visit([&](const auto& s) -> std::span<const byte> { return s.bin; }, s);
}

std::vector<byte> Serializer::takeFromWriter()
{
    if (isReading())
        return {};

    HG_DEFER(s = {});
    return std::move(std::get<Writer>(s).bin);
}

/**
 * Returns true if the serializer is reading from the data
 */
bool Serializer::isReading() const
{
    return std::holds_alternative<Reader>(s);
}

/**
 * Returns true if the serializer is writing to the data
 */
bool Serializer::isWriting() const
{
    return std::holds_alternative<Writer>(s);
}


bool Serializer::serializeData(std::span<byte> data)
{
    return match(s,
    [&](Reader& r)
    {
        if (r.pos + data.size() > r.bin.size())
        {
            setError("Serializer hit unexpected end of data");
            r.pos = r.bin.size();
            return false;
        }

        for (byte& b : data)
        {
            b = r.bin[r.pos++];
        }
        return true;
    },
    [&](Writer& w)
    {
        for (byte b : data)
        {
            w.bin.emplace_back(b);
        }
        return true;
    });
}

} // namespace hg
