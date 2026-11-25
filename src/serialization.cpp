#include "hg/serialization.hpp"

#include "hg/defer.hpp"
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

u64 Serializer::remaining() const
{
    return match(s,
        [&](const Reader& r) -> u64
        {
            return assertCast<u64>(r.bin.size() - r.pos);
        },
        [&](const Writer&) -> u64
        {
            return std::numeric_limits<u64>::max();
        });
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

        std::memcpy(data.data(), r.bin.data() + r.pos, data.size());
        r.pos += data.size();
        return true;
    },
    [&](Writer& w)
    {
        u64 pos = w.bin.size();
        w.bin.resize(pos + data.size());
        std::memcpy(w.bin.data() + pos, data.data(), data.size());
        return true;
    });
}

} // namespace hg
