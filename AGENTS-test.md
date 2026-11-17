# Writing Tests

- most tests should quickly test one piece of functionality
- some tests should integrate a whole subsystem
- make sure tests cover all api surface area
- make sure to test for all invariants
- test constexpr items at compile time
- use `LifeCycle` in `tests.hpp` to test object lifecycles

format:
```
#include "tests.hpp"

using namespace hg;

TEST(testName) {
    ...
    ASSERT(condition);
    ...
    static_assert(constexpr condition);
    ...
}

...
```

