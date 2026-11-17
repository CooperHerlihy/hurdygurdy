#include "hg/error.hpp"
#include "hg/utility.hpp"

#include <algorithm>
#include <cstring>

namespace hg {

thread_local static char errorData[4096];
thread_local static u32 errorLength = 0;

std::string_view getError()
{
    return {errorData, errorLength};
}

void setError(std::string_view error)
{
    u32 newLength = assertCast<u32>(std::min(error.length(), (sizeof(errorData) - 1)));
    std::memcpy(errorData, error.data(), newLength);
    errorData[newLength] = '\0';
    errorLength = newLength;
}

void logError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", errorLength, errorData);
}

} // namespace hg
