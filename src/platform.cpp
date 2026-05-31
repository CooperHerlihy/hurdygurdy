#include "hurdygurdy.hpp"

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)dlopen(cstr, RTLD_LAZY);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\": %s\n", cstr, dlerror());

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        dlclose(lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = dlsym(lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\": %s\n", cstr, dlerror());

    return fn;
}

#elif defined(HG_PLATFORM_WINDOWS)

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)LoadLibraryA(cstr);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\"\n", cstr);

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        FreeLibrary((HMODULE)lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = GetProcAddress((HMODULE)lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\": %s\n", cstr, dlerror());

    return fn;
}

#endif

