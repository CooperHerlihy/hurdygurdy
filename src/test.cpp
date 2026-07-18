#undef HG_NO_LOGGING
#define HG_LOGGING 1
#include "hurdygurdy.hpp"

#include <emmintrin.h>

#define TEST(cond) do { \
    if (!(cond)) \
        HG_PANIC("Test failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

using namespace hg;

/**
 * Tracks lifetime events for testing RAII correctness
 *
 * Each instance gets a unique ID.  Stats tracks aggregate counts
 * (alive, default-constructed, copy-constructed, moved, destroyed).
 * Call stats.reset() before a test block, then verify at scope exits.
 *
 * The Tag parameter enables independent counters per subsystem:
 *   using MyTypeLifecycle = LifecycleT<struct MyTypeTag>;
 */
template<typename Tag = void>
struct LifecycleT {
    struct Stats {
        i64 alive = 0;
        i64 ctors = 0;
        i64 copies = 0;
        i64 moves = 0;
        i64 dtors = 0;

        void reset() { *this = Stats{}; }
    };

    static Stats stats;
    static u64 s_nextId;

    bool valid = false;
    u64 id;

    LifecycleT()
        : valid(true)
        , id(s_nextId++)
    {
        stats.alive++;
        stats.ctors++;
    }

    LifecycleT(const LifecycleT& o)
        : valid(o.valid)
        , id(s_nextId++)
    {
        if (valid)
        {
            stats.alive++;
            stats.copies++;
        }
    }

    LifecycleT(LifecycleT&& o)
        : valid(o.valid)
        , id(o.id)
    {
        o.valid = false;
        stats.moves++;
    }

    ~LifecycleT()
    {
        if (valid)
        {
            stats.alive--;
            stats.dtors++;
        }
    }

    LifecycleT& operator=(const LifecycleT&) = delete;
    LifecycleT& operator=(LifecycleT&&) = delete;
};

template<typename Tag>
typename LifecycleT<Tag>::Stats LifecycleT<Tag>::stats{};

template<typename Tag>
u64 LifecycleT<Tag>::s_nextId = 0;

using Lifecycle = LifecycleT<>;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    HG_LOG("Tests Begun\n");

    Clock timer{};

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
    //
    // Functions covered:
    // - setError(StringView): set a plain error string
    // - setError(fmt, args...): set a formatted error string
    // - getError(): retrieve the current error
    // - logError(): log the error to stderr (smoke test only)
    // - Maybe<T>: some(), none(), orElse(), expect(), copy/move
    {
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

    // ============================================================================
    // StringView operators
    // ============================================================================
    //
    // StringView is a non-owning view into a string (chars + length).
    // Equality compares length first, then memcmp.
    //
    // Functions covered:
    // - operator==(StringView, StringView)
    // - operator!=(StringView, StringView)

    // Equal strings
    {
        StringView a{"hello"};
        StringView b{"hello"};
        TEST(a == b);
        TEST(!(a != b));
    }

    // Different strings
    {
        StringView a{"hello"};
        StringView b{"world"};
        TEST(a != b);
        TEST(!(a == b));
    }

    // Empty strings are equal
    {
        StringView a{};
        StringView b{};
        TEST(a == b);
    }

    // Empty vs non-empty
    {
        StringView a{};
        StringView b{"x"};
        TEST(a != b);
    }

    // Same content, different pointer (should still compare equal)
    {
        const char* s1 = "abcdef";
        const char* s2 = "abcdef";
        StringView a{s1, 3};
        StringView b{s2, 3};
        TEST(a == b);
    }

    // Different lengths, same prefix
    {
        StringView a{"hello", 5};
        StringView b{"hello world", 5};
        TEST(a == b);
    }

    // Different lengths, same prefix (one longer)
    {
        StringView a{"hello world", 11};
        StringView b{"hello", 5};
        TEST(a != b);
    }

    // nullptr handling — constructing from nullptr gives empty view
    {
        const char* nullStr = nullptr;
        StringView sv{nullStr};
        TEST(sv.chars == nullptr);
        TEST(sv.length == 0);

        StringView empty{};
        TEST(sv == empty);
    }

    // StringView from (ptr, length) with zero length
    {
        const char* data = "hello";
        StringView sv{data, u64{0}};
        TEST(sv.length == 0);
        TEST(StringView{} == sv);
    }

    // StringView from begin/end pointers
    {
        const char* data = "hello world";
        StringView sv{data + 6, data + 11};
        TEST(sv == "world");
    }

    // StringView from begin/end with equal pointers (empty range)
    {
        const char* data = "hello";
        StringView sv{data, data};
        TEST(sv.length == 0);
    }

    // StringView indexing
    {
        StringView sv{"hello"};
        TEST(sv[0] == 'h');
        TEST(sv[1] == 'e');
        TEST(sv[2] == 'l');
        TEST(sv[3] == 'l');
        TEST(sv[4] == 'o');
    }

    // StringView range-for
    {
        StringView sv{"abc"};
        char result[4]{};
        u64 i = 0;
        for (char c : sv)
        {
            result[i] = c;
            ++i;
        }
        result[i] = '\0';
        TEST(StringView{result} == "abc");
    }

    // StringView from const char* implicit conversion (long string)
    {
        StringView sv{"this is a fairly long string that should work fine"};
        TEST(sv.length == 50);
        TEST(sv == "this is a fairly long string that should work fine");
    }

    // ============================================================================
    // BinaryView
    // ============================================================================
    //
    // BinaryView is a non-owning view into binary data (data pointer + size).
    // read() and read<T>() copy bytes out at an offset.
    //
    // Functions covered:
    // - BinaryView() — default constructor
    // - BinaryView(void*, u64) — ptr+size constructor
    // - read(u64, void*, u64)
    // - read<T>(u64)

    // Default-constructed BinaryView is empty
    {
        BinaryView bv{};
        TEST(bv.data == nullptr);
        TEST(bv.size == 0);
    }

    // Create from pointer and size
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        TEST(bv.data == &val);
        TEST(bv.size == sizeof(val));
    }

    // read<T>() copies typed data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = bv.read<u32>(0);
        TEST(result == 42);
    }

    // read() copies raw data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = 0;
        bv.read(0, &result, sizeof(result));
        TEST(result == 42);
    }

    // read<T>() at offset
    {
        u8 data[8] = {0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
        BinaryView bv{data, 8};
        u32 result = bv.read<u32>(4);
        TEST(result == 0xDDCCBBAA);
    }

    // ============================================================================
    // Span<T>
    // ============================================================================
    //
    // Span is a non-owning typed view (pointer + count). Supports array,
    // ptr+count, begin+end constructors, indexing, and range-for.
    //
    // Functions covered:
    // - Span() — default
    // - Span(T*, u64) — ptr + count
    // - Span(T*, T*) — begin + end
    // - Span(T (&)[N]) — array constructor
    // - operator[]
    // - begin() / end()

    // Default-constructed Span is empty
    {
        Span<i32> s;
        TEST(s.vals == nullptr);
        TEST(s.count == 0);
    }

    // Construct from pointer and count
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s{vals, 3};
        TEST(s.vals == vals);
        TEST(s.count == 3);
        TEST(s[0] == 10);
        TEST(s[2] == 30);
    }

    // Construct from begin and end
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s{vals, vals + 3};
        TEST(s.count == 3);
        TEST(s[0] == 10);
        TEST(s[1] == 20);
    }

    // Construct from array
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s = vals;
        TEST(s.count == 3);
        TEST(s[0] == 10);
    }

    // Range-for over Span
    {
        i32 vals[4] = {1, 2, 3, 4};
        Span<i32> s{vals, 4};
        i32 sum = 0;
        for (i32 v : s)
            sum += v;
        TEST(sum == 10);
    }

    // begin() / end() give correct boundaries
    {
        i32 vals[2] = {100, 200};
        Span<i32> s{vals, 2};
        TEST(s.begin() == vals);
        TEST(s.end() == vals + 2);
    }

    // ============================================================================
    // Span<void>
    // ============================================================================
    //
    // Span<void> is a type-erased non-owning view. Same constructors as
    // Span<T> but indexing returns void*.
    //
    // Functions covered:
    // - Span<void>() — default
    // - Span<void>(void*, u64) — ptr + count
    // - Span<void>(void*, void*) — begin + end
    // - operator[]

    // Default-constructed Span<void> is empty
    {
        Span<void> s;
        TEST(s.vals == nullptr);
        TEST(s.count == 0);
    }

    // Construct from pointer and count
    {
        f32 vals[3] = {1.0f, 2.0f, 3.0f};
        Span<void> s{static_cast<void*>(vals), 3};
        TEST(s.vals == vals);
        TEST(s.count == 3);
    }

    // Construct from begin and end
    {
        u8 data[4] = {10, 20, 30, 40};
        Span<void> s{data, data + 4};
        TEST(s.count == 4);
        void* ptr = s[2];
        TEST(ptr == static_cast<void*>(data + 2));
    }

    // ============================================================================
    // Maybe
    // ============================================================================
    //
    // Maybe<T> is a lightweight optional type used for recoverable error
    // handling. It holds a boolean `has` and a union containing the value.
    // some() creates a filled Maybe, none() creates an empty one.
    // orElse(default) returns the value or a fallback; expect(msg) returns
    // the value or panics.
    //
    // Functions covered:
    // - some<T>(args...): create a filled Maybe
    // - none<T>(): create an empty Maybe
    // - has: check whether a value is present
    // - val: access the value (direct union access)
    // - orElse(T): unwrap or return default
    // - expect(StringView): unwrap or panic
    // - copy construction and assignment
    // - move construction and assignment
    {
        // ------------------------------------------------------------------
        // some() / none() with trivial types
        // ------------------------------------------------------------------

        // some() creates a filled Maybe
        {
            Maybe<i32> m = some<i32>(42);
            TEST(m.has);
            TEST(m.val == 42);
        }

        // none() creates an empty Maybe
        {
            Maybe<i32> m = none<i32>();
            TEST(!m.has);
        }

        // some() with u32
        {
            Maybe<u32> m = some<u32>(100u);
            TEST(m.has);
            TEST(m.val == 100u);
        }

        // none() leaves the value uninitialized (but has is false)
        {
            Maybe<u32> m = none<u32>();
            TEST(!m.has);
        }

        // some() with floating point
        {
            Maybe<f32> m = some<f32>(3.14f);
            TEST(m.has);
            TEST(std::abs(m.val - 3.14f) <= FLT_EPSILON);
        }

        // some() with a boolean
        {
            Maybe<bool> m = some<bool>(true);
            TEST(m.has);
            TEST(m.val == true);
        }

        // ------------------------------------------------------------------
        // orElse()
        // ------------------------------------------------------------------

        // orElse returns the value when present
        {
            Maybe<i32> m = some<i32>(42);
            i32 result = m.orElse(-1);
            TEST(result == 42);
            TEST(!m.has); // value was moved out
        }

        // orElse returns the default when empty
        {
            Maybe<i32> m = none<i32>();
            i32 result = m.orElse(-1);
            TEST(result == -1);
            TEST(!m.has);
        }

        // orElse with floating point
        {
            Maybe<f32> m = none<f32>();
            f32 result = m.orElse(1.0f);
            TEST(std::abs(result - 1.0f) <= FLT_EPSILON);
        }

        // orElse can be called on an already-consumed Maybe (no-op)
        {
            Maybe<i32> m = some<i32>(42);
            m.orElse(-1);
            TEST(!m.has); // consumed

            i32 result = m.orElse(-2);
            TEST(result == -2);
        }

        // ------------------------------------------------------------------
        // expect() (positive cases — negative case would panic/abort)
        // ------------------------------------------------------------------

        // expect returns the value when present
        {
            Maybe<i32> m = some<i32>(42);
            i32 result = m.expect("should have value");
            TEST(result == 42);
            TEST(!m.has); // value was moved out
        }

        // expect with string type
        {
            ArenaScope arena = getScratch();
            Maybe<StringBuilder> m = some<StringBuilder>(arena, "hello");
            StringBuilder result = m.expect("string should exist");
            TEST(result == "hello");
            TEST(!m.has);
        }

        // ------------------------------------------------------------------
        // Copy semantics
        // ------------------------------------------------------------------

        // Copy construct a filled Maybe
        {
            Maybe<i32> a = some<i32>(42);
            Maybe<i32> b{a};

            TEST(a.has);
            TEST(a.val == 42);
            TEST(b.has);
            TEST(b.val == 42);
        }

        // Copy construct an empty Maybe
        {
            Maybe<i32> a = none<i32>();
            Maybe<i32> b{a};

            TEST(!a.has);
            TEST(!b.has);
        }

        // Copy assign a filled Maybe
        {
            Maybe<i32> a = some<i32>(42);
            Maybe<i32> b = none<i32>();
            b = a;

            TEST(a.has);
            TEST(a.val == 42);
            TEST(b.has);
            TEST(b.val == 42);
        }

        // Copy assign an empty Maybe
        {
            Maybe<i32> a = none<i32>();
            Maybe<i32> b = some<i32>(10);
            b = a;

            TEST(!a.has);
            TEST(!b.has);
        }

        // Self-copy-assignment is a no-op
        {
            Maybe<i32> m = some<i32>(42);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
            m = m;
#pragma clang diagnostic pop

            TEST(m.has);
            TEST(m.val == 42);
        }

        // Copy assign a filled Maybe onto another filled Maybe destroys old value
        {
            Maybe<i32> a = some<i32>(42);
            Maybe<i32> b = some<i32>(10);

            // Both alive before
            TEST(a.has && a.val == 42);
            TEST(b.has && b.val == 10);

            b = a;

            TEST(a.has && a.val == 42);
            TEST(b.has && b.val == 42);
        }

        // ------------------------------------------------------------------
        // Move semantics
        // ------------------------------------------------------------------

        // Move construct a filled Maybe
        {
            Maybe<i32> a = some<i32>(42);
            Maybe<i32> b{std::move(a)};

            TEST(!a.has); // moved-from is empty
            TEST(b.has);
            TEST(b.val == 42);
        }

        // Move construct an empty Maybe
        {
            Maybe<i32> a = none<i32>();
            Maybe<i32> b{std::move(a)};

            TEST(!a.has);
            TEST(!b.has);
        }

        // Move assign a filled Maybe
        {
            Maybe<i32> a = some<i32>(42);
            Maybe<i32> b = none<i32>();
            b = std::move(a);

            TEST(!a.has); // moved-from is empty
            TEST(b.has);
            TEST(b.val == 42);
        }

        // Move assign an empty Maybe
        {
            Maybe<i32> a = none<i32>();
            Maybe<i32> b = some<i32>(10);
            b = std::move(a);

            TEST(!a.has);
            TEST(!b.has);
        }

        // Self-move-assignment is a no-op
        {
            Maybe<i32> m = some<i32>(42);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
            m = std::move(m);
#pragma clang diagnostic pop

            TEST(m.has);
            TEST(m.val == 42);
        }

        // ------------------------------------------------------------------
        // Maybe with non-trivial types
        // ------------------------------------------------------------------

        // some() with String (non-trivial: has ~String(), copy deleted)
        {
            Maybe<String> m = some<String>(String::create("hello world"));
            TEST(m.has);
            TEST(m.val == "hello world");
        }

        // Move a Maybe<String>
        {
            Maybe<String> a = some<String>(String::create("move me"));
            Maybe<String> b = std::move(a);

            TEST(!a.has);
            TEST(b.has);
            TEST(b.val == "move me");
        }

        // Move-assign a Maybe<String>
        {
            Maybe<String> a = some<String>(String::create("first"));
            Maybe<String> b = some<String>(String::create("second"));
            b = std::move(a);

            TEST(!a.has);
            TEST(b.has);
            TEST(b.val == "first");
        }

        // ------------------------------------------------------------------
        // Maybe with custom struct
        // ------------------------------------------------------------------

        // some() with a plain-old-data struct
        {
            struct Pod {
                i64 a;
                f32 b;
            };

            Maybe<Pod> m = some<Pod>(Pod{-12, 3.14f});
            TEST(m.has);
            TEST(m.val.a == -12);
            TEST(std::abs(m.val.b - 3.14f) <= FLT_EPSILON);
        }

        // none() with a struct type
        {
            struct Pod {
                i64 a;
                f32 b;
            };

            Maybe<Pod> m = none<Pod>();
            TEST(!m.has);
        }

        // ------------------------------------------------------------------
        // Maybe lifecycle: destruction tracking
        // ------------------------------------------------------------------
        //
        // Use a tracked type to verify constructors and destructors are
        // called exactly once per object across all Maybe operations.

        // (Lifecycle struct defined at file scope above)

        // none<T>() should not construct or destroy anything
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> m = none<Lifecycle>();
                TEST(!m.has);
                TEST(Lifecycle::stats.alive == 0);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // some<T>() constructs, Maybe destructor destroys
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> m = some<Lifecycle>();
                TEST(m.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Move construct from filled: value transferred, no extra construction
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
                {
                    Maybe<Lifecycle> b = std::move(a);
                    TEST(!a.has);
                    TEST(b.has);
                    TEST(Lifecycle::stats.alive == 1);
                }
                TEST(Lifecycle::stats.alive == 0);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Move construct from empty: no construction
        {
            Lifecycle::stats.reset();
            Maybe<Lifecycle> a = none<Lifecycle>();
            Maybe<Lifecycle> b = std::move(a);
            TEST(!a.has);
            TEST(!b.has);
            TEST(Lifecycle::stats.alive == 0);
        }

        // Move assign (filled → empty): value transferred
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                Maybe<Lifecycle> b = none<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
                b = std::move(a);
                TEST(!a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Move assign (filled → filled): old dest destroyed
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                Maybe<Lifecycle> b = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 2);
                b = std::move(a);
                TEST(!a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Copy construct from filled: new copy constructed
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
                {
                    Maybe<Lifecycle> b{a};
                    TEST(a.has);
                    TEST(b.has);
                    TEST(Lifecycle::stats.alive == 2);
                }
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Copy assign (filled → filled): old dest destroyed, new copy
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                Maybe<Lifecycle> b = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 2);
                b = a;
                TEST(a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 2);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Copy assign (empty → filled): dest destroyed, no construct
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = none<Lifecycle>();
                Maybe<Lifecycle> b = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
                b = a;
                TEST(!a.has);
                TEST(!b.has);
                TEST(Lifecycle::stats.alive == 0);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Copy assign (filled → empty): copy constructed into dest
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> a = some<Lifecycle>();
                Maybe<Lifecycle> b = none<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
                b = a;
                TEST(a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 2);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Self-copy-assign: no-op
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> m = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
                m = m;
#pragma clang diagnostic pop
                TEST(m.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Self-move-assign: no-op
        {
            Lifecycle::stats.reset();
            {
                Maybe<Lifecycle> m = some<Lifecycle>();
                TEST(Lifecycle::stats.alive == 1);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
                m = std::move(m);
#pragma clang diagnostic pop
                TEST(m.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }

        // Move assign (empty → empty): no-op
        {
            Lifecycle::stats.reset();
            Maybe<Lifecycle> a = none<Lifecycle>();
            Maybe<Lifecycle> b = none<Lifecycle>();
            b = std::move(a);
            TEST(!a.has);
            TEST(!b.has);
            TEST(Lifecycle::stats.alive == 0);
        }
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================
    //
    // isPowerOf2 checks whether a value is a power of two.
    // align rounds a value up to the next alignment boundary.
    // endianReverse16/32/64 swap byte order.
    //
    // Functions covered:
    // - isPowerOf2(u64)
    // - align(uptr, uptr)
    // - endianReverse16(u16)
    // - endianReverse32(u32)
    // - endianReverse64(u64)

    // isPowerOf2: powers of two
    {
        TEST(isPowerOf2(1));
        TEST(isPowerOf2(2));
        TEST(isPowerOf2(4));
        TEST(isPowerOf2(1024));
        TEST(isPowerOf2(0x80000000ull));
    }

    // isPowerOf2: not powers of two
    {
        TEST(!isPowerOf2(0));
        TEST(!isPowerOf2(3));
        TEST(!isPowerOf2(5));
        TEST(!isPowerOf2(1023));
        TEST(!isPowerOf2(0x80000001ull));
    }

    // align: already aligned
    {
        TEST(align(0, 4) == 0);
        TEST(align(4, 4) == 4);
        TEST(align(1024, 256) == 1024);
    }

    // align: needs rounding up
    {
        TEST(align(1, 4) == 4);
        TEST(align(3, 4) == 4);
        TEST(align(5, 4) == 8);
        TEST(align(1025, 256) == 1280);
    }

    // align: alignment of 1 (no-op)
    {
        TEST(align(0, 1) == 0);
        TEST(align(42, 1) == 42);
    }

    // endianReverse16
    {
        TEST(endianReverse16(0x1234) == 0x3412);
        TEST(endianReverse16(endianReverse16(0x1234)) == 0x1234);
        TEST(endianReverse16(0x0000) == 0x0000);
        TEST(endianReverse16(0xFFFF) == 0xFFFF);
    }

    // endianReverse32
    {
        TEST(endianReverse32(0x12345678) == 0x78563412);
        TEST(endianReverse32(endianReverse32(0x12345678)) == 0x12345678);
        TEST(endianReverse32(0x00000000) == 0x00000000);
        TEST(endianReverse32(0xFFFFFFFF) == 0xFFFFFFFF);
    }

    // endianReverse64
    {
        u64 val = 0x0102030405060708ull;
        TEST(endianReverse64(val) == 0x0807060504030201ull);
        TEST(endianReverse64(endianReverse64(val)) == val);
        TEST(endianReverse64(0x0000000000000000ull) == 0x0000000000000000ull);
        TEST(endianReverse64(0xFFFFFFFFFFFFFFFFull) == 0xFFFFFFFFFFFFFFFFull);
    }

    // ============================================================================
    // Memory
    // ============================================================================
    //
    // heapAlloc / heapFree allocate and free general-purpose memory.
    // Arena is a bump allocator.  ArenaScope provides RAII head-restore.
    // getScratch returns a thread-local arena that doesn't conflict with
    // active arenas.
    //
    // Functions covered:
    // - heapAlloc(u64, u64)
    // - heapAlloc<T>(u64)
    // - heapFree(void*, u64)
    // - heapFree<T>(T*, u64)
    // - Arena() — default
    // - Arena(u64) — capacity constructor
    // - ~Arena()
    // - Arena::alloc(u64, u64)
    // - Arena::alloc<T>(u64)
    // - Arena::extend(void*, u64, u64)
    // - Arena::extend<T>(T*, u64, u64)
    // - Arena(Arena&&) — move construct
    // - Arena& operator=(Arena&&) — move assign
    // - ArenaScope() — default
    // - ArenaScope(Arena*) — scope constructor
    // - ~ArenaScope() — restores head
    // - operator Arena*()
    // - ArenaScope::alloc(u64, u64)
    // - ArenaScope::alloc<T>(u64)
    // - ArenaScope::extend(void*, u64, u64)
    // - ArenaScope::extend<T>(T*, u64, u64)
    // - ArenaScope(ArenaScope&&) — move construct
    // - ArenaScope& operator=(ArenaScope&&) — move assign
    // - getScratch(...)

    // ------------------------------------------------------------------
    // heapAlloc / heapFree
    // ------------------------------------------------------------------

    // heapAlloc returns non-null pointer
    {
        void* p = heapAlloc(64, 1);
        TEST(p != nullptr);
        heapFree(p, 64);
    }

    // heapAlloc zero size returns valid pointer (malloc(0) is non-null)
    {
        void* p = heapAlloc(0, 1);
        TEST(p != nullptr);
        heapFree(p, 0);
    }

    // heapAlloc<T> template — allocates typed array
    {
        u32* arr = heapAlloc<u32>(4);
        TEST(arr != nullptr);
        arr[0] = 10;
        arr[3] = 40;
        TEST(arr[0] == 10);
        TEST(arr[3] == 40);
        heapFree<u32>(arr, 4);
    }

    // ------------------------------------------------------------------
    // Arena — default constructor
    // ------------------------------------------------------------------

    // Default-constructed Arena has null memory, zero capacity
    {
        Arena a{};
        TEST(a.memory == nullptr);
        TEST(a.capacity == 0);
        TEST(a.head == 0);
        // Double-destroy is safe (nullptr check in ~Arena)
    }

    // ------------------------------------------------------------------
    // Arena — capacity constructor
    // ------------------------------------------------------------------

    // Capacity constructor allocates memory
    {
        Arena a{256};
        TEST(a.memory != nullptr);
        TEST(a.capacity == 256);
        TEST(a.head == 0);
    }

    // ------------------------------------------------------------------
    // Arena — alloc
    // ------------------------------------------------------------------

    // Allocate from arena
    {
        Arena a{256};
        u32* p = static_cast<u32*>(a.alloc(4, 4));
        TEST(p != nullptr);
        TEST(a.head == 4);
        *p = 42;
        TEST(*p == 42);
    }

    // Allocate respects alignment (bumps to next aligned address)
    {
        Arena a{256};
        // Allocate 1 byte at alignment 1 — head becomes 1
        a.alloc(1, 1);
        // Allocate 4 bytes at alignment 4 — head aligns from 1 to 4, then +4 = 8
        void* p = a.alloc(4, 4);
        TEST(p != nullptr);
        TEST(a.head == 8);
        // The pointer should be 4-byte aligned
        TEST((reinterpret_cast<uptr>(p) & 3) == 0);
    }

    // Allocate exact capacity succeeds
    {
        Arena a{16};
        void* p = a.alloc(16, 1);
        TEST(p != nullptr);
        TEST(a.head == 16);
    }

    // Allocate out of memory returns nullptr and sets error
    {
        setError("");
        Arena a{16};
        a.alloc(16, 1); // fills arena
        void* p = a.alloc(1, 1); // overflows
        TEST(p == nullptr);
        TEST(getError().length > 0);
        setError(""); // clear for subsequent tests
    }

    // alloc<T> template — typed convenience
    {
        Arena a{256};
        u32* p = a.alloc<u32>(3);
        TEST(p != nullptr);
        p[0] = 10;
        p[1] = 20;
        p[2] = 30;
        TEST(p[0] == 10);
        TEST(p[2] == 30);
    }

    // ------------------------------------------------------------------
    // Arena — extend
    // ------------------------------------------------------------------

    // Extend the last allocation — succeeds
    {
        Arena a{256};
        u32* p = a.alloc<u32>(2);
        TEST(p != nullptr);
        TEST(a.head == 8);
        bool ok = a.extend(p, 2, 4);
        TEST(ok);
        TEST(a.head == 16);
    }

    // Extend non-last allocation — fails
    {
        Arena a{256};
        u32* first = a.alloc<u32>(2);
        a.alloc<u32>(2);
        bool ok = a.extend(first, 2, 4);
        TEST(!ok);
        // head unchanged
        TEST(a.head == 16);
    }

    // Extend beyond capacity — fails
    {
        Arena a{16};
        u8* p = static_cast<u8*>(a.alloc(4, 1));
        bool ok = a.extend(p, 4, 20);
        TEST(!ok);
    }

    // ------------------------------------------------------------------
    // Arena — move semantics
    // ------------------------------------------------------------------

    // Move construct — transfers ownership
    {
        Arena a{256};
        TEST(a.memory != nullptr);
        Arena b{std::move(a)};
        TEST(a.memory == nullptr); // moved-from is empty
        TEST(a.capacity == 0);
        TEST(a.head == 0);
        TEST(b.memory != nullptr);
        TEST(b.capacity == 256);
    }

    // Move assign — transfers ownership and frees old
    {
        Arena old{64};
        Arena a{128};
        old = std::move(a);
        TEST(a.memory == nullptr);
        TEST(old.memory != nullptr);
        TEST(old.capacity == 128);
    }

    // Self-move-assign is a no-op (moved-from state but no double-free)
    {
        Arena a{64};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        a = std::move(a);
#pragma clang diagnostic pop
        // moved-from state is valid (memory == nullptr)
        // no crash on destruction
    }

    // ------------------------------------------------------------------
    // ArenaScope — default constructor
    // ------------------------------------------------------------------

    // Default-constructed ArenaScope has null arena
    {
        ArenaScope scope{};
        TEST(scope.arena == nullptr);
        TEST(scope.head == 0);
    }

    // ------------------------------------------------------------------
    // ArenaScope — RAII head restoration
    // ------------------------------------------------------------------

    // Scope restores head on destruction
    {
        Arena a{256};
        a.alloc(16, 1); // head = 16
        {
            ArenaScope scope{&a};
            TEST(scope.arena == &a);
            a.alloc(32, 1); // head = 48
            TEST(a.head == 48);
        }
        TEST(a.head == 16); // restored
    }

    // Nested scopes restore correctly
    {
        Arena a{256};
        a.alloc(8, 1); // head = 8
        {
            ArenaScope outer{&a};
            a.alloc(8, 1); // head = 16
            {
                ArenaScope inner{&a};
                a.alloc(8, 1); // head = 24
                TEST(a.head == 24);
            }
            TEST(a.head == 16); // restored by inner
        }
        TEST(a.head == 8); // restored by outer
    }

    // Scope with null arena is safe
    {
        ArenaScope scope{};
        // No crash on destruction
    }

    // ------------------------------------------------------------------
    // ArenaScope — operator Arena*
    // ------------------------------------------------------------------

    // Implicit conversion to Arena* works
    {
        ArenaScope scope;
        Arena* ap = scope; // implicit conversion
        TEST(ap == scope.arena);
    }

    // ------------------------------------------------------------------
    // ArenaScope — forward alloc/extend
    // ------------------------------------------------------------------

    // ArenaScope::alloc forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = static_cast<u32*>(scope.alloc(4, 4));
        TEST(p != nullptr);
        TEST(a.head == 4);
    }

    // ArenaScope::alloc<T> forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = scope.alloc<u32>(3);
        TEST(p != nullptr);
        p[0] = 10;
        TEST(p[0] == 10);
    }

    // ArenaScope::extend forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = scope.alloc<u32>(2);
        TEST(p != nullptr);
        bool ok = scope.extend(p, 2, 4);
        TEST(ok);
        TEST(a.head == 16);
    }

    // ------------------------------------------------------------------
    // ArenaScope — move semantics
    // ------------------------------------------------------------------

    // Move construct
    {
        Arena a{256};
        ArenaScope scope{&a};
        ArenaScope other{std::move(scope)};
        TEST(scope.arena == nullptr); // moved-from
        TEST(other.arena == &a);
    }

    // Move assign
    {
        Arena a{256};
        Arena b{256};
        ArenaScope x{&a};
        ArenaScope y{&b};
        x = std::move(y);
        TEST(y.arena == nullptr); // moved-from
        TEST(x.arena == &b);
    }

    // ------------------------------------------------------------------
    // getScratch
    // ------------------------------------------------------------------

    // getScratch returns a valid arena with allocatable memory
    {
        ArenaScope scope = getScratch();
        Arena* ap = scope;
        TEST(ap != nullptr);
        TEST(ap->memory != nullptr);
        TEST(ap->capacity > 0);
        void* p = ap->alloc(64, 4);
        TEST(p != nullptr);
    }

    // Multiple getScratch calls without conflicts return the same arena
    {
        ArenaScope a = getScratch();
        ArenaScope b = getScratch();
        TEST(a.arena == b.arena);
    }

    // getScratch with conflict returns a different arena
    {
        ArenaScope a = getScratch();
        u32* p = a.alloc<u32>(1);
        TEST(p != nullptr);
        // Passing a's arena as conflict forces getScratch to find another
        Arena const* conflicts[] = {a.arena};
        ArenaScope b = getScratch(conflicts, 1);
        TEST(b.arena != a.arena);
        // The second arena is also usable
        u32* q = b.alloc<u32>(1);
        TEST(q != nullptr);
        *q = 42;
        TEST(*q == 42);
    }

    // ============================================================================
    // Concurrency
    // ============================================================================
    //
    // SpinLock is a basic spinlock mutex.  Fence is a completion counter.
    // callPar pushes work to a thread pool.  forPar iterates in parallel over
    // a range.  helpThreads processes work items while waiting on a fence.
    //
    // Functions covered:
    // - SpinLock::acquire()
    // - SpinLock::tryAcquire()
    // - SpinLock::release()
    // - Fence::add(u32)
    // - Fence::signal(u32)
    // - Fence::isComplete()
    // - Fence::wait(f64)
    // - Fence::waitIndefinite()
    // - helpThreads(Fence*, f64)
    // - callPar(Fence*, void*, void (*)(void*))
    // - forPar(u64, u64, void*, void (*)(void*, u64))
    // - forPar(u64, u64, F)  [template lambda]

    // Run the entire concurrency test suite multiple times to flush out
    // rare race conditions
    for (u32 concurrencyIter = 0; concurrencyIter < 20; ++concurrencyIter)
    {

    // ------------------------------------------------------------------
    // SpinLock — single-threaded basics
    // ------------------------------------------------------------------

    // Acquire then release
    {
        SpinLock lock{};
        lock.acquire();
        lock.release();
    }

    // tryAcquire succeeds when lock is free
    {
        SpinLock lock{};
        bool ok = lock.tryAcquire();
        TEST(ok);
        lock.release();
    }

    // tryAcquire fails when lock is already held
    {
        SpinLock lock{};
        lock.acquire();
        // In single-threaded context this is the only holder
        // We're testing that calling tryAcquire while we hold it fails
        // (The lock doesn't track thread identity, so it sees acquired==true)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
        bool ok = lock.tryAcquire();
#pragma clang diagnostic pop
        TEST(!ok);
        lock.release();
    }

    // ------------------------------------------------------------------
    // Fence — basics
    // ------------------------------------------------------------------

    // Default fence is complete
    {
        Fence fence{};
        TEST(fence.isComplete());
    }

    // add() makes fence incomplete
    {
        Fence fence{};
        fence.add();
        TEST(!fence.isComplete());
    }

    // signal() makes fence complete again
    {
        Fence fence{};
        fence.add();
        fence.signal();
        TEST(fence.isComplete());
    }

    // add(n) / signal(n) balanced
    {
        Fence fence{};
        fence.add(5);
        TEST(!fence.isComplete());
        fence.signal(5);
        TEST(fence.isComplete());
    }

    // add(3) / signal(1) still incomplete
    {
        Fence fence{};
        fence.add(3);
        fence.signal(1);
        TEST(!fence.isComplete());
        fence.signal(2);
        TEST(fence.isComplete());
    }

    // wait() on already-complete fence returns immediately
    {
        Fence fence{};
        bool ok = fence.wait(1.0);
        TEST(ok);
    }

    // wait() with timeout when fence never completes returns false
    {
        Fence fence{};
        fence.add();
        bool ok = fence.wait(0.0001);
        TEST(!ok);
        fence.signal();
    }

    // waitIndefinite() blocks until complete
    {
        Fence fence{};
        fence.add();
        // Signal from another thread
        std::thread t{[&fence] { fence.signal(); }};
        fence.waitIndefinite();
        TEST(fence.isComplete());
        t.join();
    }

    // ------------------------------------------------------------------
    // callPar — single worker
    // ------------------------------------------------------------------

    // Single work item via callPar
    {
        Fence fence{};
        bool executed = false;
        callPar(&fence, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        bool ok = fence.wait(2.0);
        TEST(ok);
        TEST(executed);
    }

    // Multiple work items
    {
        Fence fence{};
        bool a = false, b = false, c = false;
        callPar(&fence, &a, [](void* p) { *static_cast<bool*>(p) = true; });
        callPar(&fence, &b, [](void* p) { *static_cast<bool*>(p) = true; });
        callPar(&fence, &c, [](void* p) { *static_cast<bool*>(p) = true; });
        bool ok = fence.wait(2.0);
        TEST(ok);
        TEST(a && b && c);
    }

    // callPar with null fence (fire-and-forget, no crash)
    {
        callPar(nullptr, nullptr, [](void*)
        {
            // just verify no crash with null fence
        });
        // Use a separate fence to verify work eventually completes
        Fence fence{};
        bool executed = false;
        callPar(&fence, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        fence.wait(2.0);
        TEST(executed);
    }

    // Many sequential work items
    {
        Fence fence{};
        static constexpr u32 count = 100;
        bool vals[count]{};
        for (u32 i = 0; i < count; ++i)
        {
            callPar(&fence, &vals[i], [](void* p)
            {
                *static_cast<bool*>(p) = true;
            });
        }
        bool ok = fence.wait(2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // ------------------------------------------------------------------
    // helpThreads
    // ------------------------------------------------------------------

    // helpThreads completes work and returns true
    {
        Fence fence{};
        bool executed = false;
        callPar(&fence, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        TEST(executed);
    }

    // helpThreads with many items
    {
        Fence fence{};
        static constexpr u32 count = 1000;
        bool vals[count]{};
        for (u32 i = 0; i < count; ++i)
        {
            callPar(&fence, &vals[i], [](void* p)
            {
                *static_cast<bool*>(p) = true;
            });
        }
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // ------------------------------------------------------------------
    // forPar — C callback
    // ------------------------------------------------------------------

    // Process range with C callback
    {
        static constexpr u64 count = 100;
        bool vals[count]{};
        forPar(u64{0}, u64{count}, vals, [](void* data, u64 idx)
        {
            static_cast<bool*>(data)[idx] = true;
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // Large range stress
    {
        static constexpr u64 count = 10000;
        std::atomic<u32> atomicSum{0};
        forPar(u64{0}, u64{count}, &atomicSum, [](void* data, u64)
        {
            static_cast<std::atomic<u32>*>(data)->fetch_add(1);
        });
        // The function adds 1 per iteration, regardless of which thread runs it
        // Since each iteration is independent, the total should be count
        // But since fetch_add is atomic, the sum is correct
        // Actually no — the function's data pointer is shared, but the
        // iterations are independent and each adds exactly 1.
        // Since each callPar processes a chunk of iterations, each
        // iteration calls fetch_add(1). With count=10000, the result
        // should be count exactly.
        // (The old thread pool test used a mutex; here we use atomic
        // to avoid the SpinLock assertion issue.)
        u32 total = atomicSum.load();
        // Note: each iteration of forPar is not guaranteed to execute
        // its own atomic add — the C callback form processes chunks
        // internally. Each chunk calls fn(data, idx) for each idx,
        // which calls fetch_add(1). So count == total.
        TEST(total == count);
    }

    // ------------------------------------------------------------------
    // forPar — template lambda
    // ------------------------------------------------------------------

    // Process range with lambda
    {
        static constexpr u64 count = 100;
        bool vals[count]{};
        forPar(u64{0}, u64{count}, [&](u64 idx)
        {
            vals[idx] = true;
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // Large range with lambda
    {
        static constexpr u64 count = 10000;
        std::atomic<u32> sum{0};
        forPar(u64{0}, u64{count}, [&](u64)
        {
            sum.fetch_add(1);
        });
        TEST(sum.load() == count);
    }

    // ------------------------------------------------------------------
    // Multi-threaded stress: concurrent producers
    // ------------------------------------------------------------------
    //
    // Multiple threads call callPar concurrently, then the main thread
    // waits on the fence with helpThreads.

    // Two threads producing work
    {
        Fence fence{};
        static constexpr u32 count = 50;
        bool vals[count]{};

        std::thread t1{[&]
        {
            for (u32 i = 0; i < count / 2; ++i)
            {
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
            }
        }};

        std::thread t2{[&]
        {
            for (u32 i = count / 2; i < count; ++i)
            {
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
            }
        }};

        t1.join();
        t2.join();

        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // Four threads producing work
    {
        Fence fence{};
        static constexpr u32 count = 200;
        bool vals[count]{};
        static constexpr u32 threadCount = 4;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&, t]
            {
                u32 begin = (count / threadCount) * t;
                u32 end = (t == threadCount - 1) ? count : begin + count / threadCount;
                for (u32 i = begin; i < end; ++i)
                {
                    callPar(&fence, &vals[i], [](void* p)
                    {
                        *static_cast<bool*>(p) = true;
                    });
                }
            }};
        }

        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();

        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // ------------------------------------------------------------------
    // Multi-threaded stress: concurrent fence add/signal
    // ------------------------------------------------------------------

    // Multiple threads signal the same fence, main thread waits
    {
        Fence fence{};
        static constexpr u32 threadCount = 8;
        static constexpr u32 signalsPerThread = 100;
        fence.add(threadCount * signalsPerThread);

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&fence]
            {
                for (u32 i = 0; i < signalsPerThread; ++i)
                    fence.signal();
            }};
        }

        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();

        TEST(fence.isComplete());
    }

    // ------------------------------------------------------------------
    // SpinLock stress test
    // ------------------------------------------------------------------

    // SpinLock: repeated acquire/release cycle
    {
        SpinLock lock{};
        static constexpr u32 iterations = 1000;
        for (u32 i = 0; i < iterations; ++i)
        {
            lock.acquire();
            lock.release();
        }
    }

    // SpinLock: tryAcquire/release cycle
    {
        SpinLock lock{};
        static constexpr u32 iterations = 100;
        for (u32 i = 0; i < iterations; ++i)
        {
            bool ok = lock.tryAcquire();
            TEST(ok);
            lock.release();
        }
    }

    // SpinLock: multi-threaded mutual exclusion
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 10000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    lock.acquire();
                    ++shared;
                    lock.release();
                }
            }};
        }

        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();

        TEST(shared == threadCount * incrementsPerThread);
    }

    // SpinLockScope: basic acquire/release via RAII
    {
        SpinLock lock{};
        {
            SpinLockScope scope{&lock};
            // lock is held
        }
        // lock is released — can re-acquire
        lock.acquire();
        lock.release();
    }

    // SpinLockScope: null lock is safe
    {
        SpinLockScope scope{};
        TEST(scope.lock == nullptr);
        // no crash on destruction
    }

    // SpinLockScope: move construct transfers ownership
    {
        SpinLock lock{};
        {
            SpinLockScope inner{&lock};
            TEST(inner.lock == &lock);
            SpinLockScope moved{std::move(inner)};
            TEST(inner.lock == nullptr);  // moved-from is null
            TEST(moved.lock == &lock);
            // moved releases on destruction
        }
        // lock was released by moved's destructor
        lock.acquire();
        lock.release();
    }

    // SpinLockScope: move assign transfers ownership
    {
        SpinLock lock{};
        SpinLockScope a{&lock};
        SpinLockScope b{};
        b = std::move(a);
        TEST(a.lock == nullptr);
        TEST(b.lock == &lock);
        // b releases on scope exit
    }

    // SpinLockScope: self-move-assign is safe (no-op, no crash)
    {
        SpinLock lock{};
        SpinLockScope scope{&lock};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        scope = std::move(scope);
#pragma clang diagnostic pop
        // self-move is no-op, scope still holds lock
        // no crash on destruction
    }

    // SpinLockScope: multi-threaded with RAII guard
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 5000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    SpinLockScope scope{&lock};
                    ++shared;
                }
            }};
        }

        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();

        TEST(shared == threadCount * incrementsPerThread);
    }

    // ------------------------------------------------------------------
    // Edge cases
    // ------------------------------------------------------------------

    // Fence: add(0) and signal(0) are no-ops
    {
        Fence fence{};
        fence.add(0);
        TEST(fence.isComplete());
        fence.add();
        fence.signal(0);
        TEST(!fence.isComplete());
        fence.signal();
        TEST(fence.isComplete());
    }

    // Fence: signal without matching add is caught by assertion (tested in debug)
    // No explicit test — the assertion prevents silent underflow.

    // callPar with function modifying caller's local through pointer
    {
        Fence fence{};
        u32 result = 0;
        callPar(&fence, &result, [](void* p)
        {
            *static_cast<u32*>(p) = 42;
        });
        fence.wait(2.0);
        TEST(result == 42);
    }

    // forPar with contiguous array modification
    {
        static constexpr u64 count = 1000;
        u32 vals[count]{};
        forPar(u64{0}, u64{count}, vals, [](void* data, u64 idx)
        {
            static_cast<u32*>(data)[idx] = static_cast<u32>(idx + 1);
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i] == i + 1);
    }

    // forPar lambda with reference capture — modifying array
    {
        static constexpr u64 count = 500;
        u32 vals[count]{};
        forPar(u64{0}, u64{count}, [&](u64 idx)
        {
            vals[idx] = static_cast<u32>(idx * 2);
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i] == i * 2);
    }

    } // concurrencyIter

    // ============================================================================
    // cString
    // ============================================================================
    //
    // Creates a null-terminated C string from a StringView by allocating
    // from an arena.
    //
    // Functions covered:
    // - cString(Arena*, StringView)

    // Normal case
    {
        ArenaScope arena = getScratch();
        char* c = cString(arena, "hello");
        TEST(c != nullptr);
        TEST(c[0] == 'h');
        TEST(c[1] == 'e');
        TEST(c[2] == 'l');
        TEST(c[3] == 'l');
        TEST(c[4] == 'o');
        TEST(c[5] == '\0');
    }

    // Empty string
    {
        ArenaScope arena = getScratch();
        char* c = cString(arena, "");
        TEST(c != nullptr);
        TEST(c[0] == '\0');
    }

    // String with null data and zero length
    {
        ArenaScope arena = getScratch();
        StringView empty{};
        char* c = cString(arena, empty);
        TEST(c != nullptr);
        TEST(c[0] == '\0');
    }

    // String with data and length (non-null-terminated input)
    {
        ArenaScope arena = getScratch();
        StringView sv{"hello world", 5};
        char* c = cString(arena, sv);
        TEST(c != nullptr);
        TEST(c[0] == 'h');
        TEST(c[5] == '\0');
        TEST(StringView{c} == "hello");
    }

    // ============================================================================
    // StringBuilder
    // ============================================================================
    //
    // StringBuilder is an arena-allocated mutable string. It supports insert,
    // append, and prepend for both strings and individual characters.
    // StringBuilder converts implicitly to StringView for comparison.
    //
    // Functions covered:
    // - StringBuilder(Arena*, StringView) — construction
    // - StringBuilder() — default construction (empty)
    // - insert(u64 idx, StringView)
    // - insert(u64 idx, char)
    // - append(StringView)
    // - append(char)
    // - prepend(StringView)
    // - prepend(char)
    // - operator==(StringBuilder, StringBuilder)
    // - operator==(StringBuilder, StringView)

    // Default construction is empty
    {
        StringBuilder sb{};
        TEST(sb.chars == nullptr);
        TEST(sb.length == 0);
        TEST(sb.arena == nullptr);
    }

    // Construction from a string
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        TEST(sb.length == 5);
        TEST(sb == "hello");
    }

    // Construction with an empty string defaults to empty builder
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena};
        TEST(sb.length == 0);
        TEST(sb == "");
    }

    // Construction from a StringView
    {
        ArenaScope arena = getScratch();
        StringView sv{"world"};
        StringBuilder sb{arena, sv};
        TEST(sb == "world");
    }

    // Construction from partial StringView
    {
        ArenaScope arena = getScratch();
        StringView sv{"hello world", 5};
        StringBuilder sb{arena, sv};
        TEST(sb == "hello");
    }

    // Append a string to a builder that already has content
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        sb.append(" world");
        TEST(sb == "hello world");
    }

    // Append a string to an empty builder
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena};
        sb.append("hello");
        TEST(sb == "hello");
    }

    // Append a char
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        sb.append('!');
        TEST(sb == "hello!");
    }

    // Append a char to an empty builder
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena};
        sb.append('x');
        TEST(sb == "x");
    }

    // Append multiple times (triggers reallocation)
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "a"};
        sb.append("b");
        sb.append("c");
        sb.append("d");
        sb.append("e");
        TEST(sb == "abcde");
    }

    // Prepend a string
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "world"};
        sb.prepend("hello ");
        TEST(sb == "hello world");
    }

    // Prepend to an empty builder
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena};
        sb.prepend("hello");
        TEST(sb == "hello");
    }

    // Prepend a char
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "ello"};
        sb.prepend('h');
        TEST(sb == "hello");
    }

    // Insert at the beginning
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "world"};
        sb.insert(0, "hello ");
        TEST(sb == "hello world");
    }

    // Insert in the middle
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello world"};
        sb.insert(5, ",");
        TEST(sb == "hello, world");
    }

    // Insert at the end (same as append)
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        sb.insert(5, " world");
        TEST(sb == "hello world");
    }

    // Insert a char in the middle
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello world"};
        sb.insert(5, ',');
        TEST(sb == "hello, world");
    }

    // Insert with empty string (no-op)
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        sb.insert(3, "");
        TEST(sb == "hello");
    }

    // StringBuilder equality with another StringBuilder
    {
        ArenaScope arena = getScratch();
        StringBuilder a{arena, "hello"};
        StringBuilder b{arena, "hello"};
        StringBuilder c{arena, "world"};

        TEST(a == b);
        TEST(a != c);
    }

    // StringBuilder equality with StringView (via implicit conversion)
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        TEST(sb == StringView{"hello"});
    }

    // Index operator
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena, "hello"};
        TEST(sb[0] == 'h');
        TEST(sb[4] == 'o');
    }

    // Large string with many appends
    {
        ArenaScope arena = getScratch();
        StringBuilder sb{arena};
        for (u32 i = 0; i < 100; ++i)
            sb.append('x');
        TEST(sb.length == 100);
        for (u32 i = 0; i < 100; ++i)
            TEST(sb[i] == 'x');
    }

    // ============================================================================
    // String
    // ============================================================================
    //
    // String is a heap-allocated owning string (move-only). Created via
    // String::create().
    //
    // Functions covered:
    // - String::create(StringView)
    // - ~String()
    // - String(String&&) — move construct
    // - String& operator=(String&&) — move assign
    // - operator==(String, String)
    // - operator!=(String, String)
    // - operator StringView()
    // - operator[]

    // Create from a string literal
    {
        String s = String::create("hello");
        TEST(s.length == 5);
        TEST(s == "hello");
        TEST(s[0] == 'h');
        TEST(s[4] == 'o');
    }

    // Create from empty string
    {
        String s = String::create("");
        TEST(s.length == 0);
        TEST(s == "");
    }

    // Create from partial StringView
    {
        StringView sv{"hello world", 5};
        String s = String::create(sv);
        TEST(s == "hello");
    }

    // Move construct
    {
        String a = String::create("hello");
        String b{std::move(a)};
        TEST(a.chars == nullptr);
        TEST(a.length == 0);
        TEST(b == "hello");
    }

    // Move assign
    {
        String a = String::create("hello");
        String b = String::create("world");
        b = std::move(a);
        TEST(a.chars == nullptr);
        TEST(a.length == 0);
        TEST(b == "hello");
    }

    // String equality
    {
        String a = String::create("hello");
        String b = String::create("hello");
        String c = String::create("world");
        TEST(a == b);
        TEST(!(a == c));
        TEST(a != c);
    }

    // String equality with StringView (via implicit conversion)
    {
        String s = String::create("hello");
        TEST(s == StringView{"hello"});
    }

    // String equality with const char* (via implicit conversion to StringView)
    {
        String s = String::create("hello");
        TEST(s == "hello");
    }

    // ------------------------------------------------------------------
    // String destruction & lifecycle
    // ------------------------------------------------------------------
    //
    // ~String() calls heapFree(chars, length).  free(nullptr) is a no-op,
    // so moved-from Strings (chars == nullptr) are safe to destroy.
    // These tests verify no double-free, no leak, and correct ownership
    // transfer across every code path.

    // Create, move, destroy in order (ownership transfer)
    {
        String a = String::create("hello");
        {
            String b = std::move(a);
            TEST(a.chars == nullptr);
            TEST(b == "hello");
        }
        // b destroyed — frees "hello"
        TEST(a.chars == nullptr);
        // a destroyed — no-op (chars == nullptr)
    }

    // Chain of moves through multiple Strings
    {
        String a = String::create("alpha");
        String b = String::create("beta");
        String c = std::move(a);    // c owns "alpha", a is null
        b = std::move(c);           // b frees "beta", takes "alpha", c is null
        TEST(b == "alpha");
        TEST(a.chars == nullptr);
        TEST(c.chars == nullptr);
        // b destroyed — frees "alpha"
        // a,c destroyed — no-op
    }

    // Self-move-assignment
    {
        String a = String::create("hello");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        a = std::move(a);
#pragma clang diagnostic pop
        TEST(a == "hello");
    }

    // Empty String create and move
    {
        String empty = String::create("");
        TEST(empty == "");
        String moved = std::move(empty);
        TEST(empty.chars == nullptr);
        TEST(moved == "");
        // moved destroyed — frees (size 0 allocation if any)
        // empty destroyed — no-op
    }

    // Move-assign onto self after a prior move (edge case: two moved-from Strings)
    {
        String a = String::create("hello");
        String b = String::create("world");
        a = std::move(b);  // a frees "hello", takes "world", b is null
        TEST(a == "world");
        TEST(b.chars == nullptr);
        // Now assign b (moved-from) to a
        b = std::move(a);  // b is null, so ~String() on b is no-op; b takes "world", a is null
        TEST(b == "world");
        TEST(a.chars == nullptr);
        // b destroyed — frees "world"
        // a destroyed — no-op
    }

    // ============================================================================
    // isWhitespace / isNumeral
    // ============================================================================
    //
    // Character classification functions.
    //
    // Functions covered:
    // - isWhitespace(char)
    // - isNumeral(char)

    // isWhitespace: space
    {
        TEST(isWhitespace(' '));
    }

    // isWhitespace: tab
    {
        TEST(isWhitespace('\t'));
    }

    // isWhitespace: newline
    {
        TEST(isWhitespace('\n'));
    }

    // isWhitespace: carriage return
    {
        TEST(isWhitespace('\r'));
    }

    // isWhitespace: non-whitespace chars are false
    {
        TEST(!isWhitespace('a'));
        TEST(!isWhitespace('0'));
        TEST(!isWhitespace('.'));
        TEST(!isWhitespace('\0'));
        TEST(!isWhitespace('_'));
    }

    // isNumeral: digits 0-9
    {
        TEST(isNumeral('0'));
        TEST(isNumeral('1'));
        TEST(isNumeral('2'));
        TEST(isNumeral('3'));
        TEST(isNumeral('4'));
        TEST(isNumeral('5'));
        TEST(isNumeral('6'));
        TEST(isNumeral('7'));
        TEST(isNumeral('8'));
        TEST(isNumeral('9'));
    }

    // isNumeral: non-digits
    {
        TEST(!isNumeral('a'));
        TEST(!isNumeral('z'));
        TEST(!isNumeral('A'));
        TEST(!isNumeral('Z'));
        TEST(!isNumeral('.'));
        TEST(!isNumeral('-'));
        TEST(!isNumeral('+'));
        TEST(!isNumeral(' '));
        TEST(!isNumeral('\0'));
        TEST(!isNumeral('/'));  // before '0'
        TEST(!isNumeral(':'));  // after '9'
    }

    // ============================================================================
    // isInteger
    // ============================================================================
    //
    // Checks whether a string is a valid base-10 integer, optionally with
    // a leading + or - sign.
    //
    // Functions covered:
    // - isInteger(StringView)

    // Single digits
    {
        TEST(isInteger("0"));
        TEST(isInteger("1"));
        TEST(isInteger("2"));
        TEST(isInteger("3"));
        TEST(isInteger("4"));
        TEST(isInteger("5"));
        TEST(isInteger("6"));
        TEST(isInteger("7"));
        TEST(isInteger("8"));
        TEST(isInteger("9"));
    }

    // Multi-digit numbers
    {
        TEST(isInteger("42"));
        TEST(isInteger("100"));
        TEST(isInteger("1234567890"));
    }

    // With leading sign
    {
        TEST(isInteger("+12"));
        TEST(isInteger("-12"));
        TEST(isInteger("+0"));
        TEST(isInteger("-0"));
    }

    // Leading zeros
    {
        TEST(isInteger("00"));
        TEST(isInteger("00042"));
    }

    // Empty string
    {
        TEST(!isInteger(""));
    }

    // Non-numeric characters
    {
        TEST(!isInteger("hello"));
        TEST(!isInteger("12a"));
        TEST(!isInteger("a12"));
        TEST(!isInteger("1.0"));
        TEST(!isInteger("--12"));
        TEST(!isInteger("+-12"));
        TEST(!isInteger("12-"));
        TEST(!isInteger("12+"));
    }

    // Just a sign (no digits)
    {
        TEST(!isInteger("+"));
        TEST(!isInteger("-"));
    }

    // ============================================================================
    // isFloat
    // ============================================================================
    //
    // Checks whether a string is a valid base-10 floating point number,
    // optionally with decimal point, exponent (e), and trailing f suffix.
    //
    // Functions covered:
    // - isFloat(StringView)

    // Simple decimals
    {
        TEST(isFloat("0.0"));
        TEST(isFloat("1.0"));
        TEST(isFloat("2.5"));
        TEST(isFloat("99.99"));
    }

    // Leading decimal point
    {
        TEST(isFloat(".1"));
        TEST(isFloat(".5"));
        TEST(isFloat(".12345"));
    }

    // Trailing decimal point
    {
        TEST(isFloat("1."));
        TEST(isFloat("100."));
    }

    // With sign
    {
        TEST(isFloat("+1.0"));
        TEST(isFloat("-1.0"));
        TEST(isFloat("+.5"));
        TEST(isFloat("-.5"));
    }

    // With exponent
    {
        TEST(isFloat("1e3"));
        TEST(isFloat("1e+3"));
        TEST(isFloat("1e-3"));
        TEST(isFloat("1.5e3"));
        TEST(isFloat(".5e3"));
    }

    // With f suffix
    {
        TEST(isFloat("1.0f"));
        TEST(isFloat("+10.f"));
        TEST(isFloat("-999.999f"));
        TEST(isFloat("1e3f"));
        TEST(isFloat("1.e3f"));
        TEST(isFloat(".1e3"));
    }

    // Integer-only strings (no decimal or exponent) — isFloat returns true
    // if there's a decimal or exponent, false for plain integers
    {
        TEST(!isFloat("1"));
        TEST(!isFloat("42"));
        TEST(!isFloat("+12"));
        TEST(!isFloat("-12"));
    }

    // Empty string
    {
        TEST(!isFloat(""));
    }

    // Invalid strings
    {
        TEST(!isFloat("hello"));
        TEST(!isFloat("1.0ff"));
        TEST(!isFloat("1.0.0"));
        TEST(!isFloat("1e3.0"));
        TEST(!isFloat("--1.0"));
        TEST(!isFloat("1ef"));
        TEST(!isFloat("e1"));
    }

    // Just a decimal point
    {
        TEST(!isFloat("."));
    }

    // Just an exponent
    {
        TEST(!isFloat("e"));
        TEST(!isFloat("e1"));
    }

    // ============================================================================
    // stringToInteger
    // ============================================================================
    //
    // Parses a base-10 integer string into an i64. Asserts the input is
    // a valid integer (call isInteger first).
    //
    // Functions covered:
    // - stringToInteger(StringView)

    // Single digits
    {
        TEST(stringToInteger("0") == 0);
        TEST(stringToInteger("1") == 1);
        TEST(stringToInteger("2") == 2);
        TEST(stringToInteger("3") == 3);
        TEST(stringToInteger("4") == 4);
        TEST(stringToInteger("5") == 5);
        TEST(stringToInteger("6") == 6);
        TEST(stringToInteger("7") == 7);
        TEST(stringToInteger("8") == 8);
        TEST(stringToInteger("9") == 9);
    }

    // Multi-digit
    {
        TEST(stringToInteger("42") == 42);
        TEST(stringToInteger("100") == 100);
        TEST(stringToInteger("1234567890") == 1234567890);
    }

    // With sign
    {
        TEST(stringToInteger("+12") == 12);
        TEST(stringToInteger("-12") == -12);
        TEST(stringToInteger("+0") == 0);
        TEST(stringToInteger("-0") == 0);
    }

    // Leading zeros
    {
        TEST(stringToInteger("00") == 0);
        TEST(stringToInteger("00042") == 42);
    }

    // Large values
    {
        TEST(stringToInteger("2147483647") == 2147483647);   // i32 max
        TEST(stringToInteger("2147483648") == 2147483648);
    }

    // Negative large values
    {
        TEST(stringToInteger("-2147483648") == -2147483648);  // i32 min
    }

    // ============================================================================
    // stringToFloat
    // ============================================================================
    //
    // Parses a base-10 floating point string into an f64. Handles decimal
    // points, exponents, signs, and f suffix.
    //
    // Functions covered:
    // - stringToFloat(StringView)

    // Basic decimals
    {
        TEST(std::abs(stringToFloat("0.0") - 0.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("1.0") - 1.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("2.5") - 2.5) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("99.99") - 99.99) <= 1e-10);
    }

    // Leading decimal
    {
        TEST(std::abs(stringToFloat(".1") - 0.1) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat(".5") - 0.5) <= FLT_EPSILON);
    }

    // Trailing decimal
    {
        TEST(std::abs(stringToFloat("1.") - 1.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("100.") - 100.0) <= FLT_EPSILON);
    }

    // With sign
    {
        TEST(std::abs(stringToFloat("+1.0") - 1.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("-1.0") + 1.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("+.5") - 0.5) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("-.5") + 0.5) <= FLT_EPSILON);
    }

    // With exponent
    {
        TEST(std::abs(stringToFloat("1e3") - 1000.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("1e+3") - 1000.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("1e-3") - 0.001) <= 1e-10);
        TEST(std::abs(stringToFloat("1.5e3") - 1500.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat(".5e3") - 500.0) <= FLT_EPSILON);
    }

    // With f suffix
    {
        TEST(std::abs(stringToFloat("1.0f") - 1.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("+10.f") - 10.0) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("-999.999f") + 999.999) <= 1e-10);
        TEST(std::abs(stringToFloat("1e3f") - 1000.0) <= FLT_EPSILON);
    }

    // Zero
    {
        TEST(std::abs(stringToFloat("0.0")) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat(".0")) <= FLT_EPSILON);
        TEST(std::abs(stringToFloat("0.")) <= FLT_EPSILON);
    }

    // ============================================================================
    // integerToString
    // ============================================================================
    //
    // Formats an i64 into a base-10 string allocated from an arena.
    //
    // Functions covered:
    // - integerToString(Arena*, i64)

    // Zero
    {
        ArenaScope arena = getScratch();
        StringBuilder sb = integerToString(arena, 0);
        TEST(sb == "0");
    }

    // Positive single digit
    {
        ArenaScope arena = getScratch();
        TEST(integerToString(arena, 1) == "1");
        TEST(integerToString(arena, 9) == "9");
    }

    // Negative single digit
    {
        ArenaScope arena = getScratch();
        TEST(integerToString(arena, -1) == "-1");
        TEST(integerToString(arena, -9) == "-9");
    }

    // Multi-digit
    {
        ArenaScope arena = getScratch();
        TEST(integerToString(arena, 42) == "42");
        TEST(integerToString(arena, 100) == "100");
        TEST(integerToString(arena, 1234567890) == "1234567890");
    }

    // Negative multi-digit
    {
        ArenaScope arena = getScratch();
        TEST(integerToString(arena, -42) == "-42");
        TEST(integerToString(arena, -1000000) == "-1000000");
    }

    // Large values (within f64 exact-representation range to avoid
    // precision loss in the f64 division used by integerToString)
    {
        ArenaScope arena = getScratch();
        TEST(integerToString(arena, 9000000000000000LL) == "9000000000000000");
    }

    // ============================================================================
    // floatToString
    // ============================================================================
    //
    // Formats an f64 into a base-10 string with a specified number of
    // decimal places, allocated from an arena.
    //
    // Functions covered:
    // - floatToString(Arena*, f64, u32 decimalCount)

    // Zero
    {
        ArenaScope arena = getScratch();
        TEST(floatToString(arena, 0.0, 1) == "0.0");
    }

    // Positive values with varying decimal places
    {
        ArenaScope arena = getScratch();
        TEST(floatToString(arena, 1.0, 0) == "1.");
        TEST(floatToString(arena, 2.0, 1) == "2.0");
        TEST(floatToString(arena, 3.0, 2) == "3.00");
        TEST(floatToString(arena, 4.0, 3) == "4.000");
    }

    // Negative values
    {
        ArenaScope arena = getScratch();
        TEST(floatToString(arena, -1.0, 1) == "-1.0");
        TEST(floatToString(arena, -2.0, 2) == "-2.00");
    }

    // Fractional values
    {
        ArenaScope arena = getScratch();
        TEST(floatToString(arena, 0.5, 1) == "0.5");
        TEST(floatToString(arena, 3.14, 2) == "3.14");
        TEST(floatToString(arena, -0.5, 1) == "-0.5");
    }

    // Zero decimal places (zero case returns "0.0" regardless)
    {
        ArenaScope arena = getScratch();
        TEST(floatToString(arena, 0.0, 0) == "0.0");
        TEST(floatToString(arena, 100.0, 0) == "100.");
    }

    // ============================================================================
    // BinaryBuilder
    // ============================================================================
    //
    // BinaryBuilder is an arena-backed builder for binary data. Supports
    // resize, append, overwrite, read, and implicit BinaryView conversion.
    //
    // Functions covered:
    // - BinaryBuilder() — default
    // - BinaryBuilder(Arena*, u64) — arena + optional initial size
    // - operator BinaryView()
    // - read(u64, void*, u64)
    // - read<T>(u64)
    // - resize(u64)
    // - overwrite(u64, const void*, u64)
    // - overwrite<T>(u64, const T&)
    // - append(const void*, u64)
    // - append<T>(const T&)

    // Default-constructed builder has null arena
    {
        BinaryBuilder bb;
        TEST(bb.arena == nullptr);
        TEST(bb.data == nullptr);
        TEST(bb.size == 0);
    }

    // Create with arena and initial size
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 8};
        TEST(bb.arena == arena);
        TEST(bb.data != nullptr);
        TEST(bb.size == 8);
    }

    // Create with arena and zero size
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 0};
        TEST(bb.arena == arena);
        TEST(bb.data != nullptr); // alloc(0,1) returns a valid pointer
        TEST(bb.size == 0);
    }

    // resize grows the builder
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.resize(8);
        TEST(bb.size == 8);
    }

    // append raw data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u32 val = 42;
        bb.append(&val, sizeof(val));
        TEST(bb.size == sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 42);
    }

    // append<T> typed data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u32 val = 0xDEADBEEF;
        bb.append(val);
        TEST(bb.size == sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 0xDEADBEEF);
    }

    // overwrite existing data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        u32 val = 123;
        bb.overwrite(0, &val, sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 123);
    }

    // overwrite<T> typed data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.overwrite(0, static_cast<u32>(789));
        u32 result = bb.read<u32>(0);
        TEST(result == 789);
    }

    // Read raw data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.overwrite(0, static_cast<u32>(0xAABBCCDD));
        u32 result = 0;
        bb.read(0, &result, sizeof(result));
        TEST(result == 0xAABBCCDD);
    }

    // Implicit conversion to BinaryView
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        BinaryView bv = bb;
        TEST(bv.data == bb.data);
        TEST(bv.size == bb.size);
    }

    // Append multiple values
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u8 a = 0xAA;
        u8 b = 0xBB;
        bb.append(a);
        bb.append(b);
        TEST(bb.size == 2);
        u8 ra = bb.read<u8>(0);
        u8 rb = bb.read<u8>(1);
        TEST(ra == 0xAA);
        TEST(rb == 0xBB);
    }

    // ============================================================================
    // Binary
    // ============================================================================
    //
    // Binary is an owning, heap-allocated, move-only binary data block.
    //
    // Functions covered:
    // - Binary() — default
    // - Binary::create(BinaryView) — factory
    // - ~Binary() — destructor
    // - Binary(Binary&&) — move construct
    // - Binary& operator=(Binary&&) — move assign
    // - read(u64, void*, u64)
    // - read<T>(u64)
    // - operator BinaryView()

    // Default-constructed Binary is empty
    {
        Binary b;
        TEST(b.data == nullptr);
        TEST(b.size == 0);
    }

    // Create from BinaryView
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        TEST(b.size == sizeof(val));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Create from empty BinaryView
    {
        BinaryView bv{};
        Binary b = Binary::create(bv);
        TEST(b.size == 0);
        // data may be non-null (heapAlloc(0,1) returns valid pointer)
    }

    // Move construct
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        Binary a = Binary::create(bv);
        Binary b = std::move(a);
        TEST(a.data == nullptr);
        TEST(a.size == 0);
        TEST(b.size == sizeof(val));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Move assign
    {
        u32 val1 = 42;
        u32 val2 = 99;
        BinaryView bv1{&val1, sizeof(val1)};
        BinaryView bv2{&val2, sizeof(val2)};
        Binary a = Binary::create(bv1);
        Binary b = Binary::create(bv2);
        b = std::move(a);
        TEST(a.data == nullptr);
        TEST(a.size == 0);
        TEST(b.size == sizeof(val1));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Implicit conversion to BinaryView
    {
        u32 val = 0xCAFEBABE;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        BinaryView bv2 = b;
        TEST(bv2.size == b.size);
        u32 result = bv2.read<u32>(0);
        TEST(result == 0xCAFEBABE);
    }

    // Create, scope-exit destroys (no double-free crash)
    {
        u32 val = 12345;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        TEST(b.read<u32>(0) == 12345);
    }
    // Allocate after — if heap is corrupt, we crash
    {
        Binary b2 = Binary::create(BinaryView{});
        TEST(b2.size == 0);
    }

//     // Arena
//     {
//         void* block = malloc(1024);
//         HG_DEFER(free(block));
//
//         Arena arena{block, 1024, 0};
//
//         for (u32 i = 0; i < 3; ++i)
//         {
//             TEST(arena.memory != nullptr);
//             TEST(arena.capacity == 1024);
//             TEST(arena.head == 0);
//
//             u32* allocU32 = arena.alloc<u32>(1);
//             TEST(allocU32 == arena.memory);
//
//             u64* allocU64 = arena.alloc<u64>(2);
//             TEST(reinterpret_cast<u8*>(allocU64) == reinterpret_cast<u8*>(allocU32) + 8);
//
//             u8* allocU8 = arena.alloc<u8>(1);
//             TEST(allocU8 == reinterpret_cast<u8*>(allocU32) + 24);
//
//             struct Big {
//                 u8 data[32];
//             };
//             Big* allocBig = arena.alloc<Big>(1);
//             TEST(reinterpret_cast<u8*>(allocBig) == reinterpret_cast<u8*>(allocU32) + 25);
//
//             Big* reallocBig = arena.realloc(allocBig, 1, 2);
//             TEST(reallocBig == allocBig);
//
//             Big* reallocBigSame = arena.realloc(reallocBig, 2, 2);
//             TEST(reallocBigSame == reallocBig);
//
//             memset(reallocBig, 2, 2 * sizeof(*reallocBig));
//             u8* allocInterrupt = arena.alloc<u8>(1);
//             static_cast<void>(allocInterrupt);
//
//             Big* reallocBig2 = arena.realloc(reallocBig, 2, 4);
//             TEST(reallocBig2 != reallocBig);
//             TEST(memcmp(reallocBig, reallocBig2, 2 * sizeof(*reallocBig)) == 0);
//
//             arena.head = 0;
//         }
//     }
//
//     // StringBuilder
//     {
//         ArenaScope arena = getScratch();
//
//         StringBuilder a{arena, "a"};
//         TEST(a[0] == 'a');
//         TEST(a.length == 1);
//
//         StringBuilder abc{arena, "abc"};
//         TEST(abc[0] == 'a');
//         TEST(abc[1] == 'b');
//         TEST(abc[2] == 'c');
//         TEST(abc.length == 3);
//
//         a.append("bc");
//         TEST(a == abc);
//
//         StringBuilder str{};
//
//         str.append("hello");
//         TEST(str == "hello");
//
//         str.append(" there");
//         TEST(str == "hello there");
//
//         str.prepend("why ");
//         TEST(str == "why hello there");
//
//         str.insert(3, ",");
//         TEST(str == "why, hello there");
//
//         str.prepend("aaaaaaaaaaaaaaaaaaaaaaaa ");
//         TEST(str == "aaaaaaaaaaaaaaaaaaaaaaaa why, hello there");
//     }
//
//     // string utils
//     {
//         ArenaScope arena = getScratch();
//
//         TEST(isWhitespace(' '));
//         TEST(isWhitespace('\t'));
//         TEST(isWhitespace('\n'));
//
//         TEST(isNumeral('0'));
//         TEST(isNumeral('1'));
//         TEST(isNumeral('2'));
//         TEST(isNumeral('3'));
//         TEST(isNumeral('4'));
//         TEST(isNumeral('5'));
//         TEST(isNumeral('5'));
//         TEST(isNumeral('6'));
//         TEST(isNumeral('7'));
//         TEST(isNumeral('8'));
//         TEST(isNumeral('9'));
//
//         TEST(!isNumeral('0' - 1));
//         TEST(!isNumeral('9' + 1));
//
//         TEST(!isNumeral('x'));
//         TEST(!isNumeral('a'));
//         TEST(!isNumeral('b'));
//         TEST(!isNumeral('c'));
//         TEST(!isNumeral('d'));
//         TEST(!isNumeral('e'));
//         TEST(!isNumeral('f'));
//         TEST(!isNumeral('X'));
//         TEST(!isNumeral('A'));
//         TEST(!isNumeral('B'));
//         TEST(!isNumeral('C'));
//         TEST(!isNumeral('D'));
//         TEST(!isNumeral('E'));
//         TEST(!isNumeral('F'));
//
//         TEST(!isNumeral('.'));
//         TEST(!isNumeral('+'));
//         TEST(!isNumeral('-'));
//         TEST(!isNumeral('*'));
//         TEST(!isNumeral('/'));
//         TEST(!isNumeral('='));
//         TEST(!isNumeral('#'));
//         TEST(!isNumeral('&'));
//         TEST(!isNumeral('^'));
//         TEST(!isNumeral('~'));
//
//         TEST(isInteger("0"));
//         TEST(isInteger("1"));
//         TEST(isInteger("2"));
//         TEST(isInteger("3"));
//         TEST(isInteger("4"));
//         TEST(isInteger("5"));
//         TEST(isInteger("6"));
//         TEST(isInteger("7"));
//         TEST(isInteger("8"));
//         TEST(isInteger("9"));
//         TEST(isInteger("10"));
//
//         TEST(isInteger("12"));
//         TEST(isInteger("42"));
//         TEST(isInteger("100"));
//         TEST(isInteger("123456789"));
//         TEST(isInteger("-12"));
//         TEST(isInteger("-42"));
//         TEST(isInteger("-100"));
//         TEST(isInteger("-123456789"));
//         TEST(isInteger("+12"));
//         TEST(isInteger("+42"));
//         TEST(isInteger("+100"));
//         TEST(isInteger("+123456789"));
//
//         TEST(!isInteger("hello"));
//         TEST(!isInteger("not a number"));
//         TEST(!isInteger("number"));
//         TEST(!isInteger("integer"));
//         TEST(!isInteger("0.0"));
//         TEST(!isInteger("1.0"));
//         TEST(!isInteger(".10"));
//         TEST(!isInteger("1e2"));
//         TEST(!isInteger("1f"));
//         TEST(!isInteger("0xff"));
//         TEST(!isInteger("--42"));
//         TEST(!isInteger("++42"));
//         TEST(!isInteger("42-"));
//         TEST(!isInteger("42+"));
//         TEST(!isInteger("4 2"));
//         TEST(!isInteger("4+2"));
//
//         TEST(isFloat("0.0"));
//         TEST(isFloat("1."));
//         TEST(isFloat("2.0"));
//         TEST(isFloat("3."));
//         TEST(isFloat("4.0"));
//         TEST(isFloat("5."));
//         TEST(isFloat("6.0"));
//         TEST(isFloat("7."));
//         TEST(isFloat("8.0"));
//         TEST(isFloat("9."));
//         TEST(isFloat("10.0"));
//
//         TEST(isFloat("0.0"));
//         TEST(isFloat(".1"));
//         TEST(isFloat("0.2"));
//         TEST(isFloat(".3"));
//         TEST(isFloat("0.4"));
//         TEST(isFloat(".5"));
//         TEST(isFloat("0.6"));
//         TEST(isFloat(".7"));
//         TEST(isFloat("0.8"));
//         TEST(isFloat(".9"));
//         TEST(isFloat("0.10"));
//
//         TEST(isFloat("1.0"));
//         TEST(isFloat("+10.f"));
//         TEST(isFloat(".10"));
//         TEST(isFloat("-999.999f"));
//         TEST(isFloat("1e3"));
//         TEST(isFloat("1e3"));
//         TEST(isFloat("+1.e3f"));
//         TEST(isFloat(".1e3"));
//
//         TEST(!isFloat("hello"));
//         TEST(!isFloat("not a number"));
//         TEST(!isFloat("number"));
//         TEST(!isFloat("float"));
//         TEST(!isFloat("1.0ff"));
//         TEST(!isFloat("0x1.0"));
//         TEST(!isFloat("-0x1.0"));
//
//         TEST(stringToInteger("0") == 0);
//         TEST(stringToInteger("1") == 1);
//         TEST(stringToInteger("2") == 2);
//         TEST(stringToInteger("3") == 3);
//         TEST(stringToInteger("4") == 4);
//         TEST(stringToInteger("5") == 5);
//         TEST(stringToInteger("6") == 6);
//         TEST(stringToInteger("7") == 7);
//         TEST(stringToInteger("8") == 8);
//         TEST(stringToInteger("9") == 9);
//
//         TEST(stringToInteger("0000000") == 0);
//         TEST(stringToInteger("+0000001") == +1);
//         TEST(stringToInteger("0000002") == 2);
//         TEST(stringToInteger("-0000003") == -3);
//         TEST(stringToInteger("0000004") == 4);
//         TEST(stringToInteger("+0000005") == +5);
//         TEST(stringToInteger("0000006") == 6);
//         TEST(stringToInteger("-0000007") == -7);
//         TEST(stringToInteger("0000008") == 8);
//         TEST(stringToInteger("+0000009") == +9);
//
//         TEST(stringToInteger("0000000") == 0);
//         TEST(stringToInteger("1000000") == 1000000);
//         TEST(stringToInteger("2000000") == 2000000);
//         TEST(stringToInteger("3000000") == 3000000);
//         TEST(stringToInteger("4000000") == 4000000);
//         TEST(stringToInteger("5000000") == 5000000);
//         TEST(stringToInteger("6000000") == 6000000);
//         TEST(stringToInteger("7000000") == 7000000);
//         TEST(stringToInteger("8000000") == 8000000);
//         TEST(stringToInteger("9000000") == 9000000);
//         TEST(stringToInteger("1234567890") == 1234567890);
//
//         TEST(stringToFloat("0.0") == 0.0);
//         TEST(stringToFloat("1.0f") == 1.0);
//         TEST(stringToFloat("2.0") == 2.0);
//         TEST(stringToFloat("3.0f") == 3.0);
//         TEST(stringToFloat("4.0") == 4.0);
//         TEST(stringToFloat("5.0f") == 5.0);
//         TEST(stringToFloat("6.0") == 6.0);
//         TEST(stringToFloat("7.0f") == 7.0);
//         TEST(stringToFloat("8.0") == 8.0);
//         TEST(stringToFloat("9.0f") == 9.0);
//
//         TEST(stringToFloat("0e1") == 0.0);
//         TEST(stringToFloat("1e2f") == 1e2);
//         TEST(stringToFloat("2e3") == 2e3);
//         TEST(stringToFloat("3e4f") == 3e4);
//         TEST(stringToFloat("4e5") == 4e5);
//         TEST(stringToFloat("5e6f") == 5e6);
//         TEST(stringToFloat("6e7") == 6e7);
//         TEST(stringToFloat("7e8f") == 7e8);
//         TEST(stringToFloat("8e9") == 8e9);
//         TEST(stringToFloat("9e10f") == 9e10);
//
//         TEST(stringToFloat("0e1") == 0.0);
//         TEST(stringToFloat("1e2f") == 1e2);
//         TEST(stringToFloat("2e3") == 2e3);
//         TEST(stringToFloat("3e4f") == 3e4);
//         TEST(stringToFloat("4e5") == 4e5);
//         TEST(stringToFloat("5e6f") == 5e6);
//         TEST(stringToFloat("6e7") == 6e7);
//         TEST(stringToFloat("7e8f") == 7e8);
//         TEST(stringToFloat("8e9") == 8e9);
//         TEST(stringToFloat("9e10f") == 9e10);
//
//         TEST(stringToFloat(".1") == .1);
//         TEST(stringToFloat("+.1") == +.1);
//         TEST(stringToFloat("-.1") == -.1);
//         TEST(stringToFloat("+.1e5") == +.1e5);
//
//         TEST(integerToString(arena, 0) == "0");
//         TEST(integerToString(arena, -1) == "-1");
//         TEST(integerToString(arena, 2) == "2");
//         TEST(integerToString(arena, -3) == "-3");
//         TEST(integerToString(arena, 4) == "4");
//         TEST(integerToString(arena, -5) == "-5");
//         TEST(integerToString(arena, 6) == "6");
//         TEST(integerToString(arena, -7) == "-7");
//         TEST(integerToString(arena, 8) == "8");
//         TEST(integerToString(arena, -9) == "-9");
//
//         TEST(integerToString(arena, 0000000) == "0");
//         TEST(integerToString(arena, -1000000) == "-1000000");
//         TEST(integerToString(arena, 2000000) == "2000000");
//         TEST(integerToString(arena, -3000000) == "-3000000");
//         TEST(integerToString(arena, 4000000) == "4000000");
//         TEST(integerToString(arena, -5000000) == "-5000000");
//         TEST(integerToString(arena, 6000000) == "6000000");
//         TEST(integerToString(arena, -7000000) == "-7000000");
//         TEST(integerToString(arena, 8000000) == "8000000");
//         TEST(integerToString(arena, -9000000) == "-9000000");
//         TEST(integerToString(arena, 1234567890) == "1234567890");
//
//         TEST(floatToString(arena, 0.0, 10) == "0.0");
//         TEST(floatToString(arena, -1.0f, 1) == "-1.0");
//         TEST(floatToString(arena, 2.0, 2) == "2.00");
//         TEST(floatToString(arena, -3.0f, 3) == "-3.000");
//         TEST(floatToString(arena, 4.0, 4) == "4.0000");
//         TEST(floatToString(arena, -5.0f, 5) == "-5.00000");
//         TEST(floatToString(arena, 6.0, 6) == "6.000000");
//         TEST(floatToString(arena, -7.0f, 7) == "-7.0000000");
//         TEST(floatToString(arena, 8.0, 8) == "8.00000000");
//         TEST(floatToString(arena, -9.0f, 9) == "-9.000000000");
//
//         TEST(floatToString(arena, 0e0, 1) == "0.0");
//         TEST(floatToString(arena, -1e1f, 0) == "-10.");
//         TEST(floatToString(arena, 2e2, 1) == "200.0");
//         TEST(floatToString(arena, -3e3f, 0) == "-3000.");
//         TEST(floatToString(arena, 4e4, 1) == "40000.0");
//         TEST(floatToString(arena, -5e5f, 0) == "-500000.");
//         TEST(floatToString(arena, 6e6, 1) == "6000000.0");
//         TEST(floatToString(arena, -7e7f, 0) == "-70000000.");
//         TEST(floatToString(arena, 8e8, 1) == "800000000.0");
//         TEST(floatToString(arena, -9e9f, 0) == "-8999999488.");
//
//         TEST(floatToString(arena, -0e-0, 3) == "0.0");
//         TEST(floatToString(arena, 1e-1f, 3) == "0.100");
//         TEST(floatToString(arena, -2e-2, 3) == "-0.020");
//         TEST(floatToString(arena, 3e-3f, 3) == "0.003");
//         TEST(floatToString(arena, -4e-0, 3) == "-4.000");
//         TEST(floatToString(arena, 5e-1f, 3) == "0.500");
//         TEST(floatToString(arena, -6e-2, 3) == "-0.060");
//         TEST(floatToString(arena, 7e-3f, 3) == "0.007");
//         TEST(floatToString(arena, -8e-0, 3) == "-8.000");
//         TEST(floatToString(arena, 9e-1f, 3) == "0.899");
//     }
//
//     // thread pool
//     {
//         {
//             Fence* fence = fenceCreate();
//             HG_DEFER(fenceDestroy(fence));
//
//             bool a = false;
//             bool b = false;
//
//             callPar(fence, &a, [](void *pa)
//             {
//                 *static_cast<bool*>(pa) = true;
//             });
//             callPar(fence, &b, [](void *pb)
//             {
//                 *static_cast<bool*>(pb) = true;
//             });
//
//             fenceWait(fence, 2.0);
//
//             TEST(fenceWait(fence, 2.0));
//
//             TEST(a == true);
//             TEST(b == true);
//         }
//
//         {
//             Fence* fence = fenceCreate();
//             HG_DEFER(fenceDestroy(fence));
//
//             bool vals[100]{};
//             for (bool& val : vals)
//             {
//                 callPar(fence, &val, [](void* data)
//                 {
//                     *static_cast<bool*>(data) = true;
//                 });
//             }
//
//             TEST(helpThreads(fence, 2.0));
//
//             for (bool& val : vals)
//             {
//                 TEST(val == true);
//             }
//         }
//
//         {
//             bool vals[100]{};
//
//             auto fn = [](void* pvals, u64 idx)
//             {
//                 (static_cast<bool*>(pvals))[idx] = true;
//             };
//             forPar(0, std::size(vals), vals, fn);
//
//             for (bool& val : vals)
//             {
//                 TEST(val == true);
//             }
//         }
//
//         {
//             Fence* fence = fenceCreate();
//             HG_DEFER(fenceDestroy(fence));
//
//             for (u32 n = 0; n < 3; ++n)
//             {
//                 std::atomic_bool start{false};
//                 std::thread producers[4];
//
//                 bool vals[100]{};
//
//                 auto fn = [](void* pval)
//                 {
//                     *static_cast<bool*>(pval) = !*static_cast<bool*>(pval);
//                 };
//
//                 auto prodFn = [&](u32 idx)
//                 {
//                     while (!start)
//                     {
//                         _mm_pause();
//                     }
//                     u32 begin = idx * 25;
//                     u32 end = begin + 25;
//                     for (u32 i = begin; i < end; ++i)
//                     {
//                         callPar(fence, vals + i, fn);
//                     }
//                 };
//                 for (u32 j = 0; j < std::size(producers); ++j)
//                 {
//                     producers[j] = std::thread(prodFn, j);
//                 }
//
//                 start.store(true);
//                 for (auto& thread : producers)
//                 {
//                     thread.join();
//                 }
//
//                 TEST(helpThreads(fence, 2.0));
//                 for (auto val : vals)
//                 {
//                     TEST(val == true);
//                 }
//             }
//         }
//     }
//
//     // Mutex
//     {
//         struct Capture {
//             Spinlock* mtx;
//             u32 count;
//         };
//         Capture c{};
//
//         c.mtx = mutexCreate();
//         HG_DEFER(mutexDestroy(c.mtx));
//
//         c.count = 0;
//         forPar(0, 100, &c, [](void* pc, u64)
//         {
//             Capture* c = static_cast<Capture*>(pc);
//             mutexAcquire(c->mtx);
//             HG_DEFER(mutexRelease(c->mtx));
//             for (u32 i = 0; i < 10000; ++i)
//             {
//                 ++c->count;
//             }
//         });
//         TEST(c.count == 1000000);
//     }
//
//     // Serialization
//     {
//         ArenaScope arena = getScratch();
//
//         struct PlainOldData {
//             i64 a;
//             u16 b;
//             f32 c;
//             bool d;
//             u32 e[3];
//         };
//
//         PlainOldData pod{};
//         pod.a = -12,
//         pod.b = 42,
//         pod.c = 2.5f,
//         pod.d = true,
//         pod.e[0] = {2};
//         pod.e[1] = {4};
//         pod.e[2] = {6};
//
//         {
//             Serializer writer = serialWriter(arena);
//             serialize(&writer, &pod);
//
//             PlainOldData podCopy{};
//
//             Serializer reader = serialReader(arena, writer.current);
//             serialize(&reader, &podCopy);
//
//             TEST(memcmp(&podCopy, &pod, sizeof(pod)) == 0);
//         }
//
//         {
//             Serializer writer = serialWriter(arena);
//             serialize(&writer, &pod);
//
//             BinaryView bin = binaryWriteSerial(arena, &writer);
//
//             PlainOldData podCopy{};
//
//             Serializer reader = binaryReadSerial(arena, bin);
//             serialize(&reader, &podCopy);
//
//             TEST(memcmp(&podCopy, &pod, sizeof(pod)) == 0);
//         }
//
//         // {
//         //     Serializer writer = serialWriter(arena);
//         //     serialize(arena, &writer, "data", &pod);
//         //
//         //     StringView json = jsonWriteSerial(arena, writer);
//         //
//         //     PlainOldData podCopy{};
//         //
//         //     Serializer reader = jsonReadSerial(arena, json);
//         //     serialize(arena, &reader, "data", &podCopy);
//         //
//         //     TEST(memEqual(&podCopy, &pod, sizeof(pod)));
//         // }
//
//         struct Data {
//             i64 a;
//             u16 b;
//             f32 c;
//             bool d;
//             u32 e[3];
//             String f;
//         };
//
//         Data data{};
//         data.a = -12,
//         data.b = 42,
//         data.c = 2.5f,
//         data.d = true,
//         data.e[0] = {2};
//         data.e[1] = {4};
//         data.e[2] = {6};
//         data.f = String::create("hello");
//
//         auto serializeData = [](Serializer* s, Data* val)
//         {
//             serializeObject(s,
//                 &val->a,
//                 &val->b,
//                 &val->c,
//                 &val->d,
//                 &val->e,
//                 &val->f);
//         };
//
//         {
//             Serializer writer = serialWriter(arena);
//             serializeData(&writer, &data);
//
//             // StringView json = jsonWriteSerial(arena, &writer);
//             // debug("json: %.*s\n", (int)json.length, json.chars);
//
//             Data dataCopy{};
//
//             Serializer reader = serialReader(arena, writer.current);
//             serializeData(&reader, &dataCopy);
//
//             TEST(memcmp(&dataCopy, &data, sizeof(data)) != 0);
//             TEST(data.a == dataCopy.a);
//             TEST(data.b == dataCopy.b);
//             TEST(data.c == dataCopy.c);
//             TEST(data.d == dataCopy.d);
//             TEST(data.e[0] == dataCopy.e[0]);
//             TEST(data.e[1] == dataCopy.e[1]);
//             TEST(data.e[2] == dataCopy.e[2]);
//             TEST(data.f == dataCopy.f);
//         }
//
//         {
//             Serializer writer = serialWriter(arena);
//             serializeData(&writer, &data);
//
//             BinaryView bin = binaryWriteSerial(arena, &writer);
//
//             Data dataCopy{};
//
//             Serializer reader = binaryReadSerial(arena, bin);
//             serializeData(&reader, &dataCopy);
//
//             TEST(memcmp(&dataCopy, &data, sizeof(data)) != 0);
//             TEST(data.a == dataCopy.a);
//             TEST(data.b == dataCopy.b);
//             TEST(data.c == dataCopy.c);
//             TEST(data.d == dataCopy.d);
//             TEST(data.e[0] == dataCopy.e[0]);
//             TEST(data.e[1] == dataCopy.e[1]);
//             TEST(data.e[2] == dataCopy.e[2]);
//             TEST(data.f == dataCopy.f);
//         }
//
// //         {
// //             Serializer writer = serialWriter(arena);
// //             serializeData(&writer, &data);
// //
// //             StringView json = jsonWriteSerial(arena, &writer);
// //
// //             debug("json: %.*s\n", (int)json.length, json.chars);
// //             TEST(json ==
// // R"({
// //     "a" : -12,
// //     "b" : 42,
// //     "c" : 2.500000,
// //     "d" : true,
// //     "e" : [
// //         2,
// //         4,
// //         6
// //     ],
// //     "f" : "hello"
// // }
// // )");
// //
// //             Data dataCopy{};
// //
// //             Serializer reader = jsonReadSerial(arena, json);
// //             serializeData(arena, &reader, "data", &dataCopy);
// //
// //             TEST(!memEqual(&dataCopy, &data, sizeof(data)));
// //             TEST(data.a == dataCopy.a);
// //             TEST(data.b == dataCopy.b);
// //             TEST(data.c == dataCopy.c);
// //             TEST(data.d == dataCopy.d);
// //             TEST(data.e[0] == dataCopy.e[0]);
// //             TEST(data.e[1] == dataCopy.e[1]);
// //             TEST(data.e[2] == dataCopy.e[2]);
// //             TEST(data.f == dataCopy.f);
// //         }
//     }
//
//     // Json
//     {
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     1234
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors != nullptr);
//             TEST(json.file != nullptr);
//
//             JsonError* error = json.errors;
//             TEST(error->next == nullptr);
//             TEST(error->msg == "on line 4, struct has a literal instead of a field\n");
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf"
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors != nullptr);
//             TEST(json.file != nullptr);
//
//             JsonError* error = json.errors;
//             TEST(error->next == nullptr);
//             TEST(error->msg == "on line 4, struct has a literal instead of a field\n");
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf":
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors != nullptr);
//             TEST(json.file != nullptr);
//
//             JsonError* error = json.errors;
//             TEST(error->next != nullptr);
//             TEST(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
//             error = error->next;
//             TEST(error->next == nullptr);
//             TEST(error->msg == "on line 4, found unexpected token \"}\"\n");
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": true
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_bool);
//             TEST(field->value->boolean == true);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": false
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_bool);
//             TEST(field->value->boolean == false);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": asdf
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors != nullptr);
//             TEST(json.file != nullptr);
//
//             JsonError* error = json.errors;
//             TEST(error->next != nullptr);
//             TEST(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
//             error = error->next;
//             TEST(error->next == nullptr);
//             TEST(error->msg == "on line 3, found unexpected token \"asdf\"\n");
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields == nullptr);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": "asdf"
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_string);
//             TEST(field->value->string == "asdf");
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": 1234
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_integer);
//             TEST(field->value->integer == 1234);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": 1234.0
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_float);
//             TEST(field->value->floating == 1234.0);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": 1234.0,
//                     "hjkl": 5678.0
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next != nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_float);
//             TEST(field->value->floating == 1234.0);
//
//             field = field->next;
//             TEST(field->next == nullptr);
//             TEST(field->name == "hjkl");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_float);
//             TEST(field->value->floating == 5678.0);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": [1, 2, 3, 4]
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             JsonElem* elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 1);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 2);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 3);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 4);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": [1 2 3 4]
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             JsonElem* elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 1);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 2);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 3);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 4);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": [1, 2, "3", 4]
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors != nullptr);
//             TEST(json.file != nullptr);
//
//             JsonError* error = json.errors;
//             TEST(error->next == nullptr);
//             TEST(error->msg ==
//                 "on line 3, array has element which is not the same type as the first valid element\n");
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             JsonElem* elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 1);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 2);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_integer);
//             TEST(elem->value->integer == 4);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "asdf": {
//                         "a": 1,
//                         "s": 2.0,
//                         "d": 3,
//                         "f": 4.0,
//                     }
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* node = json.file;
//             TEST(node->type == JsonType_struct);
//             TEST(node->jstruct.fields != nullptr);
//
//             JsonField* field = node->jstruct.fields;
//             TEST(field->next == nullptr);
//             TEST(field->name == "asdf");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_struct);
//             TEST(field->value->array.elems != nullptr);
//
//             JsonField* subField = field->value->jstruct.fields;
//             TEST(subField->next != nullptr);
//             TEST(subField->name == "a");
//             TEST(subField->value != nullptr);
//             TEST(subField->value->type == JsonType_integer);
//             TEST(subField->value->integer == 1);
//
//             subField = subField->next;
//             TEST(subField->next != nullptr);
//             TEST(subField->name == "s");
//             TEST(subField->value != nullptr);
//             TEST(subField->value->type == JsonType_float);
//             TEST(subField->value->floating == 2.0);
//
//             subField = subField->next;
//             TEST(subField->next != nullptr);
//             TEST(subField->name == "d");
//             TEST(subField->value != nullptr);
//             TEST(subField->value->type == JsonType_integer);
//             TEST(subField->value->integer == 3);
//
//             subField = subField->next;
//             TEST(subField->next == nullptr);
//             TEST(subField->name == "f");
//             TEST(subField->value != nullptr);
//             TEST(subField->value->type == JsonType_float);
//             TEST(subField->value->floating == 4.0);
//         }
//
//         {
//             ArenaScope arena = getScratch();
//
//             StringView file = R"(
//                 {
//                     "player": {
//                         "transform": {
//                             "position": [1.0, 0.0, -1.0],
//                             "scale": [1.0, 1.0, 1.0],
//                             "rotation": [1.0, 0.0, 0.0, 0.0]
//                         },
//                         "sprite": {
//                             "texture": "tex.png",
//                             "uvPos": [0.0, 0.0],
//                             "uvSize": [1.0, 1.0]
//                         }
//                     }
//                 }
//             )";
//
//             Json json = parseJson(arena, file);
//
//             TEST(json.errors == nullptr);
//             TEST(json.file != nullptr);
//
//             JsonNode* mainStruct = json.file;
//             TEST(mainStruct->type == JsonType_struct);
//             TEST(mainStruct->jstruct.fields != nullptr);
//
//             JsonField* player = mainStruct->jstruct.fields;
//             TEST(player->next == nullptr);
//             TEST(player->name == "player");
//             TEST(player->value != nullptr);
//             TEST(player->value->type == JsonType_struct);
//             TEST(player->value->jstruct.fields != nullptr);
//
//             JsonField* component = player->value->jstruct.fields;
//             TEST(component->next != nullptr);
//             TEST(component->name == "transform");
//             TEST(component->value != nullptr);
//             TEST(component->value->type == JsonType_struct);
//             TEST(component->value->jstruct.fields != nullptr);
//
//             JsonField* field = component->value->jstruct.fields;
//             TEST(field->next != nullptr);
//             TEST(field->name == "position");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             JsonElem* elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == -1.0);
//
//             field = field->next;
//             TEST(field->next != nullptr);
//             TEST(field->name == "scale");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             field = field->next;
//             TEST(field->next == nullptr);
//             TEST(field->name == "rotation");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             elem = elem->next;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             component = component->next;
//             TEST(component->next == nullptr);
//             TEST(component->name == "sprite");
//             TEST(component->value != nullptr);
//             TEST(component->value->type == JsonType_struct);
//             TEST(component->value->jstruct.fields != nullptr);
//
//             field = component->value->jstruct.fields;
//             TEST(field->next != nullptr);
//             TEST(field->name == "texture");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_string);
//             TEST(field->value->string == "tex.png");
//
//             field = field->next;
//             TEST(field->next != nullptr);
//             TEST(field->name == "uvPos");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 0.0);
//
//             field = field->next;
//             TEST(field->next == nullptr);
//             TEST(field->name == "uvSize");
//             TEST(field->value != nullptr);
//             TEST(field->value->type == JsonType_array);
//             TEST(field->value->array.elems != nullptr);
//
//             elem = field->value->array.elems;
//             TEST(elem->next != nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//
//             elem = elem->next;
//             TEST(elem->next == nullptr);
//             TEST(elem->value != nullptr);
//             TEST(elem->value->type == JsonType_float);
//             TEST(elem->value->floating == 1.0);
//         }
//     }
//
//     // // Array
//     // {
//     //     Array<u32> arr = arrayCreate<u32>(0, 2);
//     //     HG_DEFER(arrayDestroy(&arr));
//     //
//     //     TEST(arr.count == 0);
//     //     TEST(arr.capacity >= 2);
//     //
//     //     *arrayPush(&arr) = 10;
//     //     *arrayPush(&arr) = 20;
//     //
//     //     TEST(arr.count == 2);
//     //     TEST(arr[0] == 10);
//     //     TEST(arr[1] == 20);
//     //
//     //     arrayResize(&arr, 4);
//     //
//     //     TEST(arr.count == 4);
//     //
//     //     arr[2] = 30;
//     //     arr[3] = 40;
//     //
//     //     TEST(arr[2] == 30);
//     //     TEST(arr[3] == 40);
//     //
//     //     u32 popped = arrayPop(&arr);
//     //
//     //     TEST(popped == 40);
//     //     TEST(arr.count == 3);
//     //
//     //     arrayResize(&arr, 1);
//     //
//     //     TEST(arr.count == 1);
//     //     TEST(arr[0] == 10);
//     //
//     //     ArenaScope arena = getScratch();
//     //
//     //     Array<u32> temp = arrayTemp<u32>(arena, 0, 4);
//     //
//     //     *arrayPushTemp(arena, &temp) = 123;
//     //     *arrayPushTemp(arena, &temp) = 456;
//     //
//     //     TEST(temp.count == 2);
//     //     TEST(temp[0] == 123);
//     //     TEST(temp[1] == 456);
//     // }
//
//     // ArrayAny
//     {
//         ArrayAny arr = arrayAnyCreate(sizeof(u32), alignof(u32), 0, 2);
//         HG_DEFER(arrayAnyDestroy(&arr));
//
//         TEST(arr.count == 0);
//         TEST(arr.capacity >= 2);
//         TEST(arr.width == sizeof(u32));
//         TEST(arr.align == alignof(u32));
//
//         *static_cast<u32*>(arrayAnyPush(&arr)) = 10;
//         *static_cast<u32*>(arrayAnyPush(&arr)) = 20;
//
//         TEST(arr.count == 2);
//         TEST(*static_cast<u32*>(arr[0]) == 10);
//         TEST(*static_cast<u32*>(arr[1]) == 20);
//
//         arrayAnyResize(&arr, 4);
//
//         TEST(arr.count == 4);
//
//         *static_cast<u32*>(arr[2]) = 30;
//         *static_cast<u32*>(arr[3]) = 40;
//
//         TEST(*static_cast<u32*>(arr[2]) == 30);
//         TEST(*static_cast<u32*>(arr[3]) == 40);
//
//         u32 popped = 0;
//         arrayAnyPop(&arr, &popped);
//
//         TEST(popped == 40);
//         TEST(arr.count == 3);
//
//         arrayAnyResize(&arr, 1);
//
//         TEST(arr.count == 1);
//         TEST(*static_cast<u32*>(arr[0]) == 10);
//
//         ArenaScope arena = getScratch();
//
//         ArrayAny temp = arrayAnyTemp(arena, sizeof(u32), alignof(u32), 0, 2);
//
//         *static_cast<u32*>(arrayAnyPushTemp(arena, &temp)) = 123;
//         *static_cast<u32*>(arrayAnyPushTemp(arena, &temp)) = 456;
//
//         TEST(temp.count == 2);
//         TEST(*static_cast<u32*>(temp[0]) == 123);
//         TEST(*static_cast<u32*>(temp[1]) == 456);
//
//         arrayAnyPushTemp(arena, &temp);
//
//         TEST(temp.count == 3);
//     }
//
//     // // Queue
//     // {
//     //     Queue<u32> queue = queueCreate<u32>(4);
//     //
//     //     TEST(queue.count == 0);
//     //     TEST(queue.capacity >= 4);
//     //
//     //     queuePushBack(&queue, 1);
//     //     queuePushBack(&queue, 2);
//     //     queuePushBack(&queue, 3);
//     //     queuePushBack(&queue, 4);
//     //
//     //     TEST(queue.count == 4);
//     //
//     //     TEST(queuePopFront(&queue) == 1);
//     //     TEST(queuePopFront(&queue) == 2);
//     //
//     //     TEST(queue.count == 2);
//     //
//     //     queuePushBack(&queue, 5);
//     //     queuePushBack(&queue, 6);
//     //
//     //     TEST(queue.count == 4);
//     //
//     //     queuePushBack(&queue, 7);
//     //     queuePushBack(&queue, 8);
//     //
//     //     TEST(queue.count == 6);
//     //     TEST(queue.capacity >= 6);
//     //
//     //     TEST(queuePopFront(&queue) == 3);
//     //     TEST(queuePopFront(&queue) == 4);
//     //     TEST(queuePopFront(&queue) == 5);
//     //     TEST(queuePopFront(&queue) == 6);
//     //     TEST(queuePopFront(&queue) == 7);
//     //     TEST(queuePopFront(&queue) == 8);
//     //
//     //     TEST(queue.count == 0);
//     //
//     //     queuePushFront(&queue, 10);
//     //     queuePushFront(&queue, 20);
//     //     queuePushFront(&queue, 30);
//     //
//     //     TEST(queue.count == 3);
//     //
//     //     TEST(queuePopFront(&queue) == 30);
//     //     TEST(queuePopFront(&queue) == 20);
//     //     TEST(queuePopFront(&queue) == 10);
//     //
//     //     TEST(queue.count == 0);
//     //
//     //     queuePushBack(&queue, 1);
//     //     queuePushBack(&queue, 2);
//     //     queuePushFront(&queue, 0);
//     //     queuePushFront(&queue, -1);
//     //
//     //     TEST(queue.count == 4);
//     //
//     //     TEST(queuePopBack(&queue) == 2);
//     //     TEST(queuePopBack(&queue) == 1);
//     //     TEST(queuePopBack(&queue) == 0);
//     //     TEST(queuePopBack(&queue) == (u32)-1);
//     //
//     //     TEST(queue.count == 0);
//     //
//     //     queueDestroy(&queue);
//     // }
//
//     // // Set
//     // {
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         constexpr u32 count = 128;
//     //
//     //         Set<u32> set = setTemp<u32>(arena, count);
//     //
//     //         for (u32 i = 0; i < 3; ++i)
//     //         {
//     //             TEST(set.count == 0);
//     //             TEST(!setHas(&set, 0));
//     //             TEST(!setHas(&set, 1));
//     //             TEST(!setHas(&set, 12));
//     //             TEST(!setHas(&set, 42));
//     //             TEST(!setHas(&set, 100000));
//     //
//     //             setAdd(&set, 1);
//     //             TEST(set.count == 1);
//     //             TEST(setHas(&set, 1));
//     //
//     //             setRemove(&set, 1);
//     //             TEST(set.count == 0);
//     //             TEST(!setHas(&set, 1));
//     //
//     //             TEST(!setHas(&set, 12));
//     //             TEST(!setHas(&set, 12 + count));
//     //
//     //             setAdd(&set, 12);
//     //             TEST(set.count == 1);
//     //             TEST(setHas(&set, 12));
//     //             TEST(!setHas(&set, 12 + count));
//     //
//     //             setAdd(&set, 12 + count);
//     //             TEST(set.count == 2);
//     //             TEST(setHas(&set, 12));
//     //             TEST(setHas(&set, 12 + count));
//     //
//     //             setAdd(&set, 12 + count * 2);
//     //             TEST(set.count == 3);
//     //             TEST(setHas(&set, 12));
//     //             TEST(setHas(&set, 12 + count));
//     //             TEST(setHas(&set, 12 + count * 2));
//     //
//     //             setRemove(&set, 12);
//     //             TEST(set.count == 2);
//     //             TEST(!setHas(&set, 12));
//     //             TEST(setHas(&set, 12 + count));
//     //
//     //             setAdd(&set, 42);
//     //             TEST(set.count == 3);
//     //             TEST(setHas(&set, 42));
//     //
//     //             setRemove(&set, 12 + count);
//     //             TEST(set.count == 2);
//     //             TEST(!setHas(&set, 12));
//     //             TEST(!setHas(&set, 12 + count));
//     //
//     //             setRemove(&set, 42);
//     //             TEST(set.count == 1);
//     //             TEST(!setHas(&set, 42));
//     //
//     //             setRemove(&set, 12 + count * 2);
//     //             TEST(set.count == 0);
//     //             TEST(!setHas(&set, 12));
//     //             TEST(!setHas(&set, 12 + count));
//     //             TEST(!setHas(&set, 12 + count * 2));
//     //
//     //             setReset(&set);
//     //         }
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         using StrHash = u64;
//     //
//     //         Set<StrHash> set = setTemp<StrHash>(arena, 128);
//     //
//     //         StrHash a = hash("a");
//     //         StrHash b = hash("b");
//     //         StrHash ab = hash("ab");
//     //         StrHash scf = hash("supercalifragilisticexpialidocious");
//     //
//     //         TEST(!setHas(&set, a));
//     //         TEST(!setHas(&set, b));
//     //         TEST(!setHas(&set, ab));
//     //         TEST(!setHas(&set, scf));
//     //
//     //         setAdd(&set, a);
//     //         setAdd(&set, b);
//     //         setAdd(&set, ab);
//     //         setAdd(&set, scf);
//     //
//     //         TEST(setHas(&set, a));
//     //         TEST(setHas(&set, b));
//     //         TEST(setHas(&set, ab));
//     //         TEST(setHas(&set, scf));
//     //
//     //         setRemove(&set, a);
//     //         setRemove(&set, b);
//     //         setRemove(&set, ab);
//     //         setRemove(&set, scf);
//     //
//     //         TEST(!setHas(&set, a));
//     //         TEST(!setHas(&set, b));
//     //         TEST(!setHas(&set, ab));
//     //         TEST(!setHas(&set, scf));
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Set<const char*> set = setTemp<const char*>(arena, 128);
//     //
//     //         const char* a = "a";
//     //         const char* b = "b";
//     //         const char* ab = "ab";
//     //         const char* scf = "supercalifragilisticexpialidocious";
//     //
//     //         TEST(!setHas(&set, a));
//     //         TEST(!setHas(&set, b));
//     //         TEST(!setHas(&set, ab));
//     //         TEST(!setHas(&set, scf));
//     //
//     //         setAdd(&set, a);
//     //         setAdd(&set, b);
//     //         setAdd(&set, ab);
//     //         setAdd(&set, scf);
//     //
//     //         TEST(setHas(&set, a));
//     //         TEST(setHas(&set, b));
//     //         TEST(setHas(&set, ab));
//     //         TEST(setHas(&set, scf));
//     //
//     //         setRemove(&set, a);
//     //         setRemove(&set, b);
//     //         setRemove(&set, ab);
//     //         setRemove(&set, scf);
//     //
//     //         TEST(!setHas(&set, a));
//     //         TEST(!setHas(&set, b));
//     //         TEST(!setHas(&set, ab));
//     //         TEST(!setHas(&set, scf));
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Set<StringBuilder> set = setTemp<StringBuilder>(arena, 128);
//     //
//     //         TEST(!setHas(&set, StringBuilder{arena, "a"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "b"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "ab"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "supercalifragilisticexpialidocious"}));
//     //
//     //         setAdd(&set, StringBuilder{arena, "a"});
//     //         setAdd(&set, StringBuilder{arena, "b"});
//     //         setAdd(&set, StringBuilder{arena, "ab"});
//     //         setAdd(&set, StringBuilder{arena, "supercalifragilisticexpialidocious"});
//     //
//     //         TEST(setHas(&set, StringBuilder{arena, "a"}));
//     //         TEST(setHas(&set, StringBuilder{arena, "b"}));
//     //         TEST(setHas(&set, StringBuilder{arena, "ab"}));
//     //         TEST(setHas(&set, StringBuilder{arena, "supercalifragilisticexpialidocious"}));
//     //
//     //         setRemove(&set, StringBuilder{arena, "a"});
//     //         setRemove(&set, StringBuilder{arena, "b"});
//     //         setRemove(&set, StringBuilder{arena, "ab"});
//     //         setRemove(&set, StringBuilder{arena, "supercalifragilisticexpialidocious"});
//     //
//     //         TEST(!setHas(&set, StringBuilder{arena, "a"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "b"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "ab"}));
//     //         TEST(!setHas(&set, StringBuilder{arena, "supercalifragilisticexpialidocious"}));
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Set<StringView> set = setTemp<StringView>(arena, 128);
//     //
//     //         TEST(!setHas(&set, "a"));
//     //         TEST(!setHas(&set, "b"));
//     //         TEST(!setHas(&set, "ab"));
//     //         TEST(!setHas(&set, "supercalifragilisticexpialidocious"));
//     //
//     //         setAdd(&set, "a");
//     //         setAdd(&set, "b");
//     //         setAdd(&set, "ab");
//     //         setAdd(&set, "supercalifragilisticexpialidocious");
//     //
//     //         TEST(setHas(&set, "a"));
//     //         TEST(setHas(&set, "b"));
//     //         TEST(setHas(&set, "ab"));
//     //         TEST(setHas(&set, "supercalifragilisticexpialidocious"));
//     //
//     //         setRemove(&set, "a");
//     //         setRemove(&set, "b");
//     //         setRemove(&set, "ab");
//     //         setRemove(&set, "supercalifragilisticexpialidocious");
//     //
//     //         TEST(!setHas(&set, "a"));
//     //         TEST(!setHas(&set, "b"));
//     //         TEST(!setHas(&set, "ab"));
//     //         TEST(!setHas(&set, "supercalifragilisticexpialidocious"));
//     //     }
//     // }
//     //
//     // // Map
//     // {
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         constexpr u32 count = 128;
//     //
//     //         Map<u32, u32> map = mapTemp<u32, u32>(arena, count);
//     //
//     //         for (u32 i = 0; i < 3; ++i)
//     //         {
//     //             TEST(map.count == 0);
//     //             TEST(mapGet(&map, 0) == nullptr);
//     //             TEST(mapGet(&map, 1) == nullptr);
//     //             TEST(mapGet(&map, 12) == nullptr);
//     //             TEST(mapGet(&map, 42) == nullptr);
//     //             TEST(mapGet(&map, 100000) == nullptr);
//     //
//     //             mapAdd(&map, 1, 1);
//     //             TEST(map.count == 1);
//     //             TEST(mapGet(&map, 1) != nullptr);
//     //             TEST(*mapGet(&map, 1) == 1);
//     //
//     //             mapRemove(&map, 1);
//     //             TEST(map.count == 0);
//     //             TEST(mapGet(&map, 1) == nullptr);
//     //
//     //             TEST(mapGet(&map, 12) == nullptr);
//     //             TEST(mapGet(&map, 12 + count) == nullptr);
//     //
//     //             mapAdd(&map, 12, 42);
//     //             TEST(map.count == 1);
//     //             TEST(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
//     //             TEST(mapGet(&map, 12 + count) == nullptr);
//     //
//     //             mapAdd(&map, 12 + count, 100);
//     //             TEST(map.count == 2);
//     //             TEST(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
//     //             TEST(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);
//     //
//     //             mapAdd(&map, 12 + count * 2, 200);
//     //             TEST(map.count == 3);
//     //             TEST(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
//     //             TEST(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);
//     //             TEST(mapGet(&map, 12 + count * 2) != nullptr && *mapGet(&map, 12 + count * 2) == 200);
//     //
//     //             mapRemove(&map, 12);
//     //             TEST(map.count == 2);
//     //             TEST(mapGet(&map, 12) == nullptr);
//     //             TEST(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);
//     //
//     //             mapAdd(&map, 42, 12);
//     //             TEST(map.count == 3);
//     //             TEST(mapGet(&map, 42) != nullptr && *mapGet(&map, 42) == 12);
//     //
//     //             mapRemove(&map, 12 + count);
//     //             TEST(map.count == 2);
//     //             TEST(mapGet(&map, 12) == nullptr);
//     //             TEST(mapGet(&map, 12 + count) == nullptr);
//     //
//     //             mapRemove(&map, 42);
//     //             TEST(map.count == 1);
//     //             TEST(mapGet(&map, 42) == nullptr);
//     //
//     //             mapRemove(&map, 12 + count * 2);
//     //             TEST(map.count == 0);
//     //             TEST(mapGet(&map, 12) == nullptr);
//     //             TEST(mapGet(&map, 12 + count) == nullptr);
//     //             TEST(mapGet(&map, 12 + count * 2) == nullptr);
//     //
//     //             mapReset(&map);
//     //         }
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         using StrHash = u64;
//     //
//     //         Map<StrHash, u32> map = mapTemp<StrHash, u32>(arena, 128);
//     //
//     //         StrHash a = hash("a");
//     //         StrHash b = hash("b");
//     //         StrHash ab = hash("ab");
//     //         StrHash scf = hash("supercalifragilisticexpialidocious");
//     //
//     //         TEST(mapGet(&map, a) == nullptr);
//     //         TEST(mapGet(&map, b) == nullptr);
//     //         TEST(mapGet(&map, ab) == nullptr);
//     //         TEST(mapGet(&map, scf) == nullptr);
//     //
//     //         mapAdd(&map, a, 1);
//     //         mapAdd(&map, b, 2);
//     //         mapAdd(&map, ab, 3);
//     //         mapAdd(&map, scf, 4);
//     //
//     //         TEST(mapGet(&map, a) != nullptr && *mapGet(&map, a) == 1);
//     //         TEST(mapGet(&map, b) != nullptr && *mapGet(&map, b) == 2);
//     //         TEST(mapGet(&map, ab) != nullptr && *mapGet(&map, ab) == 3);
//     //         TEST(mapGet(&map, scf) != nullptr && *mapGet(&map, scf) == 4);
//     //
//     //         mapRemove(&map, a);
//     //         mapRemove(&map, b);
//     //         mapRemove(&map, ab);
//     //         mapRemove(&map, scf);
//     //
//     //         TEST(mapGet(&map, a) == nullptr);
//     //         TEST(mapGet(&map, b) == nullptr);
//     //         TEST(mapGet(&map, ab) == nullptr);
//     //         TEST(mapGet(&map, scf) == nullptr);
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Map<const char*, u32> map = mapTemp<const char*, u32>(arena, 128);
//     //
//     //         const char* a = "a";
//     //         const char* b = "b";
//     //         const char* ab = "ab";
//     //         const char* scf = "supercalifragilisticexpialidocious";
//     //
//     //         TEST(mapGet(&map, a) == nullptr);
//     //         TEST(mapGet(&map, b) == nullptr);
//     //         TEST(mapGet(&map, ab) == nullptr);
//     //         TEST(mapGet(&map, scf) == nullptr);
//     //
//     //         mapAdd(&map, a, 1);
//     //         mapAdd(&map, b, 2);
//     //         mapAdd(&map, ab, 3);
//     //         mapAdd(&map, scf, 4);
//     //
//     //         TEST(mapGet(&map, a) != nullptr && *mapGet(&map, a) == 1);
//     //         TEST(mapGet(&map, b) != nullptr && *mapGet(&map, b) == 2);
//     //         TEST(mapGet(&map, ab) != nullptr && *mapGet(&map, ab) == 3);
//     //         TEST(mapGet(&map, scf) != nullptr && *mapGet(&map, scf) == 4);
//     //
//     //         mapRemove(&map, a);
//     //         mapRemove(&map, b);
//     //         mapRemove(&map, ab);
//     //         mapRemove(&map, scf);
//     //
//     //         TEST(mapGet(&map, a) == nullptr);
//     //         TEST(mapGet(&map, b) == nullptr);
//     //         TEST(mapGet(&map, ab) == nullptr);
//     //         TEST(mapGet(&map, scf) == nullptr);
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Map<StringBuilder, u32> map = mapTemp<StringBuilder, u32>(arena, 128);
//     //
//     //         TEST(mapGet(&map, StringBuilder{arena, "a"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "b"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "ab"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"}) == nullptr);
//     //
//     //         mapAdd(&map, StringBuilder{arena, "a"}, 1);
//     //         mapAdd(&map, StringBuilder{arena, "b"}, 2);
//     //         mapAdd(&map, StringBuilder{arena, "ab"}, 3);
//     //         mapAdd(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"}, 4);
//     //
//     //         TEST(mapGet(&map, StringBuilder{arena, "a"}) != nullptr);
//     //         TEST(*mapGet(&map, StringBuilder{arena, "a"}) == 1);
//     //         TEST(mapGet(&map, StringBuilder{arena, "b"}) != nullptr);
//     //         TEST(*mapGet(&map, StringBuilder{arena, "b"}) == 2);
//     //         TEST(mapGet(&map, StringBuilder{arena, "ab"}) != nullptr);
//     //         TEST(*mapGet(&map, StringBuilder{arena, "ab"}) == 3);
//     //         TEST(mapGet(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"}) != nullptr);
//     //         TEST(*mapGet(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"}) == 4);
//     //
//     //         mapRemove(&map, StringBuilder{arena, "a"});
//     //         mapRemove(&map, StringBuilder{arena, "b"});
//     //         mapRemove(&map, StringBuilder{arena, "ab"});
//     //         mapRemove(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"});
//     //
//     //         TEST(mapGet(&map, StringBuilder{arena, "a"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "b"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "ab"}) == nullptr);
//     //         TEST(mapGet(&map, StringBuilder{arena, "supercalifragilisticexpialidocious"}) == nullptr);
//     //     }
//     //
//     //     {
//     //         ArenaScope arena = getScratch();
//     //
//     //         Map<StringView, u32> map = mapTemp<StringView, u32>(arena, 6);
//     //
//     //         TEST(mapGet(&map, "a") == nullptr);
//     //         TEST(mapGet(&map, "b") == nullptr);
//     //         TEST(mapGet(&map, "ab") == nullptr);
//     //         TEST(mapGet(&map, "supercalifragilisticexpialidocious") == nullptr);
//     //
//     //         mapAdd(&map, "a", 1);
//     //         mapAdd(&map, "b", 2);
//     //         mapAdd(&map, "ab", 3);
//     //         mapAdd(&map, "supercalifragilisticexpialidocious", 4);
//     //
//     //         TEST(mapGet(&map, "a") != nullptr);
//     //         TEST(*mapGet(&map, "a") == 1);
//     //         TEST(mapGet(&map, "b") != nullptr);
//     //         TEST(*mapGet(&map, "b") == 2);
//     //         TEST(mapGet(&map, "ab") != nullptr);
//     //         TEST(*mapGet(&map, "ab") == 3);
//     //         TEST(mapGet(&map, "supercalifragilisticexpialidocious") != nullptr);
//     //         TEST(*mapGet(&map, "supercalifragilisticexpialidocious") == 4);
//     //
//     //         mapRemove(&map, "a");
//     //         mapRemove(&map, "b");
//     //         mapRemove(&map, "ab");
//     //         mapRemove(&map, "supercalifragilisticexpialidocious");
//     //
//     //         TEST(mapGet(&map, "a") == nullptr);
//     //         TEST(mapGet(&map, "b") == nullptr);
//     //         TEST(mapGet(&map, "ab") == nullptr);
//     //         TEST(mapGet(&map, "supercalifragilisticexpialidocious") == nullptr);
//     //     }
//     // }
//
//     // Pool
//     {
//         Pool pool = poolCreate<u32>();
//         HG_DEFER(poolDestroy(&pool));
//
//         u32* a = static_cast<u32*>(poolAlloc(&pool));
//         u32* b = static_cast<u32*>(poolAlloc(&pool));
//         u32* c = static_cast<u32*>(poolAlloc(&pool));
//
//         TEST(a != nullptr);
//         TEST(b != nullptr);
//         TEST(c != nullptr);
//
//         *a = 1;
//         *b = 2;
//         *c = 3;
//
//         TEST(*a == 1);
//         TEST(*b == 2);
//         TEST(*c == 3);
//
//         poolFree(&pool, b);
//         poolFree(&pool, c);
//
//         u32* d = static_cast<u32*>(poolAlloc(&pool));
//         u32* e = static_cast<u32*>(poolAlloc(&pool));
//
//         TEST(d == c);
//         TEST(e == b);
//
//         *d = 40;
//         *e = 50;
//
//         TEST(*d == 40);
//         TEST(*e == 50);
//
//         constexpr u32 n = 1500;
//
//         ArenaScope arena = getScratch();
//
//         ArrayTemp<u32*> ptrs = ArrayTemp<u32*>(arena, 0, n);
//
//         for (u32 i = 0; i < n; ++i)
//         {
//             u32* p = static_cast<u32*>(poolAlloc(&pool));
//             ptrs.push(p);
//             TEST(p != nullptr);
//         }
//
//         TEST(pool.itemStores.count >= 2);
//
//         for (u32 i = 0; i < ptrs.count; ++i)
//         {
//             for (u32 j = i + 1; j < ptrs.count; ++j)
//             {
//                 TEST(ptrs[i] != ptrs[j]);
//             }
//         }
//
//         for (u32 i = 0; i < ptrs.count; ++i)
//         {
//             poolFree(&pool, ptrs[i]);
//         }
//     }
//
//     // HandlePool
//     {
//         HandlePool pool = handlePoolCreate();
//         HG_DEFER(handlePoolDestroy(&pool));
//
//         Handle u1 = handlePoolAlloc(&pool);
//         TEST(handlePoolAlive(&pool, u1));
//         TEST(u1.id == 1);
//
//         Handle u2 = handlePoolAlloc(&pool);
//         TEST(handlePoolAlive(&pool, u2));
//         TEST(u2.id == 2);
//
//         handlePoolFree(&pool, u1);
//         TEST(!handlePoolAlive(&pool, u1));
//
//         Handle u12 = handlePoolAlloc(&pool);
//         TEST(handlePoolAlive(&pool, u12));
//         TEST(!handlePoolAlive(&pool, u1));
//         TEST(u12.id != 1);
//         TEST(handleIdx(u12) == 1);
//
//         handlePoolReset(&pool);
//         TEST(!handlePoolAlive(&pool, u1));
//         TEST(!handlePoolAlive(&pool, u2));
//         TEST(!handlePoolAlive(&pool, u12));
//     }
//
//     // Mat
//     {
//         Mat2 mat{
//             {1.0f, 0.0f},
//             {1.0f, 0.0f},
//         };
//         Vec2 vec{1.0f, 1.0f};
//
//         Mat2 identity{
//             {1.0f, 0.0f},
//             {0.0f, 1.0f},
//         };
//         TEST(identity * mat == mat);
//         TEST(identity * vec == vec);
//
//         Mat2 matRotated{
//             {0.0f, 1.0f},
//             {0.0f, 1.0f},
//         };
//         Vec2 vecRotated{-1.0f, 1.0f};
//
//         Mat2 rotation{
//             {0.0f, 1.0f},
//             {-1.0f, 0.0f},
//         };
//         TEST(rotation * mat == matRotated);
//         TEST(rotation * vec == vecRotated);
//
//         TEST((identity * rotation) * mat == identity * (rotation * mat));
//         TEST((identity * rotation) * vec == identity * (rotation * vec));
//         TEST((rotation * rotation) * mat == rotation * (rotation * mat));
//         TEST((rotation * rotation) * vec == rotation * (rotation * vec));
//     }
//
//     // Quat
//     {
//         Mat3 identityMat = Mat3{1.0f};
//         Vec3 upVec{0.0f, -1.0f, 0.0f};
//         Quat rotation = quatAxisAngle({0.0f, 0.0f, -1.0f}, -static_cast<f32>(HG_PI) * 0.5f);
//
//         Vec3 rotatedVec = vecRot3(rotation, upVec);
//         Mat3 rotatedMat = matRot3(rotation, identityMat);
//
//         Vec3 matRotatedVec = rotatedMat * upVec;
//
//         TEST(abs(rotatedVec.x - 1.0f) < FLT_EPSILON
//               && abs(rotatedVec.y - 0.0f) < FLT_EPSILON
//               && abs(rotatedVec.y - 0.0f) < FLT_EPSILON);
//
//         TEST(abs(matRotatedVec.x - rotatedVec.x) < FLT_EPSILON
//               && abs(matRotatedVec.y - rotatedVec.y) < FLT_EPSILON
//               && abs(matRotatedVec.y - rotatedVec.z) < FLT_EPSILON);
//     }
//
//     // Circle
//     {
//         {
//             Circle circle{{0.0f, 0.0f}, 5.0f};
//             TEST(containsPointCircle({0.0f, 0.0f}, circle));
//         }
//
//         {
//             Circle circle{{0.0f, 0.0f}, 5.0f};
//             TEST(containsPointCircle({3.0f, 4.0f}, circle));
//         }
//
//         {
//             Circle circle{{0.0f, 0.0f}, 5.0f};
//             TEST(containsPointCircle({5.0f, 0.0f}, circle));
//         }
//
//         {
//             Circle circle{{0.0f, 0.0f}, 5.0f};
//             TEST(!containsPointCircle({5.01f, 0.0f}, circle));
//         }
//
//         {
//             Circle circle{{2.0f, -3.0f}, 2.0f};
//             TEST(containsPointCircle({2.0f, -3.0f}, circle));
//             TEST(containsPointCircle({4.0f, -3.0f}, circle));
//             TEST(!containsPointCircle({4.1f, -3.0f}, circle));
//         }
//
//         {
//             Circle circle{{0.0f, 0.0f}, 0.0f};
//             TEST(containsPointCircle({0.0f, 0.0f}, circle));
//             TEST(!containsPointCircle({0.01f, 0.0f}, circle));
//         }
//     }
//
//     // Rect
//     {
//         {
//             Rect rect = rectEmpty();
//             TEST(!containsPointRect({0.0f, 0.0f}, rect));
//             TEST(!containsPointRect({1.0f, 1.0f}, rect));
//         }
//
//         {
//             Rect rect = rectEmpty();
//             rect = rectAddPoint(rect, {2.0f, 3.0f});
//             TEST(containsPointRect({2.0f, 3.0f}, rect));
//         }
//
//         {
//             Rect rect = rectEmpty();
//             rect = rectAddPoint(rect, {1.0f, 2.0f});
//             TEST(containsPointRect({1.0f, 2.0f}, rect));
//         }
//
//         {
//             Rect rect = rectEmpty();
//             rect = rectAddPoint(rect, {2.0f, 2.0f});
//             rect = rectAddPoint(rect, {5.0f, 7.0f});
//
//             TEST(containsPointRect({2.0f, 2.0f}, rect));
//             TEST(containsPointRect({5.0f, 7.0f}, rect));
//             TEST(containsPointRect({3.0f, 4.0f}, rect));
//         }
//
//         {
//             Rect rect = rectEmpty();
//             rect = rectAddPoint(rect, {5.0f, 5.0f});
//             rect = rectAddPoint(rect, {-2.0f, -3.0f});
//
//             TEST(containsPointRect({-2.0f, -3.0f}, rect));
//             TEST(containsPointRect({5.0f, 5.0f}, rect));
//             TEST(containsPointRect({0.0f, 0.0f}, rect));
//         }
//
//         {
//             Rect rect = rectEmpty();
//             rect = rectAddPoint(rect, {1.0f, 1.0f});
//             rect = rectAddPoint(rect, {1.0f, 1.0f});
//
//             TEST(containsPointRect({1.0f, 1.0f}, rect));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
//             TEST(containsPointRect({5.0f, 2.5f}, rect));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
//             TEST(containsPointRect({0.0f, 0.0f}, rect));
//             TEST(containsPointRect({10.0f, 5.0f}, rect));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
//             TEST(!containsPointRect({-0.01f, 0.0f}, rect));
//             TEST(!containsPointRect({10.01f, 5.0f}, rect));
//             TEST(!containsPointRect({5.0f, 5.01f}, rect));
//         }
//
//         {
//             Rect rect{{-5.0f, -3.0f}, {-3.0f, 5.0f}};
//             TEST(containsPointRect({-4.0f, 0.0f}, rect));
//             TEST(!containsPointRect({-2.9f, 0.0f}, rect));
//         }
//
//         {
//             Rect rect{{2.0f, 2.0f}, {2.0f, 2.0f}};
//             TEST(containsPointRect({2.0f, 2.0f}, rect));
//             TEST(!containsPointRect({2.01f, 2.0f}, rect));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Vec2 p = closestPointRect({-5.0f, 5.0f}, rect);
//
//             TEST(p.x == 0.0f);
//             TEST(p.y == 5.0f);
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Vec2 p = closestPointRect({15.0f, 5.0f}, rect);
//
//             TEST(p.x == 10.0f);
//             TEST(p.y == 5.0f);
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Vec2 p = closestPointRect({5.0f, -3.0f}, rect);
//
//             TEST(p.x == 5.0f);
//             TEST(p.y == 0.0f);
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Vec2 p = closestPointRect({-3.0f, 15.0f}, rect);
//
//             TEST(p.x == 0.0f);
//             TEST(p.y == 10.0f);
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Vec2 p = closestPointRect({5.0f, 5.0f}, rect);
//
//             TEST(p.x == 5.0f);
//             TEST(p.y == 5.0f);
//         }
//
//         {
//             Rect a{{0.0f, 0.0f}, {5.0f, 5.0}};
//             Rect b{{3.0f, 3.0f}, {8.0f, 8.0f}};
//
//             TEST(intersectRects(a, b));
//             TEST(intersectRects(b, a));
//         }
//
//         {
//             Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
//             Rect b{{5.0f, 0.0f}, {7.0f, 2.0f}};
//
//             TEST(intersectRects(a, b));
//         }
//
//         {
//             Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
//             Rect b{{5.0f, 5.0f}, {7.0f, 7.0f}};
//
//             TEST(intersectRects(a, b));
//         }
//
//         {
//             Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
//             Rect b{{5.01f, 0.0f}, {7.01f, 2.0f}};
//
//             TEST(!intersectRects(a, b));
//         }
//
//         {
//             Rect a{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Rect b{{2.0f, 2.0f}, {4.0f, 4.0f}};
//
//             TEST(intersectRects(a, b));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Circle circle{{5.0f, 5.0f}, 2.0f};
//
//             TEST(intersectRectCircle(rect, circle));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Circle circle{{12.0f, 5.0f}, 2.0f};
//
//             TEST(intersectRectCircle(rect, circle));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Circle circle{{13.0f, 5.0f}, 2.0f};
//
//             TEST(!intersectRectCircle(rect, circle));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Circle circle{{12.0f, 12.0f}, std::sqrt(8.0f) + FLT_EPSILON};
//
//             TEST(intersectRectCircle(rect, circle));
//         }
//
//         {
//             Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
//             Circle circle{{13.0f, 13.0f}, 2.0f};
//
//             TEST(!intersectRectCircle(rect, circle));
//         }
//     }
//
//     // Ray2D
//     {
//         {
//             Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Ray2D b{{5.0f, -5.0f}, {0.0f, 1.0f}};
//
//             Hit2D hit;
//             TEST(intersectRays2D(a, b, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Ray2D b{{5.0f, 5.0f}, {0.0f, 1.0f}};
//
//             TEST(!intersectRays2D(a, b, nullptr));
//         }
//
//         {
//             Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Ray2D b{{-5.0f, -5.0f}, {0.0f, 1.0f}};
//
//             TEST(!intersectRays2D(a, b, nullptr));
//         }
//
//         {
//             Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Ray2D b{{0.0f, 1.0f}, {1.0f, 0.0f}};
//
//             TEST(!intersectRays2D(a, b, nullptr));
//         }
//
//         {
//             Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Ray2D b{{0.0f, 0.0f}, {0.0f, 1.0f}};
//
//             Hit2D hit;
//             TEST(intersectRays2D(a, b, &hit));
//             TEST(std::abs(hit.dist) <= FLT_EPSILON);
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Line2D line{{5.0f, -2.0f}, {5.0f, 2.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayLine2D(ray, line, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Line2D line{{-5.0f, -2.0f}, {-5.0f, 2.0f}};
//
//             TEST(!intersectRayLine2D(ray, line, nullptr));
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Line2D line{{5.0f, 1.0f}, {8.0f, 1.0f}};
//
//             TEST(!intersectRayLine2D(ray, line, nullptr));
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Line2D line{{5.0f, 0.0f}, {5.0f, 5.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayLine2D(ray, line, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray2D ray{{5.0f, 0.0f}, {1.0f, 0.0f}};
//             Line2D line{{5.0f, -5.0f}, {5.0f, 5.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayLine2D(ray, line, &hit));
//             TEST(std::abs(hit.dist) <= FLT_EPSILON);
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectRayCircle(ray, circle, &hit));
//             TEST(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{0.0f, 2.0f}, {1.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectRayCircle(ray, circle, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray2D ray{{0.0f, 3.0f}, {1.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectRayCircle(ray, circle, nullptr));
//         }
//
//         {
//             Ray2D ray{{10.0f, 0.0f}, {1.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectRayCircle(ray, circle, &hit));
//             TEST(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{20.0f, 0.0f}, {1.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectRayCircle(ray, circle, nullptr));
//         }
//
//         {
//             Ray2D ray{{0.0f, 5.0f}, {1.0f, 0.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{20.0f, 5.0f}, {-1.0f, 0.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{12.5f, -5.0f}, {0.0f, 1.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {0.0f, -1.0f}));
//         }
//
//         {
//             Ray2D ray{{12.5f, 20.0f}, {0.0f, -1.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {0.0f, 1.0f}));
//         }
//
//         {
//             Ray2D ray{{0.0f, 0.0f}, {1.0f, 1.0f}};
//             Rect rect{{10.0f, 10.0f}, {15.0f, 15.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray2D ray{{0.0f, 5.0f}, {-1.0f, 0.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             TEST(!intersectRayRect(ray, rect, nullptr));
//         }
//
//         {
//             Ray2D ray{{12.5f, 5.0f}, {1.0f, 0.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectRayRect(ray, rect, &hit));
//             TEST(hit.dist == 0.0f);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Ray2D ray{{0.0f, 20.0f}, {1.0f, 0.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             TEST(!intersectRayRect(ray, rect, nullptr));
//         }
//
//     }
//
//     // Line2D
//     {
//         {
//             Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Line2D b{{5.0f, -5.0f}, {5.0f, 5.0f}};
//
//             Hit2D hit;
//             TEST(intersectLines2D(a, b, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Line2D b{{15.0f, -5.0f}, {15.0f, 5.0f}};
//
//             TEST(!intersectLines2D(a, b, nullptr));
//         }
//
//         {
//             Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Line2D b{{10.0f, 0.0f}, {10.0f, 5.0f}};
//
//             Hit2D hit;
//             TEST(intersectLines2D(a, b, &hit));
//             TEST(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Line2D b{{0.0f, 1.0f}, {10.0f, 1.0f}};
//
//             TEST(!intersectLines2D(a, b, nullptr));
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Ray2D ray{{5.0f, -5.0f}, {0.0f, 1.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRay2D(line, ray, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Ray2D ray{{15.0f, -5.0f}, {0.0f, 1.0f}};
//
//             TEST(!intersectLineRay2D(line, ray, nullptr));
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Ray2D ray{{10.0f, -5.0f}, {0.0f, 1.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRay2D(line, ray, &hit));
//             TEST(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
//             Ray2D ray{{5.0f, 5.0f}, {0.0f, 1.0f}};
//
//             TEST(!intersectLineRay2D(line, ray, nullptr));
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {20.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectLineCircle(line, circle, &hit));
//             TEST(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Line2D line{{0.0f, 2.0f}, {20.0f, 2.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectLineCircle(line, circle, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//         }
//
//         {
//             Line2D line{{0.0f, 3.0f}, {20.0f, 3.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectLineCircle(line, circle, nullptr));
//         }
//
//         {
//             Line2D line{{0.0f, 0.0f}, {5.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectLineCircle(line, circle, nullptr));
//         }
//
//         {
//             Line2D line{{10.0f, 0.0f}, {20.0f, 0.0f}};
//             Circle circle{{10.0f, 0.0f}, 2.0f};
//
//             Hit2D hit;
//             TEST(intersectLineCircle(line, circle, &hit));
//             TEST(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {1.0f, 0.0f}));
//         }
//
//         {
//             Line2D line{{0.0f, 5.0f}, {20.0f, 5.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRect(line, rect, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {-1.0f, 0.0f}));
//         }
//
//         {
//             Line2D line{{20.0f, 5.0f}, {0.0f, 5.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRect(line, rect, &hit));
//             TEST(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {1.0f, 0.0f}));
//         }
//
//         {
//             Line2D line{{12.5f, -5.0f}, {12.5f, 15.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRect(line, rect, &hit));
//             TEST(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {0.0f, -1.0f}));
//         }
//
//         {
//             Line2D line{{0.0f, 20.0f}, {20.0f, 20.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             TEST(!intersectLineRect(line, rect, nullptr));
//         }
//
//         {
//             Line2D line{{12.5f, 5.0f}, {17.5f, 5.0f}};
//             Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};
//
//             Hit2D hit;
//             TEST(intersectLineRect(line, rect, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq2(hit.normal, {1.0f, 0.0f}));
//         }
//     }
//
//     // Sphere
//     {
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(containsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(containsPointSphere({3.0f, 4.0f, 0.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(containsPointSphere({5.0f, 0.0f, 0.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(!containsPointSphere({5.01f, 0.0f, 0.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{2.0f, -3.0f, 4.0f}, 2.0f};
//             TEST(containsPointSphere({2.0f, -3.0f, 4.0f}, sphere));
//             TEST(containsPointSphere({4.0f, -3.0f, 4.0f}, sphere));
//             TEST(!containsPointSphere({4.1f, -3.0f, 4.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
//             TEST(containsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
//             TEST(!containsPointSphere({0.01f, 0.0f, 0.0f}, sphere));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(std::abs(distPointSphere({10.0f, 0.0f, 0.0f}, sphere) - 5.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(std::abs(distPointSphere({5.0f, 0.0f, 0.0f}, sphere)) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             TEST(std::abs(distPointSphere({0.0f, 0.0f, 0.0f}, sphere) + 5.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere sphere{{2.0f, 3.0f, 4.0f}, 2.0f};
//             TEST(std::abs(distPointSphere({6.0f, 3.0f, 4.0f}, sphere) - 2.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Vec3 p = closestPointSphere({10.0f, 0.0f, 0.0f}, sphere);
//
//             TEST(vecEq3(p, {5.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Vec3 p = closestPointSphere({0.0f, 10.0f, 0.0f}, sphere);
//
//             TEST(vecEq3(p, {0.0f, 5.0f, 0.0f}));
//         }
//
//         {
//             Sphere sphere{{2.0f, 1.0f, -3.0f}, 3.0f};
//             Vec3 p = closestPointSphere({5.0f, 1.0f, -3.0f}, sphere);
//
//             TEST(vecEq3(p, {5.0f, 1.0f, -3.0f}));
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Vec3 p = closestPointSphere({0.0f, 0.0f, 0.0f}, sphere);
//
//             TEST(distPointSphere(p, sphere) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
//             Vec3 p = closestPointSphere({10.0f, 2.0f, -5.0f}, sphere);
//
//             TEST(vecEq3(p, {0.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{8.0f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(intersectSpheres(a, b));
//             TEST(intersectSpheres(b, a));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{10.0f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(intersectSpheres(a, b));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{10.1f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(!intersectSpheres(a, b));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{0.0f, 0.0f, 0.0f}, 2.0f};
//
//             TEST(intersectSpheres(a, b));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 0.0f};
//             Sphere b{{0.0f, 0.0f, 0.0f}, 0.0f};
//
//             TEST(intersectSpheres(a, b));
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{20.0f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(std::abs(distSpheres(a, b) - 10.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{10.0f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(std::abs(distSpheres(a, b)) <= FLT_EPSILON);
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{5.0f, 0.0f, 0.0f}, 5.0f};
//
//             TEST(distSpheres(a, b) < 0.0f);
//         }
//
//         {
//             Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
//             Sphere b{{0.0f, 0.0f, 0.0f}, 1.0f};
//
//             TEST(distSpheres(a, b) < 0.0f);
//         }
//     }
//
//     // Box
//     {
//         {
//             Box box = boxEmpty();
//
//             TEST(!containsPointBox({0.0f, 0.0f, 0.0f}, box));
//             TEST(!containsPointBox({1.0f, 1.0f, 1.0f}, box));
//         }
//
//         {
//             Box box = boxEmpty();
//             box = boxAddPoint(box, {2.0f, 3.0f, 4.0f});
//
//             TEST(containsPointBox({2.0f, 3.0f, 4.0f}, box));
//         }
//
//         {
//             Box box = boxEmpty();
//             box = boxAddPoint(box, {1.0f, 2.0f, 3.0f});
//
//             TEST(containsPointBox({1.0f, 2.0f, 3.0f}, box));
//         }
//
//         {
//             Box box = boxEmpty();
//             box = boxAddPoint(box, {2.0f, 2.0f, 2.0f});
//             box = boxAddPoint(box, {5.0f, 7.0f, 11.0f});
//
//             TEST(containsPointBox({2.0f, 2.0f, 2.0f}, box));
//             TEST(containsPointBox({5.0f, 7.0f, 11.0f}, box));
//             TEST(containsPointBox({3.0f, 4.0f, 5.0f}, box));
//         }
//
//         {
//             Box box = boxEmpty();
//             box = boxAddPoint(box, {5.0f, 5.0f, 5.0f});
//             box = boxAddPoint(box, {-2.0f, -3.0f, -4.0f});
//
//             TEST(containsPointBox({-2.0f, -3.0f, -4.0f}, box));
//             TEST(containsPointBox({5.0f, 5.0f, 5.0f}, box));
//             TEST(containsPointBox({0.0f, 0.0f, 0.0f}, box));
//         }
//
//         {
//             Box box = boxEmpty();
//             box = boxAddPoint(box, {1.0f, 1.0f, 1.0f});
//             box = boxAddPoint(box, {1.0f, 1.0f, 1.0f});
//
//             TEST(containsPointBox({1.0f, 1.0f, 1.0f}, box));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
//             TEST(containsPointBox({5.0f, 2.5f, 4.0f}, box));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
//             TEST(containsPointBox({0.0f, 0.0f, 0.0f}, box));
//             TEST(containsPointBox({10.0f, 5.0f, 8.0f}, box));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
//             TEST(!containsPointBox({-0.01f, 0.0f, 0.0f}, box));
//             TEST(!containsPointBox({10.01f, 5.0f, 8.0f}, box));
//             TEST(!containsPointBox({5.0f, 5.01f, 4.0f}, box));
//             TEST(!containsPointBox({5.0f, 2.5f, 8.01f}, box));
//         }
//
//         {
//             Box box{{-5.0f, -3.0f, -2.0f}, {-3.0f, 5.0f, 4.0f}};
//             TEST(containsPointBox({-4.0f, 0.0f, 0.0f}, box));
//             TEST(!containsPointBox({-2.9f, 0.0f, 0.0f}, box));
//         }
//
//         {
//             Box box{{2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}};
//             TEST(containsPointBox({2.0f, 2.0f, 2.0f}, box));
//             TEST(!containsPointBox({2.01f, 2.0f, 2.0f}, box));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({-5.0f, 5.0f, 5.0f}, box);
//
//             TEST(vecEq3(p, {0.0f, 5.0f, 5.0f}));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({15.0f, 5.0f, 5.0f}, box);
//
//             TEST(vecEq3(p, {10.0f, 5.0f, 5.0f}));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({5.0f, -3.0f, 5.0f}, box);
//
//             TEST(vecEq3(p, {5.0f, 0.0f, 5.0f}));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({5.0f, 5.0f, 15.0f}, box);
//
//             TEST(vecEq3(p, {5.0f, 5.0f, 10.0f}));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({-5.0f, 15.0f, -3.0f}, box);
//
//             TEST(vecEq3(p, {0.0f, 10.0f, 0.0f}));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Vec3 p = closestPointBox({5.0f, 5.0f, 5.0f}, box);
//
//             TEST(vecEq3(p, {5.0f, 5.0f, 5.0f}));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}};
//
//             TEST(intersectBox(a, b));
//             TEST(intersectBox(b, a));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{5.0f, 0.0f, 0.0f}, {7.0f, 2.0f, 2.0f}};
//
//             TEST(intersectBox(a, b));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{0.0f, 5.0f, 0.0f}, {2.0f, 7.0f, 2.0f}};
//
//             TEST(intersectBox(a, b));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{0.0f, 0.0f, 5.0f}, {2.0f, 2.0f, 7.0f}};
//
//             TEST(intersectBox(a, b));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{5.0f, 5.0f, 5.0f}, {7.0f, 7.0f, 7.0f}};
//
//             TEST(intersectBox(a, b));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
//             Box b{{5.01f, 0.0f, 0.0f}, {7.01f, 2.0f, 2.0f}};
//
//             TEST(!intersectBox(a, b));
//         }
//
//         {
//             Box a{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Box b{{2.0f, 2.0f, 2.0f}, {4.0f, 4.0f, 4.0f}};
//
//             TEST(intersectBox(a, b));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{5.0f, 5.0f, 5.0f}, 2.0f};
//
//             TEST(intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{12.0f, 5.0f, 5.0f}, 2.0f};
//
//             TEST(intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{5.0f, 12.0f, 5.0f}, 2.0f};
//
//             TEST(intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{5.0f, 5.0f, 12.0f}, 2.0f};
//
//             TEST(intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{13.0f, 5.0f, 5.0f}, 2.0f};
//
//             TEST(!intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{12.0f, 12.0f, 12.0f}, std::sqrt(12.0f)};
//
//             TEST(intersectBoxSphere(box, sphere));
//         }
//
//         {
//             Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
//             Sphere sphere{{13.0f, 13.0f, 13.0f}, 2.0f};
//
//             TEST(!intersectBoxSphere(box, sphere));
//         }
//     }
//
//     // Plane
//     {
//         {
//             Plane plane = planeFromPoint({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//             TEST(vecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
//             TEST(std::abs(plane.dist) <= FLT_EPSILON);
//         }
//
//         {
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//             TEST(vecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
//             TEST(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Plane plane = planeFromPoint({3.0f, 2.0f, -1.0f}, {1.0f, 0.0f, 0.0f});
//             TEST(vecEq3(plane.normal, {1.0f, 0.0f, 0.0f}));
//             TEST(std::abs(plane.dist - 3.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//             Plane plane = planeFromTri(tri);
//
//             TEST(vecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
//             TEST(std::abs(plane.dist) <= FLT_EPSILON);
//         }
//
//         {
//             Tri tri{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Plane plane = planeFromTri(tri);
//
//             TEST(vecEq3(plane.normal, {0.0f, 0.0f, -1.0f}));
//             TEST(std::abs(plane.dist) <= FLT_EPSILON);
//         }
//
//         {
//             Tri tri{{1.0f, 2.0f, 5.0f}, {4.0f, 2.0f, 5.0f}, {1.0f, 6.0f, 5.0f}};
//             Plane plane = planeFromTri(tri);
//
//             TEST(vecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
//             TEST(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
//         }
//     }
//
//     // Ray3D
//     {
//         {
//             Ray3D ray{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectRaySphere(ray, sphere, &hit));
//             TEST(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectRaySphere(ray, sphere, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray3D ray{{0.0f, 3.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectRaySphere(ray, sphere, nullptr));
//         }
//
//         {
//             Ray3D ray{{10.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectRaySphere(ray, sphere, &hit));
//             TEST(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{20.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectRaySphere(ray, sphere, nullptr));
//         }
//
//         {
//             Ray3D ray{{0.0f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayBox(ray, box, &hit));
//             TEST(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{20.0f, 5.0f, 5.0f}, {-1.0f, 0.0f, 0.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayBox(ray, box, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{12.5f, -5.0f, 5.0f}, {0.0f, 1.0f, 0.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayBox(ray, box, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{12.5f, 5.0f, -5.0f}, {0.0f, 0.0f, 1.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayBox(ray, box, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
//         }
//
//         {
//             Ray3D ray{{0.0f, 20.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             TEST(!intersectRayBox(ray, box, nullptr));
//         }
//
//         {
//             Ray3D ray{{12.5f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayBox(ray, box, &hit));
//             TEST(std::abs(hit.dist) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{0.25f, 0.25f, -1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayTri(ray, tri, &hit));
//             TEST(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
//         }
//
//         {
//             Ray3D ray{{0.5f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayTri(ray, tri, &hit));
//             TEST(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray3D ray{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectRayTri(ray, tri, &hit));
//             TEST(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray3D ray{{0.75f, 0.75f, -1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             TEST(!intersectRayTri(ray, tri, nullptr));
//         }
//
//         {
//             Ray3D ray{{0.25f, 0.25f, 1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             TEST(!intersectRayTri(ray, tri, nullptr));
//         }
//
//         {
//             Ray3D ray{{0.25f, 0.25f, -1.0f}, {1.0f, 0.0f, 0.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             TEST(!intersectRayTri(ray, tri, nullptr));
//         }
//
//         {
//             Ray3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectRayPlane(ray, plane, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectRayPlane(ray, plane, &hit));
//             TEST(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
//         }
//
//         {
//             Ray3D ray{{0.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             TEST(!intersectRayPlane(ray, plane, nullptr));
//         }
//
//         {
//             Ray3D ray{{0.0f, 2.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             TEST(!intersectRayPlane(ray, plane, nullptr));
//         }
//
//         {
//             Ray3D ray{{1.0f, 5.0f, 2.0f}, {0.0f, 1.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectRayPlane(ray, plane, &hit));
//             TEST(std::abs(hit.dist) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
//         }
//
//         {
//             Ray3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -2.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectRayPlane(ray, plane, &hit));
//             TEST(std::abs(hit.dist - 2.5f) <= FLT_EPSILON);
//         }
//     }
//
//     // Line3D
//     {
//         {
//             Line3D line{{0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectLineSphere(line, sphere, &hit));
//             TEST(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 2.0f, 0.0f}, {20.0f, 2.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectLineSphere(line, sphere, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//         }
//
//         {
//             Line3D line{{0.0f, 3.0f, 0.0f}, {20.0f, 3.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectLineSphere(line, sphere, nullptr));
//         }
//
//         {
//             Line3D line{{0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             TEST(!intersectLineSphere(line, sphere, nullptr));
//         }
//
//         {
//             Line3D line{{10.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
//             Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};
//
//             Hit3D hit;
//             TEST(intersectLineSphere(line, sphere, &hit));
//             TEST(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
//         }
//
//         {
//             Line3D line{{0.0f, 5.0f, 5.0f}, {20.0f, 5.0f, 5.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineBox(line, box, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{20.0f, 5.0f, 5.0f}, {0.0f, 5.0f, 5.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineBox(line, box, &hit));
//             TEST(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{12.5f, -5.0f, 5.0f}, {12.5f, 15.0f, 5.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineBox(line, box, &hit));
//             TEST(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{12.5f, 5.0f, -5.0f}, {12.5f, 5.0f, 15.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineBox(line, box, &hit));
//             TEST(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 20.0f, 5.0f}, {20.0f, 20.0f, 5.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             TEST(!intersectLineBox(line, box, nullptr));
//         }
//
//         {
//             Line3D line{{12.5f, 5.0f, 5.0f}, {17.5f, 5.0f, 5.0f}};
//             Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineBox(line, box, &hit));
//             TEST(std::abs(hit.dist - 0.5) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineTri(line, tri, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
//         }
//
//         {
//             Line3D line{{0.5f, 0.0f, -1.0f}, {0.5f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineTri(line, tri, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//         }
//
//         {
//             Line3D line{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineTri(line, tri, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//         }
//
//         {
//             Line3D line{{0.75f, 0.75f, -1.0f}, {0.75f, 0.75f, 1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             TEST(!intersectLineTri(line, tri, nullptr));
//         }
//
//         {
//             Line3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, -0.5f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             TEST(!intersectLineTri(line, tri, nullptr));
//         }
//
//         {
//             Line3D line{{0.25f, 0.25f, 1.0f}, {0.25f, 0.25f, -1.0f}};
//             Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
//
//             Hit3D hit;
//             TEST(intersectLineTri(line, tri, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 0.0f, 1.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectLinePlane(line, plane, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 10.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectLinePlane(line, plane, &hit));
//             TEST(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 10.0f, 0.0f}, {10.0f, 10.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             TEST(!intersectLinePlane(line, plane, nullptr));
//         }
//
//         {
//             Line3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             TEST(!intersectLinePlane(line, plane, nullptr));
//         }
//
//         {
//             Line3D line{{1.0f, 5.0f, 2.0f}, {1.0f, 10.0f, 2.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             Hit3D hit;
//             TEST(intersectLinePlane(line, plane, &hit));
//             TEST(std::abs(hit.dist) <= FLT_EPSILON);
//             TEST(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
//         }
//
//         {
//             Line3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 20.0f, 0.0f}};
//             Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
//
//             TEST(!intersectLinePlane(line, plane, nullptr));
//         }
//     }
//
//     // // AssetManager and Binary
//     // {
//     //     {
//     //         BinaryAsset* bin1 = assetCreate<Binary>();
//     //         TEST(bin1 != nullptr);
//     //         TEST(bin1->path == "");
//     //
//     //         BinaryAsset* bin2 = assetCreate<Binary>();
//     //         TEST(bin2 != nullptr);
//     //         TEST(bin2->path == "");
//     //         TEST(bin2 != bin1);
//     //
//     //         assetUnload(bin1);
//     //         assetUnload(bin2);
//     //     }
//     //
//     //     {
//     //         BinaryAsset* bin = assetLoad<Binary>("file_does_not_exist.bin");
//     //         TEST(bin->asset.data == nullptr);
//     //         TEST(bin->asset.size == 0);
//     //         assetUnload(bin);
//     //     }
//     //
//     //     u32 saveData[]{12, 42, 100, 128};
//     //
//     //     {
//     //         BinaryView bin{saveData, sizeof(saveData)};
//     //
//     //         binaryStore(bin, "dir/does/not/exist.bin");
//     //
//     //         FILE* fileHandle = fopen("dir/does/not/exist.bin", "rb");
//     //         TEST(fileHandle == nullptr);
//     //     }
//     //
//     //     {
//     //         BinaryView bin{saveData, sizeof(saveData)};
//     //
//     //         StringView filePath = "hg_test_dir/file_bin_test.bin";
//     //
//     //         binaryStore(bin, filePath);
//     //
//     //         BinaryAsset* newBin = assetLoad<Binary>(filePath);
//     //
//     //         TEST(newBin->asset.data != nullptr);
//     //         TEST(newBin->asset.data != saveData);
//     //         TEST(newBin->asset.size == sizeof(saveData));
//     //         TEST(memEqual(saveData, newBin->asset.data, newBin->asset.size));
//     //
//     //         BinaryAsset* newBin2 = assetLoad<Binary>(filePath);
//     //         TEST(newBin2 == newBin);
//     //
//     //         assetUnload(newBin);
//     //         assetUnload(newBin2);
//     //     }
//     // }
//
//     // Image
//     {
//         struct color {
//             u8 r, g, b, a;
//
//             operator u32() { return *reinterpret_cast<u32*>(this); }
//         };
//
//         u32 red =    color{0xff, 0x00, 0x00, 0xff};
//         u32 green =  color{0x00, 0xff, 0x00, 0xff};
//         u32 blue =   color{0x00, 0x00, 0xff, 0xff};
//         u32 yellow = color{0xff, 0xff, 0x00, 0xff};
//
//         u32 saveData[2][2]{
//             {red, green},
//             {blue, yellow},
//         };
//
//         StringView path = "hg_test_dir/image_test.png";
//
//         TextureData testImage{};
//         testImage.width = 2;
//         testImage.height = 2;
//         testImage.depth = 1;
//         testImage.format = Format_r8g8b8a8_srgb;
//         testImage.pixels = saveData;
//
//         {
//             textureStorePng(&testImage, path);
//
//             TextureDataAsset* image = assetLoad<TextureData>(path);
//             HG_DEFER(assetUnload(image));
//             TEST(image->asset.width == testImage.width);
//             TEST(image->asset.height == testImage.height);
//             TEST(memcmp(image->asset.pixels, saveData, sizeof(saveData)) == 0);
//         }
//     }
//
//     // Ecs basics
//     {
//         Ecs ecs = ecsCreate();
//         HG_DEFER(ecsDestroy(&ecs));
//
//         HG_ECS_REGISTER_TYPE(&ecs, u32);
//         HG_ECS_REGISTER_TYPE(&ecs, u64);
//
//         Entity e1 = ecsSpawn(&ecs);
//         Entity e2 = ecsSpawn(&ecs);
//         Entity e3 = {};
//         TEST(ecsAlive(&ecs, e1));
//         TEST(ecsAlive(&ecs, e2));
//         TEST(!ecsAlive(&ecs, e3));
//
//         ecsDespawn(&ecs, e1);
//         TEST(!ecsAlive(&ecs, e1));
//         e3 = ecsSpawn(&ecs);
//         TEST(ecsAlive(&ecs, e3));
//         TEST(handleIdx(e3.handle) == handleIdx(e1.handle) && e3.handle.id != e1.handle.id);
//
//         e1 = ecsSpawn(&ecs);
//         TEST(ecsAlive(&ecs, e1));
//
//         {
//             TEST(!ecsHas<u32>(&ecs, e1));
//             TEST(!ecsHas<u32>(&ecs, e2));
//             TEST(!ecsHas<u32>(&ecs, e3));
//
//             *ecsAdd<u32>(&ecs, e1) = 1;
//             TEST(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
//             TEST(!ecsHas<u32>(&ecs, e2));
//             TEST(!ecsHas<u32>(&ecs, e3));
//             *ecsAdd<u32>(&ecs, e2) = 2;
//             TEST(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
//             TEST(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
//             TEST(!ecsHas<u32>(&ecs, e3));
//             *ecsAdd<u32>(&ecs, e3) = 3;
//             TEST(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
//             TEST(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
//             TEST(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);
//
//             ecsRemove<u32>(&ecs, e1);
//             TEST(!ecsHas<u32>(&ecs, e1));
//             TEST(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
//             TEST(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);
//             ecsRemove<u32>(&ecs, e2);
//             TEST(!ecsHas<u32>(&ecs, e1));
//             TEST(!ecsHas<u32>(&ecs, e2));
//             TEST(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);
//             ecsRemove<u32>(&ecs, e3);
//             TEST(!ecsHas<u32>(&ecs, e1));
//             TEST(!ecsHas<u32>(&ecs, e2));
//             TEST(!ecsHas<u32>(&ecs, e3));
//         }
//
//         {
//             bool hasUnknown = false;
//             ecsForEach<u32>(&ecs, [&](Entity, u32*)
//             {
//                 hasUnknown = true;
//             });
//             TEST(!hasUnknown);
//
//             TEST(ecsCount<u32>(&ecs) == 0);
//             TEST(ecsCount<u64>(&ecs) == 0);
//         }
//
//         {
//             *ecsAdd<u32>(&ecs, e1) = 12;
//             *ecsAdd<u32>(&ecs, e2) = 42;
//             *ecsAdd<u32>(&ecs, e3) = 100;
//             TEST(ecsCount<u32>(&ecs) == 3);
//             TEST(ecsCount<u64>(&ecs) == 0);
//
//             bool hasUnknown = false;
//             bool has12 = false;
//             bool has42 = false;
//             bool has100 = false;
//             ecsForEach<u32>(&ecs, [&](Entity e, u32* c)
//             {
//                 switch (*c)
//                 {
//                     case 12:
//                         has12 = e.handle.id == e1.handle.id;
//                         break;
//                     case 42:
//                         has42 = e.handle.id == e2.handle.id;
//                         break;
//                     case 100:
//                         has100 = e.handle.id == e3.handle.id;
//                         break;
//                     default:
//                         hasUnknown = true;
//                         break;
//                 }
//             });
//             TEST(has12);
//             TEST(has42);
//             TEST(has100);
//             TEST(!hasUnknown);
//         }
//
//         {
//             *ecsAdd<u64>(&ecs, e2) = 2042;
//             *ecsAdd<u64>(&ecs, e3) = 2100;
//             TEST(ecsCount<u32>(&ecs) == 3);
//             TEST(ecsCount<u64>(&ecs) == 2);
//
//             bool hasUnknown = false;
//             bool has12 = false;
//             bool has42 = false;
//             bool has100 = false;
//             bool has2042 = false;
//             bool has2100 = false;
//             ecsForEach<u32, u64>(&ecs, [&](Entity e, u32* comp32, u64* comp64)
//             {
//                 switch (*comp32)
//                 {
//                     case 12:
//                         has12 = e.handle.id == e1.handle.id;
//                         break;
//                     case 42:
//                         has42 = e.handle.id == e2.handle.id;
//                         break;
//                     case 100:
//                         has100 = e.handle.id == e3.handle.id;
//                         break;
//                     default:
//                         hasUnknown = true;
//                         break;
//                 }
//                 switch (*comp64)
//                 {
//                     case 2042:
//                         has2042 = e.handle.id == e2.handle.id;
//                         break;
//                     case 2100:
//                         has2100 = e.handle.id == e3.handle.id;
//                         break;
//                     default:
//                         hasUnknown = true;
//                         break;
//                 }
//             });
//             TEST(!has12);
//             TEST(has42);
//             TEST(has100);
//             TEST(has2042);
//             TEST(has2100);
//             TEST(!hasUnknown);
//         }
//
//         {
//             ecsDespawn(&ecs, e1);
//             TEST(ecsCount<u32>(&ecs) == 2);
//             TEST(ecsCount<u64>(&ecs) == 2);
//
//             bool hasUnknown = false;
//             bool has12 = false;
//             bool has42 = false;
//             bool has100 = false;
//             ecsForEach<u32>(&ecs, [&](Entity e, u32* c)
//             {
//                 switch (*c)
//                 {
//                     case 12:
//                         has12 = e.handle.id == e1.handle.id;
//                         break;
//                     case 42:
//                         has42 = e.handle.id == e2.handle.id;
//                         break;
//                     case 100:
//                         has100 = e.handle.id == e3.handle.id;
//                         break;
//                     default:
//                         hasUnknown = true;
//                         break;
//                 }
//             });
//             TEST(!has12);
//             TEST(has42);
//             TEST(has100);
//             TEST(!hasUnknown);
//         }
//
//         {
//             ecsDespawn(&ecs, e2);
//             TEST(ecsCount<u32>(&ecs) == 1);
//             TEST(ecsCount<u64>(&ecs) == 1);
//         }
//     }
//
//     // Ecs concurrency
//     {
//         Ecs ecs = ecsCreate();
//         HG_DEFER(ecsDestroy(&ecs));
//
//         HG_ECS_REGISTER_TYPE(&ecs, u32);
//         HG_ECS_REGISTER_TYPE(&ecs, u64);
//
//         {
//             for (u32 i = 0; i < 4; ++i)
//             {
//                 Entity e = ecsSpawn(&ecs);
//                 switch (i % 3)
//                 {
//                     case 0:
//                         *ecsAdd<u32>(&ecs, e) = 12;
//                         *ecsAdd<u64>(&ecs, e) = 42;
//                         break;
//                     case 1:
//                         *ecsAdd<u32>(&ecs, e) = 12;
//                         break;
//                     case 2:
//                         *ecsAdd<u64>(&ecs, e) = 42;
//                         break;
//                 }
//             }
//
//             bool success;
//             ecsForPar<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 *c += 4;
//             });
//             success = true;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != 16)
//                     success = false;
//             });
//             TEST(success);
//
//             ecsForPar<u64>(&ecs, [&](Entity, u64* c)
//             {
//                 *c += 3;
//             });
//             success = true;
//             ecsForEach<u64>(&ecs, [&](Entity, u64* c)
//             {
//                 if (*c != 45)
//                     success = false;
//             });
//             TEST(success);
//
//             ecsForPar<u32, u64>(&ecs, [&](Entity, u32* c32, u64* c64)
//             {
//                 *c64 -= *c32;
//             });
//             success = true;
//             ecsForEach<u64>(&ecs, [&](Entity e, u64* c)
//             {
//                 if (ecsHas<u32>(&ecs, e))
//                 {
//                     if (*c != 29)
//                         success = false;
//                 } else {
//                     if (*c != 45)
//                         success = false;
//                 }
//             });
//             TEST(success);
//         }
//     }
//
//     // Ecs sort
//     {
//         Ecs ecs = ecsCreate();
//         HG_DEFER(ecsDestroy(&ecs));
//
//         HG_ECS_REGISTER_TYPE(&ecs, u32);
//         HG_ECS_REGISTER_TYPE(&ecs, u64);
//
//         auto comparison = [](void*, Ecs* ecs, Entity lhs, Entity rhs)
//         {
//             return *ecsGet<u32>(ecs, lhs) < *ecsGet<u32>(ecs, rhs);
//         };
//
//         {
//             *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = 42;
//
//             ecsSort<u32>(&ecs, nullptr, comparison);
//
//             bool success = true;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != 42)
//                     success = false;
//             });
//             TEST(success);
//
//             ecsReset(&ecs);
//         }
//
//         {
//             u32 smallScramble1[]{1, 0};
//             for (u32 i = 0; i < std::size(smallScramble1); ++i)
//             {
//                 *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = smallScramble1[i];
//             }
//
//             {
//                 ecsSort<u32>(&ecs, nullptr, comparison);
//
//                 bool success = true;
//                 u32 elem = 0;
//                 ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//                 {
//                     if (*c != elem)
//                         success = false;
//                     ++elem;
//                 });
//                 TEST(success);
//             }
//
//             {
//                 ecsSort<u32>(&ecs, nullptr, comparison);
//
//                 bool success = true;
//                 u32 elem = 0;
//                 ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//                 {
//                     if (*c != elem)
//                         success = false;
//                     ++elem;
//                 });
//                 TEST(success);
//             }
//
//             ecsReset(&ecs);
//         }
//
//         {
//             u32 mediumScramble1[]{8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
//             for (u32 i = 0; i < std::size(mediumScramble1); ++i)
//             {
//                 *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = mediumScramble1[i];
//             }
//             ecsSort<u32>(&ecs, nullptr, comparison);
//
//             bool success = true;
//             u32 elem = 0;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != elem)
//                     success = false;
//                 ++elem;
//             });
//             TEST(success);
//
//             ecsReset(&ecs);
//         }
//
//         {
//             u32 mediumScramble2[]{3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
//             for (u32 i = 0; i < std::size(mediumScramble2); ++i)
//             {
//                 *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = mediumScramble2[i];
//             }
//             ecsSort<u32>(&ecs, nullptr, comparison);
//             ecsSort<u32>(&ecs, nullptr, comparison);
//
//             bool success = true;
//             u32 elem = 0;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != elem)
//                     success = false;
//                 ++elem;
//             });
//             TEST(success);
//
//             ecsReset(&ecs);
//         }
//
//         {
//             for (u32 i = 127; i < 128; --i)
//             {
//                 *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = i;
//             }
//             ecsSort<u32>(&ecs, nullptr, comparison);
//
//             bool success = true;
//             u32 elem = 0;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != elem)
//                     success = false;
//                 ++elem;
//             });
//             TEST(success);
//
//             ecsReset(&ecs);
//         }
//
//         {
//             for (u32 i = 127; i < 128; --i)
//             {
//                 *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = i / 2;
//             }
//             ecsSort<u32>(&ecs, nullptr, comparison);
//             ecsSort<u32>(&ecs, nullptr, comparison);
//
//             bool success = true;
//             u32 elem = 0;
//             ecsForEach<u32>(&ecs, [&](Entity, u32* c)
//             {
//                 if (*c != elem / 2)
//                     success = false;
//                 ++elem;
//             });
//             TEST(success);
//
//             ecsReset(&ecs);
//         }
//     }
//
//     // Ecs serialization
//     {
//         ArenaScope arena = getScratch();
//
//         SerialNode* scene{};
//
//         {
//             Ecs ecs = ecsCreate();
//             HG_DEFER(ecsDestroy(&ecs));
//
//             HG_ECS_REGISTER_TYPE(&ecs, Node);
//             HG_ECS_REGISTER_TYPE(&ecs, u32);
//
//             Entity root = ecsSpawn(&ecs);
//             Entity a = ecsSpawn(&ecs);
//             Entity b = ecsSpawn(&ecs);
//
//             nodeAdd(&ecs, root);
//             nodeAdd(&ecs, a);
//             nodeAdd(&ecs, b);
//
//             *ecsAdd<u32>(&ecs, a) = 12;
//             *ecsAdd<u32>(&ecs, b) = 42;
//
//             nodeAddChild(&ecs, root, b);
//             nodeAddChild(&ecs, root, a);
//
//             Serializer s = serialWriter(arena);
//             serialize(&s, &ecs);
//             scene = s.current;
//         }
//
//         {
//             Ecs ecs = ecsCreate();
//             HG_DEFER(ecsDestroy(&ecs));
//
//             HG_ECS_REGISTER_TYPE(&ecs, Node);
//             HG_ECS_REGISTER_TYPE(&ecs, u32);
//
//             Serializer s = serialReader(arena, scene);
//             serialize(&s, &ecs);
//
//             Entity root = ecsEntities<Node>(&ecs)[0];
//
//             TEST(ecsHas<Node>(&ecs, root));
//             Node* rootNode = ecsGet<Node>(&ecs, root);
//             TEST(rootNode->parent.handle == handleNull);
//             TEST(rootNode->nextSibling.handle == handleNull);
//             TEST(rootNode->prevSibling.handle == handleNull);
//             TEST(rootNode->firstChild.handle != handleNull);
//
//             Entity a = rootNode->firstChild;
//             TEST(a.handle != handleNull);
//
//             TEST(ecsHas<Node>(&ecs, a));
//             Node* aNode = ecsGet<Node>(&ecs, a);
//             TEST(aNode->parent == root);
//             TEST(aNode->prevSibling.handle == handleNull);
//             TEST(aNode->nextSibling.handle != handleNull);
//             TEST(aNode->firstChild.handle == handleNull);
//
//             Entity b = aNode->nextSibling;
//             TEST(b.handle != handleNull);
//
//             TEST(ecsHas<Node>(&ecs, b));
//             Node* bNode = ecsGet<Node>(&ecs, b);
//             TEST(bNode->parent == root);
//             TEST(bNode->prevSibling == a);
//             TEST(bNode->nextSibling.handle == handleNull);
//             TEST(bNode->firstChild.handle == handleNull);
//
//             TEST(ecsHas<u32>(&ecs, a));
//             TEST(*ecsGet<u32>(&ecs, a) == 12);
//
//             TEST(ecsHas<u32>(&ecs, b));
//             TEST(*ecsGet<u32>(&ecs, b) == 42);
//         }
//     }
//
//     // Node
//     {
//         Ecs ecs = ecsCreate();
//         HG_DEFER(ecsDestroy(&ecs));
//
//         HG_ECS_REGISTER_TYPE(&ecs, Node);
//
//         {
//             Entity a = ecsSpawn(&ecs);
//             Entity b = ecsSpawn(&ecs);
//             Entity aa = ecsSpawn(&ecs);
//             Entity ab = ecsSpawn(&ecs);
//
//             *nodeAdd(&ecs, a) = {};
//             *nodeAdd(&ecs, b) = {};
//             *nodeAdd(&ecs, aa) = {};
//             *nodeAdd(&ecs, ab) = {};
//
//             nodeAddChild(&ecs, a, aa);
//             nodeAddChild(&ecs, a, ab);
//
//             TEST(ecsAlive(&ecs, a));
//             TEST(ecsAlive(&ecs, b));
//             TEST(ecsAlive(&ecs, aa));
//             TEST(ecsAlive(&ecs, ab));
//
//             nodeDestroy(&ecs, a);
//
//             TEST(!ecsAlive(&ecs, a));
//             TEST(ecsAlive(&ecs, b));
//             TEST(!ecsAlive(&ecs, aa));
//             TEST(!ecsAlive(&ecs, ab));
//
//             ecsDespawn(&ecs, b);
//         }
//
//         {
//             Entity a = ecsSpawn(&ecs);
//             Entity b = ecsSpawn(&ecs);
//             Entity aa = ecsSpawn(&ecs);
//             Entity ab = ecsSpawn(&ecs);
//             Entity aba = ecsSpawn(&ecs);
//             Entity abb = ecsSpawn(&ecs);
//
//             *nodeAdd(&ecs, a) = {};
//             *nodeAdd(&ecs, b) = {};
//             *nodeAdd(&ecs, aa) = {};
//             *nodeAdd(&ecs, ab) = {};
//             *nodeAdd(&ecs, aba) = {};
//             *nodeAdd(&ecs, abb) = {};
//
//             nodeAddChild(&ecs, ab, aba);
//             nodeAddChild(&ecs, ab, abb);
//             nodeAddChild(&ecs, a, aa);
//             nodeAddChild(&ecs, a, ab);
//
//             TEST(ecsAlive(&ecs, a));
//             TEST(ecsAlive(&ecs, b));
//             TEST(ecsAlive(&ecs, aa));
//             TEST(ecsAlive(&ecs, ab));
//             TEST(ecsAlive(&ecs, aba));
//             TEST(ecsAlive(&ecs, abb));
//
//             nodeDestroy(&ecs, ab);
//
//             TEST(ecsAlive(&ecs, a));
//             TEST(ecsAlive(&ecs, b));
//             TEST(ecsAlive(&ecs, aa));
//             TEST(!ecsAlive(&ecs, ab));
//             TEST(!ecsAlive(&ecs, aba));
//             TEST(!ecsAlive(&ecs, abb));
//
//             nodeDestroy(&ecs, a);
//
//             TEST(!ecsAlive(&ecs, a));
//             TEST(ecsAlive(&ecs, b));
//             TEST(!ecsAlive(&ecs, aa));
//             TEST(!ecsAlive(&ecs, ab));
//             TEST(!ecsAlive(&ecs, aba));
//             TEST(!ecsAlive(&ecs, abb));
//
//             ecsDespawn(&ecs, b);
//         }
//     }
//
//    HG_WARN("Mesh test : TODO\n");

    HG_LOG("All tests passed in %fms\n", timer.tick() * 1000.0f);
    return 0;
}

