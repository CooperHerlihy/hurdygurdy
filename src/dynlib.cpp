#include "hg_dynlib.hpp"
#include "hg_strings.hpp"
#include "hg_error.hpp"
#include "hg_memory.hpp"

namespace hg {

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

Maybe<Library> Library::load(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, path);

    Maybe<Library> lib = some<Library>();
    lib->lib = dlopen(cstr, RTLD_LAZY);
    if (lib->lib == nullptr)
    {
        setError("Could not load dynamic library \"%s\": %s", cstr, dlerror());
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        dlclose(lib);
}

Maybe<void*> Library::findFunction(StringView name)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, name);

    void* fn = dlsym(lib, cstr);
    if (fn == nullptr)
    {
        setError("Could not load function symbol \"%s\": %s", cstr, dlerror());
        return {};
    }

    return some<void*>(fn);
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

Maybe<Library> Library::load(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, path);

    Maybe<Library> lib = some<Library>();
    lib->lib = static_cast<void*>(LoadLibraryA(cstr));
    if (lib->lib == nullptr)
    {
        setError("Could not load dynamic library \"%s\"", cstr);
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        FreeLibrary(static_cast<HMODULE>(lib));
}

Maybe<void*> Library::findFunction(StringView name)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, name);

    void* fn = reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(lib), cstr));
    if (fn == nullptr)
    {
        setError("Could not load function symbol \"%s\"", cstr);
        return {};
    }

    return some<void*>(fn);
}

#endif

} // namespace hg
