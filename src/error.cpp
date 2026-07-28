#include "hg/error.hpp"

#include <algorithm>

namespace hg {

thread_local static char errorData[4096];
thread_local static u64 errorLength = 0;

StringView getError()
{
    return {errorData, errorLength};
}

void setError(StringView error)
{
    u64 newLength = std::min(error.length, sizeof(errorData));
    memcpy(errorData, error.chars, newLength);
    errorLength = newLength;
}

void logError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", (int)errorLength, errorData);
}

} // namespace hg
