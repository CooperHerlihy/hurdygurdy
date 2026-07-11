#include "hurdygurdy.hpp"

using namespace hg;

int main()
{
    init();
    HG_DEFER(deinit());

    test();

    HG_LOG("All tests passed\n");
    return 0;
}
