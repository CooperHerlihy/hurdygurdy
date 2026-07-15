#undef HG_NO_LOGGING
#define HG_LOGGING 1
#undef HG_NO_ASSERTIONS
#define HG_ASSERTIONS 1
#include "hurdygurdy.hpp"

#include <emmintrin.h>

namespace hg {

void test()
{
    HG_LOG("Tests Begun\n");

    Clock timer{};
    clockTick(&timer);

    // Arena
    {
        void* block = malloc(1024);
        HG_DEFER(free(block));

        Arena arena{block, 1024, 0};

        for (u32 i = 0; i < 3; ++i)
        {
            HG_ASSERT(arena.memory != nullptr);
            HG_ASSERT(arena.capacity == 1024);
            HG_ASSERT(arena.head == 0);

            u32* allocU32 = arenaAlloc<u32>(&arena, 1);
            HG_ASSERT(allocU32 == arena.memory);

            u64* allocU64 = arenaAlloc<u64>(&arena, 2);
            HG_ASSERT(reinterpret_cast<u8*>(allocU64) == reinterpret_cast<u8*>(allocU32) + 8);

            u8* allocU8 = arenaAlloc<u8>(&arena, 1);
            HG_ASSERT(allocU8 == reinterpret_cast<u8*>(allocU32) + 24);

            struct Big {
                u8 data[32];
            };
            Big* allocBig = arenaAlloc<Big>(&arena, 1);
            HG_ASSERT(reinterpret_cast<u8*>(allocBig) == reinterpret_cast<u8*>(allocU32) + 25);

            Big* reallocBig = arenaRealloc(&arena, allocBig, 1, 2);
            HG_ASSERT(reallocBig == allocBig);

            Big* reallocBigSame = arenaRealloc(&arena, reallocBig, 2, 2);
            HG_ASSERT(reallocBigSame == reallocBig);

            memClear(reallocBig, 2 * sizeof(*reallocBig), 2);
            u8* allocInterrupt = arenaAlloc<u8>(&arena, 1);
            static_cast<void>(allocInterrupt);

            Big* reallocBig2 = arenaRealloc(&arena, reallocBig, 2, 4);
            HG_ASSERT(reallocBig2 != reallocBig);
            HG_ASSERT(memEqual(reallocBig, reallocBig2, 2 * sizeof(*reallocBig)));

            arena.head = 0;
        }
    }

    // String
    {
        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        StringBuilder a = stringCopy(arena, "a");
        HG_ASSERT(a[0] == 'a');
        HG_ASSERT(a.length == 1);

        StringBuilder abc = stringCopy(arena, "abc");
        HG_ASSERT(abc[0] == 'a');
        HG_ASSERT(abc[1] == 'b');
        HG_ASSERT(abc[2] == 'c');
        HG_ASSERT(abc.length == 3);

        stringAppend(arena, &a, "bc");
        HG_ASSERT(a == abc);

        StringBuilder str{};

        stringAppend(arena, &str, "hello");
        HG_ASSERT(str == stringCopy(arena, "hello"));

        stringAppend(arena, &str, " there");
        HG_ASSERT(str == stringCopy(arena, "hello there"));

        stringPrepend(arena, &str, "why ");
        HG_ASSERT(str == stringCopy(arena, "why hello there"));

        stringInsert(arena, &str, 3, ",");
        HG_ASSERT(str == stringCopy(arena, "why, hello there"));

        stringPrepend(arena, &str, "aaaaaaaaaaaaaaaaaaaaaaaa ");
        HG_ASSERT(str == stringCopy(arena, "aaaaaaaaaaaaaaaaaaaaaaaa why, hello there"));
    }

    // string utils
    {
        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        HG_ASSERT(isWhitespace(' '));
        HG_ASSERT(isWhitespace('\t'));
        HG_ASSERT(isWhitespace('\n'));

        HG_ASSERT(isNumeral('0'));
        HG_ASSERT(isNumeral('1'));
        HG_ASSERT(isNumeral('2'));
        HG_ASSERT(isNumeral('3'));
        HG_ASSERT(isNumeral('4'));
        HG_ASSERT(isNumeral('5'));
        HG_ASSERT(isNumeral('5'));
        HG_ASSERT(isNumeral('6'));
        HG_ASSERT(isNumeral('7'));
        HG_ASSERT(isNumeral('8'));
        HG_ASSERT(isNumeral('9'));

        HG_ASSERT(!isNumeral('0' - 1));
        HG_ASSERT(!isNumeral('9' + 1));

        HG_ASSERT(!isNumeral('x'));
        HG_ASSERT(!isNumeral('a'));
        HG_ASSERT(!isNumeral('b'));
        HG_ASSERT(!isNumeral('c'));
        HG_ASSERT(!isNumeral('d'));
        HG_ASSERT(!isNumeral('e'));
        HG_ASSERT(!isNumeral('f'));
        HG_ASSERT(!isNumeral('X'));
        HG_ASSERT(!isNumeral('A'));
        HG_ASSERT(!isNumeral('B'));
        HG_ASSERT(!isNumeral('C'));
        HG_ASSERT(!isNumeral('D'));
        HG_ASSERT(!isNumeral('E'));
        HG_ASSERT(!isNumeral('F'));

        HG_ASSERT(!isNumeral('.'));
        HG_ASSERT(!isNumeral('+'));
        HG_ASSERT(!isNumeral('-'));
        HG_ASSERT(!isNumeral('*'));
        HG_ASSERT(!isNumeral('/'));
        HG_ASSERT(!isNumeral('='));
        HG_ASSERT(!isNumeral('#'));
        HG_ASSERT(!isNumeral('&'));
        HG_ASSERT(!isNumeral('^'));
        HG_ASSERT(!isNumeral('~'));

        HG_ASSERT(isInteger("0"));
        HG_ASSERT(isInteger("1"));
        HG_ASSERT(isInteger("2"));
        HG_ASSERT(isInteger("3"));
        HG_ASSERT(isInteger("4"));
        HG_ASSERT(isInteger("5"));
        HG_ASSERT(isInteger("6"));
        HG_ASSERT(isInteger("7"));
        HG_ASSERT(isInteger("8"));
        HG_ASSERT(isInteger("9"));
        HG_ASSERT(isInteger("10"));

        HG_ASSERT(isInteger("12"));
        HG_ASSERT(isInteger("42"));
        HG_ASSERT(isInteger("100"));
        HG_ASSERT(isInteger("123456789"));
        HG_ASSERT(isInteger("-12"));
        HG_ASSERT(isInteger("-42"));
        HG_ASSERT(isInteger("-100"));
        HG_ASSERT(isInteger("-123456789"));
        HG_ASSERT(isInteger("+12"));
        HG_ASSERT(isInteger("+42"));
        HG_ASSERT(isInteger("+100"));
        HG_ASSERT(isInteger("+123456789"));

        HG_ASSERT(!isInteger("hello"));
        HG_ASSERT(!isInteger("not a number"));
        HG_ASSERT(!isInteger("number"));
        HG_ASSERT(!isInteger("integer"));
        HG_ASSERT(!isInteger("0.0"));
        HG_ASSERT(!isInteger("1.0"));
        HG_ASSERT(!isInteger(".10"));
        HG_ASSERT(!isInteger("1e2"));
        HG_ASSERT(!isInteger("1f"));
        HG_ASSERT(!isInteger("0xff"));
        HG_ASSERT(!isInteger("--42"));
        HG_ASSERT(!isInteger("++42"));
        HG_ASSERT(!isInteger("42-"));
        HG_ASSERT(!isInteger("42+"));
        HG_ASSERT(!isInteger("4 2"));
        HG_ASSERT(!isInteger("4+2"));

        HG_ASSERT(isFloat("0.0"));
        HG_ASSERT(isFloat("1."));
        HG_ASSERT(isFloat("2.0"));
        HG_ASSERT(isFloat("3."));
        HG_ASSERT(isFloat("4.0"));
        HG_ASSERT(isFloat("5."));
        HG_ASSERT(isFloat("6.0"));
        HG_ASSERT(isFloat("7."));
        HG_ASSERT(isFloat("8.0"));
        HG_ASSERT(isFloat("9."));
        HG_ASSERT(isFloat("10.0"));

        HG_ASSERT(isFloat("0.0"));
        HG_ASSERT(isFloat(".1"));
        HG_ASSERT(isFloat("0.2"));
        HG_ASSERT(isFloat(".3"));
        HG_ASSERT(isFloat("0.4"));
        HG_ASSERT(isFloat(".5"));
        HG_ASSERT(isFloat("0.6"));
        HG_ASSERT(isFloat(".7"));
        HG_ASSERT(isFloat("0.8"));
        HG_ASSERT(isFloat(".9"));
        HG_ASSERT(isFloat("0.10"));

        HG_ASSERT(isFloat("1.0"));
        HG_ASSERT(isFloat("+10.f"));
        HG_ASSERT(isFloat(".10"));
        HG_ASSERT(isFloat("-999.999f"));
        HG_ASSERT(isFloat("1e3"));
        HG_ASSERT(isFloat("1e3"));
        HG_ASSERT(isFloat("+1.e3f"));
        HG_ASSERT(isFloat(".1e3"));

        HG_ASSERT(!isFloat("hello"));
        HG_ASSERT(!isFloat("not a number"));
        HG_ASSERT(!isFloat("number"));
        HG_ASSERT(!isFloat("float"));
        HG_ASSERT(!isFloat("1.0ff"));
        HG_ASSERT(!isFloat("0x1.0"));
        HG_ASSERT(!isFloat("-0x1.0"));

        HG_ASSERT(stringToInteger("0") == 0);
        HG_ASSERT(stringToInteger("1") == 1);
        HG_ASSERT(stringToInteger("2") == 2);
        HG_ASSERT(stringToInteger("3") == 3);
        HG_ASSERT(stringToInteger("4") == 4);
        HG_ASSERT(stringToInteger("5") == 5);
        HG_ASSERT(stringToInteger("6") == 6);
        HG_ASSERT(stringToInteger("7") == 7);
        HG_ASSERT(stringToInteger("8") == 8);
        HG_ASSERT(stringToInteger("9") == 9);

        HG_ASSERT(stringToInteger("0000000") == 0);
        HG_ASSERT(stringToInteger("+0000001") == +1);
        HG_ASSERT(stringToInteger("0000002") == 2);
        HG_ASSERT(stringToInteger("-0000003") == -3);
        HG_ASSERT(stringToInteger("0000004") == 4);
        HG_ASSERT(stringToInteger("+0000005") == +5);
        HG_ASSERT(stringToInteger("0000006") == 6);
        HG_ASSERT(stringToInteger("-0000007") == -7);
        HG_ASSERT(stringToInteger("0000008") == 8);
        HG_ASSERT(stringToInteger("+0000009") == +9);

        HG_ASSERT(stringToInteger("0000000") == 0);
        HG_ASSERT(stringToInteger("1000000") == 1000000);
        HG_ASSERT(stringToInteger("2000000") == 2000000);
        HG_ASSERT(stringToInteger("3000000") == 3000000);
        HG_ASSERT(stringToInteger("4000000") == 4000000);
        HG_ASSERT(stringToInteger("5000000") == 5000000);
        HG_ASSERT(stringToInteger("6000000") == 6000000);
        HG_ASSERT(stringToInteger("7000000") == 7000000);
        HG_ASSERT(stringToInteger("8000000") == 8000000);
        HG_ASSERT(stringToInteger("9000000") == 9000000);
        HG_ASSERT(stringToInteger("1234567890") == 1234567890);

        HG_ASSERT(stringToFloat("0.0") == 0.0);
        HG_ASSERT(stringToFloat("1.0f") == 1.0);
        HG_ASSERT(stringToFloat("2.0") == 2.0);
        HG_ASSERT(stringToFloat("3.0f") == 3.0);
        HG_ASSERT(stringToFloat("4.0") == 4.0);
        HG_ASSERT(stringToFloat("5.0f") == 5.0);
        HG_ASSERT(stringToFloat("6.0") == 6.0);
        HG_ASSERT(stringToFloat("7.0f") == 7.0);
        HG_ASSERT(stringToFloat("8.0") == 8.0);
        HG_ASSERT(stringToFloat("9.0f") == 9.0);

        HG_ASSERT(stringToFloat("0e1") == 0.0);
        HG_ASSERT(stringToFloat("1e2f") == 1e2);
        HG_ASSERT(stringToFloat("2e3") == 2e3);
        HG_ASSERT(stringToFloat("3e4f") == 3e4);
        HG_ASSERT(stringToFloat("4e5") == 4e5);
        HG_ASSERT(stringToFloat("5e6f") == 5e6);
        HG_ASSERT(stringToFloat("6e7") == 6e7);
        HG_ASSERT(stringToFloat("7e8f") == 7e8);
        HG_ASSERT(stringToFloat("8e9") == 8e9);
        HG_ASSERT(stringToFloat("9e10f") == 9e10);

        HG_ASSERT(stringToFloat("0e1") == 0.0);
        HG_ASSERT(stringToFloat("1e2f") == 1e2);
        HG_ASSERT(stringToFloat("2e3") == 2e3);
        HG_ASSERT(stringToFloat("3e4f") == 3e4);
        HG_ASSERT(stringToFloat("4e5") == 4e5);
        HG_ASSERT(stringToFloat("5e6f") == 5e6);
        HG_ASSERT(stringToFloat("6e7") == 6e7);
        HG_ASSERT(stringToFloat("7e8f") == 7e8);
        HG_ASSERT(stringToFloat("8e9") == 8e9);
        HG_ASSERT(stringToFloat("9e10f") == 9e10);

        HG_ASSERT(stringToFloat(".1") == .1);
        HG_ASSERT(stringToFloat("+.1") == +.1);
        HG_ASSERT(stringToFloat("-.1") == -.1);
        HG_ASSERT(stringToFloat("+.1e5") == +.1e5);

        HG_ASSERT(integerToString(arena, 0) == "0");
        HG_ASSERT(integerToString(arena, -1) == "-1");
        HG_ASSERT(integerToString(arena, 2) == "2");
        HG_ASSERT(integerToString(arena, -3) == "-3");
        HG_ASSERT(integerToString(arena, 4) == "4");
        HG_ASSERT(integerToString(arena, -5) == "-5");
        HG_ASSERT(integerToString(arena, 6) == "6");
        HG_ASSERT(integerToString(arena, -7) == "-7");
        HG_ASSERT(integerToString(arena, 8) == "8");
        HG_ASSERT(integerToString(arena, -9) == "-9");

        HG_ASSERT(integerToString(arena, 0000000) == "0");
        HG_ASSERT(integerToString(arena, -1000000) == "-1000000");
        HG_ASSERT(integerToString(arena, 2000000) == "2000000");
        HG_ASSERT(integerToString(arena, -3000000) == "-3000000");
        HG_ASSERT(integerToString(arena, 4000000) == "4000000");
        HG_ASSERT(integerToString(arena, -5000000) == "-5000000");
        HG_ASSERT(integerToString(arena, 6000000) == "6000000");
        HG_ASSERT(integerToString(arena, -7000000) == "-7000000");
        HG_ASSERT(integerToString(arena, 8000000) == "8000000");
        HG_ASSERT(integerToString(arena, -9000000) == "-9000000");
        HG_ASSERT(integerToString(arena, 1234567890) == "1234567890");

        HG_ASSERT(floatToString(arena, 0.0, 10) == "0.0");
        HG_ASSERT(floatToString(arena, -1.0f, 1) == "-1.0");
        HG_ASSERT(floatToString(arena, 2.0, 2) == "2.00");
        HG_ASSERT(floatToString(arena, -3.0f, 3) == "-3.000");
        HG_ASSERT(floatToString(arena, 4.0, 4) == "4.0000");
        HG_ASSERT(floatToString(arena, -5.0f, 5) == "-5.00000");
        HG_ASSERT(floatToString(arena, 6.0, 6) == "6.000000");
        HG_ASSERT(floatToString(arena, -7.0f, 7) == "-7.0000000");
        HG_ASSERT(floatToString(arena, 8.0, 8) == "8.00000000");
        HG_ASSERT(floatToString(arena, -9.0f, 9) == "-9.000000000");

        HG_ASSERT(floatToString(arena, 0e0, 1) == "0.0");
        HG_ASSERT(floatToString(arena, -1e1f, 0) == "-10.");
        HG_ASSERT(floatToString(arena, 2e2, 1) == "200.0");
        HG_ASSERT(floatToString(arena, -3e3f, 0) == "-3000.");
        HG_ASSERT(floatToString(arena, 4e4, 1) == "40000.0");
        HG_ASSERT(floatToString(arena, -5e5f, 0) == "-500000.");
        HG_ASSERT(floatToString(arena, 6e6, 1) == "6000000.0");
        HG_ASSERT(floatToString(arena, -7e7f, 0) == "-70000000.");
        HG_ASSERT(floatToString(arena, 8e8, 1) == "800000000.0");
        HG_ASSERT(floatToString(arena, -9e9f, 0) == "-8999999488.");

        HG_ASSERT(floatToString(arena, -0e-0, 3) == "0.0");
        HG_ASSERT(floatToString(arena, 1e-1f, 3) == "0.100");
        HG_ASSERT(floatToString(arena, -2e-2, 3) == "-0.020");
        HG_ASSERT(floatToString(arena, 3e-3f, 3) == "0.003");
        HG_ASSERT(floatToString(arena, -4e-0, 3) == "-4.000");
        HG_ASSERT(floatToString(arena, 5e-1f, 3) == "0.500");
        HG_ASSERT(floatToString(arena, -6e-2, 3) == "-0.060");
        HG_ASSERT(floatToString(arena, 7e-3f, 3) == "0.007");
        HG_ASSERT(floatToString(arena, -8e-0, 3) == "-8.000");
        HG_ASSERT(floatToString(arena, 9e-1f, 3) == "0.899");
    }

    // thread pool
    {
        {
            Fence* fence = fenceCreate();
            HG_DEFER(fenceDestroy(fence));

            bool a = false;
            bool b = false;

            callPar(fence, &a, [](void *pa)
            {
                *static_cast<bool*>(pa) = true;
            });
            callPar(fence, &b, [](void *pb)
            {
                *static_cast<bool*>(pb) = true;
            });

            fenceWait(fence, 2.0);

            HG_ASSERT(fenceWait(fence, 2.0));

            HG_ASSERT(a == true);
            HG_ASSERT(b == true);
        }

        {
            Fence* fence = fenceCreate();
            HG_DEFER(fenceDestroy(fence));

            bool vals[100]{};
            for (bool& val : vals)
            {
                callPar(fence, &val, [](void* data)
                {
                    *static_cast<bool*>(data) = true;
                });
            }

            HG_ASSERT(helpThreads(fence, 2.0));

            for (bool& val : vals)
            {
                HG_ASSERT(val == true);
            }
        }

        {
            bool vals[100]{};

            auto fn = [](void* pvals, u64 idx)
            {
                (static_cast<bool*>(pvals))[idx] = true;
            };
            forPar(0, std::size(vals), vals, fn);

            for (bool& val : vals)
            {
                HG_ASSERT(val == true);
            }
        }

        {
            Fence* fence = fenceCreate();
            HG_DEFER(fenceDestroy(fence));

            for (u32 n = 0; n < 3; ++n)
            {
                std::atomic_bool start{false};
                std::thread producers[4];

                bool vals[100]{};

                auto fn = [](void* pval)
                {
                    *static_cast<bool*>(pval) = !*static_cast<bool*>(pval);
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
                        callPar(fence, vals + i, fn);
                    }
                };
                for (u32 j = 0; j < std::size(producers); ++j)
                {
                    producers[j] = std::thread(prodFn, j);
                }

                start.store(true);
                for (auto& thread : producers)
                {
                    thread.join();
                }

                HG_ASSERT(helpThreads(fence, 2.0));
                for (auto val : vals)
                {
                    HG_ASSERT(val == true);
                }
            }
        }
    }

    // Mutex
    {
        struct Capture {
            Spinlock* mtx;
            u32 count;
        };
        Capture c{};

        c.mtx = mutexCreate();
        HG_DEFER(mutexDestroy(c.mtx));

        c.count = 0;
        forPar(0, 100, &c, [](void* pc, u64)
        {
            Capture* c = static_cast<Capture*>(pc);
            mutexAcquire(c->mtx);
            HG_DEFER(mutexRelease(c->mtx));
            for (u32 i = 0; i < 10000; ++i)
            {
                ++c->count;
            }
        });
        HG_ASSERT(c.count == 1000000);
    }

    // Serialization
    {
        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        struct PlainOldData {
            i64 a;
            u16 b;
            f32 c;
            bool d;
            u32 e[3];
        };

        PlainOldData pod{};
        pod.a = -12,
        pod.b = 42,
        pod.c = 2.5f,
        pod.d = true,
        pod.e[0] = {2};
        pod.e[1] = {4};
        pod.e[2] = {6};

        {
            Serializer writer = serialWriter(arena);
            serialize(&writer, &pod);

            PlainOldData podCopy{};

            Serializer reader = serialReader(arena, writer.current);
            serialize(&reader, &podCopy);

            HG_ASSERT(memEqual(&podCopy, &pod, sizeof(pod)));
        }

        {
            Serializer writer = serialWriter(arena);
            serialize(&writer, &pod);

            Binary bin = binaryWriteSerial(arena, &writer);

            PlainOldData podCopy{};

            Serializer reader = binaryReadSerial(arena, bin);
            serialize(&reader, &podCopy);

            HG_ASSERT(memEqual(&podCopy, &pod, sizeof(pod)));
        }

        // {
        //     Serializer writer = serialWriter(arena);
        //     serialize(arena, &writer, "data", &pod);
        //
        //     StringView json = jsonWriteSerial(arena, writer);
        //
        //     PlainOldData podCopy{};
        //
        //     Serializer reader = jsonReadSerial(arena, json);
        //     serialize(arena, &reader, "data", &podCopy);
        //
        //     HG_ASSERT(memEqual(&podCopy, &pod, sizeof(pod)));
        // }

        struct Data {
            i64 a;
            u16 b;
            f32 c;
            bool d;
            u32 e[3];
            StringBuilder f;
        };

        Data data{};
        data.a = -12,
        data.b = 42,
        data.c = 2.5f,
        data.d = true,
        data.e[0] = {2};
        data.e[1] = {4};
        data.e[2] = {6};
        data.f = stringCopy(arena, "hello");

        auto serializeData = [](Serializer* s, Data* val)
        {
            serializeObject(s,
                &val->a,
                &val->b,
                &val->c,
                &val->d,
                &val->e,
                &val->f);
        };

        {
            Serializer writer = serialWriter(arena);
            serializeData(&writer, &data);

            // StringView json = jsonWriteSerial(arena, &writer);
            // debug("json: %.*s\n", (int)json.length, json.chars);

            Data dataCopy{};

            Serializer reader = serialReader(arena, writer.current);
            serializeData(&reader, &dataCopy);

            HG_ASSERT(!memEqual(&dataCopy, &data, sizeof(data)));
            HG_ASSERT(data.a == dataCopy.a);
            HG_ASSERT(data.b == dataCopy.b);
            HG_ASSERT(data.c == dataCopy.c);
            HG_ASSERT(data.d == dataCopy.d);
            HG_ASSERT(data.e[0] == dataCopy.e[0]);
            HG_ASSERT(data.e[1] == dataCopy.e[1]);
            HG_ASSERT(data.e[2] == dataCopy.e[2]);
            HG_ASSERT(data.f == dataCopy.f);
        }

        {
            Serializer writer = serialWriter(arena);
            serializeData(&writer, &data);

            Binary bin = binaryWriteSerial(arena, &writer);

            Data dataCopy{};

            Serializer reader = binaryReadSerial(arena, bin);
            serializeData(&reader, &dataCopy);

            HG_ASSERT(!memEqual(&dataCopy, &data, sizeof(data)));
            HG_ASSERT(data.a == dataCopy.a);
            HG_ASSERT(data.b == dataCopy.b);
            HG_ASSERT(data.c == dataCopy.c);
            HG_ASSERT(data.d == dataCopy.d);
            HG_ASSERT(data.e[0] == dataCopy.e[0]);
            HG_ASSERT(data.e[1] == dataCopy.e[1]);
            HG_ASSERT(data.e[2] == dataCopy.e[2]);
            HG_ASSERT(data.f == dataCopy.f);
        }

//         {
//             Serializer writer = serialWriter(arena);
//             serializeData(&writer, &data);
//
//             StringView json = jsonWriteSerial(arena, &writer);
//
//             debug("json: %.*s\n", (int)json.length, json.chars);
//             HG_ASSERT(json ==
// R"({
//     "a" : -12,
//     "b" : 42,
//     "c" : 2.500000,
//     "d" : true,
//     "e" : [
//         2,
//         4,
//         6
//     ],
//     "f" : "hello"
// }
// )");
//
//             Data dataCopy{};
//
//             Serializer reader = jsonReadSerial(arena, json);
//             serializeData(arena, &reader, "data", &dataCopy);
//
//             HG_ASSERT(!memEqual(&dataCopy, &data, sizeof(data)));
//             HG_ASSERT(data.a == dataCopy.a);
//             HG_ASSERT(data.b == dataCopy.b);
//             HG_ASSERT(data.c == dataCopy.c);
//             HG_ASSERT(data.d == dataCopy.d);
//             HG_ASSERT(data.e[0] == dataCopy.e[0]);
//             HG_ASSERT(data.e[1] == dataCopy.e[1]);
//             HG_ASSERT(data.e[2] == dataCopy.e[2]);
//             HG_ASSERT(data.f == dataCopy.f);
//         }
    }

    // Json
    {
        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    1234
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors != nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonError* error = json.errors;
            HG_ASSERT(error->next == nullptr);
            HG_ASSERT(error->msg == "on line 4, struct has a literal instead of a field\n");

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf"
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors != nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonError* error = json.errors;
            HG_ASSERT(error->next == nullptr);
            HG_ASSERT(error->msg == "on line 4, struct has a literal instead of a field\n");

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf":
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors != nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonError* error = json.errors;
            HG_ASSERT(error->next != nullptr);
            HG_ASSERT(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
            error = error->next;
            HG_ASSERT(error->next == nullptr);
            HG_ASSERT(error->msg == "on line 4, found unexpected token \"}\"\n");

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": true
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_bool);
            HG_ASSERT(field->value->boolean == true);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": false
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_bool);
            HG_ASSERT(field->value->boolean == false);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": asdf
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors != nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonError* error = json.errors;
            HG_ASSERT(error->next != nullptr);
            HG_ASSERT(error->msg == "on line 4, struct has a field named \"asdf\" which has no value\n");
            error = error->next;
            HG_ASSERT(error->next == nullptr);
            HG_ASSERT(error->msg == "on line 3, found unexpected token \"asdf\"\n");

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": "asdf"
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_string);
            HG_ASSERT(field->value->string == "asdf");
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": 1234
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_integer);
            HG_ASSERT(field->value->integer == 1234);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": 1234.0
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_float);
            HG_ASSERT(field->value->floating == 1234.0);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": 1234.0,
                    "hjkl": 5678.0
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next != nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_float);
            HG_ASSERT(field->value->floating == 1234.0);

            field = field->next;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "hjkl");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_float);
            HG_ASSERT(field->value->floating == 5678.0);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": [1, 2, 3, 4]
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            JsonElem* elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 1);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 2);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 3);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 4);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": [1 2 3 4]
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            JsonElem* elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 1);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 2);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 3);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 4);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": [1, 2, "3", 4]
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors != nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonError* error = json.errors;
            HG_ASSERT(error->next == nullptr);
            HG_ASSERT(error->msg ==
                "on line 3, array has element which is not the same type as the first valid element\n");

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            JsonElem* elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 1);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 2);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_integer);
            HG_ASSERT(elem->value->integer == 4);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
                {
                    "asdf": {
                        "a": 1,
                        "s": 2.0,
                        "d": 3,
                        "f": 4.0,
                    }
                }
            )";

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* node = json.file;
            HG_ASSERT(node->type == JsonType_struct);
            HG_ASSERT(node->jstruct.fields != nullptr);

            JsonField* field = node->jstruct.fields;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "asdf");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_struct);
            HG_ASSERT(field->value->array.elems != nullptr);

            JsonField* subField = field->value->jstruct.fields;
            HG_ASSERT(subField->next != nullptr);
            HG_ASSERT(subField->name == "a");
            HG_ASSERT(subField->value != nullptr);
            HG_ASSERT(subField->value->type == JsonType_integer);
            HG_ASSERT(subField->value->integer == 1);

            subField = subField->next;
            HG_ASSERT(subField->next != nullptr);
            HG_ASSERT(subField->name == "s");
            HG_ASSERT(subField->value != nullptr);
            HG_ASSERT(subField->value->type == JsonType_float);
            HG_ASSERT(subField->value->floating == 2.0);

            subField = subField->next;
            HG_ASSERT(subField->next != nullptr);
            HG_ASSERT(subField->name == "d");
            HG_ASSERT(subField->value != nullptr);
            HG_ASSERT(subField->value->type == JsonType_integer);
            HG_ASSERT(subField->value->integer == 3);

            subField = subField->next;
            HG_ASSERT(subField->next == nullptr);
            HG_ASSERT(subField->name == "f");
            HG_ASSERT(subField->value != nullptr);
            HG_ASSERT(subField->value->type == JsonType_float);
            HG_ASSERT(subField->value->floating == 4.0);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            String file = R"(
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

            Json json = parseJson(arena, file);

            HG_ASSERT(json.errors == nullptr);
            HG_ASSERT(json.file != nullptr);

            JsonNode* mainStruct = json.file;
            HG_ASSERT(mainStruct->type == JsonType_struct);
            HG_ASSERT(mainStruct->jstruct.fields != nullptr);

            JsonField* player = mainStruct->jstruct.fields;
            HG_ASSERT(player->next == nullptr);
            HG_ASSERT(player->name == "player");
            HG_ASSERT(player->value != nullptr);
            HG_ASSERT(player->value->type == JsonType_struct);
            HG_ASSERT(player->value->jstruct.fields != nullptr);

            JsonField* component = player->value->jstruct.fields;
            HG_ASSERT(component->next != nullptr);
            HG_ASSERT(component->name == "transform");
            HG_ASSERT(component->value != nullptr);
            HG_ASSERT(component->value->type == JsonType_struct);
            HG_ASSERT(component->value->jstruct.fields != nullptr);

            JsonField* field = component->value->jstruct.fields;
            HG_ASSERT(field->next != nullptr);
            HG_ASSERT(field->name == "position");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            JsonElem* elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == -1.0);

            field = field->next;
            HG_ASSERT(field->next != nullptr);
            HG_ASSERT(field->name == "scale");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            field = field->next;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "rotation");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            elem = elem->next;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            component = component->next;
            HG_ASSERT(component->next == nullptr);
            HG_ASSERT(component->name == "sprite");
            HG_ASSERT(component->value != nullptr);
            HG_ASSERT(component->value->type == JsonType_struct);
            HG_ASSERT(component->value->jstruct.fields != nullptr);

            field = component->value->jstruct.fields;
            HG_ASSERT(field->next != nullptr);
            HG_ASSERT(field->name == "texture");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_string);
            HG_ASSERT(field->value->string == "tex.png");

            field = field->next;
            HG_ASSERT(field->next != nullptr);
            HG_ASSERT(field->name == "uvPos");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 0.0);

            field = field->next;
            HG_ASSERT(field->next == nullptr);
            HG_ASSERT(field->name == "uvSize");
            HG_ASSERT(field->value != nullptr);
            HG_ASSERT(field->value->type == JsonType_array);
            HG_ASSERT(field->value->array.elems != nullptr);

            elem = field->value->array.elems;
            HG_ASSERT(elem->next != nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);

            elem = elem->next;
            HG_ASSERT(elem->next == nullptr);
            HG_ASSERT(elem->value != nullptr);
            HG_ASSERT(elem->value->type == JsonType_float);
            HG_ASSERT(elem->value->floating == 1.0);
        }
    }

    // Array
    {
        Array<u32> arr = arrayCreate<u32>(0, 2);
        HG_DEFER(arrayDestroy(&arr));

        HG_ASSERT(arr.count == 0);
        HG_ASSERT(arr.capacity >= 2);

        *arrayPush(&arr) = 10;
        *arrayPush(&arr) = 20;

        HG_ASSERT(arr.count == 2);
        HG_ASSERT(arr[0] == 10);
        HG_ASSERT(arr[1] == 20);

        arrayResize(&arr, 4);

        HG_ASSERT(arr.count == 4);

        arr[2] = 30;
        arr[3] = 40;

        HG_ASSERT(arr[2] == 30);
        HG_ASSERT(arr[3] == 40);

        u32 popped = arrayPop(&arr);

        HG_ASSERT(popped == 40);
        HG_ASSERT(arr.count == 3);

        arrayResize(&arr, 1);

        HG_ASSERT(arr.count == 1);
        HG_ASSERT(arr[0] == 10);

        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        Array<u32> temp = arrayTemp<u32>(arena, 0, 4);

        *arrayPushTemp(arena, &temp) = 123;
        *arrayPushTemp(arena, &temp) = 456;

        HG_ASSERT(temp.count == 2);
        HG_ASSERT(temp[0] == 123);
        HG_ASSERT(temp[1] == 456);
    }

    // ArrayAny
    {
        ArrayAny arr = arrayAnyCreate(sizeof(u32), alignof(u32), 0, 2);
        HG_DEFER(arrayAnyDestroy(&arr));

        HG_ASSERT(arr.count == 0);
        HG_ASSERT(arr.capacity >= 2);
        HG_ASSERT(arr.width == sizeof(u32));
        HG_ASSERT(arr.align == alignof(u32));

        *static_cast<u32*>(arrayAnyPush(&arr)) = 10;
        *static_cast<u32*>(arrayAnyPush(&arr)) = 20;

        HG_ASSERT(arr.count == 2);
        HG_ASSERT(*static_cast<u32*>(arr[0]) == 10);
        HG_ASSERT(*static_cast<u32*>(arr[1]) == 20);

        arrayAnyResize(&arr, 4);

        HG_ASSERT(arr.count == 4);

        *static_cast<u32*>(arr[2]) = 30;
        *static_cast<u32*>(arr[3]) = 40;

        HG_ASSERT(*static_cast<u32*>(arr[2]) == 30);
        HG_ASSERT(*static_cast<u32*>(arr[3]) == 40);

        u32 popped = 0;
        arrayAnyPop(&arr, &popped);

        HG_ASSERT(popped == 40);
        HG_ASSERT(arr.count == 3);

        arrayAnyResize(&arr, 1);

        HG_ASSERT(arr.count == 1);
        HG_ASSERT(*static_cast<u32*>(arr[0]) == 10);

        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        ArrayAny temp = arrayAnyTemp(arena, sizeof(u32), alignof(u32), 0, 2);

        *static_cast<u32*>(arrayAnyPushTemp(arena, &temp)) = 123;
        *static_cast<u32*>(arrayAnyPushTemp(arena, &temp)) = 456;

        HG_ASSERT(temp.count == 2);
        HG_ASSERT(*static_cast<u32*>(temp[0]) == 123);
        HG_ASSERT(*static_cast<u32*>(temp[1]) == 456);

        arrayAnyPushTemp(arena, &temp);

        HG_ASSERT(temp.count == 3);
    }

    // Queue
    {
        Queue<u32> queue = queueCreate<u32>(4);

        HG_ASSERT(queue.count == 0);
        HG_ASSERT(queue.capacity >= 4);

        queuePushBack(&queue, 1);
        queuePushBack(&queue, 2);
        queuePushBack(&queue, 3);
        queuePushBack(&queue, 4);

        HG_ASSERT(queue.count == 4);

        HG_ASSERT(queuePopFront(&queue) == 1);
        HG_ASSERT(queuePopFront(&queue) == 2);

        HG_ASSERT(queue.count == 2);

        queuePushBack(&queue, 5);
        queuePushBack(&queue, 6);

        HG_ASSERT(queue.count == 4);

        queuePushBack(&queue, 7);
        queuePushBack(&queue, 8);

        HG_ASSERT(queue.count == 6);
        HG_ASSERT(queue.capacity >= 6);

        HG_ASSERT(queuePopFront(&queue) == 3);
        HG_ASSERT(queuePopFront(&queue) == 4);
        HG_ASSERT(queuePopFront(&queue) == 5);
        HG_ASSERT(queuePopFront(&queue) == 6);
        HG_ASSERT(queuePopFront(&queue) == 7);
        HG_ASSERT(queuePopFront(&queue) == 8);

        HG_ASSERT(queue.count == 0);

        queuePushFront(&queue, 10);
        queuePushFront(&queue, 20);
        queuePushFront(&queue, 30);

        HG_ASSERT(queue.count == 3);

        HG_ASSERT(queuePopFront(&queue) == 30);
        HG_ASSERT(queuePopFront(&queue) == 20);
        HG_ASSERT(queuePopFront(&queue) == 10);

        HG_ASSERT(queue.count == 0);

        queuePushBack(&queue, 1);
        queuePushBack(&queue, 2);
        queuePushFront(&queue, 0);
        queuePushFront(&queue, -1);

        HG_ASSERT(queue.count == 4);

        HG_ASSERT(queuePopBack(&queue) == 2);
        HG_ASSERT(queuePopBack(&queue) == 1);
        HG_ASSERT(queuePopBack(&queue) == 0);
        HG_ASSERT(queuePopBack(&queue) == (u32)-1);

        HG_ASSERT(queue.count == 0);

        queueDestroy(&queue);
    }

    // Set
    {
        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            constexpr u32 count = 128;

            Set<u32> set = setTemp<u32>(arena, count);

            for (u32 i = 0; i < 3; ++i)
            {
                HG_ASSERT(set.count == 0);
                HG_ASSERT(!setHas(&set, 0));
                HG_ASSERT(!setHas(&set, 1));
                HG_ASSERT(!setHas(&set, 12));
                HG_ASSERT(!setHas(&set, 42));
                HG_ASSERT(!setHas(&set, 100000));

                setAdd(&set, 1);
                HG_ASSERT(set.count == 1);
                HG_ASSERT(setHas(&set, 1));

                setRemove(&set, 1);
                HG_ASSERT(set.count == 0);
                HG_ASSERT(!setHas(&set, 1));

                HG_ASSERT(!setHas(&set, 12));
                HG_ASSERT(!setHas(&set, 12 + count));

                setAdd(&set, 12);
                HG_ASSERT(set.count == 1);
                HG_ASSERT(setHas(&set, 12));
                HG_ASSERT(!setHas(&set, 12 + count));

                setAdd(&set, 12 + count);
                HG_ASSERT(set.count == 2);
                HG_ASSERT(setHas(&set, 12));
                HG_ASSERT(setHas(&set, 12 + count));

                setAdd(&set, 12 + count * 2);
                HG_ASSERT(set.count == 3);
                HG_ASSERT(setHas(&set, 12));
                HG_ASSERT(setHas(&set, 12 + count));
                HG_ASSERT(setHas(&set, 12 + count * 2));

                setRemove(&set, 12);
                HG_ASSERT(set.count == 2);
                HG_ASSERT(!setHas(&set, 12));
                HG_ASSERT(setHas(&set, 12 + count));

                setAdd(&set, 42);
                HG_ASSERT(set.count == 3);
                HG_ASSERT(setHas(&set, 42));

                setRemove(&set, 12 + count);
                HG_ASSERT(set.count == 2);
                HG_ASSERT(!setHas(&set, 12));
                HG_ASSERT(!setHas(&set, 12 + count));

                setRemove(&set, 42);
                HG_ASSERT(set.count == 1);
                HG_ASSERT(!setHas(&set, 42));

                setRemove(&set, 12 + count * 2);
                HG_ASSERT(set.count == 0);
                HG_ASSERT(!setHas(&set, 12));
                HG_ASSERT(!setHas(&set, 12 + count));
                HG_ASSERT(!setHas(&set, 12 + count * 2));

                setReset(&set);
            }
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            using StrHash = u64;

            Set<StrHash> set = setTemp<StrHash>(arena, 128);

            StrHash a = hash("a");
            StrHash b = hash("b");
            StrHash ab = hash("ab");
            StrHash scf = hash("supercalifragilisticexpialidocious");

            HG_ASSERT(!setHas(&set, a));
            HG_ASSERT(!setHas(&set, b));
            HG_ASSERT(!setHas(&set, ab));
            HG_ASSERT(!setHas(&set, scf));

            setAdd(&set, a);
            setAdd(&set, b);
            setAdd(&set, ab);
            setAdd(&set, scf);

            HG_ASSERT(setHas(&set, a));
            HG_ASSERT(setHas(&set, b));
            HG_ASSERT(setHas(&set, ab));
            HG_ASSERT(setHas(&set, scf));

            setRemove(&set, a);
            setRemove(&set, b);
            setRemove(&set, ab);
            setRemove(&set, scf);

            HG_ASSERT(!setHas(&set, a));
            HG_ASSERT(!setHas(&set, b));
            HG_ASSERT(!setHas(&set, ab));
            HG_ASSERT(!setHas(&set, scf));
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Set<const char*> set = setTemp<const char*>(arena, 128);

            const char* a = "a";
            const char* b = "b";
            const char* ab = "ab";
            const char* scf = "supercalifragilisticexpialidocious";

            HG_ASSERT(!setHas(&set, a));
            HG_ASSERT(!setHas(&set, b));
            HG_ASSERT(!setHas(&set, ab));
            HG_ASSERT(!setHas(&set, scf));

            setAdd(&set, a);
            setAdd(&set, b);
            setAdd(&set, ab);
            setAdd(&set, scf);

            HG_ASSERT(setHas(&set, a));
            HG_ASSERT(setHas(&set, b));
            HG_ASSERT(setHas(&set, ab));
            HG_ASSERT(setHas(&set, scf));

            setRemove(&set, a);
            setRemove(&set, b);
            setRemove(&set, ab);
            setRemove(&set, scf);

            HG_ASSERT(!setHas(&set, a));
            HG_ASSERT(!setHas(&set, b));
            HG_ASSERT(!setHas(&set, ab));
            HG_ASSERT(!setHas(&set, scf));
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Set<StringBuilder> set = setTemp<StringBuilder>(arena, 128);

            HG_ASSERT(!setHas(&set, stringCopy(arena, "a")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "b")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "ab")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "supercalifragilisticexpialidocious")));

            setAdd(&set, stringCopy(arena, "a"));
            setAdd(&set, stringCopy(arena, "b"));
            setAdd(&set, stringCopy(arena, "ab"));
            setAdd(&set, stringCopy(arena, "supercalifragilisticexpialidocious"));

            HG_ASSERT(setHas(&set, stringCopy(arena, "a")));
            HG_ASSERT(setHas(&set, stringCopy(arena, "b")));
            HG_ASSERT(setHas(&set, stringCopy(arena, "ab")));
            HG_ASSERT(setHas(&set, stringCopy(arena, "supercalifragilisticexpialidocious")));

            setRemove(&set, stringCopy(arena, "a"));
            setRemove(&set, stringCopy(arena, "b"));
            setRemove(&set, stringCopy(arena, "ab"));
            setRemove(&set, stringCopy(arena, "supercalifragilisticexpialidocious"));

            HG_ASSERT(!setHas(&set, stringCopy(arena, "a")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "b")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "ab")));
            HG_ASSERT(!setHas(&set, stringCopy(arena, "supercalifragilisticexpialidocious")));
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Set<String> set = setTemp<String>(arena, 128);

            HG_ASSERT(!setHas(&set, "a"));
            HG_ASSERT(!setHas(&set, "b"));
            HG_ASSERT(!setHas(&set, "ab"));
            HG_ASSERT(!setHas(&set, "supercalifragilisticexpialidocious"));

            setAdd(&set, "a");
            setAdd(&set, "b");
            setAdd(&set, "ab");
            setAdd(&set, "supercalifragilisticexpialidocious");

            HG_ASSERT(setHas(&set, "a"));
            HG_ASSERT(setHas(&set, "b"));
            HG_ASSERT(setHas(&set, "ab"));
            HG_ASSERT(setHas(&set, "supercalifragilisticexpialidocious"));

            setRemove(&set, "a");
            setRemove(&set, "b");
            setRemove(&set, "ab");
            setRemove(&set, "supercalifragilisticexpialidocious");

            HG_ASSERT(!setHas(&set, "a"));
            HG_ASSERT(!setHas(&set, "b"));
            HG_ASSERT(!setHas(&set, "ab"));
            HG_ASSERT(!setHas(&set, "supercalifragilisticexpialidocious"));
        }
    }

    // Map
    {
        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            constexpr u32 count = 128;

            Map<u32, u32> map = mapTemp<u32, u32>(arena, count);

            for (u32 i = 0; i < 3; ++i)
            {
                HG_ASSERT(map.count == 0);
                HG_ASSERT(mapGet(&map, 0) == nullptr);
                HG_ASSERT(mapGet(&map, 1) == nullptr);
                HG_ASSERT(mapGet(&map, 12) == nullptr);
                HG_ASSERT(mapGet(&map, 42) == nullptr);
                HG_ASSERT(mapGet(&map, 100000) == nullptr);

                mapAdd(&map, 1, 1);
                HG_ASSERT(map.count == 1);
                HG_ASSERT(mapGet(&map, 1) != nullptr);
                HG_ASSERT(*mapGet(&map, 1) == 1);

                mapRemove(&map, 1);
                HG_ASSERT(map.count == 0);
                HG_ASSERT(mapGet(&map, 1) == nullptr);

                HG_ASSERT(mapGet(&map, 12) == nullptr);
                HG_ASSERT(mapGet(&map, 12 + count) == nullptr);

                mapAdd(&map, 12, 42);
                HG_ASSERT(map.count == 1);
                HG_ASSERT(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
                HG_ASSERT(mapGet(&map, 12 + count) == nullptr);

                mapAdd(&map, 12 + count, 100);
                HG_ASSERT(map.count == 2);
                HG_ASSERT(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
                HG_ASSERT(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);

                mapAdd(&map, 12 + count * 2, 200);
                HG_ASSERT(map.count == 3);
                HG_ASSERT(mapGet(&map, 12) != nullptr && *mapGet(&map, 12) == 42);
                HG_ASSERT(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);
                HG_ASSERT(mapGet(&map, 12 + count * 2) != nullptr && *mapGet(&map, 12 + count * 2) == 200);

                mapRemove(&map, 12);
                HG_ASSERT(map.count == 2);
                HG_ASSERT(mapGet(&map, 12) == nullptr);
                HG_ASSERT(mapGet(&map, 12 + count) != nullptr && *mapGet(&map, 12 + count) == 100);

                mapAdd(&map, 42, 12);
                HG_ASSERT(map.count == 3);
                HG_ASSERT(mapGet(&map, 42) != nullptr && *mapGet(&map, 42) == 12);

                mapRemove(&map, 12 + count);
                HG_ASSERT(map.count == 2);
                HG_ASSERT(mapGet(&map, 12) == nullptr);
                HG_ASSERT(mapGet(&map, 12 + count) == nullptr);

                mapRemove(&map, 42);
                HG_ASSERT(map.count == 1);
                HG_ASSERT(mapGet(&map, 42) == nullptr);

                mapRemove(&map, 12 + count * 2);
                HG_ASSERT(map.count == 0);
                HG_ASSERT(mapGet(&map, 12) == nullptr);
                HG_ASSERT(mapGet(&map, 12 + count) == nullptr);
                HG_ASSERT(mapGet(&map, 12 + count * 2) == nullptr);

                mapReset(&map);
            }
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            using StrHash = u64;

            Map<StrHash, u32> map = mapTemp<StrHash, u32>(arena, 128);

            StrHash a = hash("a");
            StrHash b = hash("b");
            StrHash ab = hash("ab");
            StrHash scf = hash("supercalifragilisticexpialidocious");

            HG_ASSERT(mapGet(&map, a) == nullptr);
            HG_ASSERT(mapGet(&map, b) == nullptr);
            HG_ASSERT(mapGet(&map, ab) == nullptr);
            HG_ASSERT(mapGet(&map, scf) == nullptr);

            mapAdd(&map, a, 1);
            mapAdd(&map, b, 2);
            mapAdd(&map, ab, 3);
            mapAdd(&map, scf, 4);

            HG_ASSERT(mapGet(&map, a) != nullptr && *mapGet(&map, a) == 1);
            HG_ASSERT(mapGet(&map, b) != nullptr && *mapGet(&map, b) == 2);
            HG_ASSERT(mapGet(&map, ab) != nullptr && *mapGet(&map, ab) == 3);
            HG_ASSERT(mapGet(&map, scf) != nullptr && *mapGet(&map, scf) == 4);

            mapRemove(&map, a);
            mapRemove(&map, b);
            mapRemove(&map, ab);
            mapRemove(&map, scf);

            HG_ASSERT(mapGet(&map, a) == nullptr);
            HG_ASSERT(mapGet(&map, b) == nullptr);
            HG_ASSERT(mapGet(&map, ab) == nullptr);
            HG_ASSERT(mapGet(&map, scf) == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Map<const char*, u32> map = mapTemp<const char*, u32>(arena, 128);

            const char* a = "a";
            const char* b = "b";
            const char* ab = "ab";
            const char* scf = "supercalifragilisticexpialidocious";

            HG_ASSERT(mapGet(&map, a) == nullptr);
            HG_ASSERT(mapGet(&map, b) == nullptr);
            HG_ASSERT(mapGet(&map, ab) == nullptr);
            HG_ASSERT(mapGet(&map, scf) == nullptr);

            mapAdd(&map, a, 1);
            mapAdd(&map, b, 2);
            mapAdd(&map, ab, 3);
            mapAdd(&map, scf, 4);

            HG_ASSERT(mapGet(&map, a) != nullptr && *mapGet(&map, a) == 1);
            HG_ASSERT(mapGet(&map, b) != nullptr && *mapGet(&map, b) == 2);
            HG_ASSERT(mapGet(&map, ab) != nullptr && *mapGet(&map, ab) == 3);
            HG_ASSERT(mapGet(&map, scf) != nullptr && *mapGet(&map, scf) == 4);

            mapRemove(&map, a);
            mapRemove(&map, b);
            mapRemove(&map, ab);
            mapRemove(&map, scf);

            HG_ASSERT(mapGet(&map, a) == nullptr);
            HG_ASSERT(mapGet(&map, b) == nullptr);
            HG_ASSERT(mapGet(&map, ab) == nullptr);
            HG_ASSERT(mapGet(&map, scf) == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Map<StringBuilder, u32> map = mapTemp<StringBuilder, u32>(arena, 128);

            HG_ASSERT(mapGet(&map, stringCopy(arena, "a")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "b")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "ab")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "supercalifragilisticexpialidocious")) == nullptr);

            mapAdd(&map, stringCopy(arena, "a"), 1);
            mapAdd(&map, stringCopy(arena, "b"), 2);
            mapAdd(&map, stringCopy(arena, "ab"), 3);
            mapAdd(&map, stringCopy(arena, "supercalifragilisticexpialidocious"), 4);

            HG_ASSERT(mapGet(&map, stringCopy(arena, "a")) != nullptr);
            HG_ASSERT(*mapGet(&map, stringCopy(arena, "a")) == 1);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "b")) != nullptr);
            HG_ASSERT(*mapGet(&map, stringCopy(arena, "b")) == 2);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "ab")) != nullptr);
            HG_ASSERT(*mapGet(&map, stringCopy(arena, "ab")) == 3);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "supercalifragilisticexpialidocious")) != nullptr);
            HG_ASSERT(*mapGet(&map, stringCopy(arena, "supercalifragilisticexpialidocious")) == 4);

            mapRemove(&map, stringCopy(arena, "a"));
            mapRemove(&map, stringCopy(arena, "b"));
            mapRemove(&map, stringCopy(arena, "ab"));
            mapRemove(&map, stringCopy(arena, "supercalifragilisticexpialidocious"));

            HG_ASSERT(mapGet(&map, stringCopy(arena, "a")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "b")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "ab")) == nullptr);
            HG_ASSERT(mapGet(&map, stringCopy(arena, "supercalifragilisticexpialidocious")) == nullptr);
        }

        {
            Arena* arena = getScratch();
            HG_ARENA_SCOPE(arena);

            Map<String, u32> map = mapTemp<String, u32>(arena, 6);

            HG_ASSERT(mapGet(&map, "a") == nullptr);
            HG_ASSERT(mapGet(&map, "b") == nullptr);
            HG_ASSERT(mapGet(&map, "ab") == nullptr);
            HG_ASSERT(mapGet(&map, "supercalifragilisticexpialidocious") == nullptr);

            mapAdd(&map, "a", 1);
            mapAdd(&map, "b", 2);
            mapAdd(&map, "ab", 3);
            mapAdd(&map, "supercalifragilisticexpialidocious", 4);

            HG_ASSERT(mapGet(&map, "a") != nullptr);
            HG_ASSERT(*mapGet(&map, "a") == 1);
            HG_ASSERT(mapGet(&map, "b") != nullptr);
            HG_ASSERT(*mapGet(&map, "b") == 2);
            HG_ASSERT(mapGet(&map, "ab") != nullptr);
            HG_ASSERT(*mapGet(&map, "ab") == 3);
            HG_ASSERT(mapGet(&map, "supercalifragilisticexpialidocious") != nullptr);
            HG_ASSERT(*mapGet(&map, "supercalifragilisticexpialidocious") == 4);

            mapRemove(&map, "a");
            mapRemove(&map, "b");
            mapRemove(&map, "ab");
            mapRemove(&map, "supercalifragilisticexpialidocious");

            HG_ASSERT(mapGet(&map, "a") == nullptr);
            HG_ASSERT(mapGet(&map, "b") == nullptr);
            HG_ASSERT(mapGet(&map, "ab") == nullptr);
            HG_ASSERT(mapGet(&map, "supercalifragilisticexpialidocious") == nullptr);
        }
    }

    // Pool
    {
        Pool pool = poolCreate<u32>();
        HG_DEFER(poolDestroy(&pool));

        u32* a = static_cast<u32*>(poolAlloc(&pool));
        u32* b = static_cast<u32*>(poolAlloc(&pool));
        u32* c = static_cast<u32*>(poolAlloc(&pool));

        HG_ASSERT(a != nullptr);
        HG_ASSERT(b != nullptr);
        HG_ASSERT(c != nullptr);

        *a = 1;
        *b = 2;
        *c = 3;

        HG_ASSERT(*a == 1);
        HG_ASSERT(*b == 2);
        HG_ASSERT(*c == 3);

        poolFree(&pool, b);
        poolFree(&pool, c);

        u32* d = static_cast<u32*>(poolAlloc(&pool));
        u32* e = static_cast<u32*>(poolAlloc(&pool));

        HG_ASSERT(d == c);
        HG_ASSERT(e == b);

        *d = 40;
        *e = 50;

        HG_ASSERT(*d == 40);
        HG_ASSERT(*e == 50);

        constexpr u32 n = 1500;

        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        Array<u32*> ptrs = arrayTemp<u32*>(arena, 0, n);

        for (u32 i = 0; i < n; ++i)
        {
            u32* p = static_cast<u32*>(poolAlloc(&pool));
            *arrayPushTemp(arena, &ptrs) = p;
            HG_ASSERT(p != nullptr);
        }

        HG_ASSERT(pool.itemStores.count >= 2);

        for (u32 i = 0; i < ptrs.count; ++i)
        {
            for (u32 j = i + 1; j < ptrs.count; ++j)
            {
                HG_ASSERT(ptrs[i] != ptrs[j]);
            }
        }

        for (u32 i = 0; i < ptrs.count; ++i)
        {
            poolFree(&pool, ptrs[i]);
        }
    }

    // HandlePool
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));

        Handle u1 = handlePoolAlloc(&pool);
        HG_ASSERT(handlePoolAlive(&pool, u1));
        HG_ASSERT(u1.id == 1);

        Handle u2 = handlePoolAlloc(&pool);
        HG_ASSERT(handlePoolAlive(&pool, u2));
        HG_ASSERT(u2.id == 2);

        handlePoolFree(&pool, u1);
        HG_ASSERT(!handlePoolAlive(&pool, u1));

        Handle u12 = handlePoolAlloc(&pool);
        HG_ASSERT(handlePoolAlive(&pool, u12));
        HG_ASSERT(!handlePoolAlive(&pool, u1));
        HG_ASSERT(u12.id != 1);
        HG_ASSERT(handleIdx(u12) == 1);

        handlePoolReset(&pool);
        HG_ASSERT(!handlePoolAlive(&pool, u1));
        HG_ASSERT(!handlePoolAlive(&pool, u2));
        HG_ASSERT(!handlePoolAlive(&pool, u12));
    }

    // Mat
    {
        Mat2 mat{
            {1.0f, 0.0f},
            {1.0f, 0.0f},
        };
        Vec2 vec{1.0f, 1.0f};

        Mat2 identity{
            {1.0f, 0.0f},
            {0.0f, 1.0f},
        };
        HG_ASSERT(identity * mat == mat);
        HG_ASSERT(identity * vec == vec);

        Mat2 matRotated{
            {0.0f, 1.0f},
            {0.0f, 1.0f},
        };
        Vec2 vecRotated{-1.0f, 1.0f};

        Mat2 rotation{
            {0.0f, 1.0f},
            {-1.0f, 0.0f},
        };
        HG_ASSERT(rotation * mat == matRotated);
        HG_ASSERT(rotation * vec == vecRotated);

        HG_ASSERT((identity * rotation) * mat == identity * (rotation * mat));
        HG_ASSERT((identity * rotation) * vec == identity * (rotation * vec));
        HG_ASSERT((rotation * rotation) * mat == rotation * (rotation * mat));
        HG_ASSERT((rotation * rotation) * vec == rotation * (rotation * vec));
    }

    // Quat
    {
        Mat3 identityMat = Mat3{1.0f};
        Vec3 upVec{0.0f, -1.0f, 0.0f};
        Quat rotation = quatAxisAngle({0.0f, 0.0f, -1.0f}, -static_cast<f32>(HG_PI) * 0.5f);

        Vec3 rotatedVec = vecRot3(rotation, upVec);
        Mat3 rotatedMat = matRot3(rotation, identityMat);

        Vec3 matRotatedVec = rotatedMat * upVec;

        HG_ASSERT(abs(rotatedVec.x - 1.0f) < FLT_EPSILON
              && abs(rotatedVec.y - 0.0f) < FLT_EPSILON
              && abs(rotatedVec.y - 0.0f) < FLT_EPSILON);

        HG_ASSERT(abs(matRotatedVec.x - rotatedVec.x) < FLT_EPSILON
              && abs(matRotatedVec.y - rotatedVec.y) < FLT_EPSILON
              && abs(matRotatedVec.y - rotatedVec.z) < FLT_EPSILON);
    }

    // Circle
    {
        {
            Circle circle{{0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointCircle({0.0f, 0.0f}, circle));
        }

        {
            Circle circle{{0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointCircle({3.0f, 4.0f}, circle));
        }

        {
            Circle circle{{0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointCircle({5.0f, 0.0f}, circle));
        }

        {
            Circle circle{{0.0f, 0.0f}, 5.0f};
            HG_ASSERT(!containsPointCircle({5.01f, 0.0f}, circle));
        }

        {
            Circle circle{{2.0f, -3.0f}, 2.0f};
            HG_ASSERT(containsPointCircle({2.0f, -3.0f}, circle));
            HG_ASSERT(containsPointCircle({4.0f, -3.0f}, circle));
            HG_ASSERT(!containsPointCircle({4.1f, -3.0f}, circle));
        }

        {
            Circle circle{{0.0f, 0.0f}, 0.0f};
            HG_ASSERT(containsPointCircle({0.0f, 0.0f}, circle));
            HG_ASSERT(!containsPointCircle({0.01f, 0.0f}, circle));
        }
    }

    // Rect
    {
        {
            Rect rect = rectEmpty();
            HG_ASSERT(!containsPointRect({0.0f, 0.0f}, rect));
            HG_ASSERT(!containsPointRect({1.0f, 1.0f}, rect));
        }

        {
            Rect rect = rectEmpty();
            rect = rectAddPoint(rect, {2.0f, 3.0f});
            HG_ASSERT(containsPointRect({2.0f, 3.0f}, rect));
        }

        {
            Rect rect = rectEmpty();
            rect = rectAddPoint(rect, {1.0f, 2.0f});
            HG_ASSERT(containsPointRect({1.0f, 2.0f}, rect));
        }

        {
            Rect rect = rectEmpty();
            rect = rectAddPoint(rect, {2.0f, 2.0f});
            rect = rectAddPoint(rect, {5.0f, 7.0f});

            HG_ASSERT(containsPointRect({2.0f, 2.0f}, rect));
            HG_ASSERT(containsPointRect({5.0f, 7.0f}, rect));
            HG_ASSERT(containsPointRect({3.0f, 4.0f}, rect));
        }

        {
            Rect rect = rectEmpty();
            rect = rectAddPoint(rect, {5.0f, 5.0f});
            rect = rectAddPoint(rect, {-2.0f, -3.0f});

            HG_ASSERT(containsPointRect({-2.0f, -3.0f}, rect));
            HG_ASSERT(containsPointRect({5.0f, 5.0f}, rect));
            HG_ASSERT(containsPointRect({0.0f, 0.0f}, rect));
        }

        {
            Rect rect = rectEmpty();
            rect = rectAddPoint(rect, {1.0f, 1.0f});
            rect = rectAddPoint(rect, {1.0f, 1.0f});

            HG_ASSERT(containsPointRect({1.0f, 1.0f}, rect));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            HG_ASSERT(containsPointRect({5.0f, 2.5f}, rect));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            HG_ASSERT(containsPointRect({0.0f, 0.0f}, rect));
            HG_ASSERT(containsPointRect({10.0f, 5.0f}, rect));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            HG_ASSERT(!containsPointRect({-0.01f, 0.0f}, rect));
            HG_ASSERT(!containsPointRect({10.01f, 5.0f}, rect));
            HG_ASSERT(!containsPointRect({5.0f, 5.01f}, rect));
        }

        {
            Rect rect{{-5.0f, -3.0f}, {-3.0f, 5.0f}};
            HG_ASSERT(containsPointRect({-4.0f, 0.0f}, rect));
            HG_ASSERT(!containsPointRect({-2.9f, 0.0f}, rect));
        }

        {
            Rect rect{{2.0f, 2.0f}, {2.0f, 2.0f}};
            HG_ASSERT(containsPointRect({2.0f, 2.0f}, rect));
            HG_ASSERT(!containsPointRect({2.01f, 2.0f}, rect));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Vec2 p = closestPointRect({-5.0f, 5.0f}, rect);

            HG_ASSERT(p.x == 0.0f);
            HG_ASSERT(p.y == 5.0f);
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Vec2 p = closestPointRect({15.0f, 5.0f}, rect);

            HG_ASSERT(p.x == 10.0f);
            HG_ASSERT(p.y == 5.0f);
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Vec2 p = closestPointRect({5.0f, -3.0f}, rect);

            HG_ASSERT(p.x == 5.0f);
            HG_ASSERT(p.y == 0.0f);
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Vec2 p = closestPointRect({-3.0f, 15.0f}, rect);

            HG_ASSERT(p.x == 0.0f);
            HG_ASSERT(p.y == 10.0f);
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Vec2 p = closestPointRect({5.0f, 5.0f}, rect);

            HG_ASSERT(p.x == 5.0f);
            HG_ASSERT(p.y == 5.0f);
        }

        {
            Rect a{{0.0f, 0.0f}, {5.0f, 5.0}};
            Rect b{{3.0f, 3.0f}, {8.0f, 8.0f}};

            HG_ASSERT(intersectRects(a, b));
            HG_ASSERT(intersectRects(b, a));
        }

        {
            Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            Rect b{{5.0f, 0.0f}, {7.0f, 2.0f}};

            HG_ASSERT(intersectRects(a, b));
        }

        {
            Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            Rect b{{5.0f, 5.0f}, {7.0f, 7.0f}};

            HG_ASSERT(intersectRects(a, b));
        }

        {
            Rect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            Rect b{{5.01f, 0.0f}, {7.01f, 2.0f}};

            HG_ASSERT(!intersectRects(a, b));
        }

        {
            Rect a{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Rect b{{2.0f, 2.0f}, {4.0f, 4.0f}};

            HG_ASSERT(intersectRects(a, b));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Circle circle{{5.0f, 5.0f}, 2.0f};

            HG_ASSERT(intersectRectCircle(rect, circle));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Circle circle{{12.0f, 5.0f}, 2.0f};

            HG_ASSERT(intersectRectCircle(rect, circle));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Circle circle{{13.0f, 5.0f}, 2.0f};

            HG_ASSERT(!intersectRectCircle(rect, circle));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Circle circle{{12.0f, 12.0f}, std::sqrt(8.0f) + FLT_EPSILON};

            HG_ASSERT(intersectRectCircle(rect, circle));
        }

        {
            Rect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            Circle circle{{13.0f, 13.0f}, 2.0f};

            HG_ASSERT(!intersectRectCircle(rect, circle));
        }
    }

    // Ray2D
    {
        {
            Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Ray2D b{{5.0f, -5.0f}, {0.0f, 1.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRays2D(a, b, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Ray2D b{{5.0f, 5.0f}, {0.0f, 1.0f}};

            HG_ASSERT(!intersectRays2D(a, b, nullptr));
        }

        {
            Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Ray2D b{{-5.0f, -5.0f}, {0.0f, 1.0f}};

            HG_ASSERT(!intersectRays2D(a, b, nullptr));
        }

        {
            Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Ray2D b{{0.0f, 1.0f}, {1.0f, 0.0f}};

            HG_ASSERT(!intersectRays2D(a, b, nullptr));
        }

        {
            Ray2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Ray2D b{{0.0f, 0.0f}, {0.0f, 1.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRays2D(a, b, &hit));
            HG_ASSERT(std::abs(hit.dist) <= FLT_EPSILON);
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Line2D line{{5.0f, -2.0f}, {5.0f, 2.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayLine2D(ray, line, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Line2D line{{-5.0f, -2.0f}, {-5.0f, 2.0f}};

            HG_ASSERT(!intersectRayLine2D(ray, line, nullptr));
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Line2D line{{5.0f, 1.0f}, {8.0f, 1.0f}};

            HG_ASSERT(!intersectRayLine2D(ray, line, nullptr));
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Line2D line{{5.0f, 0.0f}, {5.0f, 5.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayLine2D(ray, line, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            Ray2D ray{{5.0f, 0.0f}, {1.0f, 0.0f}};
            Line2D line{{5.0f, -5.0f}, {5.0f, 5.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayLine2D(ray, line, &hit));
            HG_ASSERT(std::abs(hit.dist) <= FLT_EPSILON);
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectRayCircle(ray, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Ray2D ray{{0.0f, 2.0f}, {1.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectRayCircle(ray, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            Ray2D ray{{0.0f, 3.0f}, {1.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectRayCircle(ray, circle, nullptr));
        }

        {
            Ray2D ray{{10.0f, 0.0f}, {1.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectRayCircle(ray, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            Ray2D ray{{20.0f, 0.0f}, {1.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectRayCircle(ray, circle, nullptr));
        }

        {
            Ray2D ray{{0.0f, 5.0f}, {1.0f, 0.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Ray2D ray{{20.0f, 5.0f}, {-1.0f, 0.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            Ray2D ray{{12.5f, -5.0f}, {0.0f, 1.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {0.0f, -1.0f}));
        }

        {
            Ray2D ray{{12.5f, 20.0f}, {0.0f, -1.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {0.0f, 1.0f}));
        }

        {
            Ray2D ray{{0.0f, 0.0f}, {1.0f, 1.0f}};
            Rect rect{{10.0f, 10.0f}, {15.0f, 15.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            Ray2D ray{{0.0f, 5.0f}, {-1.0f, 0.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            HG_ASSERT(!intersectRayRect(ray, rect, nullptr));
        }

        {
            Ray2D ray{{12.5f, 5.0f}, {1.0f, 0.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectRayRect(ray, rect, &hit));
            HG_ASSERT(hit.dist == 0.0f);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Ray2D ray{{0.0f, 20.0f}, {1.0f, 0.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            HG_ASSERT(!intersectRayRect(ray, rect, nullptr));
        }

    }

    // Line2D
    {
        {
            Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Line2D b{{5.0f, -5.0f}, {5.0f, 5.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLines2D(a, b, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Line2D b{{15.0f, -5.0f}, {15.0f, 5.0f}};

            HG_ASSERT(!intersectLines2D(a, b, nullptr));
        }

        {
            Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Line2D b{{10.0f, 0.0f}, {10.0f, 5.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLines2D(a, b, &hit));
            HG_ASSERT(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            Line2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Line2D b{{0.0f, 1.0f}, {10.0f, 1.0f}};

            HG_ASSERT(!intersectLines2D(a, b, nullptr));
        }

        {
            Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Ray2D ray{{5.0f, -5.0f}, {0.0f, 1.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRay2D(line, ray, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Ray2D ray{{15.0f, -5.0f}, {0.0f, 1.0f}};

            HG_ASSERT(!intersectLineRay2D(line, ray, nullptr));
        }

        {
            Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Ray2D ray{{10.0f, -5.0f}, {0.0f, 1.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRay2D(line, ray, &hit));
            HG_ASSERT(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            Line2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            Ray2D ray{{5.0f, 5.0f}, {0.0f, 1.0f}};

            HG_ASSERT(!intersectLineRay2D(line, ray, nullptr));
        }

        {
            Line2D line{{0.0f, 0.0f}, {20.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectLineCircle(line, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Line2D line{{0.0f, 2.0f}, {20.0f, 2.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectLineCircle(line, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            Line2D line{{0.0f, 3.0f}, {20.0f, 3.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectLineCircle(line, circle, nullptr));
        }

        {
            Line2D line{{0.0f, 0.0f}, {5.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectLineCircle(line, circle, nullptr));
        }

        {
            Line2D line{{10.0f, 0.0f}, {20.0f, 0.0f}};
            Circle circle{{10.0f, 0.0f}, 2.0f};

            Hit2D hit;
            HG_ASSERT(intersectLineCircle(line, circle, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            Line2D line{{0.0f, 5.0f}, {20.0f, 5.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRect(line, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            Line2D line{{20.0f, 5.0f}, {0.0f, 5.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRect(line, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            Line2D line{{12.5f, -5.0f}, {12.5f, 15.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRect(line, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {0.0f, -1.0f}));
        }

        {
            Line2D line{{0.0f, 20.0f}, {20.0f, 20.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            HG_ASSERT(!intersectLineRect(line, rect, nullptr));
        }

        {
            Line2D line{{12.5f, 5.0f}, {17.5f, 5.0f}};
            Rect rect{{10.0f, 0.0f}, {15.0f, 10.0f}};

            Hit2D hit;
            HG_ASSERT(intersectLineRect(line, rect, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq2(hit.normal, {1.0f, 0.0f}));
        }
    }

    // Sphere
    {
        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointSphere({3.0f, 4.0f, 0.0f}, sphere));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(containsPointSphere({5.0f, 0.0f, 0.0f}, sphere));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(!containsPointSphere({5.01f, 0.0f, 0.0f}, sphere));
        }

        {
            Sphere sphere{{2.0f, -3.0f, 4.0f}, 2.0f};
            HG_ASSERT(containsPointSphere({2.0f, -3.0f, 4.0f}, sphere));
            HG_ASSERT(containsPointSphere({4.0f, -3.0f, 4.0f}, sphere));
            HG_ASSERT(!containsPointSphere({4.1f, -3.0f, 4.0f}, sphere));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
            HG_ASSERT(containsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
            HG_ASSERT(!containsPointSphere({0.01f, 0.0f, 0.0f}, sphere));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(std::abs(distPointSphere({10.0f, 0.0f, 0.0f}, sphere) - 5.0f) <= FLT_EPSILON);
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(std::abs(distPointSphere({5.0f, 0.0f, 0.0f}, sphere)) <= FLT_EPSILON);
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HG_ASSERT(std::abs(distPointSphere({0.0f, 0.0f, 0.0f}, sphere) + 5.0f) <= FLT_EPSILON);
        }

        {
            Sphere sphere{{2.0f, 3.0f, 4.0f}, 2.0f};
            HG_ASSERT(std::abs(distPointSphere({6.0f, 3.0f, 4.0f}, sphere) - 2.0f) <= FLT_EPSILON);
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            Vec3 p = closestPointSphere({10.0f, 0.0f, 0.0f}, sphere);

            HG_ASSERT(vecEq3(p, {5.0f, 0.0f, 0.0f}));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            Vec3 p = closestPointSphere({0.0f, 10.0f, 0.0f}, sphere);

            HG_ASSERT(vecEq3(p, {0.0f, 5.0f, 0.0f}));
        }

        {
            Sphere sphere{{2.0f, 1.0f, -3.0f}, 3.0f};
            Vec3 p = closestPointSphere({5.0f, 1.0f, -3.0f}, sphere);

            HG_ASSERT(vecEq3(p, {5.0f, 1.0f, -3.0f}));
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            Vec3 p = closestPointSphere({0.0f, 0.0f, 0.0f}, sphere);

            HG_ASSERT(distPointSphere(p, sphere) <= FLT_EPSILON);
        }

        {
            Sphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
            Vec3 p = closestPointSphere({10.0f, 2.0f, -5.0f}, sphere);

            HG_ASSERT(vecEq3(p, {0.0f, 0.0f, 0.0f}));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{8.0f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(intersectSpheres(a, b));
            HG_ASSERT(intersectSpheres(b, a));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{10.0f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(intersectSpheres(a, b));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{10.1f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(!intersectSpheres(a, b));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{0.0f, 0.0f, 0.0f}, 2.0f};

            HG_ASSERT(intersectSpheres(a, b));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 0.0f};
            Sphere b{{0.0f, 0.0f, 0.0f}, 0.0f};

            HG_ASSERT(intersectSpheres(a, b));
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{20.0f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(std::abs(distSpheres(a, b) - 10.0f) <= FLT_EPSILON);
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{10.0f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(std::abs(distSpheres(a, b)) <= FLT_EPSILON);
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{5.0f, 0.0f, 0.0f}, 5.0f};

            HG_ASSERT(distSpheres(a, b) < 0.0f);
        }

        {
            Sphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            Sphere b{{0.0f, 0.0f, 0.0f}, 1.0f};

            HG_ASSERT(distSpheres(a, b) < 0.0f);
        }
    }

    // Box
    {
        {
            Box box = boxEmpty();

            HG_ASSERT(!containsPointBox({0.0f, 0.0f, 0.0f}, box));
            HG_ASSERT(!containsPointBox({1.0f, 1.0f, 1.0f}, box));
        }

        {
            Box box = boxEmpty();
            box = boxAddPoint(box, {2.0f, 3.0f, 4.0f});

            HG_ASSERT(containsPointBox({2.0f, 3.0f, 4.0f}, box));
        }

        {
            Box box = boxEmpty();
            box = boxAddPoint(box, {1.0f, 2.0f, 3.0f});

            HG_ASSERT(containsPointBox({1.0f, 2.0f, 3.0f}, box));
        }

        {
            Box box = boxEmpty();
            box = boxAddPoint(box, {2.0f, 2.0f, 2.0f});
            box = boxAddPoint(box, {5.0f, 7.0f, 11.0f});

            HG_ASSERT(containsPointBox({2.0f, 2.0f, 2.0f}, box));
            HG_ASSERT(containsPointBox({5.0f, 7.0f, 11.0f}, box));
            HG_ASSERT(containsPointBox({3.0f, 4.0f, 5.0f}, box));
        }

        {
            Box box = boxEmpty();
            box = boxAddPoint(box, {5.0f, 5.0f, 5.0f});
            box = boxAddPoint(box, {-2.0f, -3.0f, -4.0f});

            HG_ASSERT(containsPointBox({-2.0f, -3.0f, -4.0f}, box));
            HG_ASSERT(containsPointBox({5.0f, 5.0f, 5.0f}, box));
            HG_ASSERT(containsPointBox({0.0f, 0.0f, 0.0f}, box));
        }

        {
            Box box = boxEmpty();
            box = boxAddPoint(box, {1.0f, 1.0f, 1.0f});
            box = boxAddPoint(box, {1.0f, 1.0f, 1.0f});

            HG_ASSERT(containsPointBox({1.0f, 1.0f, 1.0f}, box));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            HG_ASSERT(containsPointBox({5.0f, 2.5f, 4.0f}, box));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            HG_ASSERT(containsPointBox({0.0f, 0.0f, 0.0f}, box));
            HG_ASSERT(containsPointBox({10.0f, 5.0f, 8.0f}, box));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            HG_ASSERT(!containsPointBox({-0.01f, 0.0f, 0.0f}, box));
            HG_ASSERT(!containsPointBox({10.01f, 5.0f, 8.0f}, box));
            HG_ASSERT(!containsPointBox({5.0f, 5.01f, 4.0f}, box));
            HG_ASSERT(!containsPointBox({5.0f, 2.5f, 8.01f}, box));
        }

        {
            Box box{{-5.0f, -3.0f, -2.0f}, {-3.0f, 5.0f, 4.0f}};
            HG_ASSERT(containsPointBox({-4.0f, 0.0f, 0.0f}, box));
            HG_ASSERT(!containsPointBox({-2.9f, 0.0f, 0.0f}, box));
        }

        {
            Box box{{2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}};
            HG_ASSERT(containsPointBox({2.0f, 2.0f, 2.0f}, box));
            HG_ASSERT(!containsPointBox({2.01f, 2.0f, 2.0f}, box));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({-5.0f, 5.0f, 5.0f}, box);

            HG_ASSERT(vecEq3(p, {0.0f, 5.0f, 5.0f}));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({15.0f, 5.0f, 5.0f}, box);

            HG_ASSERT(vecEq3(p, {10.0f, 5.0f, 5.0f}));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({5.0f, -3.0f, 5.0f}, box);

            HG_ASSERT(vecEq3(p, {5.0f, 0.0f, 5.0f}));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({5.0f, 5.0f, 15.0f}, box);

            HG_ASSERT(vecEq3(p, {5.0f, 5.0f, 10.0f}));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({-5.0f, 15.0f, -3.0f}, box);

            HG_ASSERT(vecEq3(p, {0.0f, 10.0f, 0.0f}));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Vec3 p = closestPointBox({5.0f, 5.0f, 5.0f}, box);

            HG_ASSERT(vecEq3(p, {5.0f, 5.0f, 5.0f}));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{3.0f, 3.0f, 3.0f}, {8.0f, 8.0f, 8.0f}};

            HG_ASSERT(intersectBox(a, b));
            HG_ASSERT(intersectBox(b, a));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{5.0f, 0.0f, 0.0f}, {7.0f, 2.0f, 2.0f}};

            HG_ASSERT(intersectBox(a, b));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{0.0f, 5.0f, 0.0f}, {2.0f, 7.0f, 2.0f}};

            HG_ASSERT(intersectBox(a, b));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{0.0f, 0.0f, 5.0f}, {2.0f, 2.0f, 7.0f}};

            HG_ASSERT(intersectBox(a, b));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{5.0f, 5.0f, 5.0f}, {7.0f, 7.0f, 7.0f}};

            HG_ASSERT(intersectBox(a, b));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            Box b{{5.01f, 0.0f, 0.0f}, {7.01f, 2.0f, 2.0f}};

            HG_ASSERT(!intersectBox(a, b));
        }

        {
            Box a{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Box b{{2.0f, 2.0f, 2.0f}, {4.0f, 4.0f, 4.0f}};

            HG_ASSERT(intersectBox(a, b));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{5.0f, 5.0f, 5.0f}, 2.0f};

            HG_ASSERT(intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{12.0f, 5.0f, 5.0f}, 2.0f};

            HG_ASSERT(intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{5.0f, 12.0f, 5.0f}, 2.0f};

            HG_ASSERT(intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{5.0f, 5.0f, 12.0f}, 2.0f};

            HG_ASSERT(intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{13.0f, 5.0f, 5.0f}, 2.0f};

            HG_ASSERT(!intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{12.0f, 12.0f, 12.0f}, std::sqrt(12.0f)};

            HG_ASSERT(intersectBoxSphere(box, sphere));
        }

        {
            Box box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            Sphere sphere{{13.0f, 13.0f, 13.0f}, 2.0f};

            HG_ASSERT(!intersectBoxSphere(box, sphere));
        }
    }

    // Plane
    {
        {
            Plane plane = planeFromPoint({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
            HG_ASSERT(vecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
            HG_ASSERT(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
            HG_ASSERT(vecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
            HG_ASSERT(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            Plane plane = planeFromPoint({3.0f, 2.0f, -1.0f}, {1.0f, 0.0f, 0.0f});
            HG_ASSERT(vecEq3(plane.normal, {1.0f, 0.0f, 0.0f}));
            HG_ASSERT(std::abs(plane.dist - 3.0f) <= FLT_EPSILON);
        }

        {
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
            Plane plane = planeFromTri(tri);

            HG_ASSERT(vecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
            HG_ASSERT(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            Tri tri{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Plane plane = planeFromTri(tri);

            HG_ASSERT(vecEq3(plane.normal, {0.0f, 0.0f, -1.0f}));
            HG_ASSERT(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            Tri tri{{1.0f, 2.0f, 5.0f}, {4.0f, 2.0f, 5.0f}, {1.0f, 6.0f, 5.0f}};
            Plane plane = planeFromTri(tri);

            HG_ASSERT(vecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
            HG_ASSERT(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
        }
    }

    // Ray3D
    {
        {
            Ray3D ray{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectRaySphere(ray, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            Ray3D ray{{0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectRaySphere(ray, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            Ray3D ray{{0.0f, 3.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectRaySphere(ray, sphere, nullptr));
        }

        {
            Ray3D ray{{10.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectRaySphere(ray, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            Ray3D ray{{20.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectRaySphere(ray, sphere, nullptr));
        }

        {
            Ray3D ray{{0.0f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayBox(ray, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            Ray3D ray{{20.0f, 5.0f, 5.0f}, {-1.0f, 0.0f, 0.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayBox(ray, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            Ray3D ray{{12.5f, -5.0f, 5.0f}, {0.0f, 1.0f, 0.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayBox(ray, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            Ray3D ray{{12.5f, 5.0f, -5.0f}, {0.0f, 0.0f, 1.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayBox(ray, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            Ray3D ray{{0.0f, 20.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            HG_ASSERT(!intersectRayBox(ray, box, nullptr));
        }

        {
            Ray3D ray{{12.5f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayBox(ray, box, &hit));
            HG_ASSERT(std::abs(hit.dist) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            Ray3D ray{{0.25f, 0.25f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayTri(ray, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            Ray3D ray{{0.5f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayTri(ray, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            Ray3D ray{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectRayTri(ray, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            Ray3D ray{{0.75f, 0.75f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HG_ASSERT(!intersectRayTri(ray, tri, nullptr));
        }

        {
            Ray3D ray{{0.25f, 0.25f, 1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HG_ASSERT(!intersectRayTri(ray, tri, nullptr));
        }

        {
            Ray3D ray{{0.25f, 0.25f, -1.0f}, {1.0f, 0.0f, 0.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HG_ASSERT(!intersectRayTri(ray, tri, nullptr));
        }

        {
            Ray3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectRayPlane(ray, plane, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
        }

        {
            Ray3D ray{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectRayPlane(ray, plane, &hit));
            HG_ASSERT(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            Ray3D ray{{0.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HG_ASSERT(!intersectRayPlane(ray, plane, nullptr));
        }

        {
            Ray3D ray{{0.0f, 2.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HG_ASSERT(!intersectRayPlane(ray, plane, nullptr));
        }

        {
            Ray3D ray{{1.0f, 5.0f, 2.0f}, {0.0f, 1.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectRayPlane(ray, plane, &hit));
            HG_ASSERT(std::abs(hit.dist) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            Ray3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -2.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectRayPlane(ray, plane, &hit));
            HG_ASSERT(std::abs(hit.dist - 2.5f) <= FLT_EPSILON);
        }
    }

    // Line3D
    {
        {
            Line3D line{{0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectLineSphere(line, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            Line3D line{{0.0f, 2.0f, 0.0f}, {20.0f, 2.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectLineSphere(line, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            Line3D line{{0.0f, 3.0f, 0.0f}, {20.0f, 3.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectLineSphere(line, sphere, nullptr));
        }

        {
            Line3D line{{0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HG_ASSERT(!intersectLineSphere(line, sphere, nullptr));
        }

        {
            Line3D line{{10.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
            Sphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            Hit3D hit;
            HG_ASSERT(intersectLineSphere(line, sphere, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
        }

        {
            Line3D line{{0.0f, 5.0f, 5.0f}, {20.0f, 5.0f, 5.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineBox(line, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            Line3D line{{20.0f, 5.0f, 5.0f}, {0.0f, 5.0f, 5.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineBox(line, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            Line3D line{{12.5f, -5.0f, 5.0f}, {12.5f, 15.0f, 5.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineBox(line, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            Line3D line{{12.5f, 5.0f, -5.0f}, {12.5f, 5.0f, 15.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineBox(line, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            Line3D line{{0.0f, 20.0f, 5.0f}, {20.0f, 20.0f, 5.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            HG_ASSERT(!intersectLineBox(line, box, nullptr));
        }

        {
            Line3D line{{12.5f, 5.0f, 5.0f}, {17.5f, 5.0f, 5.0f}};
            Box box{{10.0f, 0.0f, 0.0f}, {15.0f, 10.0f, 10.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineBox(line, box, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            Line3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineTri(line, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            Line3D line{{0.5f, 0.0f, -1.0f}, {0.5f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineTri(line, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            Line3D line{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineTri(line, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            Line3D line{{0.75f, 0.75f, -1.0f}, {0.75f, 0.75f, 1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HG_ASSERT(!intersectLineTri(line, tri, nullptr));
        }

        {
            Line3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, -0.5f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HG_ASSERT(!intersectLineTri(line, tri, nullptr));
        }

        {
            Line3D line{{0.25f, 0.25f, 1.0f}, {0.25f, 0.25f, -1.0f}};
            Tri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            Hit3D hit;
            HG_ASSERT(intersectLineTri(line, tri, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 0.0f, 1.0f}));
        }

        {
            Line3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectLinePlane(line, plane, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
        }

        {
            Line3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 10.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectLinePlane(line, plane, &hit));
            HG_ASSERT(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            Line3D line{{0.0f, 10.0f, 0.0f}, {10.0f, 10.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HG_ASSERT(!intersectLinePlane(line, plane, nullptr));
        }

        {
            Line3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HG_ASSERT(!intersectLinePlane(line, plane, nullptr));
        }

        {
            Line3D line{{1.0f, 5.0f, 2.0f}, {1.0f, 10.0f, 2.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            Hit3D hit;
            HG_ASSERT(intersectLinePlane(line, plane, &hit));
            HG_ASSERT(std::abs(hit.dist) <= FLT_EPSILON);
            HG_ASSERT(vecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            Line3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 20.0f, 0.0f}};
            Plane plane = planeFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HG_ASSERT(!intersectLinePlane(line, plane, nullptr));
        }
    }

    // AssetManager and Binary
    {
        {
            BinaryAsset* bin1 = assetCreate<Binary>();
            HG_ASSERT(bin1 != nullptr);
            HG_ASSERT(bin1->path == "");

            BinaryAsset* bin2 = assetCreate<Binary>();
            HG_ASSERT(bin2 != nullptr);
            HG_ASSERT(bin2->path == "");
            HG_ASSERT(bin2 != bin1);

            assetUnload(bin1);
            assetUnload(bin2);
        }

        {
            BinaryAsset* bin = assetLoad<Binary>("file_does_not_exist.bin");
            HG_ASSERT(bin->asset.data == nullptr);
            HG_ASSERT(bin->asset.size == 0);
            assetUnload(bin);
        }

        u32 saveData[]{12, 42, 100, 128};

        {
            Binary bin{saveData, sizeof(saveData)};

            binaryStore(bin, "dir/does/not/exist.bin");

            FILE* fileHandle = fopen("dir/does/not/exist.bin", "rb");
            HG_ASSERT(fileHandle == nullptr);
        }

        {
            Binary bin{saveData, sizeof(saveData)};

            String filePath = "hg_test_dir/file_bin_test.bin";

            binaryStore(bin, filePath);

            BinaryAsset* newBin = assetLoad<Binary>(filePath);

            HG_ASSERT(newBin->asset.data != nullptr);
            HG_ASSERT(newBin->asset.data != saveData);
            HG_ASSERT(newBin->asset.size == sizeof(saveData));
            HG_ASSERT(memEqual(saveData, newBin->asset.data, newBin->asset.size));

            BinaryAsset* newBin2 = assetLoad<Binary>(filePath);
            HG_ASSERT(newBin2 == newBin);

            assetUnload(newBin);
            assetUnload(newBin2);
        }
    }

    // Image
    {
        struct color {
            u8 r, g, b, a;

            operator u32() { return *reinterpret_cast<u32*>(this); }
        };

        u32 red =    color{0xff, 0x00, 0x00, 0xff};
        u32 green =  color{0x00, 0xff, 0x00, 0xff};
        u32 blue =   color{0x00, 0x00, 0xff, 0xff};
        u32 yellow = color{0xff, 0xff, 0x00, 0xff};

        u32 saveData[2][2]{
            {red, green},
            {blue, yellow},
        };

        String path = "hg_test_dir/image_test.png";

        TextureData testImage{};
        testImage.width = 2;
        testImage.height = 2;
        testImage.depth = 1;
        testImage.format = Format_r8g8b8a8_srgb;
        testImage.pixels = saveData;

        {
            textureStorePng(&testImage, path);

            TextureDataAsset* image = assetLoad<TextureData>(path);
            HG_DEFER(assetUnload(image));
            HG_ASSERT(image->asset.width == testImage.width);
            HG_ASSERT(image->asset.height == testImage.height);
            HG_ASSERT(memEqual(image->asset.pixels, saveData, sizeof(saveData)));
        }
    }

    // Ecs basics
    {
        Ecs ecs = ecsCreate();
        HG_DEFER(ecsDestroy(&ecs));

        HG_ECS_REGISTER_TYPE(&ecs, u32);
        HG_ECS_REGISTER_TYPE(&ecs, u64);

        Entity e1 = ecsSpawn(&ecs);
        Entity e2 = ecsSpawn(&ecs);
        Entity e3 = {};
        HG_ASSERT(ecsAlive(&ecs, e1));
        HG_ASSERT(ecsAlive(&ecs, e2));
        HG_ASSERT(!ecsAlive(&ecs, e3));

        ecsDespawn(&ecs, e1);
        HG_ASSERT(!ecsAlive(&ecs, e1));
        e3 = ecsSpawn(&ecs);
        HG_ASSERT(ecsAlive(&ecs, e3));
        HG_ASSERT(handleIdx(e3.handle) == handleIdx(e1.handle) && e3.handle.id != e1.handle.id);

        e1 = ecsSpawn(&ecs);
        HG_ASSERT(ecsAlive(&ecs, e1));

        {
            HG_ASSERT(!ecsHas<u32>(&ecs, e1));
            HG_ASSERT(!ecsHas<u32>(&ecs, e2));
            HG_ASSERT(!ecsHas<u32>(&ecs, e3));

            *ecsAdd<u32>(&ecs, e1) = 1;
            HG_ASSERT(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
            HG_ASSERT(!ecsHas<u32>(&ecs, e2));
            HG_ASSERT(!ecsHas<u32>(&ecs, e3));
            *ecsAdd<u32>(&ecs, e2) = 2;
            HG_ASSERT(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
            HG_ASSERT(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
            HG_ASSERT(!ecsHas<u32>(&ecs, e3));
            *ecsAdd<u32>(&ecs, e3) = 3;
            HG_ASSERT(ecsHas<u32>(&ecs, e1) && *ecsGet<u32>(&ecs, e1) == 1);
            HG_ASSERT(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
            HG_ASSERT(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);

            ecsRemove<u32>(&ecs, e1);
            HG_ASSERT(!ecsHas<u32>(&ecs, e1));
            HG_ASSERT(ecsHas<u32>(&ecs, e2) && *ecsGet<u32>(&ecs, e2) == 2);
            HG_ASSERT(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);
            ecsRemove<u32>(&ecs, e2);
            HG_ASSERT(!ecsHas<u32>(&ecs, e1));
            HG_ASSERT(!ecsHas<u32>(&ecs, e2));
            HG_ASSERT(ecsHas<u32>(&ecs, e3) && *ecsGet<u32>(&ecs, e3) == 3);
            ecsRemove<u32>(&ecs, e3);
            HG_ASSERT(!ecsHas<u32>(&ecs, e1));
            HG_ASSERT(!ecsHas<u32>(&ecs, e2));
            HG_ASSERT(!ecsHas<u32>(&ecs, e3));
        }

        {
            bool hasUnknown = false;
            ecsForEach<u32>(&ecs, [&](Entity, u32*)
            {
                hasUnknown = true;
            });
            HG_ASSERT(!hasUnknown);

            HG_ASSERT(ecsCount<u32>(&ecs) == 0);
            HG_ASSERT(ecsCount<u64>(&ecs) == 0);
        }

        {
            *ecsAdd<u32>(&ecs, e1) = 12;
            *ecsAdd<u32>(&ecs, e2) = 42;
            *ecsAdd<u32>(&ecs, e3) = 100;
            HG_ASSERT(ecsCount<u32>(&ecs) == 3);
            HG_ASSERT(ecsCount<u64>(&ecs) == 0);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            ecsForEach<u32>(&ecs, [&](Entity e, u32* c)
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
            HG_ASSERT(has12);
            HG_ASSERT(has42);
            HG_ASSERT(has100);
            HG_ASSERT(!hasUnknown);
        }

        {
            *ecsAdd<u64>(&ecs, e2) = 2042;
            *ecsAdd<u64>(&ecs, e3) = 2100;
            HG_ASSERT(ecsCount<u32>(&ecs) == 3);
            HG_ASSERT(ecsCount<u64>(&ecs) == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            bool has2042 = false;
            bool has2100 = false;
            ecsForEach<u32, u64>(&ecs, [&](Entity e, u32* comp32, u64* comp64)
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
            HG_ASSERT(!has12);
            HG_ASSERT(has42);
            HG_ASSERT(has100);
            HG_ASSERT(has2042);
            HG_ASSERT(has2100);
            HG_ASSERT(!hasUnknown);
        }

        {
            ecsDespawn(&ecs, e1);
            HG_ASSERT(ecsCount<u32>(&ecs) == 2);
            HG_ASSERT(ecsCount<u64>(&ecs) == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            ecsForEach<u32>(&ecs, [&](Entity e, u32* c)
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
            HG_ASSERT(!has12);
            HG_ASSERT(has42);
            HG_ASSERT(has100);
            HG_ASSERT(!hasUnknown);
        }

        {
            ecsDespawn(&ecs, e2);
            HG_ASSERT(ecsCount<u32>(&ecs) == 1);
            HG_ASSERT(ecsCount<u64>(&ecs) == 1);
        }
    }

    // Ecs concurrency
    {
        Ecs ecs = ecsCreate();
        HG_DEFER(ecsDestroy(&ecs));

        HG_ECS_REGISTER_TYPE(&ecs, u32);
        HG_ECS_REGISTER_TYPE(&ecs, u64);

        {
            for (u32 i = 0; i < 4; ++i)
            {
                Entity e = ecsSpawn(&ecs);
                switch (i % 3)
                {
                    case 0:
                        *ecsAdd<u32>(&ecs, e) = 12;
                        *ecsAdd<u64>(&ecs, e) = 42;
                        break;
                    case 1:
                        *ecsAdd<u32>(&ecs, e) = 12;
                        break;
                    case 2:
                        *ecsAdd<u64>(&ecs, e) = 42;
                        break;
                }
            }

            bool success;
            ecsForPar<u32>(&ecs, [&](Entity, u32* c)
            {
                *c += 4;
            });
            success = true;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != 16)
                    success = false;
            });
            HG_ASSERT(success);

            ecsForPar<u64>(&ecs, [&](Entity, u64* c)
            {
                *c += 3;
            });
            success = true;
            ecsForEach<u64>(&ecs, [&](Entity, u64* c)
            {
                if (*c != 45)
                    success = false;
            });
            HG_ASSERT(success);

            ecsForPar<u32, u64>(&ecs, [&](Entity, u32* c32, u64* c64)
            {
                *c64 -= *c32;
            });
            success = true;
            ecsForEach<u64>(&ecs, [&](Entity e, u64* c)
            {
                if (ecsHas<u32>(&ecs, e))
                {
                    if (*c != 29)
                        success = false;
                } else {
                    if (*c != 45)
                        success = false;
                }
            });
            HG_ASSERT(success);
        }
    }

    // Ecs sort
    {
        Ecs ecs = ecsCreate();
        HG_DEFER(ecsDestroy(&ecs));

        HG_ECS_REGISTER_TYPE(&ecs, u32);
        HG_ECS_REGISTER_TYPE(&ecs, u64);

        auto comparison = [](void*, Ecs* ecs, Entity lhs, Entity rhs)
        {
            return *ecsGet<u32>(ecs, lhs) < *ecsGet<u32>(ecs, rhs);
        };

        {
            *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = 42;

            ecsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != 42)
                    success = false;
            });
            HG_ASSERT(success);

            ecsReset(&ecs);
        }

        {
            u32 smallScramble1[]{1, 0};
            for (u32 i = 0; i < std::size(smallScramble1); ++i)
            {
                *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = smallScramble1[i];
            }

            {
                ecsSort<u32>(&ecs, nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecsForEach<u32>(&ecs, [&](Entity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                HG_ASSERT(success);
            }

            {
                ecsSort<u32>(&ecs, nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                ecsForEach<u32>(&ecs, [&](Entity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                HG_ASSERT(success);
            }

            ecsReset(&ecs);
        }

        {
            u32 mediumScramble1[]{8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
            for (u32 i = 0; i < std::size(mediumScramble1); ++i)
            {
                *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = mediumScramble1[i];
            }
            ecsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            HG_ASSERT(success);

            ecsReset(&ecs);
        }

        {
            u32 mediumScramble2[]{3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
            for (u32 i = 0; i < std::size(mediumScramble2); ++i)
            {
                *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = mediumScramble2[i];
            }
            ecsSort<u32>(&ecs, nullptr, comparison);
            ecsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            HG_ASSERT(success);

            ecsReset(&ecs);
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = i;
            }
            ecsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            HG_ASSERT(success);

            ecsReset(&ecs);
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                *ecsAdd<u32>(&ecs, ecsSpawn(&ecs)) = i / 2;
            }
            ecsSort<u32>(&ecs, nullptr, comparison);
            ecsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            ecsForEach<u32>(&ecs, [&](Entity, u32* c)
            {
                if (*c != elem / 2)
                    success = false;
                ++elem;
            });
            HG_ASSERT(success);

            ecsReset(&ecs);
        }
    }

    // Ecs serialization
    {
        Arena* arena = getScratch();
        HG_ARENA_SCOPE(arena);

        SerialNode* scene{};

        {
            Ecs ecs = ecsCreate();
            HG_DEFER(ecsDestroy(&ecs));

            HG_ECS_REGISTER_TYPE(&ecs, Node);
            HG_ECS_REGISTER_TYPE(&ecs, u32);

            Entity root = ecsSpawn(&ecs);
            Entity a = ecsSpawn(&ecs);
            Entity b = ecsSpawn(&ecs);

            nodeAdd(&ecs, root);
            nodeAdd(&ecs, a);
            nodeAdd(&ecs, b);

            *ecsAdd<u32>(&ecs, a) = 12;
            *ecsAdd<u32>(&ecs, b) = 42;

            nodeAddChild(&ecs, root, b);
            nodeAddChild(&ecs, root, a);

            Serializer s = serialWriter(arena);
            serialize(&s, &ecs);
            scene = s.current;
        }

        {
            Ecs ecs = ecsCreate();
            HG_DEFER(ecsDestroy(&ecs));

            HG_ECS_REGISTER_TYPE(&ecs, Node);
            HG_ECS_REGISTER_TYPE(&ecs, u32);

            Serializer s = serialReader(arena, scene);
            serialize(&s, &ecs);

            Entity root = ecsEntities<Node>(&ecs)[0];

            HG_ASSERT(ecsHas<Node>(&ecs, root));
            Node* rootNode = ecsGet<Node>(&ecs, root);
            HG_ASSERT(rootNode->parent.handle == handleNull);
            HG_ASSERT(rootNode->nextSibling.handle == handleNull);
            HG_ASSERT(rootNode->prevSibling.handle == handleNull);
            HG_ASSERT(rootNode->firstChild.handle != handleNull);

            Entity a = rootNode->firstChild;
            HG_ASSERT(a.handle != handleNull);

            HG_ASSERT(ecsHas<Node>(&ecs, a));
            Node* aNode = ecsGet<Node>(&ecs, a);
            HG_ASSERT(aNode->parent == root);
            HG_ASSERT(aNode->prevSibling.handle == handleNull);
            HG_ASSERT(aNode->nextSibling.handle != handleNull);
            HG_ASSERT(aNode->firstChild.handle == handleNull);

            Entity b = aNode->nextSibling;
            HG_ASSERT(b.handle != handleNull);

            HG_ASSERT(ecsHas<Node>(&ecs, b));
            Node* bNode = ecsGet<Node>(&ecs, b);
            HG_ASSERT(bNode->parent == root);
            HG_ASSERT(bNode->prevSibling == a);
            HG_ASSERT(bNode->nextSibling.handle == handleNull);
            HG_ASSERT(bNode->firstChild.handle == handleNull);

            HG_ASSERT(ecsHas<u32>(&ecs, a));
            HG_ASSERT(*ecsGet<u32>(&ecs, a) == 12);

            HG_ASSERT(ecsHas<u32>(&ecs, b));
            HG_ASSERT(*ecsGet<u32>(&ecs, b) == 42);
        }
    }

    // Node
    {
        Ecs ecs = ecsCreate();
        HG_DEFER(ecsDestroy(&ecs));

        HG_ECS_REGISTER_TYPE(&ecs, Node);

        {
            Entity a = ecsSpawn(&ecs);
            Entity b = ecsSpawn(&ecs);
            Entity aa = ecsSpawn(&ecs);
            Entity ab = ecsSpawn(&ecs);

            *nodeAdd(&ecs, a) = {};
            *nodeAdd(&ecs, b) = {};
            *nodeAdd(&ecs, aa) = {};
            *nodeAdd(&ecs, ab) = {};

            nodeAddChild(&ecs, a, aa);
            nodeAddChild(&ecs, a, ab);

            HG_ASSERT(ecsAlive(&ecs, a));
            HG_ASSERT(ecsAlive(&ecs, b));
            HG_ASSERT(ecsAlive(&ecs, aa));
            HG_ASSERT(ecsAlive(&ecs, ab));

            nodeDestroy(&ecs, a);

            HG_ASSERT(!ecsAlive(&ecs, a));
            HG_ASSERT(ecsAlive(&ecs, b));
            HG_ASSERT(!ecsAlive(&ecs, aa));
            HG_ASSERT(!ecsAlive(&ecs, ab));

            ecsDespawn(&ecs, b);
        }

        {
            Entity a = ecsSpawn(&ecs);
            Entity b = ecsSpawn(&ecs);
            Entity aa = ecsSpawn(&ecs);
            Entity ab = ecsSpawn(&ecs);
            Entity aba = ecsSpawn(&ecs);
            Entity abb = ecsSpawn(&ecs);

            *nodeAdd(&ecs, a) = {};
            *nodeAdd(&ecs, b) = {};
            *nodeAdd(&ecs, aa) = {};
            *nodeAdd(&ecs, ab) = {};
            *nodeAdd(&ecs, aba) = {};
            *nodeAdd(&ecs, abb) = {};

            nodeAddChild(&ecs, ab, aba);
            nodeAddChild(&ecs, ab, abb);
            nodeAddChild(&ecs, a, aa);
            nodeAddChild(&ecs, a, ab);

            HG_ASSERT(ecsAlive(&ecs, a));
            HG_ASSERT(ecsAlive(&ecs, b));
            HG_ASSERT(ecsAlive(&ecs, aa));
            HG_ASSERT(ecsAlive(&ecs, ab));
            HG_ASSERT(ecsAlive(&ecs, aba));
            HG_ASSERT(ecsAlive(&ecs, abb));

            nodeDestroy(&ecs, ab);

            HG_ASSERT(ecsAlive(&ecs, a));
            HG_ASSERT(ecsAlive(&ecs, b));
            HG_ASSERT(ecsAlive(&ecs, aa));
            HG_ASSERT(!ecsAlive(&ecs, ab));
            HG_ASSERT(!ecsAlive(&ecs, aba));
            HG_ASSERT(!ecsAlive(&ecs, abb));

            nodeDestroy(&ecs, a);

            HG_ASSERT(!ecsAlive(&ecs, a));
            HG_ASSERT(ecsAlive(&ecs, b));
            HG_ASSERT(!ecsAlive(&ecs, aa));
            HG_ASSERT(!ecsAlive(&ecs, ab));
            HG_ASSERT(!ecsAlive(&ecs, aba));
            HG_ASSERT(!ecsAlive(&ecs, abb));

            ecsDespawn(&ecs, b);
        }
    }

    HG_WARN("Mesh test : TODO\n");

    printf("HurdyGurdy: Tests Complete in %fms\n", clockTick(&timer) * 1000.0f);
}

} // namespace hg

using namespace hg;

int main()
{
    init();
    HG_DEFER(deinit());

    test();

    HG_LOG("All tests passed\n");
    return 0;
}

