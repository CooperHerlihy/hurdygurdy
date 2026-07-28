#include "hg/assets.hpp"
#include "hg/error.hpp"

#include <cstdio>

namespace hg {

template<>
void assetLoadImpl(AssetData<Binary>* data)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        setError("Could not find file to read binary: %s", cpath);
        return;
    }
    HG_DEFER(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        setError("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = static_cast<u64>(ftell(fileHandle));
    data->asset.data = heapAlloc(data->asset.size, 1);

    rewind(fileHandle);
    if (fread(const_cast<void*>(data->asset.data), 1, data->asset.size, fileHandle) != data->asset.size)
    {
        heapFree(const_cast<void*>(data->asset.data), data->asset.size);
        setError("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

bool binaryStore(BinaryView bin, StringView path)
{
    ArenaScope scratch = getScratch();

    char* cpath = cString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        setError("Failed to create file to write binary: %s", cpath);
        return false;
    }
    HG_DEFER(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        setError("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

} // namespace hg

