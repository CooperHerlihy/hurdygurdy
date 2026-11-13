#pragma once

#include <optional>

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
std::optional<HurdyGurdy> init();

} // namespace hg
