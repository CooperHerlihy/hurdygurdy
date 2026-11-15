#include "tests.hpp"
#include "hg/dynlib.hpp"
#include "hg/error.hpp"

using namespace hg;

TEST(testLibraryLoadFailure)
{
    setError("");
    std::optional<Library> lib = Library::load("hg_nonexistent_library_12345");
    ASSERT(!lib.has_value());
    ASSERT(getError().length() > 0);
}

#if defined(HG_PLATFORM_LINUX) || defined(HG_PLATFORM_WINDOWS)

#if defined(HG_PLATFORM_LINUX)
static constexpr const char* realLibrary = "libm.so.6";
static constexpr const char* realSymbol = "cos";
#else
static constexpr const char* realLibrary = "kernel32.dll";
static constexpr const char* realSymbol = "GetCurrentProcessId";
#endif

TEST(testLibraryLoadSymbolFailure)
{
    std::optional<Library> lib = Library::load(realLibrary);
    ASSERT(lib.has_value());

    setError("");
    std::optional<void*> missing = lib->loadSymbol("hg_nonexistent_symbol_12345");
    ASSERT(!missing.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testLibraryLoadSymbol)
{
    std::optional<Library> lib = Library::load(realLibrary);
    ASSERT(lib.has_value());

    std::optional<void*> found = lib->loadSymbol(realSymbol);
    ASSERT(found.has_value());
    ASSERT(*found != nullptr);
}

#endif
