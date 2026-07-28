#include "tests.hpp"

void testStrings()
{
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
}

