#pragma once

struct HurdyGurdy {
    ~HurdyGurdy() noexcept;
    HurdyGurdy() noexcept;
    HurdyGurdy(const HurdyGurdy&);
    HurdyGurdy& operator=(const HurdyGurdy&);
    HurdyGurdy(HurdyGurdy&& other) noexcept;
    HurdyGurdy& operator=(HurdyGurdy&& other) noexcept;
};

Maybe<HurdyGurdy> init();