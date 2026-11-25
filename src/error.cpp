#include "hg/error.hpp"

#include "hg/inttypes.hpp"
#include "hg/assert.hpp"

#include <cstring>

namespace hg {

thread_local static char errorData[4096];
thread_local static u32 errorLength = 0;

void setError(std::string_view error)
{
    u32 newLength = assertCast<u32>(std::min(error.length(), (sizeof(errorData) - 1)));
    std::memcpy(errorData, error.data(), newLength);
    errorData[newLength] = '\0';
    errorLength = newLength;
}

std::string_view getError()
{
    return {errorData, errorLength};
}

void printError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", errorLength, errorData);
}

} // namespace hg
