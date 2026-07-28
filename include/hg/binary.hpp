#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"

#include <cstring>

namespace hg {

/**
 * A non-owning view into binary data
 */
struct BinaryView {
    /**
     * The viewed data
     */
    const void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len) const
    {
        HG_ASSERT(idx + len <= size);
        memcpy(dst, static_cast<const u8*>(data) + idx, len);
    }

    /**
     * Read a section of data
     */
    template<typename T>
    T read(u64 idx) const
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }
};

/**
 * A binary builder
 */
struct BinaryBuilder {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The data
     */
    void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Construct empty
     */
    BinaryBuilder() noexcept = default;

    /**
     * Construct a new builder
     */
    BinaryBuilder(Arena* arenaVal, u64 sizeVal = 0)
        : arena{arenaVal}, size{sizeVal}
    {
        data = arena->alloc(sizeVal, 1);
    }

    /**
     * Implicitly convert to Binary
     */
    constexpr operator BinaryView()
    {
        return {data, size};
    }

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len) const
    {
        HG_ASSERT(idx + len <= size);
        memcpy(dst, static_cast<const u8*>(data) + idx, len);
    }

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     */
    template<typename T>
    T read(u64 idx)
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }

    /**
     * Resize the binary
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new size of the file in bytes
     */
    void resize(u64 newSize);

    /**
     * Overwrite data at the index
     *
     * Parameters
     * - idx The index into the file to overwrite
     * - src The data to write
     * - size The size of the data in bytes
     */
    void overwrite(u64 idx, const void* src, u64 len);

    /**
     * Overwrite data of arbitrary type at the index
     *
     * Parameters
     * - idx The index into the file to overwrite
     * - src The data to write
     */
    template<typename T>
    void overwrite(u64 idx, const T& src)
    {
        overwrite(idx, &src, sizeof(T));
    }

    /**
     * Append data to the end, increasing size
     */
    void append(const void* src, u64 len);

    /**
     * Append data of arbitrary type to the end, increasing size
     */
    template<typename T>
    void append(const T& src)
    {
        append(&src, sizeof(T));
    }
};

/**
 * An owner of binary data
 */
struct Binary {
    /**
     * The data
     */
    void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Construct empty
     */
    Binary() noexcept = default;

    /**
     * Create a new binary block from data
     */
    static Binary create(BinaryView data);

    /**
     * Free the binary
     */
    ~Binary() noexcept;

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len);

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     */
    template<typename T>
    T read(u64 idx)
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }

    /**
     * Implicitly convert to Binary
     */
    constexpr operator BinaryView()
    {
        return {data, size};
    }

    /**
     * Move construct
     */
    Binary(Binary&& other) noexcept
        : data{std::exchange(other.data, nullptr)}
        , size{std::exchange(other.size, 0)}
    {}

    /**
     * Move assign
     */
    Binary& operator=(Binary&& other) noexcept
    {
        if (this != &other)
        {
            this->~Binary();
            new (this) Binary{std::move(other)};
        }
        return *this;
    }

    Binary(const Binary&) = delete;
    Binary& operator=(const Binary&) = delete;
};

} // namespace hg
