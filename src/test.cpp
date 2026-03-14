#include "hurdygurdy.hpp"

#include <thread>

#include <emmintrin.h>

void hg_test() {
    printf("HurdyGurdy: Tests Begun\n");

    HgClock timer{};

    // HgMat
    {
        HgMat2 mat{
            {1.0f, 0.0f},
            {1.0f, 0.0f},
        };
        HgVec2 vec{1.0f, 1.0f};

        HgMat2 identity{
            {1.0f, 0.0f},
            {0.0f, 1.0f},
        };
        hg_assert(identity * mat == mat);
        hg_assert(identity * vec == vec);

        HgMat2 mat_rotated{
            {0.0f, 1.0f},
            {0.0f, 1.0f},
        };
        HgVec2 vec_rotated{-1.0f, 1.0f};

        HgMat2 rotation{
            {0.0f, 1.0f},
            {-1.0f, 0.0f},
        };
        hg_assert(rotation * mat == mat_rotated);
        hg_assert(rotation * vec == vec_rotated);

        hg_assert((identity * rotation) * mat == identity * (rotation * mat));
        hg_assert((identity * rotation) * vec == identity * (rotation * vec));
        hg_assert((rotation * rotation) * mat == rotation * (rotation * mat));
        hg_assert((rotation * rotation) * vec == rotation * (rotation * vec));
    }

    // HgQuat
    {
        HgMat3 identity_mat = 1.0f;
        HgVec3 up_vec{0.0f, -1.0f, 0.0f};
        HgQuat rotation = hg_axis_angle({0.0f, 0.0f, -1.0f}, -(f32)hg_pi * 0.5f);

        HgVec3 rotated_vec = hg_rotate(rotation, up_vec);
        HgMat3 rotated_mat = hg_rotate(rotation, identity_mat);

        HgVec3 mat_rotated_vec = rotated_mat * up_vec;

        hg_assert(abs(rotated_vec.x - 1.0f) < FLT_EPSILON
                    && abs(rotated_vec.y - 0.0f) < FLT_EPSILON
                    && abs(rotated_vec.y - 0.0f) < FLT_EPSILON);

        hg_assert(abs(mat_rotated_vec.x - rotated_vec.x) < FLT_EPSILON
                    && abs(mat_rotated_vec.y - rotated_vec.y) < FLT_EPSILON
                    && abs(mat_rotated_vec.y - rotated_vec.z) < FLT_EPSILON);
    }

    // HgArena
    {
        void* block = malloc(1024);
        hg_defer(free(block));

        HgArena arena{block, 1024};

        for (usize i = 0; i < 3; ++i) {
            hg_assert(arena.memory != nullptr);
            hg_assert(arena.capacity == 1024);
            hg_assert(arena.head == 0);

            u32* alloc_u32 = arena.alloc<u32>(1);
            hg_assert(alloc_u32 == arena.memory);

            u64* alloc_u64 = arena.alloc<u64>(2);
            hg_assert((u8*)alloc_u64 == (u8*)alloc_u32 + 8);

            u8* alloc_u8 = arena.alloc<u8>(1);
            hg_assert(alloc_u8 == (u8*)alloc_u32 + 24);

            struct Big {
                u8 data[32];
            };
            Big* alloc_big = arena.alloc<Big>(1);
            hg_assert((u8*)alloc_big == (u8*)alloc_u32 + 25);

            Big* realloc_big = arena.realloc(alloc_big, 1, 2);
            hg_assert(realloc_big == alloc_big);

            Big* realloc_big_same = arena.realloc(realloc_big, 2, 2);
            hg_assert(realloc_big_same == realloc_big);

            memset(realloc_big, 2, 2 * sizeof(*realloc_big));
            u8* alloc_interrupt = arena.alloc<u8>(1);
            (void)alloc_interrupt;

            Big* realloc_big2 = arena.realloc(realloc_big, 2, 4);
            hg_assert(realloc_big2 != realloc_big);
            hg_assert(memcmp(realloc_big, realloc_big2, 2 * sizeof(*realloc_big)) == 0);

            arena.head = 0;
        }
    }

    // HgString
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgString a = a.create(arena, "a");
        hg_assert(a[0] == 'a');
        hg_assert(a.capacity == 1);
        hg_assert(a.length == 1);

        HgString abc = abc.create(arena, "abc");
        hg_assert(abc[0] == 'a');
        hg_assert(abc[1] == 'b');
        hg_assert(abc[2] == 'c');
        hg_assert(abc.length == 3);
        hg_assert(abc.capacity == 3);

        a.append(arena, "bc");
        hg_assert(a == abc);

        HgString str = str.create(arena, 16);
        hg_assert(str == HgString::create(arena, 0));

        str.append(arena, "hello");
        hg_assert(str == HgString::create(arena, "hello"));

        str.append(arena, " there");
        hg_assert(str == HgString::create(arena, "hello there"));

        str.prepend(arena, "why ");
        hg_assert(str == HgString::create(arena, "why hello there"));

        str.insert(arena, 3, ",");
        hg_assert(str == HgString::create(arena, "why, hello there"));
    }

    // string utils
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        hg_assert(hg_is_whitespace(' '));
        hg_assert(hg_is_whitespace('\t'));
        hg_assert(hg_is_whitespace('\n'));

        hg_assert(hg_is_numeral_base10('0'));
        hg_assert(hg_is_numeral_base10('1'));
        hg_assert(hg_is_numeral_base10('2'));
        hg_assert(hg_is_numeral_base10('3'));
        hg_assert(hg_is_numeral_base10('4'));
        hg_assert(hg_is_numeral_base10('5'));
        hg_assert(hg_is_numeral_base10('5'));
        hg_assert(hg_is_numeral_base10('6'));
        hg_assert(hg_is_numeral_base10('7'));
        hg_assert(hg_is_numeral_base10('8'));
        hg_assert(hg_is_numeral_base10('9'));

        hg_assert(!hg_is_numeral_base10('0' - 1));
        hg_assert(!hg_is_numeral_base10('9' + 1));

        hg_assert(!hg_is_numeral_base10('x'));
        hg_assert(!hg_is_numeral_base10('a'));
        hg_assert(!hg_is_numeral_base10('b'));
        hg_assert(!hg_is_numeral_base10('c'));
        hg_assert(!hg_is_numeral_base10('d'));
        hg_assert(!hg_is_numeral_base10('e'));
        hg_assert(!hg_is_numeral_base10('f'));
        hg_assert(!hg_is_numeral_base10('X'));
        hg_assert(!hg_is_numeral_base10('A'));
        hg_assert(!hg_is_numeral_base10('B'));
        hg_assert(!hg_is_numeral_base10('C'));
        hg_assert(!hg_is_numeral_base10('D'));
        hg_assert(!hg_is_numeral_base10('E'));
        hg_assert(!hg_is_numeral_base10('F'));

        hg_assert(!hg_is_numeral_base10('.'));
        hg_assert(!hg_is_numeral_base10('+'));
        hg_assert(!hg_is_numeral_base10('-'));
        hg_assert(!hg_is_numeral_base10('*'));
        hg_assert(!hg_is_numeral_base10('/'));
        hg_assert(!hg_is_numeral_base10('='));
        hg_assert(!hg_is_numeral_base10('#'));
        hg_assert(!hg_is_numeral_base10('&'));
        hg_assert(!hg_is_numeral_base10('^'));
        hg_assert(!hg_is_numeral_base10('~'));

        hg_assert(hg_is_integer_base10("0"));
        hg_assert(hg_is_integer_base10("1"));
        hg_assert(hg_is_integer_base10("2"));
        hg_assert(hg_is_integer_base10("3"));
        hg_assert(hg_is_integer_base10("4"));
        hg_assert(hg_is_integer_base10("5"));
        hg_assert(hg_is_integer_base10("6"));
        hg_assert(hg_is_integer_base10("7"));
        hg_assert(hg_is_integer_base10("8"));
        hg_assert(hg_is_integer_base10("9"));
        hg_assert(hg_is_integer_base10("10"));

        hg_assert(hg_is_integer_base10("12"));
        hg_assert(hg_is_integer_base10("42"));
        hg_assert(hg_is_integer_base10("100"));
        hg_assert(hg_is_integer_base10("123456789"));
        hg_assert(hg_is_integer_base10("-12"));
        hg_assert(hg_is_integer_base10("-42"));
        hg_assert(hg_is_integer_base10("-100"));
        hg_assert(hg_is_integer_base10("-123456789"));
        hg_assert(hg_is_integer_base10("+12"));
        hg_assert(hg_is_integer_base10("+42"));
        hg_assert(hg_is_integer_base10("+100"));
        hg_assert(hg_is_integer_base10("+123456789"));

        hg_assert(!hg_is_integer_base10("hello"));
        hg_assert(!hg_is_integer_base10("not a number"));
        hg_assert(!hg_is_integer_base10("number"));
        hg_assert(!hg_is_integer_base10("integer"));
        hg_assert(!hg_is_integer_base10("0.0"));
        hg_assert(!hg_is_integer_base10("1.0"));
        hg_assert(!hg_is_integer_base10(".10"));
        hg_assert(!hg_is_integer_base10("1e2"));
        hg_assert(!hg_is_integer_base10("1f"));
        hg_assert(!hg_is_integer_base10("0xff"));
        hg_assert(!hg_is_integer_base10("--42"));
        hg_assert(!hg_is_integer_base10("++42"));
        hg_assert(!hg_is_integer_base10("42-"));
        hg_assert(!hg_is_integer_base10("42+"));
        hg_assert(!hg_is_integer_base10("4 2"));
        hg_assert(!hg_is_integer_base10("4+2"));

        hg_assert(hg_is_float_base10("0.0"));
        hg_assert(hg_is_float_base10("1."));
        hg_assert(hg_is_float_base10("2.0"));
        hg_assert(hg_is_float_base10("3."));
        hg_assert(hg_is_float_base10("4.0"));
        hg_assert(hg_is_float_base10("5."));
        hg_assert(hg_is_float_base10("6.0"));
        hg_assert(hg_is_float_base10("7."));
        hg_assert(hg_is_float_base10("8.0"));
        hg_assert(hg_is_float_base10("9."));
        hg_assert(hg_is_float_base10("10.0"));

        hg_assert(hg_is_float_base10("0.0"));
        hg_assert(hg_is_float_base10(".1"));
        hg_assert(hg_is_float_base10("0.2"));
        hg_assert(hg_is_float_base10(".3"));
        hg_assert(hg_is_float_base10("0.4"));
        hg_assert(hg_is_float_base10(".5"));
        hg_assert(hg_is_float_base10("0.6"));
        hg_assert(hg_is_float_base10(".7"));
        hg_assert(hg_is_float_base10("0.8"));
        hg_assert(hg_is_float_base10(".9"));
        hg_assert(hg_is_float_base10("0.10"));

        hg_assert(hg_is_float_base10("1.0"));
        hg_assert(hg_is_float_base10("+10.f"));
        hg_assert(hg_is_float_base10(".10"));
        hg_assert(hg_is_float_base10("-999.999f"));
        hg_assert(hg_is_float_base10("1e3"));
        hg_assert(hg_is_float_base10("1e3"));
        hg_assert(hg_is_float_base10("+1.e3f"));
        hg_assert(hg_is_float_base10(".1e3"));

        hg_assert(!hg_is_float_base10("hello"));
        hg_assert(!hg_is_float_base10("not a number"));
        hg_assert(!hg_is_float_base10("number"));
        hg_assert(!hg_is_float_base10("float"));
        hg_assert(!hg_is_float_base10("1.0ff"));
        hg_assert(!hg_is_float_base10("0x1.0"));
        hg_assert(!hg_is_float_base10("-0x1.0"));

        hg_assert(hg_str_to_int_base10("0") == 0);
        hg_assert(hg_str_to_int_base10("1") == 1);
        hg_assert(hg_str_to_int_base10("2") == 2);
        hg_assert(hg_str_to_int_base10("3") == 3);
        hg_assert(hg_str_to_int_base10("4") == 4);
        hg_assert(hg_str_to_int_base10("5") == 5);
        hg_assert(hg_str_to_int_base10("6") == 6);
        hg_assert(hg_str_to_int_base10("7") == 7);
        hg_assert(hg_str_to_int_base10("8") == 8);
        hg_assert(hg_str_to_int_base10("9") == 9);

        hg_assert(hg_str_to_int_base10("0000000") == 0);
        hg_assert(hg_str_to_int_base10("+0000001") == +1);
        hg_assert(hg_str_to_int_base10("0000002") == 2);
        hg_assert(hg_str_to_int_base10("-0000003") == -3);
        hg_assert(hg_str_to_int_base10("0000004") == 4);
        hg_assert(hg_str_to_int_base10("+0000005") == +5);
        hg_assert(hg_str_to_int_base10("0000006") == 6);
        hg_assert(hg_str_to_int_base10("-0000007") == -7);
        hg_assert(hg_str_to_int_base10("0000008") == 8);
        hg_assert(hg_str_to_int_base10("+0000009") == +9);

        hg_assert(hg_str_to_int_base10("0000000") == 0);
        hg_assert(hg_str_to_int_base10("1000000") == 1000000);
        hg_assert(hg_str_to_int_base10("2000000") == 2000000);
        hg_assert(hg_str_to_int_base10("3000000") == 3000000);
        hg_assert(hg_str_to_int_base10("4000000") == 4000000);
        hg_assert(hg_str_to_int_base10("5000000") == 5000000);
        hg_assert(hg_str_to_int_base10("6000000") == 6000000);
        hg_assert(hg_str_to_int_base10("7000000") == 7000000);
        hg_assert(hg_str_to_int_base10("8000000") == 8000000);
        hg_assert(hg_str_to_int_base10("9000000") == 9000000);
        hg_assert(hg_str_to_int_base10("1234567890") == 1234567890);

        hg_assert(hg_str_to_float_base10("0.0") == 0.0);
        hg_assert(hg_str_to_float_base10("1.0f") == 1.0);
        hg_assert(hg_str_to_float_base10("2.0") == 2.0);
        hg_assert(hg_str_to_float_base10("3.0f") == 3.0);
        hg_assert(hg_str_to_float_base10("4.0") == 4.0);
        hg_assert(hg_str_to_float_base10("5.0f") == 5.0);
        hg_assert(hg_str_to_float_base10("6.0") == 6.0);
        hg_assert(hg_str_to_float_base10("7.0f") == 7.0);
        hg_assert(hg_str_to_float_base10("8.0") == 8.0);
        hg_assert(hg_str_to_float_base10("9.0f") == 9.0);

        hg_assert(hg_str_to_float_base10("0e1") == 0.0);
        hg_assert(hg_str_to_float_base10("1e2f") == 1e2);
        hg_assert(hg_str_to_float_base10("2e3") == 2e3);
        hg_assert(hg_str_to_float_base10("3e4f") == 3e4);
        hg_assert(hg_str_to_float_base10("4e5") == 4e5);
        hg_assert(hg_str_to_float_base10("5e6f") == 5e6);
        hg_assert(hg_str_to_float_base10("6e7") == 6e7);
        hg_assert(hg_str_to_float_base10("7e8f") == 7e8);
        hg_assert(hg_str_to_float_base10("8e9") == 8e9);
        hg_assert(hg_str_to_float_base10("9e10f") == 9e10);

        hg_assert(hg_str_to_float_base10("0e1") == 0.0);
        hg_assert(hg_str_to_float_base10("1e2f") == 1e2);
        hg_assert(hg_str_to_float_base10("2e3") == 2e3);
        hg_assert(hg_str_to_float_base10("3e4f") == 3e4);
        hg_assert(hg_str_to_float_base10("4e5") == 4e5);
        hg_assert(hg_str_to_float_base10("5e6f") == 5e6);
        hg_assert(hg_str_to_float_base10("6e7") == 6e7);
        hg_assert(hg_str_to_float_base10("7e8f") == 7e8);
        hg_assert(hg_str_to_float_base10("8e9") == 8e9);
        hg_assert(hg_str_to_float_base10("9e10f") == 9e10);

        hg_assert(hg_str_to_float_base10(".1") == .1);
        hg_assert(hg_str_to_float_base10("+.1") == +.1);
        hg_assert(hg_str_to_float_base10("-.1") == -.1);
        hg_assert(hg_str_to_float_base10("+.1e5") == +.1e5);

        hg_assert(hg_int_to_str_base10(arena, 0) == "0");
        hg_assert(hg_int_to_str_base10(arena, -1) == "-1");
        hg_assert(hg_int_to_str_base10(arena, 2) == "2");
        hg_assert(hg_int_to_str_base10(arena, -3) == "-3");
        hg_assert(hg_int_to_str_base10(arena, 4) == "4");
        hg_assert(hg_int_to_str_base10(arena, -5) == "-5");
        hg_assert(hg_int_to_str_base10(arena, 6) == "6");
        hg_assert(hg_int_to_str_base10(arena, -7) == "-7");
        hg_assert(hg_int_to_str_base10(arena, 8) == "8");
        hg_assert(hg_int_to_str_base10(arena, -9) == "-9");

        hg_assert(hg_int_to_str_base10(arena, 0000000) == "0");
        hg_assert(hg_int_to_str_base10(arena, -1000000) == "-1000000");
        hg_assert(hg_int_to_str_base10(arena, 2000000) == "2000000");
        hg_assert(hg_int_to_str_base10(arena, -3000000) == "-3000000");
        hg_assert(hg_int_to_str_base10(arena, 4000000) == "4000000");
        hg_assert(hg_int_to_str_base10(arena, -5000000) == "-5000000");
        hg_assert(hg_int_to_str_base10(arena, 6000000) == "6000000");
        hg_assert(hg_int_to_str_base10(arena, -7000000) == "-7000000");
        hg_assert(hg_int_to_str_base10(arena, 8000000) == "8000000");
        hg_assert(hg_int_to_str_base10(arena, -9000000) == "-9000000");
        hg_assert(hg_int_to_str_base10(arena, 1234567890) == "1234567890");

        hg_assert(hg_float_to_str_base10(arena, 0.0, 10) == "0.0");
        hg_assert(hg_float_to_str_base10(arena, -1.0f, 1) == "-1.0");
        hg_assert(hg_float_to_str_base10(arena, 2.0, 2) == "2.00");
        hg_assert(hg_float_to_str_base10(arena, -3.0f, 3) == "-3.000");
        hg_assert(hg_float_to_str_base10(arena, 4.0, 4) == "4.0000");
        hg_assert(hg_float_to_str_base10(arena, -5.0f, 5) == "-5.00000");
        hg_assert(hg_float_to_str_base10(arena, 6.0, 6) == "6.000000");
        hg_assert(hg_float_to_str_base10(arena, -7.0f, 7) == "-7.0000000");
        hg_assert(hg_float_to_str_base10(arena, 8.0, 8) == "8.00000000");
        hg_assert(hg_float_to_str_base10(arena, -9.0f, 9) == "-9.000000000");

        hg_assert(hg_float_to_str_base10(arena, 0e0, 1) == "0.0");
        hg_assert(hg_float_to_str_base10(arena, -1e1f, 0) == "-10.");
        hg_assert(hg_float_to_str_base10(arena, 2e2, 1) == "200.0");
        hg_assert(hg_float_to_str_base10(arena, -3e3f, 0) == "-3000.");
        hg_assert(hg_float_to_str_base10(arena, 4e4, 1) == "40000.0");
        hg_assert(hg_float_to_str_base10(arena, -5e5f, 0) == "-500000.");
        hg_assert(hg_float_to_str_base10(arena, 6e6, 1) == "6000000.0");
        hg_assert(hg_float_to_str_base10(arena, -7e7f, 0) == "-70000000.");
        hg_assert(hg_float_to_str_base10(arena, 8e8, 1) == "800000000.0");
        hg_assert(hg_float_to_str_base10(arena, -9e9f, 0) == "-8999999488.");

        hg_assert(hg_float_to_str_base10(arena, -0e-0, 3) == "0.0");
        hg_assert(hg_float_to_str_base10(arena, 1e-1f, 3) == "0.100");
        hg_assert(hg_float_to_str_base10(arena, -2e-2, 3) == "-0.020");
        hg_assert(hg_float_to_str_base10(arena, 3e-3f, 3) == "0.003");
        hg_assert(hg_float_to_str_base10(arena, -4e-0, 3) == "-4.000");
        hg_assert(hg_float_to_str_base10(arena, 5e-1f, 3) == "0.500");
        hg_assert(hg_float_to_str_base10(arena, -6e-2, 3) == "-0.060");
        hg_assert(hg_float_to_str_base10(arena, 7e-3f, 3) == "0.007");
        hg_assert(hg_float_to_str_base10(arena, -8e-0, 3) == "-8.000");
        hg_assert(hg_float_to_str_base10(arena, 9e-1f, 3) == "0.899");
    }

    // HgJson
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors != nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_assert(error->next == nullptr);
        hg_assert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors != nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_assert(error->next == nullptr);
        hg_assert(error->msg == "on line 4, struct has a literal instead of a field\n");

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf":
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors != nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_assert(error->next != nullptr);
        hg_assert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_assert(error->next == nullptr);
        hg_assert(error->msg == "on line 4, found unexpected token \"}\"\n");

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": true
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::boolean);
        hg_assert(field->value->boolean == true);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": false
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::boolean);
        hg_assert(field->value->boolean == false);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": asdf
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors != nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_assert(error->next != nullptr);
        hg_assert(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
        error = error->next;
        hg_assert(error->next == nullptr);
        hg_assert(error->msg == "on line 3, found unexpected token \"asdf\"\n");

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields == nullptr);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": "asdf"
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::string);
        hg_assert(field->value->string == "asdf");
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::integer);
        hg_assert(field->value->integer == 1234);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::floating);
        hg_assert(field->value->floating == 1234.0);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": 1234.0,
                "hjkl": 5678.0
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next != nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::floating);
        hg_assert(field->value->floating == 1234.0);

        field = field->next;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "hjkl");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::floating);
        hg_assert(field->value->floating == 5678.0);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, 3, 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 3);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 4);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1 2 3 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 3);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 4);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgStringView file = R"(
            {
                "asdf": [1, 2, "3", 4]
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors != nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Error* error = json.errors;
        hg_assert(error->next == nullptr);
        hg_assert(error->msg ==
            "on line 3, array has element which is not the same type as the first valid element\n");

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 1);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 2);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::integer);
        hg_assert(elem->value->integer == 4);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* node = json.file;
        hg_assert(node->type == HgJson::jstruct);
        hg_assert(node->jstruct.fields != nullptr);

        HgJson::Field* field = node->jstruct.fields;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "asdf");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::jstruct);
        hg_assert(field->value->array.elems != nullptr);

        HgJson::Field* sub_field = field->value->jstruct.fields;
        hg_assert(sub_field->next != nullptr);
        hg_assert(sub_field->name == "a");
        hg_assert(sub_field->value != nullptr);
        hg_assert(sub_field->value->type == HgJson::integer);
        hg_assert(sub_field->value->integer == 1);

        sub_field = sub_field->next;
        hg_assert(sub_field->next != nullptr);
        hg_assert(sub_field->name == "s");
        hg_assert(sub_field->value != nullptr);
        hg_assert(sub_field->value->type == HgJson::floating);
        hg_assert(sub_field->value->floating == 2.0);

        sub_field = sub_field->next;
        hg_assert(sub_field->next != nullptr);
        hg_assert(sub_field->name == "d");
        hg_assert(sub_field->value != nullptr);
        hg_assert(sub_field->value->type == HgJson::integer);
        hg_assert(sub_field->value->integer == 3);

        sub_field = sub_field->next;
        hg_assert(sub_field->next == nullptr);
        hg_assert(sub_field->name == "f");
        hg_assert(sub_field->value != nullptr);
        hg_assert(sub_field->value->type == HgJson::floating);
        hg_assert(sub_field->value->floating == 4.0);
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

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
                        "uv_pos": [0.0, 0.0],
                        "uv_size": [1.0, 1.0]
                    }
                }
            }
        )";

        HgJson json = json.parse(arena, file);

        hg_assert(json.errors == nullptr);
        hg_assert(json.file != nullptr);

        HgJson::Node* main_struct = json.file;
        hg_assert(main_struct->type == HgJson::jstruct);
        hg_assert(main_struct->jstruct.fields != nullptr);

        HgJson::Field* player = main_struct->jstruct.fields;
        hg_assert(player->next == nullptr);
        hg_assert(player->name == "player");
        hg_assert(player->value != nullptr);
        hg_assert(player->value->type == HgJson::jstruct);
        hg_assert(player->value->jstruct.fields != nullptr);

        HgJson::Field* component = player->value->jstruct.fields;
        hg_assert(component->next != nullptr);
        hg_assert(component->name == "transform");
        hg_assert(component->value != nullptr);
        hg_assert(component->value->type == HgJson::jstruct);
        hg_assert(component->value->jstruct.fields != nullptr);

        HgJson::Field* field = component->value->jstruct.fields;
        hg_assert(field->next != nullptr);
        hg_assert(field->name == "position");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        HgJson::Elem* elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == -1.0);

        field = field->next;
        hg_assert(field->next != nullptr);
        hg_assert(field->name == "scale");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        field = field->next;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "rotation");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        component = component->next;
        hg_assert(component->next == nullptr);
        hg_assert(component->name == "sprite");
        hg_assert(component->value != nullptr);
        hg_assert(component->value->type == HgJson::jstruct);
        hg_assert(component->value->jstruct.fields != nullptr);

        field = component->value->jstruct.fields;
        hg_assert(field->next != nullptr);
        hg_assert(field->name == "texture");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::string);
        hg_assert(field->value->string == "tex.png");

        field = field->next;
        hg_assert(field->next != nullptr);
        hg_assert(field->name == "uv_pos");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 0.0);

        field = field->next;
        hg_assert(field->next == nullptr);
        hg_assert(field->name == "uv_size");
        hg_assert(field->value != nullptr);
        hg_assert(field->value->type == HgJson::array);
        hg_assert(field->value->array.elems != nullptr);

        elem = field->value->array.elems;
        hg_assert(elem->next != nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);

        elem = elem->next;
        hg_assert(elem->next == nullptr);
        hg_assert(elem->value != nullptr);
        hg_assert(elem->value->type == HgJson::floating);
        hg_assert(elem->value->floating == 1.0);
    }

    // HgHashMap
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        constexpr usize count = 128;

        HgHashMap<u32, u32> map = map.create(arena, count);

        for (usize i = 0; i < 3; ++i) {
            hg_assert(map.load == 0);
            hg_assert(!map.has(0));
            hg_assert(!map.has(1));
            hg_assert(!map.has(12));
            hg_assert(!map.has(42));
            hg_assert(!map.has(100000));

            map.insert(1, 1);
            hg_assert(map.load == 1);
            hg_assert(map.has(1));
            hg_assert(*map.get(1) == 1);

            map.remove(1);
            hg_assert(map.load == 0);
            hg_assert(!map.has(1));
            hg_assert(map.get(1) == nullptr);

            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));

            map.insert(12, 42);
            hg_assert(map.load == 1);
            hg_assert(map.has(12) && *map.get(12) == 42);
            hg_assert(!map.has(12 + count));

            map.insert(12 + count, 100);
            hg_assert(map.load == 2);
            hg_assert(map.has(12) && *map.get(12) == 42);
            hg_assert(map.has(12 + count) && *map.get(12 + count) == 100);

            map.insert(12 + count * 2, 200);
            hg_assert(map.load == 3);
            hg_assert(map.has(12) && *map.get(12) == 42);
            hg_assert(map.has(12 + count) && *map.get(12 + count) == 100);
            hg_assert(map.has(12 + count * 2) && *map.get(12 + count * 2) == 200);

            map.remove(12);
            hg_assert(map.load == 2);
            hg_assert(!map.has(12));
            hg_assert(map.has(12 + count) && *map.get(12 + count) == 100);

            map.insert(42, 12);
            hg_assert(map.load == 3);
            hg_assert(map.has(42) && *map.get(42) == 12);

            map.remove(12 + count);
            hg_assert(map.load == 2);
            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));

            map.remove(42);
            hg_assert(map.load == 1);
            hg_assert(!map.has(42));

            map.remove(12 + count * 2);
            hg_assert(map.load == 0);
            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));
            hg_assert(!map.has(12 + count * 2));

            map.reset();
        }
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        using StrHash = usize;

        HgHashMap<StrHash, u32> map = map.create(arena, 128);

        StrHash a = hg_hash("a");
        StrHash b = hg_hash("b");
        StrHash ab = hg_hash("ab");
        StrHash scf = hg_hash("supercalifragilisticexpialidocious");

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));

        map.insert(a, 1);
        map.insert(b, 2);
        map.insert(ab, 3);
        map.insert(scf, 4);

        hg_assert(map.has(a) && *map.get(a) == 1);
        hg_assert(map.has(b) && *map.get(b) == 2);
        hg_assert(map.has(ab) && *map.get(ab) == 3);
        hg_assert(map.has(scf) && *map.get(scf) == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashMap<const char*, u32> map = map.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));

        map.insert(a, 1);
        map.insert(b, 2);
        map.insert(ab, 3);
        map.insert(scf, 4);

        hg_assert(map.has(a) && *map.get(a) == 1);
        hg_assert(map.has(b) && *map.get(b) == 2);
        hg_assert(map.has(ab) && *map.get(ab) == 3);
        hg_assert(map.has(scf) && *map.get(scf) == 4);

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashMap<HgString, u32> map = map.create(arena, 128);

        hg_assert(!map.has(HgString::create(arena, "a")));
        hg_assert(!map.has(HgString::create(arena, "b")));
        hg_assert(!map.has(HgString::create(arena, "ab")));
        hg_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.insert(HgString::create(arena, "a"), 1);
        map.insert(HgString::create(arena, "b"), 2);
        map.insert(HgString::create(arena, "ab"), 3);
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"), 4);

        hg_assert(map.has(HgString::create(arena, "a")));
        hg_assert(*map.get(HgString::create(arena, "a")) == 1);
        hg_assert(map.has(HgString::create(arena, "b")));
        hg_assert(*map.get(HgString::create(arena, "b")) == 2);
        hg_assert(map.has(HgString::create(arena, "ab")));
        hg_assert(*map.get(HgString::create(arena, "ab")) == 3);
        hg_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
        hg_assert(*map.get(HgString::create(arena, "supercalifragilisticexpialidocious")) == 4);

        map.remove(HgString::create(arena, "a"));
        map.remove(HgString::create(arena, "b"));
        map.remove(HgString::create(arena, "ab"));
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_assert(!map.has(HgString::create(arena, "a")));
        hg_assert(!map.has(HgString::create(arena, "b")));
        hg_assert(!map.has(HgString::create(arena, "ab")));
        hg_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashMap<HgStringView, u32> map = map.create(arena, 128);

        hg_assert(!map.has("a"));
        hg_assert(!map.has("b"));
        hg_assert(!map.has("ab"));
        hg_assert(!map.has("supercalifragilisticexpialidocious"));

        map.insert(HgString::create(arena, "a"), 1);
        map.insert(HgString::create(arena, "b"), 2);
        map.insert(HgString::create(arena, "ab"), 3);
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"), 4);

        hg_assert(map.has("a"));
        hg_assert(*map.get("a") == 1);
        hg_assert(map.has("b"));
        hg_assert(*map.get("b") == 2);
        hg_assert(map.has("ab"));
        hg_assert(*map.get("ab") == 3);
        hg_assert(map.has("supercalifragilisticexpialidocious"));
        hg_assert(*map.get("supercalifragilisticexpialidocious") == 4);

        map.remove("a");
        map.remove("b");
        map.remove("ab");
        map.remove("supercalifragilisticexpialidocious");

        hg_assert(!map.has("a"));
        hg_assert(!map.has("b"));
        hg_assert(!map.has("ab"));
        hg_assert(!map.has("supercalifragilisticexpialidocious"));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashMap<u32, u32> map = map.create(arena, 64);

        bool has_any = false;
        map.for_each([&](u32, u32) {
            has_any = true;
        });
        hg_assert(!has_any);

        map.insert(12, 24);
        map.insert(42, 84);
        map.insert(100, 200);

        bool has_12 = false;
        bool has_42 = false;
        bool has_100 = false;
        bool has_other = false;
        map.for_each([&](u32 k, u32 v) {
            if (k == 12 && v == 24)
                has_12 = true;
            else if (k == 42 && v == 84)
                has_42 = true;
            else if (k == 100 && v == 200)
                has_100 = true;
            else
                has_other = true;
        });
        hg_assert(has_12);
        hg_assert(has_42);
        hg_assert(has_100);
        hg_assert(!has_other);

        map.for_each([&](u32 k, u32) {
            map.remove(k);
        });

        has_any = false;
        map.for_each([&](u32, u32) {
            has_any = true;
        });
        hg_assert(!has_any);
    }

    // HgHashSet
    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        constexpr usize count = 128;

        HgHashSet<u32> map = map.create(arena, count);

        for (usize i = 0; i < 3; ++i) {
            hg_assert(map.load == 0);
            hg_assert(!map.has(0));
            hg_assert(!map.has(1));
            hg_assert(!map.has(12));
            hg_assert(!map.has(42));
            hg_assert(!map.has(100000));

            map.insert(1);
            hg_assert(map.load == 1);
            hg_assert(map.has(1));

            map.remove(1);
            hg_assert(map.load == 0);
            hg_assert(!map.has(1));

            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));

            map.insert(12);
            hg_assert(map.load == 1);
            hg_assert(map.has(12));
            hg_assert(!map.has(12 + count));

            map.insert(12 + count);
            hg_assert(map.load == 2);
            hg_assert(map.has(12));
            hg_assert(map.has(12 + count));

            map.insert(12 + count * 2);
            hg_assert(map.load == 3);
            hg_assert(map.has(12));
            hg_assert(map.has(12 + count));
            hg_assert(map.has(12 + count * 2));

            map.remove(12);
            hg_assert(map.load == 2);
            hg_assert(!map.has(12));
            hg_assert(map.has(12 + count));

            map.insert(42);
            hg_assert(map.load == 3);
            hg_assert(map.has(42));

            map.remove(12 + count);
            hg_assert(map.load == 2);
            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));

            map.remove(42);
            hg_assert(map.load == 1);
            hg_assert(!map.has(42));

            map.remove(12 + count * 2);
            hg_assert(map.load == 0);
            hg_assert(!map.has(12));
            hg_assert(!map.has(12 + count));
            hg_assert(!map.has(12 + count * 2));

            map.reset();
        }
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        using StrHash = usize;

        HgHashSet<StrHash> map = map.create(arena, 128);

        StrHash a = hg_hash("a");
        StrHash b = hg_hash("b");
        StrHash ab = hg_hash("ab");
        StrHash scf = hg_hash("supercalifragilisticexpialidocious");

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));

        map.insert(a);
        map.insert(b);
        map.insert(ab);
        map.insert(scf);

        hg_assert(map.has(a));
        hg_assert(map.has(b));
        hg_assert(map.has(ab));
        hg_assert(map.has(scf));

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashSet<const char*> map = map.create(arena, 128);

        const char* a = "a";
        const char* b = "b";
        const char* ab = "ab";
        const char* scf = "supercalifragilisticexpialidocious";

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));

        map.insert(a);
        map.insert(b);
        map.insert(ab);
        map.insert(scf);

        hg_assert(map.has(a));
        hg_assert(map.has(b));
        hg_assert(map.has(ab));
        hg_assert(map.has(scf));

        map.remove(a);
        map.remove(b);
        map.remove(ab);
        map.remove(scf);

        hg_assert(!map.has(a));
        hg_assert(!map.has(b));
        hg_assert(!map.has(ab));
        hg_assert(!map.has(scf));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashSet<HgString> map = map.create(arena, 128);

        hg_assert(!map.has(HgString::create(arena, "a")));
        hg_assert(!map.has(HgString::create(arena, "b")));
        hg_assert(!map.has(HgString::create(arena, "ab")));
        hg_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.insert(HgString::create(arena, "a"));
        map.insert(HgString::create(arena, "b"));
        map.insert(HgString::create(arena, "ab"));
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_assert(map.has(HgString::create(arena, "a")));
        hg_assert(map.has(HgString::create(arena, "b")));
        hg_assert(map.has(HgString::create(arena, "ab")));
        hg_assert(map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));

        map.remove(HgString::create(arena, "a"));
        map.remove(HgString::create(arena, "b"));
        map.remove(HgString::create(arena, "ab"));
        map.remove(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_assert(!map.has(HgString::create(arena, "a")));
        hg_assert(!map.has(HgString::create(arena, "b")));
        hg_assert(!map.has(HgString::create(arena, "ab")));
        hg_assert(!map.has(HgString::create(arena, "supercalifragilisticexpialidocious")));
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgHashSet<HgStringView> map = map.create(arena, 128);

        hg_assert(!map.has("a"));
        hg_assert(!map.has("b"));
        hg_assert(!map.has("ab"));
        hg_assert(!map.has("supercalifragilisticexpialidocious"));

        map.insert(HgString::create(arena, "a"));
        map.insert(HgString::create(arena, "b"));
        map.insert(HgString::create(arena, "ab"));
        map.insert(HgString::create(arena, "supercalifragilisticexpialidocious"));

        hg_assert(map.has("a"));
        hg_assert(map.has("b"));
        hg_assert(map.has("ab"));
        hg_assert(map.has("supercalifragilisticexpialidocious"));

        map.remove("a");
        map.remove("b");
        map.remove("ab");
        map.remove("supercalifragilisticexpialidocious");

        hg_assert(!map.has("a"));
        hg_assert(!map.has("b"));
        hg_assert(!map.has("ab"));
        hg_assert(!map.has("supercalifragilisticexpialidocious"));
    }

    // thread pool
    {
        HgFence fence;

        bool a = false;
        bool b = false;

        hg_call_par(&fence, 1, &a, [](void *pa) {
            *(bool*)pa = true;
        });
        hg_call_par(&fence, 1, &b, [](void *pb) {
            *(bool*)pb = true;
        });

        fence.wait(2.0);

        hg_assert(fence.wait(2.0));

        hg_assert(a == true);
        hg_assert(b == true);
    }

    {
        HgFence fence;

        bool vals[100] = {};
        for (bool& val : vals) {
            hg_call_par(&fence, 1, &val, [](void* data) {
                *(bool*)data = true;
            });
        }

        hg_assert(hg_thread_pool_help(fence, 2.0));

        for (bool& val : vals) {
            hg_assert(val == true);
        }
    }

    {
        bool vals[100] = {};

        hg_for_par(sizeof(vals) / sizeof(*vals), 16, [&](usize begin, usize end) {
            hg_assert(begin < end && end <= sizeof(vals) / sizeof(*vals));
            for (; begin < end; ++begin) {
                vals[begin] = true;
            }
        });

        for (bool& val : vals) {
            hg_assert(val == true);
        }
    }

    {
        HgFence fence;

        for (u32 n = 0; n < 3; ++n) {
            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

            auto fn = [](void* pval) {
                *((bool*)pval) = !*((bool*)pval);
            };

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    hg_call_par(&fence, 1, vals + i, fn);
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }

            hg_assert(hg_thread_pool_help(fence, 2.0));
            for (auto val : vals) {
                hg_assert(val == true);
            }
        }
    }

    // io thread
    {
        HgFence fence;

        bool vals[100] = {};

        hg_io_request(&fence, 1, vals, {}, [](void* pvals, HgStringView) {
            for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
                ((bool*)pvals)[i] = true;
            }
        });

        hg_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_assert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        bool vals[100] = {};

        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                *(bool*)pval = true;
            });
        }

        hg_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_assert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        bool vals[100] = {};

        vals[0] = true;

        for (usize i = 1; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                *(bool*)pval = *((bool*)pval - 1);
            });
        }

        hg_assert(fence.wait(2.0));
        for (usize i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
            hg_assert(vals[i] == true);
        }
    }

    {
        HgFence fence;

        for (u32 n = 0; n < 3; ++n) {

            std::atomic_bool start{false};
            std::thread producers[4];

            bool vals[100] = {};

            auto prod_fn = [&](u32 idx) {
                while (!start) {
                    _mm_pause();
                }
                u32 begin = idx * 25;
                u32 end = begin + 25;
                for (u32 i = begin; i < end; ++i) {
                    hg_io_request(&fence, 1, &vals[i], {}, [](void* pval, HgStringView) {
                        *(bool*)pval = !*(bool*)pval;
                    });
                }
            };
            for (u32 j = 0; j < sizeof(producers) / sizeof(*producers); ++j) {
                producers[j] = std::thread(prod_fn, j);
            }

            start.store(true);
            for (auto& thread : producers) {
                thread.join();
            }

            hg_assert(fence.wait(2.0));
            for (auto val : vals) {
                hg_assert(val == true);
            }
        }
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        u32 save_data[] = {12, 42, 100, 128};

        const char* file_path = "hg_test_dir/file_bin_test.bin";
        HgBinary bin{};

        {
            bin.load(arena, "file_does_not_exist.bin");
            hg_assert(bin.data == nullptr);
            hg_assert(bin.size == 0);
        }

        {
            bin.data = save_data;
            bin.size = sizeof(save_data);

            bin.store("dir/does/not/exist.bin");

            FILE* file_handle = fopen("dir/does/not/exist.bin", "rb");
            hg_assert(file_handle == nullptr);
        }

        {
            bin.data = save_data;
            bin.size = sizeof(save_data);

            bin.store(file_path);
            HgBinary new_bin = bin.load(arena, file_path);

            hg_assert(new_bin.data != nullptr);
            hg_assert(new_bin.data != save_data);
            hg_assert(new_bin.size == sizeof(save_data));
            hg_assert(memcmp(save_data, new_bin.data, new_bin.size) == 0);
        }
    }

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        struct color {
            u8 r, g, b, a;

            operator u32() { return *(u32*)this; }
        };

        u32 red =    color{0xff, 0x00, 0x00, 0xff};
        u32 green =  color{0x00, 0xff, 0x00, 0xff};
        u32 blue =   color{0x00, 0x00, 0xff, 0xff};
        u32 yellow = color{0xff, 0xff, 0x00, 0xff};

        constexpr VkFormat save_format = VK_FORMAT_R8G8B8A8_SRGB;
        constexpr u32 save_width = 2;
        constexpr u32 save_height = 2;
        constexpr u32 save_depth = 1;
        u32 save_data[save_width][save_height] = {
            {red, green},
            {blue, yellow},
        };

        HgBinary bin{};

        {
            HgTexture::Info info;
            memcpy(info.identifier, HgTexture::texture_identifier, sizeof(HgTexture::texture_identifier));
            info.format = VK_FORMAT_R8G8B8A8_SRGB;
            info.width = save_width;
            info.height = save_height;
            info.depth = save_depth;
            bin.grow(arena, sizeof(HgTexture::Info));
            bin.overwrite(0, info);

            usize pixel_idx = bin.size;
            bin.grow(arena, sizeof(save_data));
            bin.overwrite(pixel_idx, save_data, sizeof(save_data));
        }

        {
            HgTexture texture = bin;

            VkFormat format;
            u32 width, height, depth;
            hg_assert(texture.get_info(&format, &width, &height, &depth));
            hg_assert(format == save_format);
            hg_assert(width == save_width);
            hg_assert(height == save_height);
            hg_assert(depth == save_depth);
            hg_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

            void* pixels = texture.get_pixels();
            hg_assert(pixels != nullptr);
            hg_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
        }

        {
            HgStringView file_path = "hg_test_dir/file_image_test.hgtex";

            bin.store(file_path);
            HgTexture file_texture = HgBinary::load(arena, file_path);

            VkFormat format;
            u32 width, height, depth;
            hg_assert(file_texture.get_info(&format, &width, &height, &depth));
            hg_assert(format == save_format);
            hg_assert(width == save_width);
            hg_assert(height == save_height);
            hg_assert(depth == save_depth);
            hg_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

            void* pixels = file_texture.get_pixels();
            hg_assert(pixels != nullptr);
            hg_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
        }

        {
            HgStringView tex_path = "tex";
            HgStringView file_path = "hg_test_dir/file_image_test.png";
            HgResource tex_id = hg_resource_id(tex_path);
            HgResource file_id = hg_resource_id(file_path);
            hg_alloc_resource(tex_id);
            hg_alloc_resource(file_id);
            *hg_get_resource(tex_id) = bin;

            HgFence fence;
            hg_export_png(&fence, 1, tex_id, file_path);
            hg_import_png(&fence, 1, file_id, file_path);
            hg_assert(fence.wait(2.0));

            HgTexture file_texture = *hg_get_resource(file_id);

            VkFormat format;
            u32 width, height, depth;
            hg_assert(file_texture.get_info(&format, &width, &height, &depth));
            hg_assert(format == save_format);
            hg_assert(width == save_width);
            hg_assert(height == save_height);
            hg_assert(depth == save_depth);
            hg_assert(width * height * depth * hg_vk_format_to_size(format) == sizeof(save_data));

            void* pixels = file_texture.get_pixels();
            hg_assert(pixels != nullptr);
            hg_assert(memcmp(save_data, pixels, sizeof(save_data)) == 0);
        }

        hg_resources_reset();
    }

    hg_warn("resource management test not implemented yet\n");

    {
        HgArena& arena = hg_get_scratch();
        HgArenaScope arena_scope{arena};

        HgECS ecs = ecs.create(1024);
        hg_defer(ecs.destroy());

        hg_defer(ecs.reset());

        HgEntity e1 = ecs.spawn();
        HgEntity e2 = ecs.spawn();
        HgEntity e3 = {};
        hg_assert(ecs.alive(e1));
        hg_assert(ecs.alive(e2));
        hg_assert(!ecs.alive(e3));

        ecs.despawn(e1);
        hg_assert(!ecs.alive(e1));
        e3 = ecs.spawn();
        hg_assert(ecs.alive(e3));
        hg_assert(e3.idx() == e1.idx() && e3 != e1);

        e1 = ecs.spawn();
        hg_assert(ecs.alive(e1));

        {
            hg_assert(!ecs.has<u32>(e1));
            hg_assert(!ecs.has<u32>(e2));
            hg_assert(!ecs.has<u32>(e3));

            ecs.add<u32>(e1) = 1;
            hg_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hg_assert(!ecs.has<u32>(e2));
            hg_assert(!ecs.has<u32>(e3));
            ecs.add<u32>(e2) = 2;
            hg_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hg_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hg_assert(!ecs.has<u32>(e3));
            ecs.add<u32>(e3) = 3;
            hg_assert(ecs.has<u32>(e1) && ecs.get<u32>(e1) == 1);
            hg_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hg_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);

            ecs.remove<u32>(e1);
            hg_assert(!ecs.has<u32>(e1));
            hg_assert(ecs.has<u32>(e2) && ecs.get<u32>(e2) == 2);
            hg_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
            ecs.remove<u32>(e2);
            hg_assert(!ecs.has<u32>(e1));
            hg_assert(!ecs.has<u32>(e2));
            hg_assert(ecs.has<u32>(e3) && ecs.get<u32>(e3) == 3);
            ecs.remove<u32>(e3);
            hg_assert(!ecs.has<u32>(e1));
            hg_assert(!ecs.has<u32>(e2));
            hg_assert(!ecs.has<u32>(e3));
        }

        {
            bool has_unknown = false;
            ecs.for_each<u32>([&](HgEntity, u32&) {
                has_unknown = true;
            });
            hg_assert(!has_unknown);

            hg_assert(ecs.count<u32>() == 0);
            hg_assert(ecs.count<u64>() == 0);
        }

        {
            ecs.add<u32>(e1) = 12;
            ecs.add<u32>(e2) = 42;
            ecs.add<u32>(e3) = 100;
            hg_assert(ecs.count<u32>() == 3);
            hg_assert(ecs.count<u64>() == 0);

            bool has_unknown = false;
            bool has_12 = false;
            bool has_42 = false;
            bool has_100 = false;
            ecs.for_each<u32>([&](HgEntity e, u32& c) {
                switch (c) {
                    case 12:
                        has_12 = e == e1;
                        break;
                    case 42:
                        has_42 = e == e2;
                        break;
                    case 100:
                        has_100 = e == e3;
                        break;
                    default:
                        has_unknown = true;
                        break;
                }
            });
            hg_assert(has_12);
            hg_assert(has_42);
            hg_assert(has_100);
            hg_assert(!has_unknown);
        }

        {
            ecs.add<u64>(e2) = 2042;
            ecs.add<u64>(e3) = 2100;
            hg_assert(ecs.count<u32>() == 3);
            hg_assert(ecs.count<u64>() == 2);

            bool has_unknown = false;
            bool has_12 = false;
            bool has_42 = false;
            bool has_100 = false;
            bool has_2042 = false;
            bool has_2100 = false;
            ecs.for_each<u32, u64>([&](HgEntity e, u32& comp32, u64& comp64) {
                switch (comp32) {
                    case 12:
                        has_12 = e == e1;
                        break;
                    case 42:
                        has_42 = e == e2;
                        break;
                    case 100:
                        has_100 = e == e3;
                        break;
                    default:
                        has_unknown = true;
                        break;
                }
                switch (comp64) {
                    case 2042:
                        has_2042 = e == e2;
                        break;
                    case 2100:
                        has_2100 = e == e3;
                        break;
                    default:
                        has_unknown = true;
                        break;
                }
            });
            hg_assert(!has_12);
            hg_assert(has_42);
            hg_assert(has_100);
            hg_assert(has_2042);
            hg_assert(has_2100);
            hg_assert(!has_unknown);
        }

        {
            ecs.despawn(e1);
            hg_assert(ecs.count<u32>() == 2);
            hg_assert(ecs.count<u64>() == 2);

            bool has_unknown = false;
            bool has_12 = false;
            bool has_42 = false;
            bool has_100 = false;
            ecs.for_each<u32>([&](HgEntity e, u32& c) {
                switch (c) {
                    case 12:
                        has_12 = e == e1;
                        break;
                    case 42:
                        has_42 = e == e2;
                        break;
                    case 100:
                        has_100 = e == e3;
                        break;
                    default:
                        has_unknown = true;
                        break;
                }
            });
            hg_assert(!has_12);
            hg_assert(has_42);
            hg_assert(has_100);
            hg_assert(!has_unknown);
        }

        {
            ecs.despawn(e2);
            hg_assert(ecs.count<u32>() == 1);
            hg_assert(ecs.count<u64>() == 1);
        }

        ecs.reset();
        hg_assert(ecs.count<u32>() == 0);
        hg_assert(ecs.count<u64>() == 0);

        {
            for (u32 i = 0; i < 300; ++i) {
                HgEntity e = ecs.spawn();
                switch (i % 3) {
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
            ecs.for_par<u32>(16, [&](HgEntity, u32& c) {
                c += 4;
            });
            success = true;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != 16)
                    success = false;
            });
            hg_assert(success);

            ecs.for_par<u64>(16, [&](HgEntity, u64& c) {
                c += 3;
            });
            success = true;
            ecs.for_each<u64>([&](HgEntity, u64 c) {
                if (c != 45)
                    success = false;
            });
            hg_assert(success);

            ecs.for_par<u32, u64>(16, [&](HgEntity, u32& c32, u64& c64) {
                c64 -= c32;
            });
            success = true;
            ecs.for_each<u64>([&](HgEntity e, u64 c) {
                if (ecs.has<u32>(e)) {
                    if (c != 29)
                        success = false;
                } else {
                    if (c != 45)
                        success = false;
                }
            });
            hg_assert(success);
        }

        ecs.reset();

        auto comparison = [](void*, HgECS& pecs, HgEntity lhs, HgEntity rhs) {
            return pecs.get<u32>(lhs) < pecs.get<u32>(rhs);
        };

        {
            ecs.add<u32>(ecs.spawn()) = 42;

            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != 42)
                    success = false;
            });
            hg_assert(success);

            ecs.reset();
        }

        {
            u32 small_scramble_1[] = {1, 0};
            for (u32 i = 0; i < sizeof(small_scramble_1) / sizeof(*small_scramble_1); ++i) {
                ecs.add<u32>(ecs.spawn()) = small_scramble_1[i];
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.for_each<u32>([&](HgEntity, u32 c) {
                    if (c != elem)
                        success = false;
                    ++elem;
                });
                hg_assert(success);
            }

            {
                ecs.sort<u32>(nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecs.for_each<u32>([&](HgEntity, u32 c) {
                    if (c != elem)
                        success = false;
                    ++elem;
                });
                hg_assert(success);
            }

            ecs.reset();
        }

        {
            u32 medium_scramble_1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
            for (u32 i = 0; i < sizeof(medium_scramble_1) / sizeof(*medium_scramble_1); ++i) {
                ecs.add<u32>(ecs.spawn()) = medium_scramble_1[i];
            }
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hg_assert(success);

            ecs.reset();
        }

        {
            u32 medium_scramble_2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
            for (u32 i = 0; i < sizeof(medium_scramble_2) / sizeof(*medium_scramble_2); ++i) {
                ecs.add<u32>(ecs.spawn()) = medium_scramble_2[i];
            }
            ecs.sort<u32>(nullptr, comparison);
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hg_assert(success);

            ecs.reset();
        }

        {
            for (u32 i = 127; i < 128; --i) {
                ecs.add<u32>(ecs.spawn()) = i;
            }
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem)
                    success = false;
                ++elem;
            });
            hg_assert(success);

            ecs.reset();
        }

        {
            for (u32 i = 127; i < 128; --i) {
                ecs.add<u32>(ecs.spawn()) = i / 2;
            }
            ecs.sort<u32>(nullptr, comparison);
            ecs.sort<u32>(nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecs.for_each<u32>([&](HgEntity, u32 c) {
                if (c != elem / 2)
                    success = false;
                ++elem;
            });
            hg_assert(success);

            ecs.reset();
        }
    }

    printf("HurdyGurdy: Tests Complete in %fms\n", timer.tick() * 1000.0f);
}

