#include "hurdygurdy.hpp"

#include <thread>

#include <emmintrin.h>

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
        HgQuat rotation = hgAxisAngle(HgVec3{0.0f, 0.0f, -1.0f}, -(f32)hgPi * 0.5f);

        HgVec3 rotatedVec = hgRotate(rotation, upVec);
        HgMat3 rotatedMat = hgRotate(rotation, identityMat);

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

        HgArena arena = {block, 1024};

        for (usize i = 0; i < 3; ++i)
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

    // HgString
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgString a = a.copy(arena, "a");
        hgAssert(a[0] == 'a');
        hgAssert(a.capacity == 1);
        hgAssert(a.length == 1);

        HgString abc = abc.copy(arena, "abc");
        hgAssert(abc[0] == 'a');
        hgAssert(abc[1] == 'b');
        hgAssert(abc[2] == 'c');
        hgAssert(abc.length == 3);
        hgAssert(abc.capacity == 3);

        a.append(arena, "bc");
        hgAssert(a == abc);

        HgString str = str.create(arena, 16);
        hgAssert(str == HgString::create(arena, 0));

        str.append(arena, "hello");
        hgAssert(str == HgString::copy(arena, "hello"));

        str.append(arena, " there");
        hgAssert(str == HgString::copy(arena, "hello there"));

        str.prepend(arena, "why ");
        hgAssert(str == HgString::copy(arena, "why hello there"));

        str.insert(arena, 3, ",");
        hgAssert(str == HgString::copy(arena, "why, hello there"));
    }

    // string utils
    {
        HgArena* arena = hgGetScratch();
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

        hgAssert(hgIsIntenger("0"));
        hgAssert(hgIsIntenger("1"));
        hgAssert(hgIsIntenger("2"));
        hgAssert(hgIsIntenger("3"));
        hgAssert(hgIsIntenger("4"));
        hgAssert(hgIsIntenger("5"));
        hgAssert(hgIsIntenger("6"));
        hgAssert(hgIsIntenger("7"));
        hgAssert(hgIsIntenger("8"));
        hgAssert(hgIsIntenger("9"));
        hgAssert(hgIsIntenger("10"));

        hgAssert(hgIsIntenger("12"));
        hgAssert(hgIsIntenger("42"));
        hgAssert(hgIsIntenger("100"));
        hgAssert(hgIsIntenger("123456789"));
        hgAssert(hgIsIntenger("-12"));
        hgAssert(hgIsIntenger("-42"));
        hgAssert(hgIsIntenger("-100"));
        hgAssert(hgIsIntenger("-123456789"));
        hgAssert(hgIsIntenger("+12"));
        hgAssert(hgIsIntenger("+42"));
        hgAssert(hgIsIntenger("+100"));
        hgAssert(hgIsIntenger("+123456789"));

        hgAssert(!hgIsIntenger("hello"));
        hgAssert(!hgIsIntenger("not a number"));
        hgAssert(!hgIsIntenger("number"));
        hgAssert(!hgIsIntenger("integer"));
        hgAssert(!hgIsIntenger("0.0"));
        hgAssert(!hgIsIntenger("1.0"));
        hgAssert(!hgIsIntenger(".10"));
        hgAssert(!hgIsIntenger("1e2"));
        hgAssert(!hgIsIntenger("1f"));
        hgAssert(!hgIsIntenger("0xff"));
        hgAssert(!hgIsIntenger("--42"));
        hgAssert(!hgIsIntenger("++42"));
        hgAssert(!hgIsIntenger("42-"));
        hgAssert(!hgIsIntenger("42+"));
        hgAssert(!hgIsIntenger("4 2"));
        hgAssert(!hgIsIntenger("4+2"));

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

        hgAssert(hgStrToInt("0") == 0);
        hgAssert(hgStrToInt("1") == 1);
        hgAssert(hgStrToInt("2") == 2);
        hgAssert(hgStrToInt("3") == 3);
        hgAssert(hgStrToInt("4") == 4);
        hgAssert(hgStrToInt("5") == 5);
        hgAssert(hgStrToInt("6") == 6);
        hgAssert(hgStrToInt("7") == 7);
        hgAssert(hgStrToInt("8") == 8);
        hgAssert(hgStrToInt("9") == 9);

        hgAssert(hgStrToInt("0000000") == 0);
        hgAssert(hgStrToInt("+0000001") == +1);
        hgAssert(hgStrToInt("0000002") == 2);
        hgAssert(hgStrToInt("-0000003") == -3);
        hgAssert(hgStrToInt("0000004") == 4);
        hgAssert(hgStrToInt("+0000005") == +5);
        hgAssert(hgStrToInt("0000006") == 6);
        hgAssert(hgStrToInt("-0000007") == -7);
        hgAssert(hgStrToInt("0000008") == 8);
        hgAssert(hgStrToInt("+0000009") == +9);

        hgAssert(hgStrToInt("0000000") == 0);
        hgAssert(hgStrToInt("1000000") == 1000000);
        hgAssert(hgStrToInt("2000000") == 2000000);
        hgAssert(hgStrToInt("3000000") == 3000000);
        hgAssert(hgStrToInt("4000000") == 4000000);
        hgAssert(hgStrToInt("5000000") == 5000000);
        hgAssert(hgStrToInt("6000000") == 6000000);
        hgAssert(hgStrToInt("7000000") == 7000000);
        hgAssert(hgStrToInt("8000000") == 8000000);
        hgAssert(hgStrToInt("9000000") == 9000000);
        hgAssert(hgStrToInt("1234567890") == 1234567890);

        hgAssert(hgStrToFloat("0.0") == 0.0);
        hgAssert(hgStrToFloat("1.0f") == 1.0);
        hgAssert(hgStrToFloat("2.0") == 2.0);
        hgAssert(hgStrToFloat("3.0f") == 3.0);
        hgAssert(hgStrToFloat("4.0") == 4.0);
        hgAssert(hgStrToFloat("5.0f") == 5.0);
        hgAssert(hgStrToFloat("6.0") == 6.0);
        hgAssert(hgStrToFloat("7.0f") == 7.0);
        hgAssert(hgStrToFloat("8.0") == 8.0);
        hgAssert(hgStrToFloat("9.0f") == 9.0);

        hgAssert(hgStrToFloat("0e1") == 0.0);
        hgAssert(hgStrToFloat("1e2f") == 1e2);
        hgAssert(hgStrToFloat("2e3") == 2e3);
        hgAssert(hgStrToFloat("3e4f") == 3e4);
        hgAssert(hgStrToFloat("4e5") == 4e5);
        hgAssert(hgStrToFloat("5e6f") == 5e6);
        hgAssert(hgStrToFloat("6e7") == 6e7);
        hgAssert(hgStrToFloat("7e8f") == 7e8);
        hgAssert(hgStrToFloat("8e9") == 8e9);
        hgAssert(hgStrToFloat("9e10f") == 9e10);

        hgAssert(hgStrToFloat("0e1") == 0.0);
        hgAssert(hgStrToFloat("1e2f") == 1e2);
        hgAssert(hgStrToFloat("2e3") == 2e3);
        hgAssert(hgStrToFloat("3e4f") == 3e4);
        hgAssert(hgStrToFloat("4e5") == 4e5);
        hgAssert(hgStrToFloat("5e6f") == 5e6);
        hgAssert(hgStrToFloat("6e7") == 6e7);
        hgAssert(hgStrToFloat("7e8f") == 7e8);
        hgAssert(hgStrToFloat("8e9") == 8e9);
        hgAssert(hgStrToFloat("9e10f") == 9e10);

        hgAssert(hgStrToFloat(".1") == .1);
        hgAssert(hgStrToFloat("+.1") == +.1);
        hgAssert(hgStrToFloat("-.1") == -.1);
        hgAssert(hgStrToFloat("+.1e5") == +.1e5);

        hgAssert(hgIntToStr(arena, 0) == "0");
        hgAssert(hgIntToStr(arena, -1) == "-1");
        hgAssert(hgIntToStr(arena, 2) == "2");
        hgAssert(hgIntToStr(arena, -3) == "-3");
        hgAssert(hgIntToStr(arena, 4) == "4");
        hgAssert(hgIntToStr(arena, -5) == "-5");
        hgAssert(hgIntToStr(arena, 6) == "6");
        hgAssert(hgIntToStr(arena, -7) == "-7");
        hgAssert(hgIntToStr(arena, 8) == "8");
        hgAssert(hgIntToStr(arena, -9) == "-9");

        hgAssert(hgIntToStr(arena, 0000000) == "0");
        hgAssert(hgIntToStr(arena, -1000000) == "-1000000");
        hgAssert(hgIntToStr(arena, 2000000) == "2000000");
        hgAssert(hgIntToStr(arena, -3000000) == "-3000000");
        hgAssert(hgIntToStr(arena, 4000000) == "4000000");
        hgAssert(hgIntToStr(arena, -5000000) == "-5000000");
        hgAssert(hgIntToStr(arena, 6000000) == "6000000");
        hgAssert(hgIntToStr(arena, -7000000) == "-7000000");
        hgAssert(hgIntToStr(arena, 8000000) == "8000000");
        hgAssert(hgIntToStr(arena, -9000000) == "-9000000");
        hgAssert(hgIntToStr(arena, 1234567890) == "1234567890");

        hgAssert(hgFloatToStr(arena, 0.0, 10) == "0.0");
        hgAssert(hgFloatToStr(arena, -1.0f, 1) == "-1.0");
        hgAssert(hgFloatToStr(arena, 2.0, 2) == "2.00");
        hgAssert(hgFloatToStr(arena, -3.0f, 3) == "-3.000");
        hgAssert(hgFloatToStr(arena, 4.0, 4) == "4.0000");
        hgAssert(hgFloatToStr(arena, -5.0f, 5) == "-5.00000");
        hgAssert(hgFloatToStr(arena, 6.0, 6) == "6.000000");
        hgAssert(hgFloatToStr(arena, -7.0f, 7) == "-7.0000000");
        hgAssert(hgFloatToStr(arena, 8.0, 8) == "8.00000000");
        hgAssert(hgFloatToStr(arena, -9.0f, 9) == "-9.000000000");

        hgAssert(hgFloatToStr(arena, 0e0, 1) == "0.0");
        hgAssert(hgFloatToStr(arena, -1e1f, 0) == "-10.");
        hgAssert(hgFloatToStr(arena, 2e2, 1) == "200.0");
        hgAssert(hgFloatToStr(arena, -3e3f, 0) == "-3000.");
        hgAssert(hgFloatToStr(arena, 4e4, 1) == "40000.0");
        hgAssert(hgFloatToStr(arena, -5e5f, 0) == "-500000.");
        hgAssert(hgFloatToStr(arena, 6e6, 1) == "6000000.0");
        hgAssert(hgFloatToStr(arena, -7e7f, 0) == "-70000000.");
        hgAssert(hgFloatToStr(arena, 8e8, 1) == "800000000.0");
        hgAssert(hgFloatToStr(arena, -9e9f, 0) == "-8999999488.");

        hgAssert(hgFloatToStr(arena, -0e-0, 3) == "0.0");
        hgAssert(hgFloatToStr(arena, 1e-1f, 3) == "0.100");
        hgAssert(hgFloatToStr(arena, -2e-2, 3) == "-0.020");
        hgAssert(hgFloatToStr(arena, 3e-3f, 3) == "0.003");
        hgAssert(hgFloatToStr(arena, -4e-0, 3) == "-4.000");
        hgAssert(hgFloatToStr(arena, 5e-1f, 3) == "0.500");
        hgAssert(hgFloatToStr(arena, -6e-2, 3) == "-0.060");
        hgAssert(hgFloatToStr(arena, 7e-3f, 3) == "0.007");
        hgAssert(hgFloatToStr(arena, -8e-0, 3) == "-8.000");
        hgAssert(hgFloatToStr(arena, 9e-1f, 3) == "0.899");
    }

    // HgJson
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgStringView file = R"(
            {
            }
        )";

        HgJson json = hgParseJson(arena, file);

        hgAssert(json.errors == nullptr);
        hgAssert(json.file != nullptr);

        HgJsonNode* node = json.file;
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::boolean);
        hgAssert(field->value->boolean == true);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::boolean);
        hgAssert(field->value->boolean == false);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields == nullptr);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::string);
        hgAssert(field->value->string == "asdf");
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::integer);
        hgAssert(field->value->integer == 1234);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::floating);
        hgAssert(field->value->floating == 1234.0);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::floating);
        hgAssert(field->value->floating == 1234.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "hjkl");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::floating);
        hgAssert(field->value->floating == 5678.0);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 3);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 3);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 1);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 2);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::integer);
        hgAssert(elem->value->integer == 4);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(node->type == HgJsonType::jstruct);
        hgAssert(node->jstruct.fields != nullptr);

        HgJsonField* field = node->jstruct.fields;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "asdf");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::jstruct);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonField* subField = field->value->jstruct.fields;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "a");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType::integer);
        hgAssert(subField->value->integer == 1);

        subField = subField->next;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "s");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType::floating);
        hgAssert(subField->value->floating == 2.0);

        subField = subField->next;
        hgAssert(subField->next != nullptr);
        hgAssert(subField->name == "d");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType::integer);
        hgAssert(subField->value->integer == 3);

        subField = subField->next;
        hgAssert(subField->next == nullptr);
        hgAssert(subField->name == "f");
        hgAssert(subField->value != nullptr);
        hgAssert(subField->value->type == HgJsonType::floating);
        hgAssert(subField->value->floating == 4.0);
    }

    {
        HgArena* arena = hgGetScratch();
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
        hgAssert(mainStruct->type == HgJsonType::jstruct);
        hgAssert(mainStruct->jstruct.fields != nullptr);

        HgJsonField* player = mainStruct->jstruct.fields;
        hgAssert(player->next == nullptr);
        hgAssert(player->name == "player");
        hgAssert(player->value != nullptr);
        hgAssert(player->value->type == HgJsonType::jstruct);
        hgAssert(player->value->jstruct.fields != nullptr);

        HgJsonField* component = player->value->jstruct.fields;
        hgAssert(component->next != nullptr);
        hgAssert(component->name == "transform");
        hgAssert(component->value != nullptr);
        hgAssert(component->value->type == HgJsonType::jstruct);
        hgAssert(component->value->jstruct.fields != nullptr);

        HgJsonField* field = component->value->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "position");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        HgJsonElem* elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == -1.0);

        field = field->next;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "scale");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "rotation");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        component = component->next;
        hgAssert(component->next == nullptr);
        hgAssert(component->name == "sprite");
        hgAssert(component->value != nullptr);
        hgAssert(component->value->type == HgJsonType::jstruct);
        hgAssert(component->value->jstruct.fields != nullptr);

        field = component->value->jstruct.fields;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "texture");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::string);
        hgAssert(field->value->string == "tex.png");

        field = field->next;
        hgAssert(field->next != nullptr);
        hgAssert(field->name == "uvPos");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 0.0);

        field = field->next;
        hgAssert(field->next == nullptr);
        hgAssert(field->name == "uvSize");
        hgAssert(field->value != nullptr);
        hgAssert(field->value->type == HgJsonType::array);
        hgAssert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hgAssert(elem->next != nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);

        elem = elem->next;
        hgAssert(elem->next == nullptr);
        hgAssert(elem->value != nullptr);
        hgAssert(elem->value->type == HgJsonType::floating);
        hgAssert(elem->value->floating == 1.0);
    }

    // HgHashMap
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        constexpr usize count = 128;

        HgHashMap<u32, u32> map = map.create(arena, count);

        for (usize i = 0; i < 3; ++i)
        {
            hgAssert(map.count == 0);
            hgAssert(!map.has(0));
            hgAssert(!map.has(1));
            hgAssert(!map.has(12));
            hgAssert(!map.has(42));
            hgAssert(!map.has(100000));

            map.add(1) = 1;
            hgAssert(map.count == 1);
            hgAssert(map.has(1));
            hgAssert(map[1] == 1);

            map.remove(1);
            hgAssert(map.count == 0);
            hgAssert(!map.has(1));

            hgAssert(!map.has(12));
            hgAssert(!map.has(12 + count));

            map.add(12) = 42;
            hgAssert(map.count == 1);
            hgAssert(map.has(12) && map[12] == 42);
            hgAssert(!map.has(12 + count));

            map.add(12 + count) = 100;
            hgAssert(map.count == 2);
            hgAssert(map.has(12) && map[12] == 42);
            hgAssert(map.has(12 + count) && map[12 + count] == 100);

            map.add(12 + count * 2) = 200;
            hgAssert(map.count == 3);
            hgAssert(map.has(12) && map[12] == 42);
            hgAssert(map.has(12 + count) && map[12 + count] == 100);
            hgAssert(map.has(12 + count * 2) && map[12 + count * 2] == 200);

            map.remove(12);
            hgAssert(map.count == 2);
            hgAssert(!map.has(12));
            hgAssert(map.has(12 + count) && map[12 + count] == 100);

            map.add(42) = 12;
            hgAssert(map.count == 3);
            hgAssert(map.has(42) && map[42] == 12);

            map.remove(12 + count);
            hgAssert(map.count == 2);
            hgAssert(!map.has(12));
            hgAssert(!map.has(12 + count));

            map.remove(42);
            hgAssert(map.count == 1);
            hgAssert(!map.has(42));

            map.remove(12 + count * 2);
            hgAssert(map.count == 0);
            hgAssert(!map.has(12));
            hgAssert(!map.has(12 + count));
            hgAssert(!map.has(12 + count * 2));

            map.reset();
        }
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        using StrHash = usize;

        HgHashMap<StrHash, u32> map = map.create(arena, 128);

        StrHash a = hgHash("a");
        StrHash b = hgHash("b");
        StrHash ab = hgHash("ab");
        StrHash scf = hgHash("supercalifragilisticexpialidocious");

        hgAssert(!map.has(a));
        hgAssert(!map.has(b));
        hgAssert(!map.has(ab));
        hgAssert(!map.has(scf));

        map.add(a) = 1;
        map.add(b) = 2;
        map.add(ab) = 3;
        map.add(scf) = 4;

        hgAssert(map.has(a) && map[a] == 1);
        hgAssert(map.has(b) && map[b] == 2);
        hgAssert(map.has(ab) && map[ab] == 3);
        hgAssert(map.has(scf) && map[scf] == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hgAssert(!map.has(a));
        hgAssert(!map.has(b));
        hgAssert(!map.has(ab));
        hgAssert(!map.has(scf));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashMap<const char*, u32> map = map.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hgAssert(!map.has(a));
        hgAssert(!map.has(b));
        hgAssert(!map.has(ab));
        hgAssert(!map.has(scf));

        map.add(a) = 1;
        map.add(b) = 2;
        map.add(ab) = 3;
        map.add(scf) = 4;

        hgAssert(map.has(a) && map[a] == 1);
        hgAssert(map.has(b) && map[b] == 2);
        hgAssert(map.has(ab) && map[ab] == 3);
        hgAssert(map.has(scf) && map[scf] == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hgAssert(!map.has(a));
        hgAssert(!map.has(b));
        hgAssert(!map.has(ab));
        hgAssert(!map.has(scf));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashMap<HgString, u32> map = map.create(arena, 128);

        hgAssert(!map.has(HgString::copy(arena, "a")));
        hgAssert(!map.has(HgString::copy(arena, "b")));
        hgAssert(!map.has(HgString::copy(arena, "ab")));
        hgAssert(!map.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));

        map.add(HgString::copy(arena, "a")) = 1;
        map.add(HgString::copy(arena, "b")) = 2;
        map.add(HgString::copy(arena, "ab")) = 3;
        map.add(HgString::copy(arena, "supercalifragilisticexpialidocious")) = 4;

        hgAssert(map.has(HgString::copy(arena, "a")));
        hgAssert(map[HgString::copy(arena, "a")] == 1);
        hgAssert(map.has(HgString::copy(arena, "b")));
        hgAssert(map[HgString::copy(arena, "b")] == 2);
        hgAssert(map.has(HgString::copy(arena, "ab")));
        hgAssert(map[HgString::copy(arena, "ab")] == 3);
        hgAssert(map.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));
        hgAssert(map[HgString::copy(arena, "supercalifragilisticexpialidocious")] == 4);

        map.remove(HgString::copy(arena, "a"));
        map.remove(HgString::copy(arena, "b"));
        map.remove(HgString::copy(arena, "ab"));
        map.remove(HgString::copy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(!map.has(HgString::copy(arena, "a")));
        hgAssert(!map.has(HgString::copy(arena, "b")));
        hgAssert(!map.has(HgString::copy(arena, "ab")));
        hgAssert(!map.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashMap<HgStringView, u32> map = map.create(arena, 128);

        hgAssert(!map.has("a"));
        hgAssert(!map.has("b"));
        hgAssert(!map.has("ab"));
        hgAssert(!map.has("supercalifragilisticexpialidocious"));

        map.add(HgString::copy(arena, "a")) = 1;
        map.add(HgString::copy(arena, "b")) = 2;
        map.add(HgString::copy(arena, "ab")) = 3;
        map.add(HgString::copy(arena, "supercalifragilisticexpialidocious")) = 4;

        hgAssert(map.has("a"));
        hgAssert(map["a"] == 1);
        hgAssert(map.has("b"));
        hgAssert(map["b"] == 2);
        hgAssert(map.has("ab"));
        hgAssert(map["ab"] == 3);
        hgAssert(map.has("supercalifragilisticexpialidocious"));
        hgAssert(map["supercalifragilisticexpialidocious"] == 4);

        map.remove("a");
        map.remove("b");
        map.remove("ab");
        map.remove("supercalifragilisticexpialidocious");

        hgAssert(!map.has("a"));
        hgAssert(!map.has("b"));
        hgAssert(!map.has("ab"));
        hgAssert(!map.has("supercalifragilisticexpialidocious"));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashMap<u32, u32> map = map.create(arena, 64);

        bool hasAny = false;
        map.forEach([&](u32, u32)
        {
            hasAny = true;
        });
        hgAssert(!hasAny);

        map.add(12) = 24;
        map.add(42) = 84;
        map.add(100) = 200;

        bool has12 = false;
        bool has42 = false;
        bool has100 = false;
        bool hasOther = false;
        map.forEach([&](u32 k, u32 v)
        {
            if (k == 12 && v == 24)
                has12 = true;
            else if (k == 42 && v == 84)
                has42 = true;
            else if (k == 100 && v == 200)
                has100 = true;
            else
                hasOther = true;
        });
        hgAssert(has12);
        hgAssert(has42);
        hgAssert(has100);
        hgAssert(!hasOther);

        map.forEach([&](u32 k, u32)
        {
            map.remove(k);
        });

        hasAny = false;
        map.forEach([&](u32, u32)
        {
            hasAny = true;
        });
        hgAssert(!hasAny);
    }

    // HgHashSet
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        constexpr usize count = 128;

        HgHashSet<u32> set = set.create(arena, count);

        for (usize i = 0; i < 3; ++i)
        {
            hgAssert(set.count == 0);
            hgAssert(!set.has(0));
            hgAssert(!set.has(1));
            hgAssert(!set.has(12));
            hgAssert(!set.has(42));
            hgAssert(!set.has(100000));

            set.add(1);
            hgAssert(set.count == 1);
            hgAssert(set.has(1));

            set.remove(1);
            hgAssert(set.count == 0);
            hgAssert(!set.has(1));

            hgAssert(!set.has(12));
            hgAssert(!set.has(12 + count));

            set.add(12);
            hgAssert(set.count == 1);
            hgAssert(set.has(12));
            hgAssert(!set.has(12 + count));

            set.add(12 + count);
            hgAssert(set.count == 2);
            hgAssert(set.has(12));
            hgAssert(set.has(12 + count));

            set.add(12 + count * 2);
            hgAssert(set.count == 3);
            hgAssert(set.has(12));
            hgAssert(set.has(12 + count));
            hgAssert(set.has(12 + count * 2));

            set.remove(12);
            hgAssert(set.count == 2);
            hgAssert(!set.has(12));
            hgAssert(set.has(12 + count));

            set.add(42);
            hgAssert(set.count == 3);
            hgAssert(set.has(42));

            set.remove(12 + count);
            hgAssert(set.count == 2);
            hgAssert(!set.has(12));
            hgAssert(!set.has(12 + count));

            set.remove(42);
            hgAssert(set.count == 1);
            hgAssert(!set.has(42));

            set.remove(12 + count * 2);
            hgAssert(set.count == 0);
            hgAssert(!set.has(12));
            hgAssert(!set.has(12 + count));
            hgAssert(!set.has(12 + count * 2));

            set.reset();
        }
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        using StrHash = usize;

        HgHashSet<StrHash> set = set.create(arena, 128);

        StrHash a = hgHash("a");
        StrHash b = hgHash("b");
        StrHash ab = hgHash("ab");
        StrHash scf = hgHash("supercalifragilisticexpialidocious");

        hgAssert(!set.has(a));
        hgAssert(!set.has(b));
        hgAssert(!set.has(ab));
        hgAssert(!set.has(scf));

        set.add(a);
        set.add(b);
        set.add(ab);
        set.add(scf);

        hgAssert(set.has(a));
        hgAssert(set.has(b));
        hgAssert(set.has(ab));
        hgAssert(set.has(scf));

        set.remove(a);
        set.remove(b);
        set.remove(ab);
        set.remove(scf);

        hgAssert(!set.has(a));
        hgAssert(!set.has(b));
        hgAssert(!set.has(ab));
        hgAssert(!set.has(scf));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashSet<const char*> set = set.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hgAssert(!set.has(a));
        hgAssert(!set.has(b));
        hgAssert(!set.has(ab));
        hgAssert(!set.has(scf));

        set.add(a);
        set.add(b);
        set.add(ab);
        set.add(scf);

        hgAssert(set.has(a));
        hgAssert(set.has(b));
        hgAssert(set.has(ab));
        hgAssert(set.has(scf));

        set.remove(a);
        set.remove(b);
        set.remove(ab);
        set.remove(scf);

        hgAssert(!set.has(a));
        hgAssert(!set.has(b));
        hgAssert(!set.has(ab));
        hgAssert(!set.has(scf));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashSet<HgString> set = set.create(arena, 128);

        hgAssert(!set.has(HgString::copy(arena, "a")));
        hgAssert(!set.has(HgString::copy(arena, "b")));
        hgAssert(!set.has(HgString::copy(arena, "ab")));
        hgAssert(!set.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));

        set.add(HgString::copy(arena, "a"));
        set.add(HgString::copy(arena, "b"));
        set.add(HgString::copy(arena, "ab"));
        set.add(HgString::copy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(set.has(HgString::copy(arena, "a")));
        hgAssert(set.has(HgString::copy(arena, "b")));
        hgAssert(set.has(HgString::copy(arena, "ab")));
        hgAssert(set.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));

        set.remove(HgString::copy(arena, "a"));
        set.remove(HgString::copy(arena, "b"));
        set.remove(HgString::copy(arena, "ab"));
        set.remove(HgString::copy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(!set.has(HgString::copy(arena, "a")));
        hgAssert(!set.has(HgString::copy(arena, "b")));
        hgAssert(!set.has(HgString::copy(arena, "ab")));
        hgAssert(!set.has(HgString::copy(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgHashSet<HgStringView> set = set.create(arena, 128);

        hgAssert(!set.has("a"));
        hgAssert(!set.has("b"));
        hgAssert(!set.has("ab"));
        hgAssert(!set.has("supercalifragilisticexpialidocious"));

        set.add(HgString::copy(arena, "a"));
        set.add(HgString::copy(arena, "b"));
        set.add(HgString::copy(arena, "ab"));
        set.add(HgString::copy(arena, "supercalifragilisticexpialidocious"));

        hgAssert(set.has("a"));
        hgAssert(set.has("b"));
        hgAssert(set.has("ab"));
        hgAssert(set.has("supercalifragilisticexpialidocious"));

        set.remove("a");
        set.remove("b");
        set.remove("ab");
        set.remove("supercalifragilisticexpialidocious");

        hgAssert(!set.has("a"));
        hgAssert(!set.has("b"));
        hgAssert(!set.has("ab"));
        hgAssert(!set.has("supercalifragilisticexpialidocious"));
    }

    // thread pool
    {
        HgFence fence;

        bool a = false;
        bool b = false;

        hgCallPar(&fence, 1, &a, [](void *pa)
        {
            *(bool*)pa = true;
        });
        hgCallPar(&fence, 1, &b, [](void *pb)
        {
            *(bool*)pb = true;
        });

        fence.wait(2.0);

        hgAssert(fence.wait(2.0));

        hgAssert(a == true);
        hgAssert(b == true);
    }

    {
        HgFence fence;

        bool vals[100] = {};
        for (bool& val : vals)
        {
            hgCallPar(&fence, 1, &val, [](void* data)
            {
                *(bool*)data = true;
            });
        }

        hgAssert(hgHelpThreadPool(fence, 2.0));

        for (bool& val : vals)
        {
            hgAssert(val == true);
        }
    }

    {
        bool vals[100] = {};

        hgForPar(sizeof(vals) / sizeof(*vals), 16, [&](usize begin, usize end)
        {
            hgAssert(begin < end && end <= sizeof(vals) / sizeof(*vals));
            for (; begin < end; ++begin)
            {
                vals[begin] = true;
            }
        });

        for (bool& val : vals)
        {
            hgAssert(val == true);
        }
    }

    {
        HgFence fence;

        for (u32 n = 0; n < 3; ++n)
        {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

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
                    hgCallPar(&fence, 1, vals + i, fn);
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

            hgAssert(hgHelpThreadPool(fence, 2.0));
            for (auto val : vals)
            {
                hgAssert(val == true);
            }
        }
    }

    // io thread
    {
        HgFence fence;

        bool vals[100] = {};

        hgRequestIO(&fence, 1, vals, {}, [](void* pvals, HgStringView)
        {
            for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
            {
                ((bool*)pvals)[i] = true;
            }
        });

        hgAssert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        bool vals[100] = {};

        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgRequestIO(&fence, 1, &vals[i], {}, [](void* pval, HgStringView)
            {
                *(bool*)pval = true;
            });
        }

        hgAssert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        bool vals[100] = {};

        vals[0] = true;

        for (usize i = 1; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgRequestIO(&fence, 1, &vals[i], {}, [](void* pval, HgStringView)
            {
                *(bool*)pval = *((bool*)pval - 1);
            });
        }

        hgAssert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i)
        {
            hgAssert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        for (u32 n = 0; n < 3; ++n)
        {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

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
                    hgRequestIO(&fence, 1, &vals[i], {}, [](void* pval, HgStringView)
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

            hgAssert(fence.wait(2.0));
            for (auto val : vals)
            {
                hgAssert(val == true);
            }
        }
    }

    hgWarn("HgResourceManager test not implemented yet : TODO\n");

    // HgBinary
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        u32 saveData[] = {12, 42, 100, 128};

        const char* filePath = "hg_test_dir/file_bin_test.bin";
        HgBinary bin{};

        {
            bin = hgLoadBinary(arena, "file_does_not_exist.bin");
            hgAssert(bin.data == nullptr);
            hgAssert(bin.size == 0);
        }

        {
            bin.data = saveData;
            bin.size = sizeof(saveData);

            hgStoreBinary(bin, "dir/does/not/exist.bin");

            FILE* fileHandle = fopen("dir/does/not/exist.bin", "rb");
            hgAssert(fileHandle == nullptr);
        }

        {
            bin.data = saveData;
            bin.size = sizeof(saveData);

            hgStoreBinary(bin, filePath);
            HgBinary newBin = hgLoadBinary(arena, filePath);

            hgAssert(newBin.data != nullptr);
            hgAssert(newBin.data != saveData);
            hgAssert(newBin.size == sizeof(saveData));
            hgAssert(memcmp(saveData, newBin.data, newBin.size) == 0);
        }
    }

    // HgTextureData
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        struct color {
            u8 r, g, b, a;

            operator u32() { return *(u32*)this; }
        };

        u32 red =    color{0xff, 0x00, 0x00, 0xff};
        u32 green =  color{0x00, 0xff, 0x00, 0xff};
        u32 blue =   color{0x00, 0x00, 0xff, 0xff};
        u32 yellow = color{0xff, 0xff, 0x00, 0xff};

        constexpr VkFormat saveFormat = VK_FORMAT_R8G8B8A8_SRGB;
        constexpr u32 saveWidth = 2;
        constexpr u32 saveHeight = 2;
        constexpr u32 saveDepth = 1;
        u32 saveData[saveWidth][saveHeight] = {
            {red, green},
            {blue, yellow},
        };

        HgBinary bin{};

        {
            HgTextureData::Info info;
            memcpy(info.identifier, HgTextureData::textureIdentifier, sizeof(HgTextureData::textureIdentifier));
            info.format = VK_FORMAT_R8G8B8A8_SRGB;
            info.width = saveWidth;
            info.height = saveHeight;
            info.depth = saveDepth;
            info.pixelsBegin = sizeof(info);
            bin.resize(arena, bin.size + sizeof(HgTextureData::Info));
            bin.overwrite(0, info);

            usize pixelIdx = bin.size;
            bin.resize(arena, bin.size + sizeof(saveData));
            bin.overwrite(pixelIdx, saveData, sizeof(saveData));
        }

        {
            HgTextureData texture = bin;

            VkFormat format;
            u32 width, height, depth;
            hgAssert(texture.getInfo(&format, &width, &height, &depth));
            hgAssert(format == saveFormat);
            hgAssert(width == saveWidth);
            hgAssert(height == saveHeight);
            hgAssert(depth == saveDepth);
            hgAssert(width * height * depth * hgVkFormatToSize(format) == sizeof(saveData));

            void* pixels = texture.getPixels();
            hgAssert(pixels != nullptr);
            hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
        }

        {
            HgStringView filePath = "hg_test_dir/file_image_test.hgtex";

            hgStoreBinary(bin, filePath);
            HgTextureData fileTexture = hgLoadBinary(arena, filePath);

            VkFormat format;
            u32 width, height, depth;
            hgAssert(fileTexture.getInfo(&format, &width, &height, &depth));
            hgAssert(format == saveFormat);
            hgAssert(width == saveWidth);
            hgAssert(height == saveHeight);
            hgAssert(depth == saveDepth);
            hgAssert(width * height * depth * hgVkFormatToSize(format) == sizeof(saveData));

            void* pixels = fileTexture.getPixels();
            hgAssert(pixels != nullptr);
            hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
        }

        {
            HgStringView texPath = "tex";
            HgStringView filePath = "hg_test_dir/file_image_test.png";
            HgResource texId = hgResourceID(texPath);
            HgResource fileId = hgResourceID(filePath);

            hgLoadEmptyResource(texId);
            HgBinary* pbinRes = hgGetResource(texId);
            *pbinRes = bin;
            hgDefer({
                *hgGetResource(texId) = {};
                hgUnloadResource(nullptr, 0, texId);
            });

            HgFence fence;
            hgExportPng(&fence, 1, texId, filePath);
            hgImportPng(&fence, 1, fileId, filePath);
            hgDefer(hgUnloadResource(nullptr, 0, fileId));
            hgAssert(fence.wait(2.0));

            HgTextureData fileTexture = *hgGetResource(fileId);

            VkFormat format;
            u32 width, height, depth;
            hgAssert(fileTexture.getInfo(&format, &width, &height, &depth));
            hgAssert(format == saveFormat);
            hgAssert(width == saveWidth);
            hgAssert(height == saveHeight);
            hgAssert(depth == saveDepth);
            hgAssert(width * height * depth * hgVkFormatToSize(format) == sizeof(saveData));

            void* pixels = fileTexture.getPixels();
            hgAssert(pixels != nullptr);
            hgAssert(memcmp(saveData, pixels, sizeof(saveData)) == 0);
        }
    }

    hgWarn("HgModelData test not implemented yet : TODO\n");
    hgWarn("HgGpuTexture test not implemented yet : TODO\n");
    hgWarn("HgGpuModel test not implemented yet : TODO\n");

    // HgECS
    {
        HgArena* arena = hgGetScratch();
        HgArenaScope arenaScope{arena};

        HgECS ecs = ecs.create(1024);
        hgDefer(ecs.destroy());

        hgDefer(ecs.reset());

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
        hgAssert(e3.idx() == e1.idx() && e3 != e1);

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
            ecs.forEach<u32>([&](HgEntity, u32&)
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
            ecs.forEach<u32>([&](HgEntity e, u32& c)
            {
                switch (c)
                {
                    case 12:
                        has12 = e == e1;
                        break;
                    case 42:
                        has42 = e == e2;
                        break;
                    case 100:
                        has100 = e == e3;
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
            ecs.forEach<u32, u64>([&](HgEntity e, u32& comp32, u64& comp64)
            {
                switch (comp32)
                {
                    case 12:
                        has12 = e == e1;
                        break;
                    case 42:
                        has42 = e == e2;
                        break;
                    case 100:
                        has100 = e == e3;
                        break;
                    default:
                        hasUnknown = true;
                        break;
                }
                switch (comp64)
                {
                    case 2042:
                        has2042 = e == e2;
                        break;
                    case 2100:
                        has2100 = e == e3;
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
            ecs.forEach<u32>([&](HgEntity e, u32& c)
            {
                switch (c)
                {
                    case 12:
                        has12 = e == e1;
                        break;
                    case 42:
                        has42 = e == e2;
                        break;
                    case 100:
                        has100 = e == e3;
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
            ecs.forPar<u32>(16, [&](HgEntity, u32& c)
            {
                c += 4;
            });
            success = true;
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != 16)
                    success = false;
            });
            hgAssert(success);

            ecs.forPar<u64>(16, [&](HgEntity, u64& c)
            {
                c += 3;
            });
            success = true;
            ecs.forEach<u64>([&](HgEntity, u64 c)
            {
                if (c != 45)
                    success = false;
            });
            hgAssert(success);

            ecs.forPar<u32, u64>(16, [&](HgEntity, u32& c32, u64& c64)
            {
                c64 -= c32;
            });
            success = true;
            ecs.forEach<u64>([&](HgEntity e, u64 c)
            {
                if (ecs.has<u32>(e))
                {
                    if (c != 29)
                        success = false;
                } else {
                    if (c != 45)
                        success = false;
                }
            });
            hgAssert(success);
        }

        ecs.reset();

        auto comparison = [](void*, HgECS* ecs, HgEntity lhs, HgEntity rhs)
        {
            return ecs->get<u32>(lhs) < ecs->get<u32>(rhs);
        };

        {
            ecs.add<u32>(ecs.spawn()) = 42;

            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != 42)
                    success = false;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            u32 smallScramble1[] = {1, 0};
            for (u32 i = 0; i < sizeof(smallScramble1) / sizeof(*smallScramble1); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = smallScramble1[i];
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.forEach<u32>([&](HgEntity, u32 c)
                {
                    if (c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.forEach<u32>([&](HgEntity, u32 c)
                {
                    if (c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            ecs.reset();
        }

        {
            u32 mediumScramble1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
            for (u32 i = 0; i < sizeof(mediumScramble1) / sizeof(*mediumScramble1); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = mediumScramble1[i];
            }
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }

        {
            u32 mediumScramble2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
            for (u32 i = 0; i < sizeof(mediumScramble2) / sizeof(*mediumScramble2); ++i)
            {
                ecs.add<u32>(ecs.spawn()) = mediumScramble2[i];
            }
            ecs.sort<u32>(nullptr, comparison);
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != elem)
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
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != elem)
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
            ecs.forEach<u32>([&](HgEntity, u32 c)
            {
                if (c != elem / 2)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            ecs.reset();
        }
    }

    printf("HurdyGurdy: Tests Complete in %fms\n", timer.tick() * 1000.0f);
}

