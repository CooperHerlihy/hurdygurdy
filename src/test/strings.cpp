#include "tests.hpp"
#include "hg/strings.hpp"

using namespace hg;

TEST(testStringViewEquality)
{
    StringView a{"hello"};
    StringView b{"hello"};
    StringView c{"world"};

    ASSERT(a == b);
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(!(a == c));
    ASSERT(b != c);

    StringView emptyA{};
    StringView emptyB{};
    ASSERT(emptyA == emptyB);
    ASSERT(!(emptyA != emptyB));
    ASSERT(emptyA != a);
    ASSERT(a != emptyA);

    const char* s1 = "abcdef";
    const char* s2 = "abcdef";
    ASSERT((StringView{s1, 3} == StringView{s2, 3}));
    ASSERT((StringView{"hello"} == StringView{"hello world", 5}));
    ASSERT(StringView{"hello"} != StringView{"hello world"});

    StringView shorter{"abc"};
    StringView longer{"abcd"};
    ASSERT(shorter != longer);
    ASSERT(longer != shorter);
    ASSERT(shorter != c);
}

TEST(testStringViewNullptr)
{
    StringView sv{nullptr, u64{0}};
    ASSERT(sv.chars == nullptr);
    ASSERT(sv.length == 0);
    ASSERT(sv == StringView{});
    ASSERT(sv == "");
    ASSERT(StringView{""} == sv);
    ASSERT(!(sv != ""));
}

TEST(testStringViewZeroLength)
{
    const char* data = "hello";
    const char* other = "world";
    StringView sv{data, u64{0}};
    StringView otherSv{other, u64{0}};
    ASSERT(sv.length == 0);
    ASSERT(StringView{} == sv);
    ASSERT(sv == otherSv);
}

TEST(testStringViewFromSpan)
{
    const char* data = "hello world";
    StringView sv{Span<const char>{data, 5}};
    ASSERT(sv == "hello");

    StringView full{Span<const char>{data, 11}};
    ASSERT(full == "hello world");
}

TEST(testStringViewBeginEnd)
{
    const char* data = "hello world";
    StringView sv{data + 6, data + 11};
    ASSERT(sv.length == 5);
    ASSERT(sv == "world");

    StringView empty{data, data};
    ASSERT(empty.length == 0);
    ASSERT(empty == StringView{});
}

TEST(testStringViewIndex)
{
    StringView sv{"hello"};
    ASSERT(sv[0] == 'h');
    ASSERT(sv[1] == 'e');
    ASSERT(sv[4] == 'o');
}

TEST(testStringViewIteration)
{
    StringView sv{"hello"};

    char result[6]{};
    u64 i = 0;
    for (char c : sv)
    {
        result[i] = c;
        ++i;
    }
    ASSERT(i == 5);
    ASSERT(StringView{result} == "hello");

    char iterResult[6]{};
    i = 0;
    for (auto it = sv.begin(); it != sv.end(); ++it)
    {
        iterResult[i] = *it;
        ++i;
    }
    ASSERT(i == 5);
    ASSERT(StringView{iterResult} == "hello");

    StringView empty{};
    ASSERT(empty.begin() == empty.end());
    ASSERT(empty.begin() == nullptr);
}

TEST(testStringViewLongString)
{
    const char data[] = "hello world this is a much longer string that spans "
        "multiple lines in the source code so that we can verify that longer "
        "strings work correctly with the string view implementation";
    StringView sv{data};
    ASSERT(sv.length == sizeof(data) - 1);
    ASSERT(sv == data);
}

TEST(testStringViewCrossTypeEquality)
{
    ArenaScope arena = getScratch();

    String owned = String::create("hello");
    StringBuilder built{arena, "hello"};

    ASSERT(StringView{"hello"} == owned);
    ASSERT(owned == StringView{"hello"});
    ASSERT(StringView{"world"} != owned);

    ASSERT(StringView{"hello"} == built);
    ASSERT(built == StringView{"hello"});
    ASSERT(StringView{"world"} != built);
}

TEST(testCString)
{
    ArenaScope arena = getScratch();

    char* c = cString(arena, "hello");
    ASSERT(c != nullptr);
    ASSERT(c[0] == 'h');
    ASSERT(c[5] == '\0');
    ASSERT(StringView{c} == "hello");

    char* empty = cString(arena, "");
    ASSERT(empty != nullptr);
    ASSERT(empty[0] == '\0');

    StringView nullView{};
    char* fromNull = cString(arena, nullView);
    ASSERT(fromNull != nullptr);
    ASSERT(fromNull[0] == '\0');

    StringView partial{"hello world", 5};
    char* partialC = cString(arena, partial);
    ASSERT(partialC != nullptr);
    ASSERT(partialC[5] == '\0');
    ASSERT(StringView{partialC} == "hello");
}

TEST(testStringBuilderConstruct)
{
    ArenaScope arena = getScratch();

    StringBuilder defaultSb{};
    ASSERT(defaultSb.chars == nullptr);
    ASSERT(defaultSb.length == 0);
    ASSERT(defaultSb.arena == nullptr);

    StringBuilder empty{arena};
    ASSERT(empty.length == 0);
    ASSERT(empty == "");

    StringBuilder fromString{arena, "hello"};
    ASSERT(fromString.length == 5);
    ASSERT(fromString == "hello");
    ASSERT(fromString[0] == 'h');
    ASSERT(fromString[4] == 'o');

    StringBuilder fromView{arena, StringView{"world"}};
    ASSERT(fromView == "world");

    StringBuilder fromPartial{arena, StringView{"hello world", 5}};
    ASSERT(fromPartial == "hello");
}

TEST(testStringBuilderAppend)
{
    ArenaScope arena = getScratch();

    StringBuilder sb{arena, "hello"};
    sb.append(" world");
    ASSERT(sb == "hello world");

    StringBuilder empty{arena};
    empty.append("hello");
    ASSERT(empty == "hello");

    StringBuilder charSb{arena, "hello"};
    charSb.append('!');
    ASSERT(charSb == "hello!");

    StringBuilder charEmpty{arena};
    charEmpty.append('x');
    ASSERT(charEmpty == "x");

    StringBuilder multi{arena, "a"};
    multi.append("b");
    multi.append("c");
    multi.append("d");
    multi.append("e");
    ASSERT(multi == "abcde");
}

TEST(testStringBuilderPrepend)
{
    ArenaScope arena = getScratch();

    StringBuilder sb{arena, "world"};
    sb.prepend("hello ");
    ASSERT(sb == "hello world");

    StringBuilder empty{arena};
    empty.prepend("hello");
    ASSERT(empty == "hello");

    StringBuilder charSb{arena, "ello"};
    charSb.prepend('h');
    ASSERT(charSb == "hello");

    StringBuilder multi{arena, "c"};
    multi.prepend("b");
    multi.prepend("a");
    ASSERT(multi == "abc");
}

TEST(testStringBuilderInsert)
{
    ArenaScope arena = getScratch();

    StringBuilder beginSb{arena, "world"};
    beginSb.insert(0, "hello ");
    ASSERT(beginSb == "hello world");

    StringBuilder middleSb{arena, "hello world"};
    middleSb.insert(5, ",");
    ASSERT(middleSb == "hello, world");

    StringBuilder endSb{arena, "hello"};
    endSb.insert(5, " world");
    ASSERT(endSb == "hello world");

    StringBuilder charSb{arena, "hello world"};
    charSb.insert(5, ',');
    ASSERT(charSb == "hello, world");

    StringBuilder emptyInsert{arena, "hello"};
    emptyInsert.insert(3, "");
    ASSERT(emptyInsert == "hello");

    StringBuilder emptySb{arena};
    emptySb.insert(0, "hello");
    ASSERT(emptySb == "hello");
}

TEST(testStringBuilderEquality)
{
    ArenaScope arena = getScratch();

    StringBuilder a{arena, "hello"};
    StringBuilder b{arena, "hello"};
    StringBuilder c{arena, "world"};

    ASSERT(a == b);
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(!(a == c));
    ASSERT(b != c);

    ASSERT(a == StringView{"hello"});
    ASSERT(StringView{"world"} != a);
}

TEST(testStringBuilderIndex)
{
    ArenaScope arena = getScratch();
    StringBuilder sb{arena, "hello"};
    ASSERT(sb[0] == 'h');
    ASSERT(sb[1] == 'e');
    ASSERT(sb[4] == 'o');
}

TEST(testStringBuilderGrowth)
{
    ArenaScope arena = getScratch();

    StringBuilder small{arena};
    for (u32 i = 0; i < 100; ++i)
        small.append('x');
    ASSERT(small.length == 100);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(small[i] == 'x');

    StringBuilder large{arena};
    for (u32 i = 0; i < 1000; ++i)
        large.append('a' + static_cast<char>(i % 26));
    ASSERT(large.length == 1000);
    for (u32 i = 0; i < 1000; ++i)
        ASSERT(large[i] == 'a' + static_cast<char>(i % 26));
}

TEST(testStringCreate)
{
    String s = String::create("hello");
    ASSERT(s.length == 5);
    ASSERT(s == "hello");
    ASSERT(s[0] == 'h');
    ASSERT(s[4] == 'o');

    String empty = String::create("");
    ASSERT(empty.length == 0);
    ASSERT(empty == "");

    String fromView = String::create(StringView{"hello world", 5});
    ASSERT(fromView == "hello");

    char data[300]{};
    for (u64 i = 0; i < 299; ++i)
        data[i] = 'a' + static_cast<char>(i % 26);
    String longString = String::create(StringView{data, 299});
    ASSERT(longString.length == 299);
    for (u64 i = 0; i < 299; ++i)
        ASSERT(longString[i] == 'a' + static_cast<char>(i % 26));
}

TEST(testStringMoveSemantics)
{
    String a = String::create("hello");
    String b{std::move(a)};
    ASSERT(a.chars == nullptr);
    ASSERT(a.length == 0);
    ASSERT(b == "hello");

    String c = String::create("world");
    b = std::move(c);
    ASSERT(c.chars == nullptr);
    ASSERT(c.length == 0);
    ASSERT(b == "world");

    String alpha = String::create("alpha");
    String beta = String::create("beta");
    String gamma = std::move(alpha);
    beta = std::move(gamma);
    ASSERT(beta == "alpha");
    ASSERT(alpha.chars == nullptr);
    ASSERT(gamma.chars == nullptr);

    String movedFrom = String::create("hello");
    String target = String::create("world");
    movedFrom = std::move(target);
    ASSERT(movedFrom == "world");
    ASSERT(target.chars == nullptr);
    target = std::move(movedFrom);
    ASSERT(target == "world");
    ASSERT(movedFrom.chars == nullptr);

    String empty = String::create("");
    ASSERT(empty == "");
    String movedEmpty = std::move(empty);
    ASSERT(empty.chars == nullptr);
    ASSERT(movedEmpty == "");
}

TEST(testStringEquality)
{
    String a = String::create("hello");
    String b = String::create("hello");
    String c = String::create("world");

    ASSERT(a == b);
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(!(a == c));
    ASSERT(b != c);

    ASSERT(a == StringView{"hello"});
    ASSERT(a == "hello");
    ASSERT(StringView{"world"} != a);
    ASSERT("world" != a);
}

TEST(testIsWhitespace)
{
    ASSERT(isWhitespace(' '));
    ASSERT(isWhitespace('\t'));
    ASSERT(isWhitespace('\n'));
    ASSERT(isWhitespace('\r'));
    ASSERT(!isWhitespace('\v'));
    ASSERT(!isWhitespace('\f'));
    ASSERT(!isWhitespace('\0'));
    ASSERT(!isWhitespace('a'));
    ASSERT(!isWhitespace('0'));
    ASSERT(!isWhitespace('.'));
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
    ASSERT(isFloat("1.5e+3"));
    ASSERT(isFloat("1.5e-3"));
    ASSERT(isFloat(".5e3"));
    ASSERT(isFloat("1e0"));
    ASSERT(isFloat("2.5e-0"));
    ASSERT(isFloat("1.0f"));
    ASSERT(isFloat("+10.f"));
    ASSERT(isFloat("-999.999f"));
    ASSERT(isFloat("1e3f"));
    ASSERT(isFloat("1e-3f"));
    ASSERT(isFloat("1.e3f"));
    ASSERT(isFloat("1.f"));
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
    ASSERT(!isFloat("1e"));
    ASSERT(!isFloat("1e+"));
    ASSERT(!isFloat("1e-"));
    ASSERT(!isFloat("e1"));
    ASSERT(!isFloat("e3"));
    ASSERT(!isFloat("."));
    ASSERT(!isFloat("e"));
    ASSERT(!isFloat("-"));
    ASSERT(!isFloat("+"));
    ASSERT(!isFloat("1.5e"));
    ASSERT(!isFloat("1.5e-"));
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
    ASSERT(stringToInteger("+00042") == 42);
    ASSERT(stringToInteger("-00042") == -42);
    ASSERT(stringToInteger("2147483647") == 2147483647);
    ASSERT(stringToInteger("2147483648") == 2147483648);
    ASSERT(stringToInteger("-2147483648") == -2147483648);
    ASSERT(stringToInteger("9223372036854775807") == 9223372036854775807LL);
    ASSERT(stringToInteger("-9223372036854775807") == -9223372036854775807LL);
}

TEST(testStringToFloat)
{
    ASSERT(std::abs(stringToFloat("0.0") - 0.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("99.99") - 99.99) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".1") - 0.1) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".5") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat(".12345") - 0.12345) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1.") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("100.") - 100.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-1.0") + 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-.5") + 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e+3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e-3") - 0.001) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1.5e3") - 1500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e+3") - 1500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e-3") - 0.0015) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".5e3") - 500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2e0") - 2.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e0") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e-0") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e0") - 1.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e1") - 10.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e2") - 100.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e2") - 250.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e1") - 15.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("5e-1") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.e3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-2.5e2") + 250.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("999.999") - 999.999) <= 1e-10);
    ASSERT(std::abs(stringToFloat("-999.999") + 999.999) <= 1e-10);
    ASSERT(std::abs(stringToFloat("123.456") - 123.456) <= 1e-10);
    ASSERT(stringToFloat("-0") == 0.0);
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
    ASSERT(integerToString(arena, -100) == "-100");
    ASSERT(integerToString(arena, -1000000) == "-1000000");
    ASSERT(integerToString(arena, -999999) == "-999999");
    ASSERT(integerToString(arena, 9000000000000000LL) == "9000000000000000");
    ASSERT(integerToString(arena, 9223372036854775807LL) == "9223372036854775807");
    ASSERT(integerToString(arena, -9223372036854775807LL) == "-9223372036854775807");
}

TEST(testFloatToString)
{
    ArenaScope arena = getScratch();
    ASSERT(floatToString(arena, 0.0, 4) == "0");
    ASSERT(floatToString(arena, 0.0, 0) == "0");
    ASSERT(floatToString(arena, 1.0, 12) == "1");
    ASSERT(floatToString(arena, 2.0, 4) == "2");
    ASSERT(floatToString(arena, 4.0, 5) == "4");
    ASSERT(floatToString(arena, -1.0, 3) == "-1");
    ASSERT(floatToString(arena, 0.5, 6) == "0.5");
    ASSERT(floatToString(arena, 0.5, 3) == "0.5");
    ASSERT(floatToString(arena, 0.125, 4) == "0.125");
    ASSERT(floatToString(arena, -0.5, 3) == "-0.5");
    ASSERT(floatToString(arena, 3.14, 3) == "3.14");
    ASSERT(floatToString(arena, 3.14159, 2) == "3.1");
    ASSERT(floatToString(arena, 3.14159, 4) == "3.142");
    ASSERT(floatToString(arena, 3.14159, 5) == "3.1416");
    ASSERT(floatToString(arena, 3.14159, 6) == "3.14159");
    ASSERT(floatToString(arena, 100.0, 0) == "1e+02");
    ASSERT(floatToString(arena, 100.0, 3) == "100");
    ASSERT(floatToString(arena, 0.0001, 1) == "0.0001");
    ASSERT(floatToString(arena, 0.00001, 1) == "1e-05");
    ASSERT(floatToString(arena, 0.0015, 2) == "0.0015");
    ASSERT(floatToString(arena, 123.456, 3) == "123");
    ASSERT(floatToString(arena, -123.456, 5) == "-123.46");
    ASSERT(floatToString(arena, 9.999, 2) == "10");
    ASSERT(floatToString(arena, 99.99, 6) == "99.99");
    ASSERT(floatToString(arena, 12345.6789, 17) == "12345.678900000001");
}

TEST(testFloatRoundTrip)
{
    ArenaScope arena = getScratch();

    f64 values[] = {
        0.0,
        -0.0,
        1.0,
        -1.0,
        0.5,
        -0.5,
        0.125,
        -0.125,
        2.5,
        -2.5,
        3.14159,
        99.99,
        123.456,
        -123.456,
        12345.6789,
        -9876.54321,
        1e300,
        1e-300,
        5e-324,
        1.7976931348623157e308,
    };

    for (f64 v : values)
    {
        StringBuilder str = floatToString(arena, v, 17);
        ASSERT(stringToFloat(str) == v);
    }
}
