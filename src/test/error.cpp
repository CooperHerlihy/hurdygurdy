#include "tests.hpp"

void testError()
{
    // ============================================================================
    // Error Handling
    // ============================================================================
    //
    // The error handling API provides thread-local error state using a
    // 4096-byte buffer per thread. Errors are set via setError() (either
    // a plain StringView or a printf-style format string) and retrieved
    // via getError(). logError() prints the current error to stderr.
    //
    // The API is used throughout the engine for recoverable failures
    // (e.g., init failures, file load failures) and pairs with Maybe<T>
    // for functions that can fail.

    // ------------------------------------------------------------------
    // setError / getError: plain string
    // ------------------------------------------------------------------

    // Setting an error and reading it back
    {
        setError("test error");
        StringView err = getError();
        TEST(err == "test error");
    }

    // Setting a new error replaces the previous one
    {
        setError("first error");
        setError("second error");
        StringView err = getError();
        TEST(err == "second error");
    }

    // Clearing the error by setting an empty string
    {
        setError("something");
        setError("");
        StringView err = getError();
        TEST(err.length == 0);
    }

    // An error exactly at the 4096-byte boundary
    {
        ArenaScope arena = getScratch();
        StringBuilder longStr{arena};
        for (u32 i = 0; i < 4096; ++i)
            longStr.append('x');
        TEST(longStr.length == 4096);

        setError(longStr);
        StringView err = getError();
        TEST(err.length == 4096);
    }

    // An error exceeding 4096 bytes is truncated
    {
        ArenaScope arena = getScratch();
        StringBuilder longStr{arena};
        for (u32 i = 0; i < 5000; ++i)
            longStr.append('x');
        TEST(longStr.length == 5000);

        setError(longStr);
        StringView err = getError();
        TEST(err.length == 4096);
    }

    // ------------------------------------------------------------------
    // setError / getError: formatted string
    // ------------------------------------------------------------------

    // Basic formatting with integers
    {
        setError("error code %d", 42);
        StringView err = getError();
        TEST(err == "error code 42");
    }

    // Formatting with a string argument
    {
        setError("failed to load \"%s\"", "texture.png");
        StringView err = getError();
        TEST(err == "failed to load \"texture.png\"");
    }

    // Formatting with multiple arguments
    {
        setError("%s:%d: %s", "file.txt", 128, "unexpected token");
        StringView err = getError();
        TEST(err == "file.txt:128: unexpected token");
    }

    // Formatted error with a long message that gets truncated.
    // snprintf truncates to 4095 bytes (sizeof(buf) - 1), then
    // setError copies the result (at most 4096 bytes).
    // Note: StringBuilder.chars is NOT null-terminated, so use %.*s.
    {
        ArenaScope arena = getScratch();
        StringBuilder longStr{arena};
        for (u32 i = 0; i < 4090; ++i)
            longStr.append('x');

        setError("%.*s", (int)longStr.length, longStr.chars);
        StringView err = getError();
        TEST(err.length == 4090);

        StringBuilder tooLong{arena};
        for (u32 i = 0; i < 5000; ++i)
            tooLong.append('x');

        setError("%.*s", (int)tooLong.length, tooLong.chars);
        err = getError();
        TEST(err.length == 4095);
    }

    // ------------------------------------------------------------------
    // getError: default / initial state
    // ------------------------------------------------------------------

    // Fresh error state is empty
    {
        // Set and clear first to ensure clean state
        setError("");
        StringView err = getError();
        TEST(err.length == 0);
        TEST(err.chars != nullptr); // points to the buffer, not null
    }

    // ------------------------------------------------------------------
    // logError: smoke test (just ensure it does not crash)
    // ------------------------------------------------------------------

    // logError with a normal error string
    {
        setError("log test message");
        // No assertion — we just verify it doesn't crash or abort
        logError();
    }

    // logError with empty error
    {
        setError("");
        logError();
    }

    // logError with long error
    {
        ArenaScope arena = getScratch();
        StringBuilder longStr{arena};
        for (u32 i = 0; i < 512; ++i)
            longStr.append('x');
        setError(longStr);
        logError();
    }
}

