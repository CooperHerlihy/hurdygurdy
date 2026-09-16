#include "hg/assets.hpp"
#include "hg/filesystem.hpp"
#include "hg/error.hpp"

namespace hg {

template<>
void assetLoadImpl(AssetData<Binary>* data)
{
    Maybe<Binary> loaded = loadFile(data->path);
    if (!loaded.has)
    {
        setError("Could not load binary asset: %.*s", (int)data->path.length, data->path.chars);
        return;
    }

    data->asset = std::move(loaded.val);
}

} // namespace hg
