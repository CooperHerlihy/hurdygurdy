#pragma once

#include "hg/maybe.hpp"

namespace hg {

/**
 * A scope guard to deinitialize the library when all users are finished
 */
struct HurdyGurdy {
    ~HurdyGurdy() noexcept;
    HurdyGurdy() noexcept;
    HurdyGurdy(const HurdyGurdy&);
    HurdyGurdy& operator=(const HurdyGurdy&);
    HurdyGurdy(HurdyGurdy&& other) noexcept;
    HurdyGurdy& operator=(HurdyGurdy&& other) noexcept;
};

/**
 * Initialize the HurdyGurdy library
 */
Maybe<HurdyGurdy> init();

} // namespace hg
