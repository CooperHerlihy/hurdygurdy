#pragma once

#include "hg/maybe.hpp"
#include "hg/strings.hpp"

namespace hg {

/**
 * A dynamically loaded library
 */
struct Library {
    /**
     * The library data
     */
    void* lib = nullptr;

    /**
     * Construct empty
     */
    Library() noexcept = default;

    /**
     * Unload the library
     */
    ~Library() noexcept;

    /**
     * Load a dynamic library
     */
    static Maybe<Library> load(StringView path);

    /**
     * Find a function pointer in the library
     */
    Maybe<void*> findFunction(StringView name);

    /**
     * Move construct
     */
    Library(Library&& other) noexcept
        : lib{std::exchange(other.lib, nullptr)}
    {}

    /**
     * Move assign
     */
    Library& operator=(Library&& other) noexcept
    {
        if (this != &other)
        {
            this->~Library();
            new (this) Library{std::move(other)};
        }
        return *this;
    }

    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
};

} // namespace hg

