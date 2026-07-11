#include "hurdygurdy.hpp"

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    hgTest();

    hgLog("All tests passed\n");
    return 0;
}
