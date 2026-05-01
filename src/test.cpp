#include "hurdygurdy.hpp"

#include <atomic>
#include <thread>

#include <emmintrin.h>

#ifdef hgAssert
#undef hgAssert
#endif

#define hgAssert(cond) do { \
    if (!(cond)) \
    { \
        hgError("Test assertion failed in " __FILE__ ":%d %s() " #cond "\n", __LINE__, __func__); \
    } \
} while(0)

void hgTest()
{
    printf("HurdyGurdy: Tests Begun\n");

    HgClock timer{};

    // HgMat
    {
        HgMat2 mat{
            HgVec2{1.0f, 0.0f},
            HgVec2{1.0f, 0.0f},
        };
        HgVec2 vec{1.0f, 1.0f};

        HgMat2 identity{
            HgVec2{1.0f, 0.0f},
            HgVec2{0.0f, 1.0f},
        };
        hgAssert(identity * mat == mat);
        hgAssert(identity * vec == vec);

        HgMat2 matRotated{
            HgVec2{0.0f, 1.0f},
            HgVec2{0.0f, 1.0f},
        };
        HgVec2 vecRotated{-1.0f, 1.0f};

        HgMat2 rotation{
            HgVec2{0.0f, 1.0f},
            HgVec2{-1.0f, 0.0f},
        };
        hgAssert(rotation * mat == matRotated);
        hgAssert(rotation * vec == vecRotated);

        hgAssert((identity * rotation) * mat == identity * (rotation * mat));
        hgAssert((identity * rotation) * vec == identity * (rotation * vec));
        hgAssert((rotation * rotation) * mat == rotation * (rotation * mat));
        hgAssert((rotation * rotation) * vec == rotation * (rotation * vec));
    }

    // HgQuat
    {
        HgMat3 identityMat = HgMat3{1.0f};
        HgVec3 upVec{0.0f, -1.0f, 0.0f};
        HgQuat rotation = hgQuatAxisAngle(HgVec3{0.0f, 0.0f, -1.0f}, -(f32)hgPi * 0.5f);

        HgVec3 rotatedVec = hgVecRotate(rotation, upVec);
        HgMat3 rotatedMat = hgMatRotate(rotation, identityMat);

        HgVec3 matRotatedVec = rotatedMat * upVec;

        hgAssert(abs(rotatedVec.x - 1.0f) < FLT_EPSILON
                    && abs(rotatedVec.y - 0.0f) < FLT_EPSILON
                    && abs(rotatedVec.y - 0.0f) < FLT_EPSILON);

        hgAssert(abs(matRotatedVec.x - rotatedVec.x) < FLT_EPSILON
                    && abs(matRotatedVec.y - rotatedVec.y) < FLT_EPSILON
                    && abs(matRotatedVec.y - rotatedVec.z) < FLT_EPSILON);
    }

    // HgArena
    {
        void* block = malloc(1024);
        hgDefer(free(block));

        HgArena arena{block, 1024, 0};

        for (u32 i = 0; i < 3; ++i)
        {
            hgAssert(arena.memory != nullptr);
            hgAssert(arena.capacity == 1024);
            hgAssert(arena.head == 0);

            u32* allocU32 = hgAlloc<u32>(&arena, 1);
            hgAssert(allocU32 == arena.memory);

            u64* allocU64 = hgAlloc<u64>(&arena, 2);
            hgAssert((u8*)allocU64 == (u8*)allocU32 + 8);

            u8* allocU8 = hgAlloc<u8>(&arena, 1);
            hgAssert(allocU8 == (u8*)allocU32 + 24);

            struct Big {
                u8 data[32];
            };
            Big* allocBig = hgAlloc<Big>(&arena, 1);
            hgAssert((u8*)allocBig == (u8*)allocU32 + 25);

            Big* reallocBig = hgRealloc(&arena, allocBig, 1, 2);
            hgAssert(reallocBig == allocBig);

            Big* reallocBigSame = hgRealloc(&arena, reallocBig, 2, 2);
            hgAssert(reallocBigSame == reallocBig);

            memset(reallocBig, 2, 2 * sizeof(*reallocBig));
            u8* allocInterrupt = hgAlloc<u8>(&arena, 1);
            (void)allocInterrupt;

            Big* reallocBig2 = hgRealloc(&arena, reallocBig, 2, 4);
            hgAssert(reallocBig2 != reallocBig);
            hgAssert(memcmp(reallocBig, reallocBig2, 2 * sizeof(*reallocBig)) == 0);

            arena.head = 0;
        }
    }

    // HgHandle and HgPool
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgPool<u32> pool = hgPoolCreate<u32>(arena, 32);

        HgHandle u1 = hgPoolAlloc(&pool);
        hgAssert(hgPoolAlive(&pool, u1));
        hgAssert(u1.id == 0);
        hgAssert(hgPoolGet(&pool, u1) != nullptr);
        *hgPoolGet(&pool, u1) = 12;
        hgAssert(*hgPoolGet(&pool, u1) == 12);

        HgHandle u2 = hgPoolAlloc(&pool);
        hgAssert(hgPoolAlive(&pool, u2));
        hgAssert(u2.id == 1);
        hgAssert(hgPoolGet(&pool, u2) != nullptr);
        *hgPoolGet(&pool, u2) = 42;
        hgAssert(*hgPoolGet(&pool, u2) == 42);

        hgPoolFree(&pool, u1);
        hgAssert(!hgPoolAlive(&pool, u1));

        HgHandle u12 = hgPoolAlloc(&pool);
        hgAssert(hgPoolAlive(&pool, u12));
        hgAssert(!hgPoolAlive(&pool, u1));
        hgAssert(u12.id != 0);
        hgAssert(hgHandleIdx(u12) == 0);
        hgAssert(hgPoolGet(&pool, u12) != nullptr);
        hgAssert(*hgPoolGet(&pool, u12) == 12);

        hgPoolReset(&pool);
        hgAssert(!hgPoolAlive(&pool, u1));
        hgAssert(!hgPoolAlive(&pool, u2));
        hgAssert(!hgPoolAlive(&pool, u12));
    }

    // HgString
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgString a = hgStringCopy(arena, "a");
        hgAssert(a[0] == 'a');
        hgAssert(a.capacity == 1);
        hgAssert(a.length == 1);

        HgString abc = hgStringCopy(arena, "abc");
        hgAssert(abc[0] == 'a');
        hgAssert(abc[1] == 'b');
        hgAssert(abc[2] == 'c');
        hgAssert(abc.length == 3);
        hgAssert(abc.capacity == 3);

        hgStringAppend(arena, &a, "bc");
        hgAssert(a == abc);

        HgString str = hgStringCreate(arena, 16);
        hgAssert(str == hgStringCreate(arena, 0));

        hgStringAppend(arena, &str, "hello");
        hgAssert(str == hgStringCopy(arena, "hello"));

        hgStringAppend(arena, &str, " there");
        hgAssert(str == hgStringCopy(arena, "hello there"));

        hgStringPrepend(arena, &str, "why ");
        hgAssert(str == hgStringCopy(arena, "why hello there"));

        hgStringInsert(arena, &str, 3, ",");
        hgAssert(str == hgStringCopy(arena, "why, hello there"));
    }

    // string utils
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        hgAssert(hgIsWhitespace(' '));
        hgAssert(hgIsWhitespace('\t'));
        hgAssert(hgIsWhitespace('\n'));

        hgAssert(hgIsNumeral('0'));
        hgAssert(hgIsNumeral('1'));
        hgAssert(hgIsNumeral('2'));
        hgAssert(hgIsNumeral('3'));
        hgAssert(hgIsNumeral('4'));
        hgAssert(hgIsNumeral('5'));
        hgAssert(hgIsNumeral('5'));
        hgAssert(hgIsNumeral('6'));
        hgAssert(hgIsNumeral('7'));
        hgAssert(hgIsNumeral('8'));
        hgAssert(hgIsNumeral('9'));

        hgAssert(!hgIsNumeral('0' - 1));
        hgAssert(!hgIsNumeral('9' + 1));

        hgAssert(!hgIsNumeral('x'));
        hgAssert(!hgIsNumeral('a'));
        hgAssert(!hgIsNumeral('b'));
        hgAssert(!hgIsNumeral('c'));
        hgAssert(!hgIsNumeral('d'));
        hgAssert(!hgIsNumeral('e'));
        hgAssert(!hgIsNumeral('f'));
        hgAssert(!hgIsNumeral('X'));
        hgAssert(!hgIsNumeral('A'));
        hgAssert(!hgIsNumeral('B'));
        hgAssert(!hgIsNumeral('C'));
        hgAssert(!hgIsNumeral('D'));
        hgAssert(!hgIsNumeral('E'));
        hgAssert(!hgIsNumeral('F'));

        hgAssert(!hgIsNumeral('.'));
        hgAssert(!hgIsNumeral('+'));
        hgAssert(!hgIsNumeral('-'));
        hgAssert(!hgIsNumeral('*'));
        hgAssert(!hgIsNumeral('/'));
        hgAssert(!hgIsNumeral('='));
        hgAssert(!hgIsNumeral('#'));
        hgAssert(!hgIsNumeral('&'));
        hgAssert(!hgIsNumeral('^'));
        hgAssert(!hgIsNumeral('~'));

        hgAssert(hgIsInteger("0"));
        hgAssert(hgIsInteger("1"));
        hgAssert(hgIsInteger("2"));
        hgAssert(hgIsInteger("3"));
        hgAssert(hgIsInteger("4"));
        hgAssert(hgIsInteger("5"));
        hgAssert(hgIsInteger("6"));
        hgAssert(hgIsInteger("7"));
        hgAssert(hgIsInteger("8"));
        hgAssert(hgIsInteger("9"));
        hgAssert(hgIsInteger("10"));

        hgAssert(hgIsInteger("12"));
        hgAssert(hgIsInteger("42"));
        hgAssert(hgIsInteger("100"));
        hgAssert(hgIsInteger("123456789"));
        hgAssert(hgIsInteger("-12"));
        hgAssert(hgIsInteger("-42"));
        hgAssert(hgIsInteger("-100"));
        hgAssert(hgIsInteger("-123456789"));
        hgAssert(hgIsInteger("+12"));
        hgAssert(hgIsInteger("+42"));
        hgAssert(hgIsInteger("+100"));
        hgAssert(hgIsInteger("+123456789"));

        hgAssert(!hgIsInteger("hello"));
        hgAssert(!hgIsInteger("not a number"));
        hgAssert(!hgIsInteger("number"));
        hgAssert(!hgIsInteger("integer"));
        hgAssert(!hgIsInteger("0.0"));
        hgAssert(!hgIsInteger("1.0"));
        hgAssert(!hgIsInteger(".10"));
        hgAssert(!hgIsInteger("1e2"));
        hgAssert(!hgIsInteger("1f"));
        hgAssert(!hgIsInteger("0xff"));
        hgAssert(!hgIsInteger("--42"));
        hgAssert(!hgIsInteger("++42"));
        hgAssert(!hgIsInteger("42-"));
        hgAssert(!hgIsInteger("42+"));
        hgAssert(!hgIsInteger("4 2"));
        hgAssert(!hgIsInteger("4+2"));

        hgAssert(hgIsFloat("0.0"));
        hgAssert(hgIsFloat("1."));
        hgAssert(hgIsFloat("2.0"));
        hgAssert(hgIsFloat("3."));
        hgAssert(hgIsFloat("4.0"));
        hgAssert(hgIsFloat("5."));
        hgAssert(hgIsFloat("6.0"));
        hgAssert(hgIsFloat("7."));
        hgAssert(hgIsFloat("8.0"));
        hgAssert(hgIsFloat("9."));
        hgAssert(hgIsFloat("10.0"));

        hgAssert(hgIsFloat("0.0"));
        hgAssert(hgIsFloat(".1"));
        hgAssert(hgIsFloat("0.2"));
        hgAssert(hgIsFloat(".3"));
        hgAssert(hgIsFloat("0.4"));
        hgAssert(hgIsFloat(".5"));
        hgAssert(hgIsFloat("0.6"));
        hgAssert(hgIsFloat(".7"));
        hgAssert(hgIsFloat("0.8"));
        hgAssert(hgIsFloat(".9"));
        hgAssert(hgIsFloat("0.10"));

        hgAssert(hgIsFloat("1.0"));
        hgAssert(hgIsFloat("+10.f"));
        hgAssert(hgIsFloat(".10"));
        hgAssert(hgIsFloat("-999.999f"));
        hgAssert(hgIsFloat("1e3"));
        hgAssert(hgIsFloat("1e3"));
        hgAssert(hgIsFloat("+1.e3f"));
        hgAssert(hgIsFloat(".1e3"));

        hgAssert(!hgIsFloat("hello"));
        hgAssert(!hgIsFloat("not a number"));
        hgAssert(!hgIsFloat("number"));
        hgAssert(!hgIsFloat("float"));
        hgAssert(!hgIsFloat("1.0ff"));
        hgAssert(!hgIsFloat("0x1.0"));
        hgAssert(!hgIsFloat("-0x1.0"));

        hgAssert(hgStringToInteger("0") == 0);
        hgAssert(hgStringToInteger("1") == 1);
        hgAssert(hgStringToInteger("2") == 2);
        hgAssert(hgStringToInteger("3") == 3);
        hgAssert(hgStringToInteger("4") == 4);
        hgAssert(hgStringToInteger("5") == 5);
        hgAssert(hgStringToInteger("6") == 6);
        hgAssert(hgStringToInteger("7") == 7);
        hgAssert(hgStringToInteger("8") == 8);
        hgAssert(hgStringToInteger("9") == 9);

        hgAssert(hgStringToInteger("0000000") == 0);
        hgAssert(hgStringToInteger("+0000001") == +1);
        hgAssert(hgStringToInteger("0000002") == 2);
        hgAssert(hgStringToInteger("-0000003") == -3);
        hgAssert(hgStringToInteger("0000004") == 4);
        hgAssert(hgStringToInteger("+0000005") == +5);
        hgAssert(hgStringToInteger("0000006") == 6);
        hgAssert(hgStringToInteger("-0000007") == -7);
        hgAssert(hgStringToInteger("0000008") == 8);
        hgAssert(hgStringToInteger("+0000009") == +9);

        hgAssert(hgStringToInteger("0000000") == 0);
        hgAssert(hgStringToInteger("1000000") == 1000000);
        hgAssert(hgStringToInteger("2000000") == 2000000);
        hgAssert(hgStringToInteger("3000000") == 3000000);
        hgAssert(hgStringToInteger("4000000") == 4000000);
        hgAssert(hgStringToInteger("5000000") == 5000000);
        hgAssert(hgStringToInteger("6000000") == 6000000);
        hgAssert(hgStringToInteger("7000000") == 7000000);
        hgAssert(hgStringToInteger("8000000") == 8000000);
        hgAssert(hgStringToInteger("9000000") == 9000000);
        hgAssert(hgStringToInteger("1234567890") == 1234567890);

        hgAssert(hgStringToFloat("0.0") == 0.0);
        hgAssert(hgStringToFloat("1.0f") == 1.0);
        hgAssert(hgStringToFloat("2.0") == 2.0);
        hgAssert(hgStringToFloat("3.0f") == 3.0);
        hgAssert(hgStringToFloat("4.0") == 4.0);
        hgAssert(hgStringToFloat("5.0f") == 5.0);
        hgAssert(hgStringToFloat("6.0") == 6.0);
        hgAssert(hgStringToFloat("7.0f") == 7.0);
        hgAssert(hgStringToFloat("8.0") == 8.0);
        hgAssert(hgStringToFloat("9.0f") == 9.0);

        hgAssert(hgStringToFloat("0e1") == 0.0);
        hgAssert(hgStringToFloat("1e2f") == 1e2);
        hgAssert(hgStringToFloat("2e3") == 2e3);
        hgAssert(hgStringToFloat("3e4f") == 3e4);
        hgAssert(hgStringToFloat("4e5") == 4e5);
        hgAssert(hgStringToFloat("5e6f") == 5e6);
        hgAssert(hgStringToFloat("6e7") == 6e7);
        hgAssert(hgStringToFloat("7e8f") == 7e8);
        hgAssert(hgStringToFloat("8e9") == 8e9);
        hgAssert(hgStringToFloat("9e10f") == 9e10);

        hgAssert(hgStringToFloat("0e1") == 0.0);
        hgAssert(hgStringToFloat("1e2f") == 1e2);
        hgAssert(hgStringToFloat("2e3") == 2e3);
        hgAssert(hgStringToFloat("3e4f") == 3e4);
        hgAssert(hgStringToFloat("4e5") == 4e5);
        hgAssert(hgStringToFloat("5e6f") == 5e6);
        hgAssert(hgStringToFloat("6e7") == 6e7);
        hgAssert(hgStringToFloat("7e8f") == 7e8);
        hgAssert(hgStringToFloat("8e9") == 8e9);
        hgAssert(hgStringToFloat("9e10f") == 9e10);

        hgAssert(hgStringToFloat(".1") == .1);
        hgAssert(hgStringToFloat("+.1") == +.1);
        hgAssert(hgStringToFloat("-.1") == -.1);
        hgAssert(hgStringToFloat("+.1e5") == +.1e5);

        hgAssert(hgIntegerToString(arena, 0) == "0");
        hgAssert(hgIntegerToString(arena, -1) == "-1");
        hgAssert(hgIntegerToString(arena, 2) == "2");
        hgAssert(hgIntegerToString(arena, -3) == "-3");
        hgAssert(hgIntegerToString(arena, 4) == "4");
        hgAssert(hgIntegerToString(arena, -5) == "-5");
        hgAssert(hgIntegerToString(arena, 6) == "6");
        hgAssert(hgIntegerToString(arena, -7) == "-7");
        hgAssert(hgIntegerToString(arena, 8) == "8");
        hgAssert(hgIntegerToString(arena, -9) == "-9");

        hgAssert(hgIntegerToString(arena, 0000000) == "0");
        hgAssert(hgIntegerToString(arena, -1000000) == "-1000000");
        hgAssert(hgIntegerToString(arena, 2000000) == "2000000");
        hgAssert(hgIntegerToString(arena, -3000000) == "-3000000");
        hgAssert(hgIntegerToString(arena, 4000000) == "4000000");
        hgAssert(hgIntegerToString(arena, -5000000) == "-5000000");
        hgAssert(hgIntegerToString(arena, 6000000) == "6000000");
        hgAssert(hgIntegerToString(arena, -7000000) == "-7000000");
        hgAssert(hgIntegerToString(arena, 8000000) == "8000000");
        hgAssert(hgIntegerToString(arena, -9000000) == "-9000000");
        hgAssert(hgIntegerToString(arena, 1234567890) == "1234567890");

        hgAssert(hgFloatToString(arena, 0.0, 10) == "0.0");
        hgAssert(hgFloatToString(arena, -1.0f, 1) == "-1.0");
        hgAssert(hgFloatToString(arena, 2.0, 2) == "2.00");
        hgAssert(hgFloatToString(arena, -3.0f, 3) == "-3.000");
        hgAssert(hgFloatToString(arena, 4.0, 4) == "4.0000");
        hgAssert(hgFloatToString(arena, -5.0f, 5) == "-5.00000");
        hgAssert(hgFloatToString(arena, 6.0, 6) == "6.000000");
        hgAssert(hgFloatToString(arena, -7.0f, 7) == "-7.0000000");
        hgAssert(hgFloatToString(arena, 8.0, 8) == "8.00000000");
        hgAssert(hgFloatToString(arena, -9.0f, 9) == "-9.000000000");

        hgAssert(hgFloatToString(arena, 0e0, 1) == "0.0");
        hgAssert(hgFloatToString(arena, -1e1f, 0) == "-10.");
        hgAssert(hgFloatToString(arena, 2e2, 1) == "200.0");
        hgAssert(hgFloatToString(arena, -3e3f, 0) == "-3000.");
        hgAssert(hgFloatToString(arena, 4e4, 1) == "40000.0");
        hgAssert(hgFloatToString(arena, -5e5f, 0) == "-500000.");
        hgAssert(hgFloatToString(arena, 6e6, 1) == "6000000.0");
        hgAssert(hgFloatToString(arena, -7e7f, 0) == "-70000000.");
        hgAssert(hgFloatToString(arena, 8e8, 1) == "800000000.0");
        hgAssert(hgFloatToString(arena, -9e9f, 0) == "-8999999488.");

        hgAssert(hgFloatToString(arena, -0e-0, 3) == "0.0");
        hgAssert(hgFloatToString(arena, 1e-1f, 3) == "0.100");
        hgAssert(hgFloatToString(arena, -2e-2, 3) == "-0.020");
        hgAssert(hgFloatToString(arena, 3e-3f, 3) == "0.003");
        hgAssert(hgFloatToString(arena, -4e-0, 3) == "-4.000");
        hgAssert(hgFloatToString(arena, 5e-1f, 3) == "0.500");
        hgAssert(hgFloatToString(arena, -6e-2, 3) == "-0.060");
        hgAssert(hgFloatToString(arena, 7e-3f, 3) == "0.007");
        hgAssert(hgFloatToString(arena, -8e-0, 3) == "-8.000");
        hgAssert(hgFloatToString(arena, 9e-1f, 3) == "0.899");
    }

    // HgJson
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                1234
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors != nullptr);
        hgAssert(json.file != nullptr);

        HgJsonError* error = json.errors;
        hgAssert(error->next == nullptr);
        hgAssert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf"
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors != nullptr);
        hgAssert(json.file != nullptr);

        HgJsonError* error = json.errors;
        hgAssert(error->next == nullptr);
        hgAssert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf":
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors != nullptr);
        hgAssert(json.file != nullptr);

        HgJsonError* error = json.errors;
        hgAssert(error->next != nullptr);
        hgAssert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hgAssert(error->next == nullptr);
        hgAssert(error->msg == "on line 4, found unexpected token \"}\"\n");

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": true
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_bool);
        hgAssert(field->value->boolean == true);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": false
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_bool);
        hgAssert(field->value->boolean == false);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": asdf
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors != nullptr);
        hgAssert(json.file != nullptr);

        HgJsonError* error = json.errors;
        hgAssert(error->next != nullptr);
        hgAssert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hgAssert(error->next == nullptr);
        hgAssert(error->msg == "on line 3, found unexpected token \"asdf\"\n");

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": "asdf"
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_string);
        hgAssert(field->value->string == "asdf");
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_integer);
        hgAssert(field->value->integer == 1234);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_float);
        hgAssert(field->value->floating == 1234.0);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0,
                "hjkl": 5678.0
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_float);
        hgAssert(field->value->floating == 1234.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "hjkl");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_float);
        hgAssert(field->value->floating == 5678.0);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, 3, 4]
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 3);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": [1 2 3 4]
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 3);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, "3", 4]
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors != nullptr);
        hgAssert(json.file != nullptr);

        HgJsonError* error = json.errors;
        hgAssert(error->next == nullptr);
        hgAssert(error->msg ==
            "on line 3, array has element which is not the same type as the first valid element\n");

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "asdf": {
                    "a": 1,
                    "s": 2.0,
                    "d": 3,
                    "f": 4.0,
                }
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType_struct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_struct);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonField* subField = field->value->jstruct.fields;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "a");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType_integer);
        hgAssert(subField->value->integer == 1);

        subField = subField->next;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "s");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType_float);
        hgAssert(subField->value->floating == 2.0);

        subField = subField->next;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "d");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType_integer);
        hgAssert(subField->value->integer == 3);

        subField = subField->next;
        hgAssert(subField->next == nullptr);
        hgAssert(subField->name == "f");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType_float);
        hgAssert(subField->value->floating == 4.0);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
                "player": {
                    "transform": {
                        "position": [1.0, 0.0, -1.0],
                        "scale": [1.0, 1.0, 1.0],
                        "rotation": [1.0, 0.0, 0.0, 0.0]
                    },
                    "sprite": {
                        "texture": "tex.png",
                        "uvPos": [0.0, 0.0],
                        "uvSize": [1.0, 1.0]
                    }
                }
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* mainStruct = json.file;
        hgAssert(mainStruct->type == HgJsonType_struct);
        hgAssert(mainStruct->jstruct.fields != nullptr);

        HgJsonField* player = mainStruct->jstruct.fields;
        hgAssert(player->next == nullptr);
        hgAssert(player->name == "player");
        hgAssert(player->value != nullptr);
        hgAssert(player->value->type == HgJsonType_struct);
        hgAssert(player->value->jstruct.fields != nullptr);

        HgJsonField* component = player->value->jstruct.fields;
        hgAssert(component->next != nullptr);
        hgAssert(component->name == "transform");
        hgAssert(component->value != nullptr);
        hgAssert(component->value->type == HgJsonType_struct);
        hgAssert(component->value->jstruct.fields != nullptr);

        HgJsonField* field = component->value->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "position");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == -1.0);

        field = field->next;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "scale");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "rotation");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        component = component->next;
        hgAssert(component->next == nullptr);
        hgAssert(component->name == "sprite");
        hgAssert(component->value != nullptr);
        hgAssert(component->value->type == HgJsonType_struct);
        hgAssert(component->value->jstruct.fields != nullptr);

        field = component->value->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "texture");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_string);
        hgAssert(field->value->string == "tex.png");

        field = field->next;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "uvPos");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 0.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "uvSize");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType_array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType_float);
        hgAssert(elem->value->floating == 1.0);
    }

    // HgHashSet
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        constexpr u32 count = 128;

        HgSet<u32> set = hgSetCreate<u32>(arena, count);

        for (u32 i = 0; i < 3; ++i)
        {
            hgAssert(set.count == 0);
            hgAssert(!hgSetHas(&set, 0));
            hgAssert(!hgSetHas(&set, 1));
            hgAssert(!hgSetHas(&set, 12));
            hgAssert(!hgSetHas(&set, 42));
            hgAssert(!hgSetHas(&set, 100000));

            hgSetAdd(&set, 1);
            hgAssert(set.count == 1);
            hgAssert(hgSetHas(&set, 1));

            hgSetRemove(&set, 1);
            hgAssert(set.count == 0);
            hgAssert(!hgSetHas(&set, 1));

            hgAssert(!hgSetHas(&set, 12));
            hgAssert(!hgSetHas(&set, 12 + count));

            hgSetAdd(&set, 12);
            hgAssert(set.count == 1);
            hgAssert(hgSetHas(&set, 12));
            hgAssert(!hgSetHas(&set, 12 + count));

            hgSetAdd(&set, 12 + count);
            hgAssert(set.count == 2);
            hgAssert(hgSetHas(&set, 12));
            hgAssert(hgSetHas(&set, 12 + count));

            hgSetAdd(&set, 12 + count * 2);
            hgAssert(set.count == 3);
            hgAssert(hgSetHas(&set, 12));
            hgAssert(hgSetHas(&set, 12 + count));
            hgAssert(hgSetHas(&set, 12 + count * 2));

            hgSetRemove(&set, 12);
            hgAssert(set.count == 2);
            hgAssert(!hgSetHas(&set, 12));
            hgAssert(hgSetHas(&set, 12 + count));

            hgSetAdd(&set, 42);
            hgAssert(set.count == 3);
            hgAssert(hgSetHas(&set, 42));

            hgSetRemove(&set, 12 + count);
            hgAssert(set.count == 2);
            hgAssert(!hgSetHas(&set, 12));
            hgAssert(!hgSetHas(&set, 12 + count));

            hgSetRemove(&set, 42);
            hgAssert(set.count == 1);
            hgAssert(!hgSetHas(&set, 42));

            hgSetRemove(&set, 12 + count * 2);
            hgAssert(set.count == 0);
            hgAssert(!hgSetHas(&set, 12));
            hgAssert(!hgSetHas(&set, 12 + count));
            hgAssert(!hgSetHas(&set, 12 + count * 2));

            hgSetReset(&set);
        }
    }

    // HgHashMap
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        constexpr u32 count = 128;

        HgMap<u32, u32> map = hgMapCreate<u32, u32>(arena, count);

        for (u32 i = 0; i < 3; ++i)
        {
            hgAssert(map.count == 0);
            hgAssert(hgMapGet(&map, 0) == nullptr);
            hgAssert(hgMapGet(&map, 1) == nullptr);
            hgAssert(hgMapGet(&map, 12) == nullptr);
            hgAssert(hgMapGet(&map, 42) == nullptr);
            hgAssert(hgMapGet(&map, 100000) == nullptr);

            hgMapAdd(&map, 1, 1);
            hgAssert(map.count == 1);
            hgAssert(hgMapGet(&map, 1) != nullptr);
            hgAssert(*hgMapGet(&map, 1) == 1);

            hgMapRemove(&map, 1);
            hgAssert(map.count == 0);
            hgAssert(hgMapGet(&map, 1) == nullptr);

            hgAssert(hgMapGet(&map, 12) == nullptr);
            hgAssert(hgMapGet(&map, 12 + count) == nullptr);

            hgMapAdd(&map, 12, 42);
            hgAssert(map.count == 1);
            hgAssert(hgMapGet(&map, 12) != nullptr && *hgMapGet(&map, 12) == 42);
            hgAssert(hgMapGet(&map, 12 + count) == nullptr);

            hgMapAdd(&map, 12 + count, 100);
            hgAssert(map.count == 2);
            hgAssert(hgMapGet(&map, 12) != nullptr && *hgMapGet(&map, 12) == 42);
            hgAssert(hgMapGet(&map, 12 + count) != nullptr && *hgMapGet(&map, 12 + count) == 100);

            hgMapAdd(&map, 12 + count * 2, 200);
            hgAssert(map.count == 3);
            hgAssert(hgMapGet(&map, 12) != nullptr && *hgMapGet(&map, 12) == 42);
            hgAssert(hgMapGet(&map, 12 + count) != nullptr && *hgMapGet(&map, 12 + count) == 100);
            hgAssert(hgMapGet(&map, 12 + count * 2) != nullptr && *hgMapGet(&map, 12 + count * 2) == 200);

            hgMapRemove(&map, 12);
            hgAssert(map.count == 2);
            hgAssert(hgMapGet(&map, 12) == nullptr);
            hgAssert(hgMapGet(&map, 12 + count) != nullptr && *hgMapGet(&map, 12 + count) == 100);

            hgMapAdd(&map, 42, 12);
            hgAssert(map.count == 3);
            hgAssert(hgMapGet(&map, 42) != nullptr && *hgMapGet(&map, 42) == 12);

            hgMapRemove(&map, 12 + count);
            hgAssert(map.count == 2);
            hgAssert(hgMapGet(&map, 12) == nullptr);
            hgAssert(hgMapGet(&map, 12 + count) == nullptr);

            hgMapRemove(&map, 42);
            hgAssert(map.count == 1);
            hgAssert(hgMapGet(&map, 42) == nullptr);

            hgMapRemove(&map, 12 + count * 2);
            hgAssert(map.count == 0);
            hgAssert(hgMapGet(&map, 12) == nullptr);
            hgAssert(hgMapGet(&map, 12 + count) == nullptr);
            hgAssert(hgMapGet(&map, 12 + count * 2) == nullptr);

            hgMapReset(&map);
        }
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        using StrHash = u64;

        HgMap<StrHash, u32> map = hgMapCreate<StrHash, u32>(arena, 128);

        StrHash a = hgHash("a");
        StrHash b = hgHash("b");
        StrHash ab = hgHash("ab");
        StrHash scf = hgHash("supercalifragilisticexpialidocious");

        hgAssert(hgMapGet(&map, a) == nullptr);
        hgAssert(hgMapGet(&map, b) == nullptr);
        hgAssert(hgMapGet(&map, ab) == nullptr);
        hgAssert(hgMapGet(&map, scf) == nullptr);

        hgMapAdd(&map, a, 1);
        hgMapAdd(&map, b, 2);
        hgMapAdd(&map, ab, 3);
        hgMapAdd(&map, scf, 4);

        hgAssert(hgMapGet(&map, a) != nullptr && *hgMapGet(&map, a) == 1);
        hgAssert(hgMapGet(&map, b) != nullptr && *hgMapGet(&map, b) == 2);
        hgAssert(hgMapGet(&map, ab) != nullptr && *hgMapGet(&map, ab) == 3);
        hgAssert(hgMapGet(&map, scf) != nullptr && *hgMapGet(&map, scf) == 4);

        hgMapRemove(&map, a);
        hgMapRemove(&map, b);
        hgMapRemove(&map, ab);
        hgMapRemove(&map, scf);

        hgAssert(hgMapGet(&map, a) == nullptr);
        hgAssert(hgMapGet(&map, b) == nullptr);
        hgAssert(hgMapGet(&map, ab) == nullptr);
        hgAssert(hgMapGet(&map, scf) == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgMap<const char*, u32> map = hgMapCreate<const char*, u32>(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hgAssert(hgMapGet(&map, a) == nullptr);
        hgAssert(hgMapGet(&map, b) == nullptr);
        hgAssert(hgMapGet(&map, ab) == nullptr);
        hgAssert(hgMapGet(&map, scf) == nullptr);

        hgMapAdd(&map, a, 1);
        hgMapAdd(&map, b, 2);
        hgMapAdd(&map, ab, 3);
        hgMapAdd(&map, scf, 4);

        hgAssert(hgMapGet(&map, a) != nullptr && *hgMapGet(&map, a) == 1);
        hgAssert(hgMapGet(&map, b) != nullptr && *hgMapGet(&map, b) == 2);
        hgAssert(hgMapGet(&map, ab) != nullptr && *hgMapGet(&map, ab) == 3);
        hgAssert(hgMapGet(&map, scf) != nullptr && *hgMapGet(&map, scf) == 4);

        hgMapRemove(&map, a);
        hgMapRemove(&map, b);
        hgMapRemove(&map, ab);
        hgMapRemove(&map, scf);

        hgAssert(hgMapGet(&map, a) == nullptr);
        hgAssert(hgMapGet(&map, b) == nullptr);
        hgAssert(hgMapGet(&map, ab) == nullptr);
        hgAssert(hgMapGet(&map, scf) == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgMap<HgString, u32> map = hgMapCreate<HgString, u32>(arena, 128);

        hgAssert(hgMapGet(&map, hgStringCopy(arena, "a")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "b")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "ab")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious")) == nullptr);

        hgMapAdd(&map, hgStringCopy(arena, "a"), 1);
        hgMapAdd(&map, hgStringCopy(arena, "b"), 2);
        hgMapAdd(&map, hgStringCopy(arena, "ab"), 3);
        hgMapAdd(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious"), 4);

        hgAssert(hgMapGet(&map, hgStringCopy(arena, "a")) != nullptr);
        hgAssert(*hgMapGet(&map, hgStringCopy(arena, "a")) == 1);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "b")) != nullptr);
        hgAssert(*hgMapGet(&map, hgStringCopy(arena, "b")) == 2);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "ab")) != nullptr);
        hgAssert(*hgMapGet(&map, hgStringCopy(arena, "ab")) == 3);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious")) != nullptr);
        hgAssert(*hgMapGet(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious")) == 4);

        hgMapRemove(&map, hgStringCopy(arena, "a"));
        hgMapRemove(&map, hgStringCopy(arena, "b"));
        hgMapRemove(&map, hgStringCopy(arena, "ab"));
        hgMapRemove(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(hgMapGet(&map, hgStringCopy(arena, "a")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "b")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "ab")) == nullptr);
        hgAssert(hgMapGet(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious")) == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgMap<HgStringView, u32> map = hgMapCreate<HgStringView, u32>(arena, 128);

        hgAssert(hgMapGet(&map, "a") == nullptr);
        hgAssert(hgMapGet(&map, "b") == nullptr);
        hgAssert(hgMapGet(&map, "ab") == nullptr);
        hgAssert(hgMapGet(&map, "supercalifragilisticexpialidocious") == nullptr);

        hgMapAdd(&map, hgStringCopy(arena, "a"), 1);
        hgMapAdd(&map, hgStringCopy(arena, "b"), 2);
        hgMapAdd(&map, hgStringCopy(arena, "ab"), 3);
        hgMapAdd(&map, hgStringCopy(arena, "supercalifragilisticexpialidocious"), 4);

        hgAssert(hgMapGet(&map, "a") != nullptr);
        hgAssert(*hgMapGet(&map, "a") == 1);
        hgAssert(hgMapGet(&map, "b") != nullptr);
        hgAssert(*hgMapGet(&map, "b") == 2);
        hgAssert(hgMapGet(&map, "ab") != nullptr);
        hgAssert(*hgMapGet(&map, "ab") == 3);
        hgAssert(hgMapGet(&map, "supercalifragilisticexpialidocious") != nullptr);
        hgAssert(*hgMapGet(&map, "supercalifragilisticexpialidocious") == 4);

        hgMapRemove(&map, "a");
        hgMapRemove(&map, "b");
        hgMapRemove(&map, "ab");
        hgMapRemove(&map, "supercalifragilisticexpialidocious");

        hgAssert(hgMapGet(&map, "a") == nullptr);
        hgAssert(hgMapGet(&map, "b") == nullptr);
        hgAssert(hgMapGet(&map, "ab") == nullptr);
        hgAssert(hgMapGet(&map, "supercalifragilisticexpialidocious") == nullptr);
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        using StrHash = u64;

        HgSet<StrHash> set = hgSetCreate<StrHash>(arena, 128);

        StrHash a = hgHash("a");
        StrHash b = hgHash("b");
        StrHash ab = hgHash("ab");
        StrHash scf = hgHash("supercalifragilisticexpialidocious");

        hgAssert(!hgSetHas(&set, a));
        hgAssert(!hgSetHas(&set, b));
        hgAssert(!hgSetHas(&set, ab));
        hgAssert(!hgSetHas(&set, scf));

        hgSetAdd(&set, a);
        hgSetAdd(&set, b);
        hgSetAdd(&set, ab);
        hgSetAdd(&set, scf);

        hgAssert(hgSetHas(&set, a));
        hgAssert(hgSetHas(&set, b));
        hgAssert(hgSetHas(&set, ab));
        hgAssert(hgSetHas(&set, scf));

        hgSetRemove(&set, a);
        hgSetRemove(&set, b);
        hgSetRemove(&set, ab);
        hgSetRemove(&set, scf);

        hgAssert(!hgSetHas(&set, a));
        hgAssert(!hgSetHas(&set, b));
        hgAssert(!hgSetHas(&set, ab));
        hgAssert(!hgSetHas(&set, scf));
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgSet<const char*> set = hgSetCreate<const char*>(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hgAssert(!hgSetHas(&set, a));
        hgAssert(!hgSetHas(&set, b));
        hgAssert(!hgSetHas(&set, ab));
        hgAssert(!hgSetHas(&set, scf));

        hgSetAdd(&set, a);
        hgSetAdd(&set, b);
        hgSetAdd(&set, ab);
        hgSetAdd(&set, scf);

        hgAssert(hgSetHas(&set, a));
        hgAssert(hgSetHas(&set, b));
        hgAssert(hgSetHas(&set, ab));
        hgAssert(hgSetHas(&set, scf));

        hgSetRemove(&set, a);
        hgSetRemove(&set, b);
        hgSetRemove(&set, ab);
        hgSetRemove(&set, scf);

        hgAssert(!hgSetHas(&set, a));
        hgAssert(!hgSetHas(&set, b));
        hgAssert(!hgSetHas(&set, ab));
        hgAssert(!hgSetHas(&set, scf));
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgSet<HgString> set = hgSetCreate<HgString>(arena, 128);

        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "a")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "b")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "ab")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious")));

        hgSetAdd(&set, hgStringCopy(arena, "a"));
        hgSetAdd(&set, hgStringCopy(arena, "b"));
        hgSetAdd(&set, hgStringCopy(arena, "ab"));
        hgSetAdd(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(hgSetHas(&set, hgStringCopy(arena, "a")));
        hgAssert(hgSetHas(&set, hgStringCopy(arena, "b")));
        hgAssert(hgSetHas(&set, hgStringCopy(arena, "ab")));
        hgAssert(hgSetHas(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious")));

        hgSetRemove(&set, hgStringCopy(arena, "a"));
        hgSetRemove(&set, hgStringCopy(arena, "b"));
        hgSetRemove(&set, hgStringCopy(arena, "ab"));
        hgSetRemove(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "a")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "b")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "ab")));
        hgAssert(!hgSetHas(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgSet<HgStringView> set = hgSetCreate<HgStringView>(arena, 128);

        hgAssert(!hgSetHas(&set, "a"));
        hgAssert(!hgSetHas(&set, "b"));
        hgAssert(!hgSetHas(&set, "ab"));
        hgAssert(!hgSetHas(&set, "supercalifragilisticexpialidocious"));

        hgSetAdd(&set, hgStringCopy(arena, "a"));
        hgSetAdd(&set, hgStringCopy(arena, "b"));
        hgSetAdd(&set, hgStringCopy(arena, "ab"));
        hgSetAdd(&set, hgStringCopy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(hgSetHas(&set, "a"));
        hgAssert(hgSetHas(&set, "b"));
        hgAssert(hgSetHas(&set, "ab"));
        hgAssert(hgSetHas(&set, "supercalifragilisticexpialidocious"));

        hgSetRemove(&set, "a");
        hgSetRemove(&set, "b");
        hgSetRemove(&set, "ab");
        hgSetRemove(&set, "supercalifragilisticexpialidocious");

        hgAssert(!hgSetHas(&set, "a"));
        hgAssert(!hgSetHas(&set, "b"));
        hgAssert(!hgSetHas(&set, "ab"));
        hgAssert(!hgSetHas(&set, "supercalifragilisticexpialidocious"));
    }

    // thread pool
    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        bool a = false;
        bool b = false;

        hgThreadsCall(fence, &a, [](void *pa)
        {
            *(bool*)pa = true;
        });
        hgThreadsCall(fence, &b, [](void *pb)
        {
            *(bool*)pb = true;
        });

        hgFenceWait(fence, 2.0);

        hgAssert(hgFenceWait(fence, 2.0));

        hgAssert(a == true);
        hgAssert(b == true);
    }

    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        bool vals[100]{};
        for (bool& val : vals)
        {
            hgThreadsCall(fence, &val, [](void* data)
            {
                *(bool*)data = true;
            });
        }

        hgAssert(hgThreadsHelp(fence, 2.0));

        for (bool& val : vals)
        {
            hgAssert(val == true);
        }
    }

    {
        bool vals[100]{};

        auto fn = [](void* pvals, u64 idx)
        {
            ((bool*)pvals)[idx] = true;
        };
        hgThreadsFor(0, sizeof(vals) / sizeof(*vals), vals, fn);

        for (bool& val : vals)
        {
            hgAssert(val == true);
        }
    }

    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        for (u32 n = 0; n < 3; ++n)
        {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100]{};

            auto fn = [](void* pval)
            {
                *((bool*)pval) = !*((bool*)pval);
            };

            auto prodFn = [&](u32 idx)
            {
                while (!start)
                {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i)
                {
                    hgThreadsCall(fence, vals + i, fn);
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j)
            {
                producers[j] = std::thread(prodFn, j);
            }

            start.store(true);
            for (auto& thread : producers)
            {
                thread.join();
            }

            hgAssert(hgThreadsHelp(fence, 2.0));
            for (auto val : vals)
            {
                hgAssert(val == true);
            }
        }
    }

    // io thread
    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        bool vals[100]{};

        hgIoRequest(fence, vals, {}, [](void* pvals, HgStringView)
        {
            for (u32 i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
            {
                ((bool*)pvals)[i] = true;
            }
        });

        hgAssert(hgFenceWait(fence, 2.0));
        for (u32 i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        bool vals[100]{};

        for (u32 i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgIoRequest(fence, &vals[i], {}, [](void* pval, HgStringView)
            {
                *(bool*)pval = true;
            });
        }

        hgAssert(hgFenceWait(fence, 2.0));
        for (u32 i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        bool vals[100]{};

        vals[0] = true;

        for (u32 i = 1; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgIoRequest(fence, &vals[i], {}, [](void* pval, HgStringView)
            {
                *(bool*)pval = *((bool*)pval - 1);
            });
        }

        hgAssert(hgFenceWait(fence, 2.0));
        for (u32 i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence = hgFenceCreate();
        hgDefer(hgFenceDestroy(fence));

        for (u32 n = 0; n < 3; ++n)
        {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100]{};

            auto prodFn = [&](u32 idx)
            {
                while (!start)
                {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i)
                {
                    hgIoRequest(fence, &vals[i], {}, [](void* pval, HgStringView)
                    {
                        *(bool*)pval = !*(bool*)pval;
                    });
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j)
            {
                producers[j] = std::thread(prodFn, j);
            }

            start.store(true);
            for (auto& thread : producers)
            {
                thread.join();
            }

            hgAssert(hgFenceWait(fence, 2.0));
            for (auto val : vals)
            {
                hgAssert(val == true);
            }
        }
    }

    hgWarn("HgAssetManager test not implemented yet : TODO\n");

    // // HgBinary
    // {
    //     HgArena* arena = hgScratch();
    //     HgArenaScope arenaScope{arena};
    //
    //     u32 saveData[]{12, 42, 100, 128};
    //
    //     const char* filePath = "hg_test_dir/file_bin_test.bin";
    //     HgBinary bin{};
    //
    //     {
    //         bin = hgLoadBinary(arena, "file_does_not_exist.bin");
    //         hgAssert(bin.data == nullptr);
    //         hgAssert(bin.size == 0);
    //     }
    //
    //     {
    //         bin.data = saveData;
    //         bin.size = sizeof(saveData);
    //
    //         hgStoreBinary(bin, "dir/does/not/exist.bin");
    //
    //         FILE* fileHandle = fopen("dir/does/not/exist.bin", "rb");
    //         hgAssert(fileHandle == nullptr);
    //     }
    //
    //     {
    //         bin.data = saveData;
    //         bin.size = sizeof(saveData);
    //
    //         hgStoreBinary(bin, filePath);
    //         HgBinary newBin = hgLoadBinary(arena, filePath);
    //
    //         hgAssert(newBin.data != nullptr);
    //         hgAssert(newBin.data != saveData);
    //         hgAssert(newBin.size == sizeof(saveData));
    //         hgAssert(memcmp(saveData, newBin.data, newBin.size) == 0);
    //     }
    // }

    // // HgTextureData
    // {
    //     HgArena* arena = hgScratch();
    //     HgArenaScope arenaScope{arena};
    //
    //     struct color {
    //         u8 r, g, b, a;
    //
    //         operator u32() { return *(u32*)this; }
    //     };
    //
    //     u32 red =    color{0xff, 0x00, 0x00, 0xff};
    //     u32 green =  color{0x00, 0xff, 0x00, 0xff};
    //     u32 blue =   color{0x00, 0x00, 0xff, 0xff};
    //     u32 yellow = color{0xff, 0xff, 0x00, 0xff};
    //
    //     constexpr HgFormat saveFormat = HgFormat_r8g8b8a8_srgb;
    //     constexpr u32 saveWidth = 2;
    //     constexpr u32 saveHeight = 2;
    //     constexpr u32 saveDepth = 1;
    //     u32 saveData[saveWidth][saveHeight]{
    //         {red, green},
    //         {blue, yellow},
    //     };
    //
    //     HgBinary bin{};
    //
    //     {
    //         HgImageData::Info info;
    //         memcpy(info.identifier, HgImageData::imageIdentifier, sizeof(HgImageData::imageIdentifier));
    //         info.format = HgFormat_r8g8b8a8_srgb;
    //         info.width = saveWidth;
    //         info.height = saveHeight;
    //         info.depth = saveDepth;
    //         info.pixelsBegin = sizeof(info);
    //         bin.resize(arena, bin.size + sizeof(HgImageData::Info));
    //         bin.overwrite(0, info);
    //
    //         u64 pixelIdx = bin.size;
    //         bin.resize(arena, bin.size + sizeof(saveData));
    //         bin.overwrite(pixelIdx, saveData, sizeof(saveData));
    //     }
    //
    //     {
    //         HgImageData texture = bin;
    //
    //         HgFormat format;
    //         u32 width, height, depth;
    //         hgAssert(texture.getInfo(&format, &width, &height, &depth));
    //         hgAssert(format == saveFormat);
    //         hgAssert(width == saveWidth);
    //         hgAssert(height == saveHeight);
    //         hgAssert(depth == saveDepth);
    //         hgAssert(width * height * depth * hgFormatToSize((HgFormat)format) == sizeof(saveData));
    //
    //         void* pixels = texture.getPixels();
    //         hgAssert(pixels != nullptr);
    //         hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
    //     }
    //
    //     {
    //         HgStringView filePath = "hg_test_dir/file_image_test.hgtex";
    //
    //         hgStoreBinary(bin, filePath);
    //         HgImageData fileTexture = hgLoadBinary(arena, filePath);
    //
    //         HgFormat format;
    //         u32 width, height, depth;
    //         hgAssert(fileTexture.getInfo(&format, &width, &height, &depth));
    //         hgAssert(format == saveFormat);
    //         hgAssert(width == saveWidth);
    //         hgAssert(height == saveHeight);
    //         hgAssert(depth == saveDepth);
    //         hgAssert(width * height * depth * hgFormatToSize((HgFormat)format) == sizeof(saveData));
    //
    //         void* pixels = fileTexture.getPixels();
    //         hgAssert(pixels != nullptr);
    //         hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
    //     }
    //
    //     {
    //         HgStringView texPath = "tex";
    //         HgStringView filePath = "hg_test_dir/file_image_test.png";
    //         HgResource texId = hgResourceID(texPath);
    //         HgResource fileId = hgResourceID(filePath);
    //
    //         hgLoadEmptyResource(texId);
    //         HgBinary* pbinRes = hgGetResource(texId);
    //         *pbinRes = bin;
    //         hgDefer({
    //             *hgGetResource(texId) = {};
    //             hgUnloadResource(HgFence{}, texId);
    //         });
    //
    //         HgFence fence = hgFenceCreate();
    //         hgDefer(hgFenceDestroy(fence));
    //
    //         hgExportPng(fence, texId, filePath);
    //         hgImportPng(fence, fileId, filePath);
    //         hgDefer(hgUnloadResource(HgFence{}, fileId));
    //         hgAssert(hgFenceWait(fence, 2.0));
    //
    //         HgImageData fileTexture = *hgGetResource(fileId);
    //
    //         HgFormat format;
    //         u32 width, height, depth;
    //         hgAssert(fileTexture.getInfo(&format, &width, &height, &depth));
    //         hgAssert(format == saveFormat);
    //         hgAssert(width == saveWidth);
    //         hgAssert(height == saveHeight);
    //         hgAssert(depth == saveDepth);
    //         hgAssert(width * height * depth * hgFormatToSize((HgFormat)format) == sizeof(saveData));
    //
    //         void* pixels = fileTexture.getPixels();
    //         hgAssert(pixels != nullptr);
    //         hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
    //     }
    // }

    hgWarn("HgMesh test not implemented yet : TODO\n");
    hgWarn("HgTexture test not implemented yet : TODO\n");
    hgWarn("HgModel test not implemented yet : TODO\n");

    // HgECS
    {
        HgArena* arena = hgScratch();
        HgArenaScope arenaScope{arena};

        HgEcs ecs = ecs.create(arena, 1024, 128);
        ecs.createComponent<u32>(arena, 1024);
        ecs.createComponent<u64>(arena, 1024);

        HgEntity e1 = ecs.spawn();
        HgEntity e2 = ecs.spawn();
        HgEntity e3 = {};
        hgAssert(ecs.alive(e1));
        hgAssert(ecs.alive(e2));
        hgAssert(!ecs.alive(e3));

        ecs.despawn(e1);
        hgAssert(!ecs.alive(e1));
        e3 = ecs.spawn();
        hgAssert(ecs.alive(e3));
        hgAssert(hgHandleIdx(e3.handle) == hgHandleIdx(e1.handle) && e3.handle.id != e1.handle.id);

        e1 = ecs.spawn();
        hgAssert(ecs.alive(e1));

        {
            hgAssert(!ecs.has<u32>(e1));
            hgAssert(!ecs.has<u32>(e2));
            hgAssert(!ecs.has<u32>(e3));

            ecs.add<u32>(e1) = 1;
            hgAssert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hgAssert(!ecs.has<u32>(e2));
            hgAssert(!ecs.has<u32>(e3));
            ecs.add<u32>(e2) = 2;
            hgAssert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hgAssert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hgAssert(!ecs.has<u32>(e3));
            ecs.add<u32>(e3) = 3;
            hgAssert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hgAssert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hgAssert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);

            ecs.remove<u32>(e1);
            hgAssert(!ecs.has<u32>(e1));
            hgAssert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hgAssert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
            ecs.remove<u32>(e2);
            hgAssert(!ecs.has<u32>(e1));
            hgAssert(!ecs.has<u32>(e2));
            hgAssert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
            ecs.remove<u32>(e3);
            hgAssert(!ecs.has<u32>(e1));
            hgAssert(!ecs.has<u32>(e2));
            hgAssert(!ecs.has<u32>(e3));
        }

        {
            bool hasUnknown = false;
            ecs.forEach<u32>([&](HgEntity, u32*)
            {
                hasUnknown = true;
            });
            hgAssert(!hasUnknown);

            hgAssert(ecs.count<u32>() == 0);
            hgAssert(ecs.count<u64>() == 0);
        }

        {
            ecs.add<u32>(e1) = 12;
            ecs.add<u32>(e2) = 42;
            ecs.add<u32>(e3) = 100;
            hgAssert(ecs.count<u32>() == 3);
            hgAssert(ecs.count<u64>() == 0);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            ecs.forEach<u32>([&](HgEntity e, u32* c)
            {
                switch (*c)
                {
                    case 12:
                        has12 = e.handle.id == e1.handle.id;
                        break;
                    case 42:
                        has42 = e.handle.id == e2.handle.id;
                        break;
                    case 100:
                        has100 = e.handle.id == e3.handle.id;
                        break;
                    default:
                        hasUnknown = true;
                        break;
                }
            });
            hgAssert(has12);
            hgAssert(has42);
            hgAssert(has100);
            hgAssert(!hasUnknown);
        }

        {
            ecs.add<u64>(e2) = 2042;
            ecs.add<u64>(e3) = 2100;
            hgAssert(ecs.count<u32>() == 3);
            hgAssert(ecs.count<u64>() == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            bool has2042 = false;
            bool has2100 = false;
            ecs.forEach<u32, u64>([&](HgEntity e, u32* comp32, u64* comp64)
            {
                switch (*comp32)
                {
                    case 12:
                        has12 = e.handle.id == e1.handle.id;
                        break;
                    case 42:
                        has42 = e.handle.id == e2.handle.id;
                        break;
                    case 100:
                        has100 = e.handle.id == e3.handle.id;
                        break;
                    default:
                        hasUnknown = true;
                        break;
                }
                switch (*comp64)
                {
                    case 2042:
                        has2042 = e.handle.id == e2.handle.id;
                        break;
                    case 2100:
                        has2100 = e.handle.id == e3.handle.id;
                        break;
                    default:
                        hasUnknown = true;
                        break;
                }
            });
            hgAssert(!has12);
            hgAssert(has42);
            hgAssert(has100);
            hgAssert(has2042);
            hgAssert(has2100);
            hgAssert(!hasUnknown);
        }

        {
            ecs.despawn(e1);
            hgAssert(ecs.count<u32>() == 2);
            hgAssert(ecs.count<u64>() == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            ecs.forEach<u32>([&](HgEntity e, u32* c)
            {
                switch (*c)
                {
                    case 12:
                        has12 = e.handle.id == e1.handle.id;
                        break;
                    case 42:
                        has42 = e.handle.id == e2.handle.id;
                        break;
                    case 100:
                        has100 = e.handle.id == e3.handle.id;
                        break;
                    default:
                        hasUnknown = true;
                        break;
                }
            });
            hgAssert(!has12);
            hgAssert(has42);
            hgAssert(has100);
            hgAssert(!hasUnknown);
        }

        {
            ecs.despawn(e2);
            hgAssert(ecs.count<u32>() == 1);
            hgAssert(ecs.count<u64>() == 1);
        }

        ecs.reset();
        hgAssert(ecs.count<u32>() == 0);
        hgAssert(ecs.count<u64>() == 0);

        {
            for (u32 i = 0; i < 300; ++i)
            {
                HgEntity e = ecs.spawn();
                switch (i % 3)
                {
                    case 0:
                        ecs.add<u32>(e) = 12;
                        ecs.add<u64>(e) = 42;
                        break;
                    case 1:
                        ecs.add<u32>(e) = 12;
                        break;
                    case 2:
                        ecs.add<u64>(e) = 42;
                        break;
                }
            }

            bool success;
            ecs.forPar<u32>([&](HgEntity, u32* c)
            {
                *c += 4;
            });
            success = true;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != 16)
                    success = false;
            });
            hgAssert(success);

            ecs.forPar<u64>([&](HgEntity, u64* c)
            {
                *c += 3;
            });
            success = true;
            ecs.forEach<u64>([&](HgEntity, u64* c)
            {
                if (*c != 45)
                    success = false;
            });
            hgAssert(success);

            ecs.forPar<u32, u64>([&](HgEntity, u32* c32, u64* c64)
            {
                *c64 -= *c32;
            });
            success = true;
            ecs.forEach<u64>([&](HgEntity e, u64* c)
            {
                if (ecs.has<u32>(e))
                {
                    if (*c != 29)
                        success = false;
                } else {
                    if (*c != 45)
                        success = false;
                }
            });
            hgAssert(success);
        }

        ecs.reset();

        auto comparison = [](void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs)
        {
            return ecs->get<u32>(lhs) < ecs->get<u32>(rhs);
        };

        {
            ecs.add<u32>(ecs.spawn()) = 42;

            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != 42)
                    success = false;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            u32 smallScramble1[]{1, 0};
            for (u32 i = 0; i < sizeof(smallScramble1) / sizeof(*smallScramble1); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = smallScramble1[i];
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.forEach<u32>([&](HgEntity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.forEach<u32>([&](HgEntity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            ecs.reset();
        }

        {
            u32 mediumScramble1[]{8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
            for (u32 i = 0; i < sizeof(mediumScramble1) / sizeof(*mediumScramble1); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = mediumScramble1[i];
            }
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            u32 mediumScramble2[]{3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
            for (u32 i = 0; i < sizeof(mediumScramble2) / sizeof(*mediumScramble2); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = mediumScramble2[i];
            }
            ecs.sort<u32>(nullptr, comparison);
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                ecs.add<u32>(ecs.spawn()) = i;
            }
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                ecs.add<u32>(ecs.spawn()) = i / 2;
            }
            ecs.sort<u32>(nullptr, comparison);
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32* c)
            {
                if (*c != elem / 2)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }
    }

    printf("HurdyGurdy: Tests Complete in %fms\n", hgClockTick(&timer) * 1000.0f);
}

