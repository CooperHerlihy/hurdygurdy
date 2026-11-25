#pragma once

#include <utility>

#define HG_MACRO_CONCAT_INTERNAL(x, y) x##y

#define HG_MACRO_CONCAT(x, y) HG_MACRO_CONCAT_INTERNAL(x, y)

/**
 * Defer a block of code to the end of the scope
 */
#define HG_DEFER(...) [[maybe_unused]] hg::internal::Defer HG_MACRO_CONCAT(defer_, __LINE__){[&]{ __VA_ARGS__ ;}}

namespace hg::internal {

/**
 * The scope guard to defer a function
 */
template<typename F>
struct Defer {
    /**
     * The function to defer
     */
    F fn;

    /**
     * Construct the defer scope
     */
    Defer(F&& fnVal)
        : fn(std::forward<F>(fnVal))
    {}

    /**
     * Call the deferred function
     */
    ~Defer()
    {
        fn();
    }
};

} // namespace hg::internal
