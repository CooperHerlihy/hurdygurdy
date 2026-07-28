#include "hg/init.hpp"

#include "internal.hpp"

namespace hg {

static u32 initialized = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized > 0)
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

    return some<HurdyGurdy>();
}

HurdyGurdy::HurdyGurdy() noexcept
{
    ++initialized;
}

HurdyGurdy::HurdyGurdy(const HurdyGurdy&)
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(const HurdyGurdy&)
{
    ++initialized;
    return *this;
}

HurdyGurdy::HurdyGurdy(HurdyGurdy&&) noexcept
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(HurdyGurdy&&) noexcept
{
    ++initialized;
    return *this;
}

HurdyGurdy::~HurdyGurdy() noexcept
{
    if (--initialized == 0)
    {
        internal::deinitAudio();
        internal::deinitGpu();
        internal::deinitPlatform();
    }
}

} // namespace hg
