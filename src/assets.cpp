#include "hg/assets.hpp"

namespace hg {

template<>
bool assetLoadImpl(AssetData<Binary>* data)
{
    Maybe<Binary> loaded = loadFile(data->path);
    if (!loaded.has)
    {
        setError("Could not load binary file: %.*s", (int)data->path.length, data->path.chars);
        return false;
    }
    data->asset = std::move(loaded.val);
    return true;
}

} // namespace hg
