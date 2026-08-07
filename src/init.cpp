#include "hg/init.hpp"

#include "internal.hpp"

namespace hg {

static bool initialized = false;
static u32 initCount = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized)
        return some<HurdyGurdy>();

    if (!internal::initPlatform())
        return {};

    if (!internal::initGpu())
    {
        internal::deinitPlatform();
        return {};
    }

    if (!internal::initAudio())
    {
        internal::deinitGpu();
        internal::deinitPlatform();
        return {};
    }

    internal::initRender2D();

    initialized = true;
    return some<HurdyGurdy>();
}

HurdyGurdy::HurdyGurdy() noexcept
{
    ++initCount;
}

HurdyGurdy::HurdyGurdy(const HurdyGurdy&)
{
    ++initCount;
}

HurdyGurdy& HurdyGurdy::operator=(const HurdyGurdy&)
{
    ++initCount;
    return *this;
}

HurdyGurdy::HurdyGurdy(HurdyGurdy&&) noexcept
{
    ++initCount;
}

HurdyGurdy& HurdyGurdy::operator=(HurdyGurdy&&) noexcept
{
    ++initCount;
    return *this;
}

HurdyGurdy::~HurdyGurdy() noexcept
{
    if (--initCount == 0 && initialized)
    {
        internal::deinitRender2D();

        internal::deinitAudio();
        internal::deinitGpu();
        internal::deinitPlatform();

        initialized = false;
    }
}

} // namespace hg
