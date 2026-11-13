#include "hg/dynlib.hpp"
#include "hg/error.hpp"
#include "hg/memory.hpp"

namespace hg {

#if defined(HG_PLATFORM_LINUX) || defined(HG_PLATFORM_APPLE)

#include <dlfcn.h>

std::optional<Library> Library::load(std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    std::optional<Library> lib = std::make_optional<Library>();
    lib->lib = dlopen(p.c_str(), RTLD_LAZY);
    if (lib->lib == nullptr)
    {
        setError("Could not load dynamic library \"{}\": {}", p.c_str(), dlerror());
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        dlclose(lib);
}

std::optional<void*> Library::loadSymbol(std::string_view name)
{
    ArenaScope scratch = getScratch();
    string_temp n{name, scratch};

    std::optional<void*> fn = dlsym(lib, n.c_str());
    if (*fn == nullptr)
    {
        setError("Could not load function symbol \"{}\": {}", n.c_str(), dlerror());
        return {};
    }

    return fn;
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::optional<Library> Library::load(std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    std::optional<Library> lib = std::make_optional<Library>();
    lib->lib = static_cast<void*>(LoadLibraryA(p.c_str()));
    if (lib->lib == nullptr)
    {
        setError("Could not load dynamic library \"{}\"", p.c_str());
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        FreeLibrary(static_cast<HMODULE>(lib));
}

std::optional<void*> Library::loadSymbol(StringView name)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    std::optional<void*> fn = reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(lib), n.c_str()));
    if (fn == nullptr)
    {
        setError("Could not load function symbol \"{}\"", n.c_str());
        return {};
    }

    return fn;
}

#endif

} // namespace hg
