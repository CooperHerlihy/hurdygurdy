#include "tests.hpp"
#include "hg/strings.hpp"

using namespace hg;

TEST(testStringViewEqual)
{
    StringView a{"hello"};
    StringView b{"hello"};
    ASSERT(a == b);
    ASSERT(!(a != b));
}

TEST(testStringViewDifferent)
{
    StringView a{"hello"};
    StringView b{"world"};
    ASSERT(a != b);
    ASSERT(!(a == b));
}

TEST(testStringViewEmptyEqual)
{
    StringView a{};
    StringView b{};
    ASSERT(a == b);
}

TEST(testStringViewEmptyVsNonEmpty)
{
    StringView a{};
    StringView b{"x"};
    ASSERT(a != b);
}

TEST(testStringViewSameContent)
{
    const char* s1 = "abcdef";
    const char* s2 = "abcdef";
    StringView a{s1, 3};
    StringView b{s2, 3};
    ASSERT(a == b);
}

TEST(testStringViewDifferentLengths)
{
    StringView a{"hello", 5};
    StringView b{"hello world", 5};
    ASSERT(a == b);
}

TEST(testStringViewNullptr)
{
    const char* nullStr = nullptr;
    StringView sv{nullStr};
    ASSERT(sv.chars == nullptr);
    ASSERT(sv.length == 0);

    StringView empty{};
    ASSERT(sv == empty);
}

TEST(testStringViewZeroLength)
{
    const char* data = "hello";
    StringView sv{data, u64{0}};
    ASSERT(sv.length == 0);
    ASSERT(StringView{} == sv);
}

TEST(testStringViewBeginEnd)
{
    const char* data = "hello world";
    StringView sv{data + 6, data + 11};
    ASSERT(sv == "world");
}

TEST(testStringViewBeginEndEqual)
{
    const char* data = "hello";
    StringView sv{data, data};
    ASSERT(sv.length == 0);
}

TEST(testStringViewIndex)
{
    StringView sv{"hello"};
    ASSERT(sv[0] == 'h');
    ASSERT(sv[1] == 'e');
    ASSERT(sv[2] == 'l');
    ASSERT(sv[3] == 'l');
    ASSERT(sv[4] == 'o');
}

TEST(testStringViewRangeFor)
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
    ASSERT(StringView{result} == "abc");
}

TEST(testStringViewLongString)
{
    StringView sv{"this is a fairly long string that should work fine"};
    ASSERT(sv.length == 50);
    ASSERT(sv == "this is a fairly long string that should work fine");
}

TEST(testCStringNormal)
{
    ArenaScope arena = getScratch();
    char* c = cString(arena, "hello");
    ASSERT(c != nullptr);
    ASSERT(c[0] == 'h');
    ASSERT(c[1] == 'e');
    ASSERT(c[2] == 'l');
    ASSERT(c[3] == 'l');
    ASSERT(c[4] == 'o');
    ASSERT(c[5] == '\0');
}

TEST(testCStringEmpty)
{
    ArenaScope arena = getScratch();
    char* c = cString(arena, "");
    ASSERT(c != nullptr);
    ASSERT(c[0] == '\0');
}

TEST(testCStringNullData)
{
    ArenaScope arena = getScratch();
    StringView empty{};
    char* c = cString(arena, empty);
    ASSERT(c != nullptr);
    ASSERT(c[0] == '\0');
}

TEST(testCStringPartialView)
{
    ArenaScope arena = getScratch();
    StringView sv{"hello world", 5};
    char* c = cString(arena, sv);
    ASSERT(c != nullptr);
    ASSERT(c[0] == 'h');
    ASSERT(c[5] == '\0');
    ASSERT(StringView{c} == "hello");
}

TEST(testStringBuilderDefault)
{
    StringBuilder sb{};
    ASSERT(sb.chars == nullptr);
    ASSERT(sb.length == 0);
    ASSERT(sb.arena == nullptr);
}

TEST(testStringBuilderFromString)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    ASSERT(sb.length == 5);
    ASSERT(sb == "hello");
}

TEST(testStringBuilderEmpty)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena};
    ASSERT(sb.length == 0);
    ASSERT(sb == "");
}

TEST(testStringBuilderFromStringView)
{
    ArenaScope arena = getScratch();
    StringView sv{"world"};
    StringBuilder sb{arena, sv};
    ASSERT(sb == "world");
}

TEST(testStringBuilderFromPartialView)
{
    ArenaScope arena = getScratch();
    StringView sv{"hello world", 5};
    StringBuilder sb{arena, sv};
    ASSERT(sb == "hello");
}

TEST(testStringBuilderAppendString)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    sb.append(" world");
    ASSERT(sb == "hello world");
}

TEST(testStringBuilderAppendToEmpty)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena};
    sb.append("hello");
    ASSERT(sb == "hello");
}

TEST(testStringBuilderAppendChar)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    sb.append('!');
    ASSERT(sb == "hello!");
}

TEST(testStringBuilderAppendCharEmpty)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena};
    sb.append('x');
    ASSERT(sb == "x");
}

TEST(testStringBuilderAppendMultiple)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "a"};
    sb.append("b");
    sb.append("c");
    sb.append("d");
    sb.append("e");
    ASSERT(sb == "abcde");
}

TEST(testStringBuilderPrependString)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "world"};
    sb.prepend("hello ");
    ASSERT(sb == "hello world");
}

TEST(testStringBuilderPrependEmpty)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena};
    sb.prepend("hello");
    ASSERT(sb == "hello");
}

TEST(testStringBuilderPrependChar)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "ello"};
    sb.prepend('h');
    ASSERT(sb == "hello");
}

TEST(testStringBuilderInsertBeginning)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "world"};
    sb.insert(0, "hello ");
    ASSERT(sb == "hello world");
}

TEST(testStringBuilderInsertMiddle)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello world"};
    sb.insert(5, ",");
    ASSERT(sb == "hello, world");
}

TEST(testStringBuilderInsertEnd)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    sb.insert(5, " world");
    ASSERT(sb == "hello world");
}

TEST(testStringBuilderInsertChar)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello world"};
    sb.insert(5, ',');
    ASSERT(sb == "hello, world");
}

TEST(testStringBuilderInsertEmpty)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    sb.insert(3, "");
    ASSERT(sb == "hello");
}

TEST(testStringBuilderEquality)
{
    ArenaScope arena = getScratch();
    StringBuilder a{arena, "hello"};
    StringBuilder b{arena, "hello"};
    StringBuilder c{arena, "world"};

    ASSERT(a == b);
    ASSERT(a != c);
}

TEST(testStringBuilderEqualityStringView)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    ASSERT(sb == StringView{"hello"});
}

TEST(testStringBuilderIndex)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    ASSERT(sb[0] == 'h');
    ASSERT(sb[4] == 'o');
}

TEST(testStringBuilderLarge)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena};
    for (u32 i = 0; i < 100; ++i)
        sb.append('x');
    ASSERT(sb.length == 100);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(sb[i] == 'x');
}

TEST(testStringCreate)
{
    String s = String::create("hello");
    ASSERT(s.length == 5);
    ASSERT(s == "hello");
    ASSERT(s[0] == 'h');
    ASSERT(s[4] == 'o');
}

TEST(testStringCreateEmpty)
{
    String s = String::create("");
    ASSERT(s.length == 0);
    ASSERT(s == "");
}

TEST(testStringCreatePartialView)
{
    StringView sv{"hello world", 5};
    String s = String::create(sv);
    ASSERT(s == "hello");
}

TEST(testStringMoveConstruct)
{
    String a = String::create("hello");
    String b{std::move(a)};
    ASSERT(a.chars == nullptr);
    ASSERT(a.length == 0);
    ASSERT(b == "hello");
}

TEST(testStringMoveAssign)
{
    String a = String::create("hello");
    String b = String::create("world");
    b = std::move(a);
    ASSERT(a.chars == nullptr);
    ASSERT(a.length == 0);
    ASSERT(b == "hello");
}

TEST(testStringEquality)
{
    String a = String::create("hello");
    String b = String::create("hello");
    String c = String::create("world");
    ASSERT(a == b);
    ASSERT(!(a == c));
    ASSERT(a != c);
}

TEST(testStringEqualityStringView)
{
    String s = String::create("hello");
    ASSERT(s == StringView{"hello"});
}

TEST(testStringEqualityCString)
{
    String s = String::create("hello");
    ASSERT(s == "hello");
}

TEST(testStringLifecycleOwnership)
{
    String a = String::create("hello");
    {
        String b = std::move(a);
        ASSERT(a.chars == nullptr);
        ASSERT(b == "hello");
    }
    ASSERT(a.chars == nullptr);
}

TEST(testStringLifecycleChainMoves)
{
    String a = String::create("alpha");
    String b = String::create("beta");
    String c = std::move(a);
    b = std::move(c);
    ASSERT(b == "alpha");
    ASSERT(a.chars == nullptr);
    ASSERT(c.chars == nullptr);
}

TEST(testStringLifecycleEmptyMove)
{
    String empty = String::create("");
    ASSERT(empty == "");
    String moved = std::move(empty);
    ASSERT(empty.chars == nullptr);
    ASSERT(moved == "");
}

TEST(testStringLifecycleMovedFromAssign)
{
    String a = String::create("hello");
    String b = String::create("world");
    a = std::move(b);
    ASSERT(a == "world");
    ASSERT(b.chars == nullptr);
    b = std::move(a);
    ASSERT(b == "world");
    ASSERT(a.chars == nullptr);
}

TEST(testIsWhitespace)
{
    ASSERT(isWhitespace(' '));
    ASSERT(isWhitespace('\t'));
    ASSERT(isWhitespace('\n'));
    ASSERT(isWhitespace('\r'));
    ASSERT(!isWhitespace('a'));
    ASSERT(!isWhitespace('0'));
    ASSERT(!isWhitespace('.'));
    ASSERT(!isWhitespace('\0'));
    ASSERT(!isWhitespace('_'));
}

TEST(testIsNumeral)
{
    ASSERT(isNumeral('0'));
    ASSERT(isNumeral('1'));
    ASSERT(isNumeral('2'));
    ASSERT(isNumeral('3'));
    ASSERT(isNumeral('4'));
    ASSERT(isNumeral('5'));
    ASSERT(isNumeral('6'));
    ASSERT(isNumeral('7'));
    ASSERT(isNumeral('8'));
    ASSERT(isNumeral('9'));
    ASSERT(!isNumeral('a'));
    ASSERT(!isNumeral('z'));
    ASSERT(!isNumeral('A'));
    ASSERT(!isNumeral('Z'));
    ASSERT(!isNumeral('.'));
    ASSERT(!isNumeral('-'));
    ASSERT(!isNumeral('+'));
    ASSERT(!isNumeral(' '));
    ASSERT(!isNumeral('\0'));
    ASSERT(!isNumeral('/'));
    ASSERT(!isNumeral(':'));
}

TEST(testIsInteger)
{
    ASSERT(isInteger("0"));
    ASSERT(isInteger("1"));
    ASSERT(isInteger("2"));
    ASSERT(isInteger("3"));
    ASSERT(isInteger("4"));
    ASSERT(isInteger("5"));
    ASSERT(isInteger("6"));
    ASSERT(isInteger("7"));
    ASSERT(isInteger("8"));
    ASSERT(isInteger("9"));
    ASSERT(isInteger("42"));
    ASSERT(isInteger("100"));
    ASSERT(isInteger("1234567890"));
    ASSERT(isInteger("+12"));
    ASSERT(isInteger("-12"));
    ASSERT(isInteger("+0"));
    ASSERT(isInteger("-0"));
    ASSERT(isInteger("00"));
    ASSERT(isInteger("00042"));
    ASSERT(!isInteger(""));
    ASSERT(!isInteger("hello"));
    ASSERT(!isInteger("12a"));
    ASSERT(!isInteger("a12"));
    ASSERT(!isInteger("1.0"));
    ASSERT(!isInteger("--12"));
    ASSERT(!isInteger("+-12"));
    ASSERT(!isInteger("12-"));
    ASSERT(!isInteger("12+"));
    ASSERT(!isInteger("+"));
    ASSERT(!isInteger("-"));
}

TEST(testIsFloat)
{
    ASSERT(isFloat("0.0"));
    ASSERT(isFloat("1.0"));
    ASSERT(isFloat("2.5"));
    ASSERT(isFloat("99.99"));
    ASSERT(isFloat(".1"));
    ASSERT(isFloat(".5"));
    ASSERT(isFloat(".12345"));
    ASSERT(isFloat("1."));
    ASSERT(isFloat("100."));
    ASSERT(isFloat("+1.0"));
    ASSERT(isFloat("-1.0"));
    ASSERT(isFloat("+.5"));
    ASSERT(isFloat("-.5"));
    ASSERT(isFloat("1e3"));
    ASSERT(isFloat("1e+3"));
    ASSERT(isFloat("1e-3"));
    ASSERT(isFloat("1.5e3"));
    ASSERT(isFloat(".5e3"));
    ASSERT(isFloat("1.0f"));
    ASSERT(isFloat("+10.f"));
    ASSERT(isFloat("-999.999f"));
    ASSERT(isFloat("1e3f"));
    ASSERT(isFloat("1.e3f"));
    ASSERT(isFloat(".1e3"));
    ASSERT(!isFloat("1"));
    ASSERT(!isFloat("42"));
    ASSERT(!isFloat("+12"));
    ASSERT(!isFloat("-12"));
    ASSERT(!isFloat(""));
    ASSERT(!isFloat("hello"));
    ASSERT(!isFloat("1.0ff"));
    ASSERT(!isFloat("1.0.0"));
    ASSERT(!isFloat("1e3.0"));
    ASSERT(!isFloat("--1.0"));
    ASSERT(!isFloat("1ef"));
    ASSERT(!isFloat("e1"));
    ASSERT(!isFloat("."));
    ASSERT(!isFloat("e"));
}

TEST(testStringToInteger)
{
    ASSERT(stringToInteger("0") == 0);
    ASSERT(stringToInteger("1") == 1);
    ASSERT(stringToInteger("2") == 2);
    ASSERT(stringToInteger("3") == 3);
    ASSERT(stringToInteger("4") == 4);
    ASSERT(stringToInteger("5") == 5);
    ASSERT(stringToInteger("6") == 6);
    ASSERT(stringToInteger("7") == 7);
    ASSERT(stringToInteger("8") == 8);
    ASSERT(stringToInteger("9") == 9);
    ASSERT(stringToInteger("42") == 42);
    ASSERT(stringToInteger("100") == 100);
    ASSERT(stringToInteger("1234567890") == 1234567890);
    ASSERT(stringToInteger("+12") == 12);
    ASSERT(stringToInteger("-12") == -12);
    ASSERT(stringToInteger("+0") == 0);
    ASSERT(stringToInteger("-0") == 0);
    ASSERT(stringToInteger("00") == 0);
    ASSERT(stringToInteger("00042") == 42);
    ASSERT(stringToInteger("2147483647") == 2147483647);
    ASSERT(stringToInteger("2147483648") == 2147483648);
    ASSERT(stringToInteger("-2147483648") == -2147483648);
}

TEST(testStringToFloat)
{
    ASSERT(std::abs(stringToFloat("0.0") - 0.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("99.99") - 99.99) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".1") - 0.1) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat(".5") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("100.") - 100.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("+1.0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-1.0") + 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("+.5") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-.5") + 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e+3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e-3") - 0.001) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1.5e3") - 1500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat(".5e3") - 500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.0f") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("+10.f") - 10.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-999.999f") + 999.999) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1e3f") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("0.0")) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat(".0")) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("0.")) <= FLT_EPSILON);
}

TEST(testIntegerToString)
{
    ArenaScope arena = getScratch();
    ASSERT(integerToString(arena, 0) == "0");
    ASSERT(integerToString(arena, 1) == "1");
    ASSERT(integerToString(arena, 9) == "9");
    ASSERT(integerToString(arena, -1) == "-1");
    ASSERT(integerToString(arena, -9) == "-9");
    ASSERT(integerToString(arena, 42) == "42");
    ASSERT(integerToString(arena, 100) == "100");
    ASSERT(integerToString(arena, 1234567890) == "1234567890");
    ASSERT(integerToString(arena, -42) == "-42");
    ASSERT(integerToString(arena, -1000000) == "-1000000");
    ASSERT(integerToString(arena, 9000000000000000LL) == "9000000000000000");
}

TEST(testFloatToString)
{
    ArenaScope arena = getScratch();
    ASSERT(floatToString(arena, 0.0, 1) == "0.0");
    ASSERT(floatToString(arena, 1.0, 0) == "1.");
    ASSERT(floatToString(arena, 2.0, 1) == "2.0");
    ASSERT(floatToString(arena, 3.0, 2) == "3.00");
    ASSERT(floatToString(arena, 4.0, 3) == "4.000");
    ASSERT(floatToString(arena, -1.0, 1) == "-1.0");
    ASSERT(floatToString(arena, -2.0, 2) == "-2.00");
    ASSERT(floatToString(arena, 0.5, 1) == "0.5");
    ASSERT(floatToString(arena, 3.14, 2) == "3.14");
    ASSERT(floatToString(arena, -0.5, 1) == "-0.5");
    ASSERT(floatToString(arena, 0.0, 0) == "0.0");
    ASSERT(floatToString(arena, 100.0, 0) == "100.");
}

TEST(testStringViewOperators)
{
    StringView a{"hello"};
    StringView b{"hello"};
    StringView c{"world"};
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(b != c);
}

TEST(testStringViewBeginEndRangeFor)
{
    const char* data = "hello";
    StringView sv{data};
    char result[6]{};
    u64 i = 0;
    for (auto it = sv.begin(); it != sv.end(); ++it)
    {
        result[i] = *it;
        ++i;
    }
    result[i] = '\0';
    ASSERT(StringView{result} == "hello");
}

TEST(testStringBuilderPrependMultiple)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "c"};
    sb.prepend("b");
    sb.prepend("a");
    ASSERT(sb == "abc");
}

TEST(testStringBuilderInsertAtBeginning)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "world"};
    sb.insert(0, "hello ");
    ASSERT(sb == "hello world");
}

TEST(testStringBuilderInsertBeyondEnd)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    sb.insert(5, " world");
    ASSERT(sb == "hello world");
}

TEST(testStringCreateLong)
{
    char data[300]{};
    for (u64 i = 0; i < 299; ++i)
        data[i] = 'a' + static_cast<char>(i % 26);
    String s = String::create(StringView{data, 299});
    ASSERT(s.length == 299);
    for (u64 i = 0; i < 299; ++i)
        ASSERT(s[i] == 'a' + static_cast<char>(i % 26));
}

TEST(testStringEqualityOperators)
{
    String a = String::create("hello");
    String b = String::create("hello");
    String c = String::create("world");
    ASSERT(a == b);
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(!(a == c));
}

TEST(testStringViewComparison)
{
    StringView a{"abc"};
    StringView b{"abcd"};
    StringView c{"ab"};
    ASSERT(a != b);
    ASSERT(a != c);
    ASSERT(b != c);
    ASSERT(a == StringView{"abc"});
}

TEST(testIsWhitespaceTabNewline)
{
    ASSERT(isWhitespace('\t'));
    ASSERT(isWhitespace('\n'));
    ASSERT(isWhitespace('\r'));
    ASSERT(!isWhitespace('a'));
    ASSERT(!isWhitespace('\0'));
}

TEST(testIsIntegerSingleDigit)
{
    ASSERT(isInteger("0"));
    ASSERT(isInteger("1"));
    ASSERT(isInteger("5"));
    ASSERT(isInteger("9"));
    ASSERT(isInteger("+3"));
    ASSERT(isInteger("-7"));
}

TEST(testIsFloatScientificNotation)
{
    ASSERT(isFloat("1e3"));
    ASSERT(isFloat("1e+3"));
    ASSERT(isFloat("1e-3"));
    ASSERT(isFloat("1.5e3"));
    ASSERT(isFloat("1.5e+3"));
    ASSERT(isFloat("1.5e-3"));
    ASSERT(isFloat(".5e3"));
    ASSERT(!isFloat("e3"));
    ASSERT(!isFloat("1e"));
    ASSERT(!isFloat("1e3.0"));
}

TEST(testStringToIntegerEdgeCases)
{
    ASSERT(stringToInteger("2147483647") == 2147483647);
    ASSERT(stringToInteger("-2147483648") == -2147483648);
    ASSERT(stringToInteger("0") == 0);
    ASSERT(stringToInteger("-0") == 0);
    ASSERT(stringToInteger("+0") == 0);
}

TEST(testFloatToStringPrecision)
{
    ArenaScope arena = getScratch();
    ASSERT(floatToString(arena, 3.14159, 2) == "3.14");
    ASSERT(floatToString(arena, 3.14159, 4) == "3.1415");
    ASSERT(floatToString(arena, 3.14159, 0) == "3.");
    ASSERT(floatToString(arena, 3.14159, 6) == "3.141590");
}

TEST(testIntegerToStringNegative)
{
    ArenaScope arena = getScratch();
    ASSERT(integerToString(arena, -1) == "-1");
    ASSERT(integerToString(arena, -42) == "-42");
    ASSERT(integerToString(arena, -100) == "-100");
    ASSERT(integerToString(arena, -999999) == "-999999");
}
