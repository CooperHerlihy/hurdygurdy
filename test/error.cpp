#include "tests.hpp"
#include "hg/error.hpp"
#include "hg/logging.hpp"

#include <string>

using namespace hg;

TEST(testErrorSetGet)
{
    setError("test error");
    std::string_view err = getError();
    ASSERT(err == "test error");
}

TEST(testErrorReplace)
{
    setError("first error");
    setError("second error");
    std::string_view err = getError();
    ASSERT(err == "second error");
}

TEST(testErrorClear)
{
    setError("something");
    setError("");
    std::string_view err = getError();
    ASSERT(err.length() == 0);
}

TEST(testErrorAtBoundary)
{
    std::string longStr(4095, 'x');
    ASSERT(longStr.length() == 4095);

    setError(longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorExceedsBoundary)
{
    std::string longStr(5000, 'x');
    ASSERT(longStr.length() == 5000);

    setError(longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorFormatInt)
{
    setError("error code {}", 42);
    std::string_view err = getError();
    ASSERT(err == "error code 42");
}

TEST(testErrorFormatString)
{
    setError("failed to load \"{}\"", "texture.png");
    std::string_view err = getError();
    ASSERT(err == "failed to load \"texture.png\"");
}

TEST(testErrorFormatMultiple)
{
    setError("{}:{}: {}", "file.txt", 128, "unexpected token");
    std::string_view err = getError();
    ASSERT(err == "file.txt:128: unexpected token");
}

TEST(testErrorFormatTruncated)
{
    std::string longStr(4090, 'x');

    setError("{}", longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4090);

    std::string tooLong(5000, 'x');

    setError("{}", tooLong);
    err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorFreshState)
{
    setError("");
    std::string_view err = getError();
    ASSERT(err.length() == 0);
    ASSERT(err.data() != nullptr);
}

TEST(testLogErrorNormal)
{
    setError("log test message");
    printError();
}

TEST(testLogErrorEmpty)
{
    setError("");
    printError();
}

TEST(testLogErrorLong)
{
    std::string longStr(512, 'x');
    setError(longStr);
    printError();
}
