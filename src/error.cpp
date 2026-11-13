#include "hg/error.hpp"

#include <algorithm>
#include <cstring>

namespace hg {

thread_local static char errorData[4096];
thread_local static u64 errorLength = 0;

std::string_view getError()
{
    return {errorData, errorLength};
}

void setError(std::string_view error)
{
    u64 newLength = std::min(error.length(), static_cast<u64>(sizeof(errorData) - 1));
    std::memcpy(errorData, error.data(), newLength);
    errorData[newLength] = '\0';
    errorLength = newLength;
}

void logError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", static_cast<int>(errorLength), errorData);
}

} // namespace hg
