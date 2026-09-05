#include "tests.hpp"
#include "hg/error.hpp"

using namespace hg;

TEST(testErrorSetGet)
{
    setError("test error");
    StringView err = getError();
    ASSERT(err == "test error");
}

TEST(testErrorReplace)
{
    setError("first error");
    setError("second error");
    StringView err = getError();
    ASSERT(err == "second error");
}

TEST(testErrorClear)
{
    setError("something");
    setError("");
    StringView err = getError();
    ASSERT(err.length == 0);
}

TEST(testErrorAtBoundary)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 4096; ++i)
        longStr.append('x');
    ASSERT(longStr.length == 4096);

    setError(longStr);
    StringView err = getError();
    ASSERT(err.length == 4096);
}

TEST(testErrorExceedsBoundary)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 5000; ++i)
        longStr.append('x');
    ASSERT(longStr.length == 5000);

    setError(longStr);
    StringView err = getError();
    ASSERT(err.length == 4096);
}

TEST(testErrorFormatInt)
{
    setError("error code %d", 42);
    StringView err = getError();
    ASSERT(err == "error code 42");
}

TEST(testErrorFormatString)
{
    setError("failed to load \"%s\"", "texture.png");
    StringView err = getError();
    ASSERT(err == "failed to load \"texture.png\"");
}

TEST(testErrorFormatMultiple)
{
    setError("%s:%d: %s", "file.txt", 128, "unexpected token");
    StringView err = getError();
    ASSERT(err == "file.txt:128: unexpected token");
}

TEST(testErrorFormatTruncated)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 4090; ++i)
        longStr.append('x');

    setError("%.*s", (int)longStr.length, longStr.chars);
    StringView err = getError();
    ASSERT(err.length == 4090);

    StringBuilder tooLong{arena};
    for (u32 i = 0; i < 5000; ++i)
        tooLong.append('x');

    setError("%.*s", (int)tooLong.length, tooLong.chars);
    err = getError();
    ASSERT(err.length == 4095);
}

TEST(testErrorFreshState)
{
    setError("");
    StringView err = getError();
    ASSERT(err.length == 0);
    ASSERT(err.chars != nullptr);
}

TEST(testLogErrorNormal)
{
    setError("log test message");
    logError();
}

TEST(testLogErrorEmpty)
{
    setError("");
    logError();
}

TEST(testLogErrorLong)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 512; ++i)
        longStr.append('x');
    setError(longStr);
    logError();
}
