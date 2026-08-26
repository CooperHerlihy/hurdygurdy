#include "tests.hpp"
#include "hg/hash.hpp"
#include "hg/strings.hpp"

void testHash()
{
    // ============================================================================
    // Hash consistency for string representations
    // ============================================================================
    //
    // hash() is specialized for StringView, String, StringBuilder, const char*,
    // and const char[N]. All must agree for equal content so heterogeneous map
    // and set lookups work.

    // Non-empty content agrees across every representation
    {
        ArenaScope arena = getScratch();
        const char* cstr = "hello";
        StringView sv{cstr};
        String s = String::create(sv);
        StringBuilder sb{arena, sv};

        u64 hSv = hash(sv);
        u64 hStr = hash(s);
        u64 hCstr = hash(cstr);
        u64 hSb = hash(sb);
        u64 hLit = hash("hello");

        TEST(hSv == hStr);
        TEST(hSv == hCstr);
        TEST(hSv == hSb);
        TEST(hSv == hLit);
    }

    // Empty string agrees across representations
    {
        StringView sv{};
        String s = String::create(sv);
        u64 hSv = hash(sv);
        u64 hStr = hash(s);
        u64 hLit = hash("");
        TEST(hSv == hStr);
        TEST(hSv == hLit);
    }

    // C-array representation equals explicit pointer representation
    {
        char buf[] = "abc";
        u64 hArr = hash(buf);
        u64 hPtr = hash((const char*)buf);
        TEST(hArr == hPtr);
    }

    // StringView with explicit length agrees with null-terminated form
    {
        const char* data = "hello world";
        StringView sv{data, 5}; // "hello"
        u64 hLen = hash(sv);
        u64 hTerm = hash(StringView{"hello"});
        TEST(hLen == hTerm);
    }

    // Distinct content hashes differently
    {
        TEST(hash(StringView{"abc"}) != hash(StringView{"abd"}));
        TEST(hash(StringView{""}) != hash(StringView{"a"}));
    }

    // ============================================================================
    // Integer hashing sanity
    // ============================================================================
    {
        TEST(hash(static_cast<u32>(5)) == hash(static_cast<u32>(5)));
        TEST(hash(static_cast<u32>(5)) != hash(static_cast<u32>(6)));
        TEST(hash(static_cast<i32>(-1)) == hash(static_cast<i32>(-1)));
        TEST(hash(static_cast<i32>(-1)) != hash(static_cast<i32>(1)));
    }
}
