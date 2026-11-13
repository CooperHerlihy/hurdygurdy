#pragma once

#include <utility>
#include <optional>
#include <string_view>

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
    static std::optional<Library> load(std::string_view path);

    /**
     * Find a function pointer in the library
     */
    std::optional<void*> loadSymbol(std::string_view name);

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

