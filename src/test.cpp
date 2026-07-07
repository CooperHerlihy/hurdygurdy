#undef HG_NO_LOGGING
#define HG_LOGGING 1
#undef HG_NO_ASSERTIONS
#define HG_ASSERTIONS 1
#include "hurdygurdy.hpp"

#include <cstdio>

#include <atomic>
#include <thread>

#include <emmintrin.h>

void hgTest()
{
    hgLog("Tests Begun\n");

    HgClock timer{};
    hgClockTick(&timer);

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

            u32* allocU32 = hgArenaAlloc<u32>(&arena, 1);
            hgAssert(allocU32 == arena.memory);

            u64* allocU64 = hgArenaAlloc<u64>(&arena, 2);
            hgAssert((u8*)allocU64 == (u8*)allocU32 + 8);

            u8* allocU8 = hgArenaAlloc<u8>(&arena, 1);
            hgAssert(allocU8 == (u8*)allocU32 + 24);

            struct Big {
                u8 data[32];
            };
            Big* allocBig = hgArenaAlloc<Big>(&arena, 1);
            hgAssert((u8*)allocBig == (u8*)allocU32 + 25);

            Big* reallocBig = hgArenaRealloc(&arena, allocBig, 1, 2);
            hgAssert(reallocBig == allocBig);

            Big* reallocBigSame = hgArenaRealloc(&arena, reallocBig, 2, 2);
            hgAssert(reallocBigSame == reallocBig);

            hgMemClear(reallocBig, 2 * sizeof(*reallocBig), 2);
            u8* allocInterrupt = hgArenaAlloc<u8>(&arena, 1);
            (void)allocInterrupt;

            Big* reallocBig2 = hgArenaRealloc(&arena, reallocBig, 2, 4);
            hgAssert(reallocBig2 != reallocBig);
            hgAssert(hgMemEqual(reallocBig, reallocBig2, 2 * sizeof(*reallocBig)));

            arena.head = 0;
        }
    }

    // HgString
    {
        HgArena* arena = hgScratch();
        hgArenaScope(arena);

        HgStringBuilder a = hgStringCopy(arena, "a");
        hgAssert(a[0] == 'a');
        hgAssert(a.length == 1);

        HgStringBuilder abc = hgStringCopy(arena, "abc");
        hgAssert(abc[0] == 'a');
        hgAssert(abc[1] == 'b');
        hgAssert(abc[2] == 'c');
        hgAssert(abc.length == 3);

        hgStringAppend(arena, &a, "bc");
        hgAssert(a == abc);

        HgStringBuilder str{};

        hgStringAppend(arena, &str, "hello");
        hgAssert(str == hgStringCopy(arena, "hello"));

        hgStringAppend(arena, &str, " there");
        hgAssert(str == hgStringCopy(arena, "hello there"));

        hgStringPrepend(arena, &str, "why ");
        hgAssert(str == hgStringCopy(arena, "why hello there"));

        hgStringInsert(arena, &str, 3, ",");
        hgAssert(str == hgStringCopy(arena, "why, hello there"));

        hgStringPrepend(arena, &str, "aaaaaaaaaaaaaaaaaaaaaaaa ");
        hgAssert(str == hgStringCopy(arena, "aaaaaaaaaaaaaaaaaaaaaaaa why, hello there"));
    }

    // string utils
    {
        HgArena* arena = hgScratch();
        hgArenaScope(arena);

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

    // thread pool
    {
        {
            HgFence* fence = hgFenceCreate();
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
            HgFence* fence = hgFenceCreate();
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
            hgThreadsFor(0, hgArrayCount(vals), vals, fn);

            for (bool& val : vals)
            {
                hgAssert(val == true);
            }
        }

        {
            HgFence* fence = hgFenceCreate();
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
                for (u32 j = 0; j < hgArrayCount(producers); ++j)
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
    }

    // HgMutex
    {
        struct Capture {
            HgMutex* mtx;
            u32 count;
        };
        Capture c{};

        c.mtx = hgMutexCreate();
        hgDefer(hgMutexDestroy(c.mtx));

        c.count = 0;
        hgThreadsFor(0, 100, &c, [](void* pc, u64)
        {
            Capture* c = (Capture*)pc;
            hgMutexAcquire(c->mtx);
            hgDefer(hgMutexRelease(c->mtx));
            for (u32 i = 0; i < 10000; ++i)
            {
                ++c->count;
            }
        });
        hgAssert(c.count == 1000000);
    }

    // Serialization
    {
        HgArena* arena = hgScratch();
        hgArenaScope(arena);

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
            HgSerializer writer = hgSerialWriter(arena);
            hgSerialize(&writer, &pod);

            PlainOldData podCopy{};

            HgSerializer reader = hgSerialReader(arena, writer.current);
            hgSerialize(&reader, &podCopy);

            hgAssert(hgMemEqual(&podCopy, &pod, sizeof(pod)));
        }

        {
            HgSerializer writer = hgSerialWriter(arena);
            hgSerialize(&writer, &pod);

            HgBinary bin = hgBinaryWriteSerial(arena, &writer);

            PlainOldData podCopy{};

            HgSerializer reader = hgBinaryReadSerial(arena, bin);
            hgSerialize(&reader, &podCopy);

            hgAssert(hgMemEqual(&podCopy, &pod, sizeof(pod)));
        }

        // {
        //     HgSerializer writer = hgSerialWriter(arena);
        //     hgSerialize(arena, &writer, "data", &pod);
        //
        //     HgStringView json = hgJsonWriteSerial(arena, writer);
        //
        //     PlainOldData podCopy{};
        //
        //     HgSerializer reader = hgJsonReadSerial(arena, json);
        //     hgSerialize(arena, &reader, "data", &podCopy);
        //
        //     hgAssert(hgMemEqual(&podCopy, &pod, sizeof(pod)));
        // }

        struct Data {
            i64 a;
            u16 b;
            f32 c;
            bool d;
            u32 e[3];
            HgStringBuilder f;
        };

        Data data{};
        data.a = -12,
        data.b = 42,
        data.c = 2.5f,
        data.d = true,
        data.e[0] = {2};
        data.e[1] = {4};
        data.e[2] = {6};
        data.f = hgStringCopy(arena, "hello");

        auto serializeData = [](HgSerializer* s, Data* val)
        {
            hgSerializeObject(s,
                &val->a,
                &val->b,
                &val->c,
                &val->d,
                &val->e,
                &val->f);
        };

        {
            HgSerializer writer = hgSerialWriter(arena);
            serializeData(&writer, &data);

            // HgStringView json = hgJsonWriteSerial(arena, &writer);
            // hgDebug("json: %.*s\n", (int)json.length, json.chars);

            Data dataCopy{};

            HgSerializer reader = hgSerialReader(arena, writer.current);
            serializeData(&reader, &dataCopy);

            hgAssert(!hgMemEqual(&dataCopy, &data, sizeof(data)));
            hgAssert(data.a == dataCopy.a);
            hgAssert(data.b == dataCopy.b);
            hgAssert(data.c == dataCopy.c);
            hgAssert(data.d == dataCopy.d);
            hgAssert(data.e[0] == dataCopy.e[0]);
            hgAssert(data.e[1] == dataCopy.e[1]);
            hgAssert(data.e[2] == dataCopy.e[2]);
            hgAssert(data.f == dataCopy.f);
        }

        {
            HgSerializer writer = hgSerialWriter(arena);
            serializeData(&writer, &data);

            HgBinary bin = hgBinaryWriteSerial(arena, &writer);

            Data dataCopy{};

            HgSerializer reader = hgBinaryReadSerial(arena, bin);
            serializeData(&reader, &dataCopy);

            hgAssert(!hgMemEqual(&dataCopy, &data, sizeof(data)));
            hgAssert(data.a == dataCopy.a);
            hgAssert(data.b == dataCopy.b);
            hgAssert(data.c == dataCopy.c);
            hgAssert(data.d == dataCopy.d);
            hgAssert(data.e[0] == dataCopy.e[0]);
            hgAssert(data.e[1] == dataCopy.e[1]);
            hgAssert(data.e[2] == dataCopy.e[2]);
            hgAssert(data.f == dataCopy.f);
        }

//         {
//             HgSerializer writer = hgSerialWriter(arena);
//             serializeData(&writer, &data);
//
//             HgStringView json = hgJsonWriteSerial(arena, &writer);
//
//             hgDebug("json: %.*s\n", (int)json.length, json.chars);
//             hgAssert(json ==
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
//             HgSerializer reader = hgJsonReadSerial(arena, json);
//             serializeData(arena, &reader, "data", &dataCopy);
//
//             hgAssert(!hgMemEqual(&dataCopy, &data, sizeof(data)));
//             hgAssert(data.a == dataCopy.a);
//             hgAssert(data.b == dataCopy.b);
//             hgAssert(data.c == dataCopy.c);
//             hgAssert(data.d == dataCopy.d);
//             hgAssert(data.e[0] == dataCopy.e[0]);
//             hgAssert(data.e[1] == dataCopy.e[1]);
//             hgAssert(data.e[2] == dataCopy.e[2]);
//             hgAssert(data.f == dataCopy.f);
//         }
    }

    // HgJson
    {
        {
            HgArena* arena = hgScratch();
            hgArenaScope(arena);

            HgString file = R"(
            )";

            HgJson json = hgParseJson(arena, file);

            hgAssert(json.errors == nullptr);
            hgAssert(json.file == nullptr);
        }

        {
            HgArena* arena = hgScratch();
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
            hgArenaScope(arena);

            HgString file = R"(
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
    }

    // HgArray
    {
        HgArray<u32> arr = hgArrayCreate<u32>(0, 2);
        hgDefer(hgArrayDestroy(&arr));

        hgAssert(arr.count == 0);
        hgAssert(arr.capacity >= 2);

        *hgArrayPush(&arr) = 10;
        *hgArrayPush(&arr) = 20;

        hgAssert(arr.count == 2);
        hgAssert(arr[0] == 10);
        hgAssert(arr[1] == 20);

        hgArrayResize(&arr, 4);

        hgAssert(arr.count == 4);

        arr[2] = 30;
        arr[3] = 40;

        hgAssert(arr[2] == 30);
        hgAssert(arr[3] == 40);

        u32 popped = hgArrayPop(&arr);

        hgAssert(popped == 40);
        hgAssert(arr.count == 3);

        hgArrayResize(&arr, 1);

        hgAssert(arr.count == 1);
        hgAssert(arr[0] == 10);

        HgArena* arena = hgScratch();
        hgArenaScope(arena);

        HgArray<u32> temp = hgArrayTemp<u32>(arena, 0, 4);

        *hgArrayPushTemp(arena, &temp) = 123;
        *hgArrayPushTemp(arena, &temp) = 456;

        hgAssert(temp.count == 2);
        hgAssert(temp[0] == 123);
        hgAssert(temp[1] == 456);
    }

    // HgArrayAny
    {
        HgArrayAny arr = hgArrayAnyCreate(sizeof(u32), alignof(u32), 0, 2);
        hgDefer(hgArrayAnyDestroy(&arr));

        hgAssert(arr.count == 0);
        hgAssert(arr.capacity >= 2);
        hgAssert(arr.width == sizeof(u32));
        hgAssert(arr.align == alignof(u32));

        *(u32*)hgArrayAnyPush(&arr) = 10;
        *(u32*)hgArrayAnyPush(&arr) = 20;

        hgAssert(arr.count == 2);
        hgAssert(*(u32*)arr[0] == 10);
        hgAssert(*(u32*)arr[1] == 20);

        hgArrayAnyResize(&arr, 4);

        hgAssert(arr.count == 4);

        *(u32*)arr[2] = 30;
        *(u32*)arr[3] = 40;

        hgAssert(*(u32*)arr[2] == 30);
        hgAssert(*(u32*)arr[3] == 40);

        u32 popped = 0;
        hgArrayAnyPop(&arr, &popped);

        hgAssert(popped == 40);
        hgAssert(arr.count == 3);

        hgArrayAnyResize(&arr, 1);

        hgAssert(arr.count == 1);
        hgAssert(*(u32*)arr[0] == 10);

        HgArena* arena = hgScratch();
        hgArenaScope(arena);

        HgArrayAny temp = hgArrayAnyTemp(arena, sizeof(u32), alignof(u32), 0, 2);

        *(u32*)hgArrayAnyPushTemp(arena, &temp) = 123;
        *(u32*)hgArrayAnyPushTemp(arena, &temp) = 456;

        hgAssert(temp.count == 2);
        hgAssert(*(u32*)temp[0] == 123);
        hgAssert(*(u32*)temp[1] == 456);

        hgArrayAnyPushTemp(arena, &temp);

        hgAssert(temp.count == 3);
    }

    // HgQueue
    {
        HgQueue<u32> queue = hgQueueCreate<u32>(4);

        hgAssert(queue.count == 0);
        hgAssert(queue.capacity >= 4);

        hgQueuePushBack(&queue, 1);
        hgQueuePushBack(&queue, 2);
        hgQueuePushBack(&queue, 3);
        hgQueuePushBack(&queue, 4);

        hgAssert(queue.count == 4);

        hgAssert(hgQueuePopFront(&queue) == 1);
        hgAssert(hgQueuePopFront(&queue) == 2);

        hgAssert(queue.count == 2);

        hgQueuePushBack(&queue, 5);
        hgQueuePushBack(&queue, 6);

        hgAssert(queue.count == 4);

        hgQueuePushBack(&queue, 7);
        hgQueuePushBack(&queue, 8);

        hgAssert(queue.count == 6);
        hgAssert(queue.capacity >= 6);

        hgAssert(hgQueuePopFront(&queue) == 3);
        hgAssert(hgQueuePopFront(&queue) == 4);
        hgAssert(hgQueuePopFront(&queue) == 5);
        hgAssert(hgQueuePopFront(&queue) == 6);
        hgAssert(hgQueuePopFront(&queue) == 7);
        hgAssert(hgQueuePopFront(&queue) == 8);

        hgAssert(queue.count == 0);

        hgQueuePushFront(&queue, 10);
        hgQueuePushFront(&queue, 20);
        hgQueuePushFront(&queue, 30);

        hgAssert(queue.count == 3);

        hgAssert(hgQueuePopFront(&queue) == 30);
        hgAssert(hgQueuePopFront(&queue) == 20);
        hgAssert(hgQueuePopFront(&queue) == 10);

        hgAssert(queue.count == 0);

        hgQueuePushBack(&queue, 1);
        hgQueuePushBack(&queue, 2);
        hgQueuePushFront(&queue, 0);
        hgQueuePushFront(&queue, -1);

        hgAssert(queue.count == 4);

        hgAssert(hgQueuePopBack(&queue) == 2);
        hgAssert(hgQueuePopBack(&queue) == 1);
        hgAssert(hgQueuePopBack(&queue) == 0);
        hgAssert(hgQueuePopBack(&queue) == (u32)-1);

        hgAssert(queue.count == 0);

        hgQueueDestroy(&queue);
    }

    // HgSet
    {
        {
            HgArena* arena = hgScratch();
            hgArenaScope(arena);

            constexpr u32 count = 128;

            HgSet<u32> set = hgSetTemp<u32>(arena, count);

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

        {
            HgArena* arena = hgScratch();
            hgArenaScope(arena);

            using StrHash = u64;

            HgSet<StrHash> set = hgSetTemp<StrHash>(arena, 128);

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
            hgArenaScope(arena);

            HgSet<const char*> set = hgSetTemp<const char*>(arena, 128);

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
            hgArenaScope(arena);

            HgSet<HgStringBuilder> set = hgSetTemp<HgStringBuilder>(arena, 128);

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
            hgArenaScope(arena);

            HgSet<HgString> set = hgSetTemp<HgString>(arena, 128);

            hgAssert(!hgSetHas(&set, "a"));
            hgAssert(!hgSetHas(&set, "b"));
            hgAssert(!hgSetHas(&set, "ab"));
            hgAssert(!hgSetHas(&set, "supercalifragilisticexpialidocious"));

            hgSetAdd(&set, "a");
            hgSetAdd(&set, "b");
            hgSetAdd(&set, "ab");
            hgSetAdd(&set, "supercalifragilisticexpialidocious");

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
    }

    // HgMap
    {
        {
            HgArena* arena = hgScratch();
            hgArenaScope(arena);

            constexpr u32 count = 128;

            HgMap<u32, u32> map = hgMapTemp<u32, u32>(arena, count);

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
            hgArenaScope(arena);

            using StrHash = u64;

            HgMap<StrHash, u32> map = hgMapTemp<StrHash, u32>(arena, 128);

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
            hgArenaScope(arena);

            HgMap<const char*, u32> map = hgMapTemp<const char*, u32>(arena, 128);

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
            hgArenaScope(arena);

            HgMap<HgStringBuilder, u32> map = hgMapTemp<HgStringBuilder, u32>(arena, 128);

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
            hgArenaScope(arena);

            HgMap<HgString, u32> map = hgMapTemp<HgString, u32>(arena, 6);

            hgAssert(hgMapGet(&map, "a") == nullptr);
            hgAssert(hgMapGet(&map, "b") == nullptr);
            hgAssert(hgMapGet(&map, "ab") == nullptr);
            hgAssert(hgMapGet(&map, "supercalifragilisticexpialidocious") == nullptr);

            hgMapAdd(&map, "a", 1);
            hgMapAdd(&map, "b", 2);
            hgMapAdd(&map, "ab", 3);
            hgMapAdd(&map, "supercalifragilisticexpialidocious", 4);

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
    }

    // HgPool
    {
        HgPool pool = hgPoolCreate<u32>();
        hgDefer(hgPoolDestroy(&pool));

        u32* a = (u32*)hgPoolAlloc(&pool);
        u32* b = (u32*)hgPoolAlloc(&pool);
        u32* c = (u32*)hgPoolAlloc(&pool);

        hgAssert(a != nullptr);
        hgAssert(b != nullptr);
        hgAssert(c != nullptr);

        *a = 1;
        *b = 2;
        *c = 3;

        hgAssert(*a == 1);
        hgAssert(*b == 2);
        hgAssert(*c == 3);

        hgPoolFree(&pool, b);
        hgPoolFree(&pool, c);

        u32* d = (u32*)hgPoolAlloc(&pool);
        u32* e = (u32*)hgPoolAlloc(&pool);

        hgAssert(d == c);
        hgAssert(e == b);

        *d = 40;
        *e = 50;

        hgAssert(*d == 40);
        hgAssert(*e == 50);

        constexpr u32 n = 1500;

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        HgArray<u32*> ptrs = hgArrayTemp<u32*>(scratch, 0, n);

        for (u32 i = 0; i < n; ++i)
        {
            u32* p = (u32*)hgPoolAlloc(&pool);
            *hgArrayPushTemp(scratch, &ptrs) = p;
            hgAssert(p != nullptr);
        }

        hgAssert(pool.itemStores.count >= 2);

        for (u32 i = 0; i < ptrs.count; ++i)
        {
            for (u32 j = i + 1; j < ptrs.count; ++j)
            {
                hgAssert(ptrs[i] != ptrs[j]);
            }
        }

        for (u32 i = 0; i < ptrs.count; ++i)
        {
            hgPoolFree(&pool, ptrs[i]);
        }
    }

    // HgHandlePool
    {
        HgHandlePool pool = hgHandlePoolCreate();
        hgDefer(hgHandlePoolDestroy(&pool));

        HgHandle u1 = hgHandlePoolAlloc(&pool);
        hgAssert(hgHandlePoolAlive(&pool, u1));
        hgAssert(u1.id == 1);

        HgHandle u2 = hgHandlePoolAlloc(&pool);
        hgAssert(hgHandlePoolAlive(&pool, u2));
        hgAssert(u2.id == 2);

        hgHandlePoolFree(&pool, u1);
        hgAssert(!hgHandlePoolAlive(&pool, u1));

        HgHandle u12 = hgHandlePoolAlloc(&pool);
        hgAssert(hgHandlePoolAlive(&pool, u12));
        hgAssert(!hgHandlePoolAlive(&pool, u1));
        hgAssert(u12.id != 1);
        hgAssert(hgHandleIdx(u12) == 1);

        hgHandlePoolReset(&pool);
        hgAssert(!hgHandlePoolAlive(&pool, u1));
        hgAssert(!hgHandlePoolAlive(&pool, u2));
        hgAssert(!hgHandlePoolAlive(&pool, u12));
    }

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
        hgAssert(identity * mat == mat);
        hgAssert(identity * vec == vec);

        HgMat2 matRotated{
            {0.0f, 1.0f},
            {0.0f, 1.0f},
        };
        HgVec2 vecRotated{-1.0f, 1.0f};

        HgMat2 rotation{
            {0.0f, 1.0f},
            {-1.0f, 0.0f},
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
        HgQuat rotation = hgQuatAxisAngle({0.0f, 0.0f, -1.0f}, -(f32)hgPi * 0.5f);

        HgVec3 rotatedVec = hgVecRot3(rotation, upVec);
        HgMat3 rotatedMat = hgMatRot3(rotation, identityMat);

        HgVec3 matRotatedVec = rotatedMat * upVec;

        hgAssert(abs(rotatedVec.x - 1.0f) < FLT_EPSILON
              && abs(rotatedVec.y - 0.0f) < FLT_EPSILON
              && abs(rotatedVec.y - 0.0f) < FLT_EPSILON);

        hgAssert(abs(matRotatedVec.x - rotatedVec.x) < FLT_EPSILON
              && abs(matRotatedVec.y - rotatedVec.y) < FLT_EPSILON
              && abs(matRotatedVec.y - rotatedVec.z) < FLT_EPSILON);
    }

    // HgCircle
    {
        {
            HgCircle circle{{0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointCircle({0.0f, 0.0f}, circle));
        }

        {
            HgCircle circle{{0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointCircle({3.0f, 4.0f}, circle));
        }

        {
            HgCircle circle{{0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointCircle({5.0f, 0.0f}, circle));
        }

        {
            HgCircle circle{{0.0f, 0.0f}, 5.0f};
            hgAssert(!hgContainsPointCircle({5.01f, 0.0f}, circle));
        }

        {
            HgCircle circle{{2.0f, -3.0f}, 2.0f};
            hgAssert(hgContainsPointCircle({2.0f, -3.0f}, circle));
            hgAssert(hgContainsPointCircle({4.0f, -3.0f}, circle));
            hgAssert(!hgContainsPointCircle({4.1f, -3.0f}, circle));
        }

        {
            HgCircle circle{{0.0f, 0.0f}, 0.0f};
            hgAssert(hgContainsPointCircle({0.0f, 0.0f}, circle));
            hgAssert(!hgContainsPointCircle({0.01f, 0.0f}, circle));
        }
    }

    // HgRect
    {
        {
            HgRect rect = hgRectEmpty();
            hgAssert(!hgContainsPointRect({0.0f, 0.0f}, rect));
            hgAssert(!hgContainsPointRect({1.0f, 1.0f}, rect));
        }

        {
            HgRect rect = hgRectEmpty();
            rect = hgRectAddPoint(rect, {2.0f, 3.0f});
            hgAssert(hgContainsPointRect({2.0f, 3.0f}, rect));
        }

        {
            HgRect rect = hgRectEmpty();
            rect = hgRectAddPoint(rect, {1.0f, 2.0f});
            hgAssert(hgContainsPointRect({1.0f, 2.0f}, rect));
        }

        {
            HgRect rect = hgRectEmpty();
            rect = hgRectAddPoint(rect, {2.0f, 2.0f});
            rect = hgRectAddPoint(rect, {5.0f, 7.0f});

            hgAssert(hgContainsPointRect({2.0f, 2.0f}, rect));
            hgAssert(hgContainsPointRect({5.0f, 7.0f}, rect));
            hgAssert(hgContainsPointRect({3.0f, 4.0f}, rect));
        }

        {
            HgRect rect = hgRectEmpty();
            rect = hgRectAddPoint(rect, {5.0f, 5.0f});
            rect = hgRectAddPoint(rect, {-2.0f, -3.0f});

            hgAssert(hgContainsPointRect({-2.0f, -3.0f}, rect));
            hgAssert(hgContainsPointRect({5.0f, 5.0f}, rect));
            hgAssert(hgContainsPointRect({0.0f, 0.0f}, rect));
        }

        {
            HgRect rect = hgRectEmpty();
            rect = hgRectAddPoint(rect, {1.0f, 1.0f});
            rect = hgRectAddPoint(rect, {1.0f, 1.0f});

            hgAssert(hgContainsPointRect({1.0f, 1.0f}, rect));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            hgAssert(hgContainsPointRect({5.0f, 2.5f}, rect));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            hgAssert(hgContainsPointRect({0.0f, 0.0f}, rect));
            hgAssert(hgContainsPointRect({10.0f, 5.0f}, rect));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 5.0f}};
            hgAssert(!hgContainsPointRect({-0.01f, 0.0f}, rect));
            hgAssert(!hgContainsPointRect({10.01f, 5.0f}, rect));
            hgAssert(!hgContainsPointRect({5.0f, 5.01f}, rect));
        }

        {
            HgRect rect{{-5.0f, -3.0f}, {2.0f, 8.0f}};
            hgAssert(hgContainsPointRect({-4.0f, 0.0f}, rect));
            hgAssert(!hgContainsPointRect({-2.9f, 0.0f}, rect));
        }

        {
            HgRect rect{{2.0f, 2.0f}, {0.0f, 0.0f}};
            hgAssert(hgContainsPointRect({2.0f, 2.0f}, rect));
            hgAssert(!hgContainsPointRect({2.01f, 2.0f}, rect));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgVec2 p = hgClosestPointRect({-5.0f, 5.0f}, rect);

            hgAssert(p.x == 0.0f);
            hgAssert(p.y == 5.0f);
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgVec2 p = hgClosestPointRect({15.0f, 5.0f}, rect);

            hgAssert(p.x == 10.0f);
            hgAssert(p.y == 5.0f);
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgVec2 p = hgClosestPointRect({5.0f, -3.0f}, rect);

            hgAssert(p.x == 5.0f);
            hgAssert(p.y == 0.0f);
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgVec2 p = hgClosestPointRect({-3.0f, 15.0f}, rect);

            hgAssert(p.x == 0.0f);
            hgAssert(p.y == 10.0f);
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgVec2 p = hgClosestPointRect({5.0f, 5.0f}, rect);

            hgAssert(p.x == 5.0f);
            hgAssert(p.y == 5.0f);
        }

        {
            HgRect a{{0.0f, 0.0f}, {5.0f, 5.0}};
            HgRect b{{3.0f, 3.0f}, {5.0f, 5.0f}};

            hgAssert(hgIntersectRects(a, b));
            hgAssert(hgIntersectRects(b, a));
        }

        {
            HgRect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            HgRect b{{5.0f, 0.0f}, {2.0f, 2.0f}};

            hgAssert(hgIntersectRects(a, b));
        }

        {
            HgRect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            HgRect b{{5.0f, 5.0f}, {2.0f, 2.0f}};

            hgAssert(hgIntersectRects(a, b));
        }

        {
            HgRect a{{0.0f, 0.0f}, {5.0f, 5.0f}};
            HgRect b{{5.01f, 0.0f}, {2.0f, 2.0f}};

            hgAssert(!hgIntersectRects(a, b));
        }

        {
            HgRect a{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgRect b{{2.0f, 2.0f}, {2.0f, 2.0f}};

            hgAssert(hgIntersectRects(a, b));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgCircle circle{{5.0f, 5.0f}, 2.0f};

            hgAssert(hgIntersectRectCircle(rect, circle));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgCircle circle{{12.0f, 5.0f}, 2.0f};

            hgAssert(hgIntersectRectCircle(rect, circle));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgCircle circle{{13.0f, 5.0f}, 2.0f};

            hgAssert(!hgIntersectRectCircle(rect, circle));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgCircle circle{{12.0f, 12.0f}, std::sqrt(8.0f) + FLT_EPSILON};

            hgAssert(hgIntersectRectCircle(rect, circle));
        }

        {
            HgRect rect{{0.0f, 0.0f}, {10.0f, 10.0f}};
            HgCircle circle{{13.0f, 13.0f}, 2.0f};

            hgAssert(!hgIntersectRectCircle(rect, circle));
        }
    }

    // HgRay2D
    {
        {
            HgRay2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgRay2D b{{5.0f, -5.0f}, {0.0f, 1.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRays2D(a, b, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgRay2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgRay2D b{{5.0f, 5.0f}, {0.0f, 1.0f}};

            hgAssert(!hgIntersectRays2D(a, b, nullptr));
        }

        {
            HgRay2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgRay2D b{{-5.0f, -5.0f}, {0.0f, 1.0f}};

            hgAssert(!hgIntersectRays2D(a, b, nullptr));
        }

        {
            HgRay2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgRay2D b{{0.0f, 1.0f}, {1.0f, 0.0f}};

            hgAssert(!hgIntersectRays2D(a, b, nullptr));
        }

        {
            HgRay2D a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgRay2D b{{0.0f, 0.0f}, {0.0f, 1.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRays2D(a, b, &hit));
            hgAssert(std::abs(hit.dist) <= FLT_EPSILON);
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgLine2D line{{5.0f, -2.0f}, {5.0f, 2.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayLine2D(ray, line, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgLine2D line{{-5.0f, -2.0f}, {-5.0f, 2.0f}};

            hgAssert(!hgIntersectRayLine2D(ray, line, nullptr));
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgLine2D line{{5.0f, 1.0f}, {8.0f, 1.0f}};

            hgAssert(!hgIntersectRayLine2D(ray, line, nullptr));
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgLine2D line{{5.0f, 0.0f}, {5.0f, 5.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayLine2D(ray, line, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            HgRay2D ray{{5.0f, 0.0f}, {1.0f, 0.0f}};
            HgLine2D line{{5.0f, -5.0f}, {5.0f, 5.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayLine2D(ray, line, &hit));
            hgAssert(std::abs(hit.dist) <= FLT_EPSILON);
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectRayCircle(ray, circle, &hit));
            hgAssert(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{0.0f, 2.0f}, {1.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectRayCircle(ray, circle, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            HgRay2D ray{{0.0f, 3.0f}, {1.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectRayCircle(ray, circle, nullptr));
        }

        {
            HgRay2D ray{{10.0f, 0.0f}, {1.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectRayCircle(ray, circle, &hit));
            hgAssert(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{20.0f, 0.0f}, {1.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectRayCircle(ray, circle, nullptr));
        }

        {
            HgRay2D ray{{0.0f, 5.0f}, {1.0f, 0.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{20.0f, 5.0f}, {-1.0f, 0.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{12.5f, -5.0f}, {0.0f, 1.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {0.0f, -1.0f}));
        }

        {
            HgRay2D ray{{12.5f, 20.0f}, {0.0f, -1.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {0.0f, 1.0f}));
        }

        {
            HgRay2D ray{{0.0f, 0.0f}, {1.0f, 1.0f}};
            HgRect rect{{10.0f, 10.0f}, {5.0f, 5.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            HgRay2D ray{{0.0f, 5.0f}, {-1.0f, 0.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            hgAssert(!hgIntersectRayRect(ray, rect, nullptr));
        }

        {
            HgRay2D ray{{12.5f, 5.0f}, {1.0f, 0.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectRayRect(ray, rect, &hit));
            hgAssert(hit.dist == 0.0f);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgRay2D ray{{0.0f, 20.0f}, {1.0f, 0.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            hgAssert(!hgIntersectRayRect(ray, rect, nullptr));
        }

    }

    // HgLine2D
    {
        {
            HgLine2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgLine2D b{{5.0f, -5.0f}, {5.0f, 5.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLines2D(a, b, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgLine2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgLine2D b{{15.0f, -5.0f}, {15.0f, 5.0f}};

            hgAssert(!hgIntersectLines2D(a, b, nullptr));
        }

        {
            HgLine2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgLine2D b{{10.0f, 0.0f}, {10.0f, 5.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLines2D(a, b, &hit));
            hgAssert(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            HgLine2D a{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgLine2D b{{0.0f, 1.0f}, {10.0f, 1.0f}};

            hgAssert(!hgIntersectLines2D(a, b, nullptr));
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgRay2D ray{{5.0f, -5.0f}, {0.0f, 1.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRay2D(line, ray, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgRay2D ray{{15.0f, -5.0f}, {0.0f, 1.0f}};

            hgAssert(!hgIntersectLineRay2D(line, ray, nullptr));
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgRay2D ray{{10.0f, -5.0f}, {0.0f, 1.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRay2D(line, ray, &hit));
            hgAssert(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {10.0f, 0.0f}};
            HgRay2D ray{{5.0f, 5.0f}, {0.0f, 1.0f}};

            hgAssert(!hgIntersectLineRay2D(line, ray, nullptr));
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {20.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectLineCircle(line, circle, &hit));
            hgAssert(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgLine2D line{{0.0f, 2.0f}, {20.0f, 2.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectLineCircle(line, circle, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            HgLine2D line{{0.0f, 3.0f}, {20.0f, 3.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectLineCircle(line, circle, nullptr));
        }

        {
            HgLine2D line{{0.0f, 0.0f}, {5.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectLineCircle(line, circle, nullptr));
        }

        {
            HgLine2D line{{10.0f, 0.0f}, {20.0f, 0.0f}};
            HgCircle circle{{10.0f, 0.0f}, 2.0f};

            HgHit2D hit;
            hgAssert(hgIntersectLineCircle(line, circle, &hit));
            hgAssert(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            HgLine2D line{{0.0f, 5.0f}, {20.0f, 5.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRect(line, rect, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {-1.0f, 0.0f}));
        }

        {
            HgLine2D line{{20.0f, 5.0f}, {0.0f, 5.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRect(line, rect, &hit));
            hgAssert(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {1.0f, 0.0f}));
        }

        {
            HgLine2D line{{12.5f, -5.0f}, {12.5f, 15.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRect(line, rect, &hit));
            hgAssert(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {0.0f, -1.0f}));
        }

        {
            HgLine2D line{{0.0f, 20.0f}, {20.0f, 20.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            hgAssert(!hgIntersectLineRect(line, rect, nullptr));
        }

        {
            HgLine2D line{{12.5f, 5.0f}, {17.5f, 5.0f}};
            HgRect rect{{10.0f, 0.0f}, {5.0f, 10.0f}};

            HgHit2D hit;
            hgAssert(hgIntersectLineRect(line, rect, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq2(hit.normal, {1.0f, 0.0f}));
        }
    }

    // HgSphere
    {
        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointSphere({3.0f, 4.0f, 0.0f}, sphere));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(hgContainsPointSphere({5.0f, 0.0f, 0.0f}, sphere));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(!hgContainsPointSphere({5.01f, 0.0f, 0.0f}, sphere));
        }

        {
            HgSphere sphere{{2.0f, -3.0f, 4.0f}, 2.0f};
            hgAssert(hgContainsPointSphere({2.0f, -3.0f, 4.0f}, sphere));
            hgAssert(hgContainsPointSphere({4.0f, -3.0f, 4.0f}, sphere));
            hgAssert(!hgContainsPointSphere({4.1f, -3.0f, 4.0f}, sphere));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
            hgAssert(hgContainsPointSphere({0.0f, 0.0f, 0.0f}, sphere));
            hgAssert(!hgContainsPointSphere({0.01f, 0.0f, 0.0f}, sphere));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(std::abs(hgDistPointSphere({10.0f, 0.0f, 0.0f}, sphere) - 5.0f) <= FLT_EPSILON);
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(std::abs(hgDistPointSphere({5.0f, 0.0f, 0.0f}, sphere)) <= FLT_EPSILON);
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            hgAssert(std::abs(hgDistPointSphere({0.0f, 0.0f, 0.0f}, sphere) + 5.0f) <= FLT_EPSILON);
        }

        {
            HgSphere sphere{{2.0f, 3.0f, 4.0f}, 2.0f};
            hgAssert(std::abs(hgDistPointSphere({6.0f, 3.0f, 4.0f}, sphere) - 2.0f) <= FLT_EPSILON);
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgVec3 p = hgClosestPointSphere({10.0f, 0.0f, 0.0f}, sphere);

            hgAssert(hgVecEq3(p, {5.0f, 0.0f, 0.0f}));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgVec3 p = hgClosestPointSphere({0.0f, 10.0f, 0.0f}, sphere);

            hgAssert(hgVecEq3(p, {0.0f, 5.0f, 0.0f}));
        }

        {
            HgSphere sphere{{2.0f, 1.0f, -3.0f}, 3.0f};
            HgVec3 p = hgClosestPointSphere({5.0f, 1.0f, -3.0f}, sphere);

            hgAssert(hgVecEq3(p, {5.0f, 1.0f, -3.0f}));
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgVec3 p = hgClosestPointSphere({0.0f, 0.0f, 0.0f}, sphere);

            hgAssert(hgDistPointSphere(p, sphere) <= FLT_EPSILON);
        }

        {
            HgSphere sphere{{0.0f, 0.0f, 0.0f}, 0.0f};
            HgVec3 p = hgClosestPointSphere({10.0f, 2.0f, -5.0f}, sphere);

            hgAssert(hgVecEq3(p, {0.0f, 0.0f, 0.0f}));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{8.0f, 0.0f, 0.0f}, 5.0f};

            hgAssert(hgIntersectSpheres(a, b));
            hgAssert(hgIntersectSpheres(b, a));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{10.0f, 0.0f, 0.0f}, 5.0f};

            hgAssert(hgIntersectSpheres(a, b));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{10.1f, 0.0f, 0.0f}, 5.0f};

            hgAssert(!hgIntersectSpheres(a, b));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{0.0f, 0.0f, 0.0f}, 2.0f};

            hgAssert(hgIntersectSpheres(a, b));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 0.0f};
            HgSphere b{{0.0f, 0.0f, 0.0f}, 0.0f};

            hgAssert(hgIntersectSpheres(a, b));
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{20.0f, 0.0f, 0.0f}, 5.0f};

            hgAssert(std::abs(hgDistSpheres(a, b) - 10.0f) <= FLT_EPSILON);
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{10.0f, 0.0f, 0.0f}, 5.0f};

            hgAssert(std::abs(hgDistSpheres(a, b)) <= FLT_EPSILON);
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{5.0f, 0.0f, 0.0f}, 5.0f};

            hgAssert(hgDistSpheres(a, b) < 0.0f);
        }

        {
            HgSphere a{{0.0f, 0.0f, 0.0f}, 5.0f};
            HgSphere b{{0.0f, 0.0f, 0.0f}, 1.0f};

            hgAssert(hgDistSpheres(a, b) < 0.0f);
        }
    }

    // HgBox
    {
        {
            HgBox box = hgBoxEmpty();

            hgAssert(!hgContainsPointBox({0.0f, 0.0f, 0.0f}, box));
            hgAssert(!hgContainsPointBox({1.0f, 1.0f, 1.0f}, box));
        }

        {
            HgBox box = hgBoxEmpty();
            box = hgBoxAddPoint(box, {2.0f, 3.0f, 4.0f});

            hgAssert(hgContainsPointBox({2.0f, 3.0f, 4.0f}, box));
        }

        {
            HgBox box = hgBoxEmpty();
            box = hgBoxAddPoint(box, {1.0f, 2.0f, 3.0f});

            hgAssert(hgContainsPointBox({1.0f, 2.0f, 3.0f}, box));
        }

        {
            HgBox box = hgBoxEmpty();
            box = hgBoxAddPoint(box, {2.0f, 2.0f, 2.0f});
            box = hgBoxAddPoint(box, {5.0f, 7.0f, 11.0f});

            hgAssert(hgContainsPointBox({2.0f, 2.0f, 2.0f}, box));
            hgAssert(hgContainsPointBox({5.0f, 7.0f, 11.0f}, box));
            hgAssert(hgContainsPointBox({3.0f, 4.0f, 5.0f}, box));
        }

        {
            HgBox box = hgBoxEmpty();
            box = hgBoxAddPoint(box, {5.0f, 5.0f, 5.0f});
            box = hgBoxAddPoint(box, {-2.0f, -3.0f, -4.0f});

            hgAssert(hgContainsPointBox({-2.0f, -3.0f, -4.0f}, box));
            hgAssert(hgContainsPointBox({5.0f, 5.0f, 5.0f}, box));
            hgAssert(hgContainsPointBox({0.0f, 0.0f, 0.0f}, box));
        }

        {
            HgBox box = hgBoxEmpty();
            box = hgBoxAddPoint(box, {1.0f, 1.0f, 1.0f});
            box = hgBoxAddPoint(box, {1.0f, 1.0f, 1.0f});

            hgAssert(hgContainsPointBox({1.0f, 1.0f, 1.0f}, box));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            hgAssert(hgContainsPointBox({5.0f, 2.5f, 4.0f}, box));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            hgAssert(hgContainsPointBox({0.0f, 0.0f, 0.0f}, box));
            hgAssert(hgContainsPointBox({10.0f, 5.0f, 8.0f}, box));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 5.0f, 8.0f}};
            hgAssert(!hgContainsPointBox({-0.01f, 0.0f, 0.0f}, box));
            hgAssert(!hgContainsPointBox({10.01f, 5.0f, 8.0f}, box));
            hgAssert(!hgContainsPointBox({5.0f, 5.01f, 4.0f}, box));
            hgAssert(!hgContainsPointBox({5.0f, 2.5f, 8.01f}, box));
        }

        {
            HgBox box{{-5.0f, -3.0f, -2.0f}, {2.0f, 8.0f, 6.0f}};
            hgAssert(hgContainsPointBox({-4.0f, 0.0f, 0.0f}, box));
            hgAssert(!hgContainsPointBox({-2.9f, 0.0f, 0.0f}, box));
        }

        {
            HgBox box{{2.0f, 2.0f, 2.0f}, {0.0f, 0.0f, 0.0f}};
            hgAssert(hgContainsPointBox({2.0f, 2.0f, 2.0f}, box));
            hgAssert(!hgContainsPointBox({2.01f, 2.0f, 2.0f}, box));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({-5.0f, 5.0f, 5.0f}, box);

            hgAssert(hgVecEq3(p, {0.0f, 5.0f, 5.0f}));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({15.0f, 5.0f, 5.0f}, box);

            hgAssert(hgVecEq3(p, {10.0f, 5.0f, 5.0f}));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({5.0f, -3.0f, 5.0f}, box);

            hgAssert(hgVecEq3(p, {5.0f, 0.0f, 5.0f}));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({5.0f, 5.0f, 15.0f}, box);

            hgAssert(hgVecEq3(p, {5.0f, 5.0f, 10.0f}));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({-5.0f, 15.0f, -3.0f}, box);

            hgAssert(hgVecEq3(p, {0.0f, 10.0f, 0.0f}));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgVec3 p = hgClosestPointBox({5.0f, 5.0f, 5.0f}, box);

            hgAssert(hgVecEq3(p, {5.0f, 5.0f, 5.0f}));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f}};

            hgAssert(hgIntersectBox(a, b));
            hgAssert(hgIntersectBox(b, a));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{5.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(hgIntersectBox(a, b));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{0.0f, 5.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(hgIntersectBox(a, b));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{0.0f, 0.0f, 5.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(hgIntersectBox(a, b));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{5.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(hgIntersectBox(a, b));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f}};
            HgBox b{{5.01f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(!hgIntersectBox(a, b));
        }

        {
            HgBox a{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgBox b{{2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}};

            hgAssert(hgIntersectBox(a, b));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{5.0f, 5.0f, 5.0f}, 2.0f};

            hgAssert(hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{12.0f, 5.0f, 5.0f}, 2.0f};

            hgAssert(hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{5.0f, 12.0f, 5.0f}, 2.0f};

            hgAssert(hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{5.0f, 5.0f, 12.0f}, 2.0f};

            hgAssert(hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{13.0f, 5.0f, 5.0f}, 2.0f};

            hgAssert(!hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{12.0f, 12.0f, 12.0f}, std::sqrt(12.0f)};

            hgAssert(hgIntersectBoxSphere(box, sphere));
        }

        {
            HgBox box{{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
            HgSphere sphere{{13.0f, 13.0f, 13.0f}, 2.0f};

            hgAssert(!hgIntersectBoxSphere(box, sphere));
        }
    }

    // HgPlane
    {
        {
            HgPlane plane = hgPlaneFromPoint({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
            hgAssert(hgVecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
            hgAssert(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
            hgAssert(hgVecEq3(plane.normal, {0.0f, 1.0f, 0.0f}));
            hgAssert(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            HgPlane plane = hgPlaneFromPoint({3.0f, 2.0f, -1.0f}, {1.0f, 0.0f, 0.0f});
            hgAssert(hgVecEq3(plane.normal, {1.0f, 0.0f, 0.0f}));
            hgAssert(std::abs(plane.dist - 3.0f) <= FLT_EPSILON);
        }

        {
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
            HgPlane plane = hgPlaneFromTri(tri);

            hgAssert(hgVecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
            hgAssert(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            HgTri tri{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgPlane plane = hgPlaneFromTri(tri);

            hgAssert(hgVecEq3(plane.normal, {0.0f, 0.0f, -1.0f}));
            hgAssert(std::abs(plane.dist) <= FLT_EPSILON);
        }

        {
            HgTri tri{{1.0f, 2.0f, 5.0f}, {4.0f, 2.0f, 5.0f}, {1.0f, 6.0f, 5.0f}};
            HgPlane plane = hgPlaneFromTri(tri);

            hgAssert(hgVecEq3(plane.normal, {0.0f, 0.0f, 1.0f}));
            hgAssert(std::abs(plane.dist - 5.0f) <= FLT_EPSILON);
        }
    }

    // HgRay3D
    {
        {
            HgRay3D ray{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectRaySphere(ray, sphere, &hit));
            hgAssert(std::abs(hit.dist - 8.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            HgRay3D ray{{0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectRaySphere(ray, sphere, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
        }

        {
            HgRay3D ray{{0.0f, 3.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectRaySphere(ray, sphere, nullptr));
        }

        {
            HgRay3D ray{{10.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectRaySphere(ray, sphere, &hit));
            hgAssert(std::abs(hit.dist - 2.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            HgRay3D ray{{20.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectRaySphere(ray, sphere, nullptr));
        }

        {
            HgRay3D ray{{0.0f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayBox(ray, box, &hit));
            hgAssert(std::abs(hit.dist - 10.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            HgRay3D ray{{20.0f, 5.0f, 5.0f}, {-1.0f, 0.0f, 0.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayBox(ray, box, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            HgRay3D ray{{12.5f, -5.0f, 5.0f}, {0.0f, 1.0f, 0.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayBox(ray, box, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            HgRay3D ray{{12.5f, 5.0f, -5.0f}, {0.0f, 0.0f, 1.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayBox(ray, box, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            HgRay3D ray{{0.0f, 20.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            hgAssert(!hgIntersectRayBox(ray, box, nullptr));
        }

        {
            HgRay3D ray{{12.5f, 5.0f, 5.0f}, {1.0f, 0.0f, 0.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayBox(ray, box, &hit));
            hgAssert(std::abs(hit.dist) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            HgRay3D ray{{0.25f, 0.25f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayTri(ray, tri, &hit));
            hgAssert(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            HgRay3D ray{{0.5f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayTri(ray, tri, &hit));
            hgAssert(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            HgRay3D ray{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectRayTri(ray, tri, &hit));
            hgAssert(std::abs(hit.dist - 1.0f) <= FLT_EPSILON);
        }

        {
            HgRay3D ray{{0.75f, 0.75f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            hgAssert(!hgIntersectRayTri(ray, tri, nullptr));
        }

        {
            HgRay3D ray{{0.25f, 0.25f, 1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            hgAssert(!hgIntersectRayTri(ray, tri, nullptr));
        }

        {
            HgRay3D ray{{0.25f, 0.25f, -1.0f}, {1.0f, 0.0f, 0.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            hgAssert(!hgIntersectRayTri(ray, tri, nullptr));
        }

        {
            HgRay3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectRayPlane(ray, plane, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
        }

        {
            HgRay3D ray{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectRayPlane(ray, plane, &hit));
            hgAssert(std::abs(hit.dist - 5.0f) <= FLT_EPSILON);
        }

        {
            HgRay3D ray{{0.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            hgAssert(!hgIntersectRayPlane(ray, plane, nullptr));
        }

        {
            HgRay3D ray{{0.0f, 2.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            hgAssert(!hgIntersectRayPlane(ray, plane, nullptr));
        }

        {
            HgRay3D ray{{1.0f, 5.0f, 2.0f}, {0.0f, 1.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectRayPlane(ray, plane, &hit));
            hgAssert(std::abs(hit.dist) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            HgRay3D ray{{0.0f, 10.0f, 0.0f}, {0.0f, -2.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectRayPlane(ray, plane, &hit));
            hgAssert(std::abs(hit.dist - 2.5f) <= FLT_EPSILON);
        }
    }

    // HgLine3D
    {
        {
            HgLine3D line{{0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectLineSphere(line, sphere, &hit));
            hgAssert(std::abs(hit.dist - 0.4f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            HgLine3D line{{0.0f, 2.0f, 0.0f}, {20.0f, 2.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectLineSphere(line, sphere, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            HgLine3D line{{0.0f, 3.0f, 0.0f}, {20.0f, 3.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectLineSphere(line, sphere, nullptr));
        }

        {
            HgLine3D line{{0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            hgAssert(!hgIntersectLineSphere(line, sphere, nullptr));
        }

        {
            HgLine3D line{{10.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
            HgSphere sphere{{10.0f, 0.0f, 0.0f}, 2.0f};

            HgHit3D hit;
            hgAssert(hgIntersectLineSphere(line, sphere, &hit));
            hgAssert(std::abs(hit.dist - 0.2f) <= FLT_EPSILON);
        }

        {
            HgLine3D line{{0.0f, 5.0f, 5.0f}, {20.0f, 5.0f, 5.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineBox(line, box, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {-1.0f, 0.0f, 0.0f}));
        }

        {
            HgLine3D line{{20.0f, 5.0f, 5.0f}, {0.0f, 5.0f, 5.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineBox(line, box, &hit));
            hgAssert(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            HgLine3D line{{12.5f, -5.0f, 5.0f}, {12.5f, 15.0f, 5.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineBox(line, box, &hit));
            hgAssert(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            HgLine3D line{{12.5f, 5.0f, -5.0f}, {12.5f, 5.0f, 15.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineBox(line, box, &hit));
            hgAssert(std::abs(hit.dist - 0.25f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            HgLine3D line{{0.0f, 20.0f, 5.0f}, {20.0f, 20.0f, 5.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            hgAssert(!hgIntersectLineBox(line, box, nullptr));
        }

        {
            HgLine3D line{{12.5f, 5.0f, 5.0f}, {17.5f, 5.0f, 5.0f}};
            HgBox box{{10.0f, 0.0f, 0.0f}, {5.0f, 10.0f, 10.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineBox(line, box, &hit));
            hgAssert(std::abs(hit.dist - 0.5) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {1.0f, 0.0f, 0.0f}));
        }

        {
            HgLine3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineTri(line, tri, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 0.0f, -1.0f}));
        }

        {
            HgLine3D line{{0.5f, 0.0f, -1.0f}, {0.5f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineTri(line, tri, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            HgLine3D line{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineTri(line, tri, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
        }

        {
            HgLine3D line{{0.75f, 0.75f, -1.0f}, {0.75f, 0.75f, 1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            hgAssert(!hgIntersectLineTri(line, tri, nullptr));
        }

        {
            HgLine3D line{{0.25f, 0.25f, -1.0f}, {0.25f, 0.25f, -0.5f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            hgAssert(!hgIntersectLineTri(line, tri, nullptr));
        }

        {
            HgLine3D line{{0.25f, 0.25f, 1.0f}, {0.25f, 0.25f, -1.0f}};
            HgTri tri{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

            HgHit3D hit;
            hgAssert(hgIntersectLineTri(line, tri, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 0.0f, 1.0f}));
        }

        {
            HgLine3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectLinePlane(line, plane, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, 1.0f, 0.0f}));
        }

        {
            HgLine3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 10.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectLinePlane(line, plane, &hit));
            hgAssert(std::abs(hit.dist - 0.5f) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            HgLine3D line{{0.0f, 10.0f, 0.0f}, {10.0f, 10.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            hgAssert(!hgIntersectLinePlane(line, plane, nullptr));
        }

        {
            HgLine3D line{{0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            hgAssert(!hgIntersectLinePlane(line, plane, nullptr));
        }

        {
            HgLine3D line{{1.0f, 5.0f, 2.0f}, {1.0f, 10.0f, 2.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            HgHit3D hit;
            hgAssert(hgIntersectLinePlane(line, plane, &hit));
            hgAssert(std::abs(hit.dist) <= FLT_EPSILON);
            hgAssert(hgVecEq3(hit.normal, {0.0f, -1.0f, 0.0f}));
        }

        {
            HgLine3D line{{0.0f, 10.0f, 0.0f}, {0.0f, 20.0f, 0.0f}};
            HgPlane plane = hgPlaneFromPoint({0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

            hgAssert(!hgIntersectLinePlane(line, plane, nullptr));
        }
    }

    // HgAssetManager and HgBinary
    {
        {
            HgBinaryAsset* bin1 = hgAssetCreate<HgBinary>();
            hgAssert(bin1 != nullptr);
            hgAssert(bin1->path == "");

            HgBinaryAsset* bin2 = hgAssetCreate<HgBinary>();
            hgAssert(bin2 != nullptr);
            hgAssert(bin2->path == "");
            hgAssert(bin2 != bin1);

            hgAssetUnload(bin1);
            hgAssetUnload(bin2);
        }

        {
            HgBinaryAsset* bin = hgAssetLoad<HgBinary>("file_does_not_exist.bin");
            hgAssert(bin->data.data == nullptr);
            hgAssert(bin->data.size == 0);
            hgAssetUnload(bin);
        }

        u32 saveData[]{12, 42, 100, 128};

        {
            HgBinary bin{saveData, sizeof(saveData)};

            hgBinaryStore(bin, "dir/does/not/exist.bin");

            FILE* fileHandle = fopen("dir/does/not/exist.bin", "rb");
            hgAssert(fileHandle == nullptr);
        }

        {
            HgBinary bin{saveData, sizeof(saveData)};

            HgString filePath = "hg_test_dir/file_bin_test.bin";

            hgBinaryStore(bin, filePath);

            HgBinaryAsset* newBin = hgAssetLoad<HgBinary>(filePath);

            hgAssert(newBin->data.data != nullptr);
            hgAssert(newBin->data.data != saveData);
            hgAssert(newBin->data.size == sizeof(saveData));
            hgAssert(hgMemEqual(saveData, newBin->data.data, newBin->data.size));

            HgBinaryAsset* newBin2 = hgAssetLoad<HgBinary>(filePath);
            hgAssert(newBin2 == newBin);

            hgAssetUnload(newBin);
            hgAssetUnload(newBin2);
        }
    }

    // HgImage
    {
        struct color {
            u8 r, g, b, a;

            operator u32() { return *(u32*)this; }
        };

        u32 red =    color{0xff, 0x00, 0x00, 0xff};
        u32 green =  color{0x00, 0xff, 0x00, 0xff};
        u32 blue =   color{0x00, 0x00, 0xff, 0xff};
        u32 yellow = color{0xff, 0xff, 0x00, 0xff};

        u32 saveData[2][2]{
            {red, green},
            {blue, yellow},
        };

        HgString path = "hg_test_dir/image_test.png";

        HgTextureData testImage{};
        testImage.width = 2;
        testImage.height = 2;
        testImage.depth = 1;
        testImage.format = HgFormat_r8g8b8a8_srgb;
        testImage.pixels = saveData;

        {
            hgTextureStorePng(&testImage, path);

            HgTextureDataAsset* image = hgAssetLoad<HgTextureData>(path);
            hgDefer(hgAssetUnload(image));
            hgAssert(image->data.width == testImage.width);
            hgAssert(image->data.height == testImage.height);
            hgAssert(hgMemEqual(image->data.pixels, saveData, sizeof(saveData)));
        }
    }

    // HgEcs basics
    {
        HgEcs ecs = hgEcsCreate();
        hgDefer(hgEcsDestroy(&ecs));

        hgEcsRegisterType(&ecs, u32);
        hgEcsRegisterType(&ecs, u64);

        HgEntity e1 = hgEcsSpawn(&ecs);
        HgEntity e2 = hgEcsSpawn(&ecs);
        HgEntity e3 = {};
        hgAssert(hgEcsAlive(&ecs, e1));
        hgAssert(hgEcsAlive(&ecs, e2));
        hgAssert(!hgEcsAlive(&ecs, e3));

        hgEcsDespawn(&ecs, e1);
        hgAssert(!hgEcsAlive(&ecs, e1));
        e3 = hgEcsSpawn(&ecs);
        hgAssert(hgEcsAlive(&ecs, e3));
        hgAssert(hgHandleIdx(e3.handle) == hgHandleIdx(e1.handle) && e3.handle.id != e1.handle.id);

        e1 = hgEcsSpawn(&ecs);
        hgAssert(hgEcsAlive(&ecs, e1));

        {
            hgAssert(!hgEcsHas<u32>(&ecs, e1));
            hgAssert(!hgEcsHas<u32>(&ecs, e2));
            hgAssert(!hgEcsHas<u32>(&ecs, e3));

            *hgEcsAdd<u32>(&ecs, e1) = 1;
            hgAssert(hgEcsHas<u32>(&ecs, e1) && *hgEcsGet<u32>(&ecs, e1) == 1);
            hgAssert(!hgEcsHas<u32>(&ecs, e2));
            hgAssert(!hgEcsHas<u32>(&ecs, e3));
            *hgEcsAdd<u32>(&ecs, e2) = 2;
            hgAssert(hgEcsHas<u32>(&ecs, e1) && *hgEcsGet<u32>(&ecs, e1) == 1);
            hgAssert(hgEcsHas<u32>(&ecs, e2) && *hgEcsGet<u32>(&ecs, e2) == 2);
            hgAssert(!hgEcsHas<u32>(&ecs, e3));
            *hgEcsAdd<u32>(&ecs, e3) = 3;
            hgAssert(hgEcsHas<u32>(&ecs, e1) && *hgEcsGet<u32>(&ecs, e1) == 1);
            hgAssert(hgEcsHas<u32>(&ecs, e2) && *hgEcsGet<u32>(&ecs, e2) == 2);
            hgAssert(hgEcsHas<u32>(&ecs, e3) && *hgEcsGet<u32>(&ecs, e3) == 3);

            hgEcsRemove<u32>(&ecs, e1);
            hgAssert(!hgEcsHas<u32>(&ecs, e1));
            hgAssert(hgEcsHas<u32>(&ecs, e2) && *hgEcsGet<u32>(&ecs, e2) == 2);
            hgAssert(hgEcsHas<u32>(&ecs, e3) && *hgEcsGet<u32>(&ecs, e3) == 3);
            hgEcsRemove<u32>(&ecs, e2);
            hgAssert(!hgEcsHas<u32>(&ecs, e1));
            hgAssert(!hgEcsHas<u32>(&ecs, e2));
            hgAssert(hgEcsHas<u32>(&ecs, e3) && *hgEcsGet<u32>(&ecs, e3) == 3);
            hgEcsRemove<u32>(&ecs, e3);
            hgAssert(!hgEcsHas<u32>(&ecs, e1));
            hgAssert(!hgEcsHas<u32>(&ecs, e2));
            hgAssert(!hgEcsHas<u32>(&ecs, e3));
        }

        {
            bool hasUnknown = false;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32*)
            {
                hasUnknown = true;
            });
            hgAssert(!hasUnknown);

            hgAssert(hgEcsCount<u32>(&ecs) == 0);
            hgAssert(hgEcsCount<u64>(&ecs) == 0);
        }

        {
            *hgEcsAdd<u32>(&ecs, e1) = 12;
            *hgEcsAdd<u32>(&ecs, e2) = 42;
            *hgEcsAdd<u32>(&ecs, e3) = 100;
            hgAssert(hgEcsCount<u32>(&ecs) == 3);
            hgAssert(hgEcsCount<u64>(&ecs) == 0);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            hgEcsForEach<u32>(&ecs, [&](HgEntity e, u32* c)
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
            *hgEcsAdd<u64>(&ecs, e2) = 2042;
            *hgEcsAdd<u64>(&ecs, e3) = 2100;
            hgAssert(hgEcsCount<u32>(&ecs) == 3);
            hgAssert(hgEcsCount<u64>(&ecs) == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            bool has2042 = false;
            bool has2100 = false;
            hgEcsForEach<u32, u64>(&ecs, [&](HgEntity e, u32* comp32, u64* comp64)
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
            hgEcsDespawn(&ecs, e1);
            hgAssert(hgEcsCount<u32>(&ecs) == 2);
            hgAssert(hgEcsCount<u64>(&ecs) == 2);

            bool hasUnknown = false;
            bool has12 = false;
            bool has42 = false;
            bool has100 = false;
            hgEcsForEach<u32>(&ecs, [&](HgEntity e, u32* c)
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
            hgEcsDespawn(&ecs, e2);
            hgAssert(hgEcsCount<u32>(&ecs) == 1);
            hgAssert(hgEcsCount<u64>(&ecs) == 1);
        }
    }

    // Ecs concurrency
    {
        HgEcs ecs = hgEcsCreate();
        hgDefer(hgEcsDestroy(&ecs));

        hgEcsRegisterType(&ecs, u32);
        hgEcsRegisterType(&ecs, u64);

        {
            for (u32 i = 0; i < 4; ++i)
            {
                HgEntity e = hgEcsSpawn(&ecs);
                switch (i % 3)
                {
                    case 0:
                        *hgEcsAdd<u32>(&ecs, e) = 12;
                        *hgEcsAdd<u64>(&ecs, e) = 42;
                        break;
                    case 1:
                        *hgEcsAdd<u32>(&ecs, e) = 12;
                        break;
                    case 2:
                        *hgEcsAdd<u64>(&ecs, e) = 42;
                        break;
                }
            }

            bool success;
            hgEcsForPar<u32>(&ecs, [&](HgEntity, u32* c)
            {
                *c += 4;
            });
            success = true;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != 16)
                    success = false;
            });
            hgAssert(success);

            hgEcsForPar<u64>(&ecs, [&](HgEntity, u64* c)
            {
                *c += 3;
            });
            success = true;
            hgEcsForEach<u64>(&ecs, [&](HgEntity, u64* c)
            {
                if (*c != 45)
                    success = false;
            });
            hgAssert(success);

            hgEcsForPar<u32, u64>(&ecs, [&](HgEntity, u32* c32, u64* c64)
            {
                *c64 -= *c32;
            });
            success = true;
            hgEcsForEach<u64>(&ecs, [&](HgEntity e, u64* c)
            {
                if (hgEcsHas<u32>(&ecs, e))
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
    }

    // Ecs sort
    {
        HgEcs ecs = hgEcsCreate();
        hgDefer(hgEcsDestroy(&ecs));

        hgEcsRegisterType(&ecs, u32);
        hgEcsRegisterType(&ecs, u64);

        auto comparison = [](void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs)
        {
            return *hgEcsGet<u32>(ecs, lhs) < *hgEcsGet<u32>(ecs, rhs);
        };

        {
            *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = 42;

            hgEcsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != 42)
                    success = false;
            });
            hgAssert(success);

            hgEcsReset(&ecs);
        }

        {
            u32 smallScramble1[]{1, 0};
            for (u32 i = 0; i < hgArrayCount(smallScramble1); ++i)
            {
                *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = smallScramble1[i];
            }

            {
                hgEcsSort<u32>(&ecs, nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            {
                hgEcsSort<u32>(&ecs, nullptr, comparison);

                bool success = true;
                u32 elem = 0;
                hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
                {
                    if (*c != elem)
                        success = false;
                    ++elem;
                });
                hgAssert(success);
            }

            hgEcsReset(&ecs);
        }

        {
            u32 mediumScramble1[]{8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
            for (u32 i = 0; i < hgArrayCount(mediumScramble1); ++i)
            {
                *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = mediumScramble1[i];
            }
            hgEcsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            hgEcsReset(&ecs);
        }

        {
            u32 mediumScramble2[]{3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
            for (u32 i = 0; i < hgArrayCount(mediumScramble2); ++i)
            {
                *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = mediumScramble2[i];
            }
            hgEcsSort<u32>(&ecs, nullptr, comparison);
            hgEcsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            hgEcsReset(&ecs);
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = i;
            }
            hgEcsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != elem)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            hgEcsReset(&ecs);
        }

        {
            for (u32 i = 127; i < 128; --i)
            {
                *hgEcsAdd<u32>(&ecs, hgEcsSpawn(&ecs)) = i / 2;
            }
            hgEcsSort<u32>(&ecs, nullptr, comparison);
            hgEcsSort<u32>(&ecs, nullptr, comparison);

            bool success = true;
            u32 elem = 0;
            hgEcsForEach<u32>(&ecs, [&](HgEntity, u32* c)
            {
                if (*c != elem / 2)
                    success = false;
                ++elem;
            });
            hgAssert(success);

            hgEcsReset(&ecs);
        }
    }

    // HgEcs serialization
    {
        HgArena* arena = hgScratch();
        hgArenaScope(arena);

        HgSerialNode* scene{};

        {
            HgEcs ecs = hgEcsCreate();
            hgDefer(hgEcsDestroy(&ecs));

            hgEcsRegisterType(&ecs, HgNode);
            hgEcsRegisterType(&ecs, u32);

            HgEntity root = hgEcsSpawn(&ecs);
            HgEntity a = hgEcsSpawn(&ecs);
            HgEntity b = hgEcsSpawn(&ecs);

            hgNodeAdd(&ecs, root);
            hgNodeAdd(&ecs, a);
            hgNodeAdd(&ecs, b);

            *hgEcsAdd<u32>(&ecs, a) = 12;
            *hgEcsAdd<u32>(&ecs, b) = 42;

            hgNodeAddChild(&ecs, root, b);
            hgNodeAddChild(&ecs, root, a);

            HgSerializer s = hgSerialWriter(arena);
            hgSerialize(&s, &ecs);
            scene = s.current;
        }

        {
            HgEcs ecs = hgEcsCreate();
            hgDefer(hgEcsDestroy(&ecs));

            hgEcsRegisterType(&ecs, HgNode);
            hgEcsRegisterType(&ecs, u32);

            HgSerializer s = hgSerialReader(arena, scene);
            hgSerialize(&s, &ecs);

            HgEntity root = hgEcsEntities<HgNode>(&ecs)[0];

            hgAssert(hgEcsHas<HgNode>(&ecs, root));
            HgNode* rootNode = hgEcsGet<HgNode>(&ecs, root);
            hgAssert(rootNode->parent.handle == hgHandleNull);
            hgAssert(rootNode->nextSibling.handle == hgHandleNull);
            hgAssert(rootNode->prevSibling.handle == hgHandleNull);
            hgAssert(rootNode->firstChild.handle != hgHandleNull);

            HgEntity a = rootNode->firstChild;
            hgAssert(a.handle != hgHandleNull);

            hgAssert(hgEcsHas<HgNode>(&ecs, a));
            HgNode* aNode = hgEcsGet<HgNode>(&ecs, a);
            hgAssert(aNode->parent == root);
            hgAssert(aNode->prevSibling.handle == hgHandleNull);
            hgAssert(aNode->nextSibling.handle != hgHandleNull);
            hgAssert(aNode->firstChild.handle == hgHandleNull);

            HgEntity b = aNode->nextSibling;
            hgAssert(b.handle != hgHandleNull);

            hgAssert(hgEcsHas<HgNode>(&ecs, b));
            HgNode* bNode = hgEcsGet<HgNode>(&ecs, b);
            hgAssert(bNode->parent == root);
            hgAssert(bNode->prevSibling == a);
            hgAssert(bNode->nextSibling.handle == hgHandleNull);
            hgAssert(bNode->firstChild.handle == hgHandleNull);

            hgAssert(hgEcsHas<u32>(&ecs, a));
            hgAssert(*hgEcsGet<u32>(&ecs, a) == 12);

            hgAssert(hgEcsHas<u32>(&ecs, b));
            hgAssert(*hgEcsGet<u32>(&ecs, b) == 42);
        }
    }

    // HgNode
    {
        HgEcs ecs = hgEcsCreate();
        hgDefer(hgEcsDestroy(&ecs));

        hgEcsRegisterType(&ecs, HgNode);

        {
            HgEntity a = hgEcsSpawn(&ecs);
            HgEntity b = hgEcsSpawn(&ecs);
            HgEntity aa = hgEcsSpawn(&ecs);
            HgEntity ab = hgEcsSpawn(&ecs);

            *hgNodeAdd(&ecs, a) = {};
            *hgNodeAdd(&ecs, b) = {};
            *hgNodeAdd(&ecs, aa) = {};
            *hgNodeAdd(&ecs, ab) = {};

            hgNodeAddChild(&ecs, a, aa);
            hgNodeAddChild(&ecs, a, ab);

            hgAssert(hgEcsAlive(&ecs, a));
            hgAssert(hgEcsAlive(&ecs, b));
            hgAssert(hgEcsAlive(&ecs, aa));
            hgAssert(hgEcsAlive(&ecs, ab));

            hgNodeDestroy(&ecs, a);

            hgAssert(!hgEcsAlive(&ecs, a));
            hgAssert(hgEcsAlive(&ecs, b));
            hgAssert(!hgEcsAlive(&ecs, aa));
            hgAssert(!hgEcsAlive(&ecs, ab));

            hgEcsDespawn(&ecs, b);
        }

        {
            HgEntity a = hgEcsSpawn(&ecs);
            HgEntity b = hgEcsSpawn(&ecs);
            HgEntity aa = hgEcsSpawn(&ecs);
            HgEntity ab = hgEcsSpawn(&ecs);
            HgEntity aba = hgEcsSpawn(&ecs);
            HgEntity abb = hgEcsSpawn(&ecs);

            *hgNodeAdd(&ecs, a) = {};
            *hgNodeAdd(&ecs, b) = {};
            *hgNodeAdd(&ecs, aa) = {};
            *hgNodeAdd(&ecs, ab) = {};
            *hgNodeAdd(&ecs, aba) = {};
            *hgNodeAdd(&ecs, abb) = {};

            hgNodeAddChild(&ecs, ab, aba);
            hgNodeAddChild(&ecs, ab, abb);
            hgNodeAddChild(&ecs, a, aa);
            hgNodeAddChild(&ecs, a, ab);

            hgAssert(hgEcsAlive(&ecs, a));
            hgAssert(hgEcsAlive(&ecs, b));
            hgAssert(hgEcsAlive(&ecs, aa));
            hgAssert(hgEcsAlive(&ecs, ab));
            hgAssert(hgEcsAlive(&ecs, aba));
            hgAssert(hgEcsAlive(&ecs, abb));

            hgNodeDestroy(&ecs, ab);

            hgAssert(hgEcsAlive(&ecs, a));
            hgAssert(hgEcsAlive(&ecs, b));
            hgAssert(hgEcsAlive(&ecs, aa));
            hgAssert(!hgEcsAlive(&ecs, ab));
            hgAssert(!hgEcsAlive(&ecs, aba));
            hgAssert(!hgEcsAlive(&ecs, abb));

            hgNodeDestroy(&ecs, a);

            hgAssert(!hgEcsAlive(&ecs, a));
            hgAssert(hgEcsAlive(&ecs, b));
            hgAssert(!hgEcsAlive(&ecs, aa));
            hgAssert(!hgEcsAlive(&ecs, ab));
            hgAssert(!hgEcsAlive(&ecs, aba));
            hgAssert(!hgEcsAlive(&ecs, abb));

            hgEcsDespawn(&ecs, b);
        }
    }

    hgWarn("HgMesh test : TODO\n");

    printf("HurdyGurdy: Tests Complete in %fms\n", hgClockTick(&timer) * 1000.0f);
}

