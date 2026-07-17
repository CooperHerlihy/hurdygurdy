#undef HG_NO_LOGGING
#define HG_LOGGING 1
#include "hurdygurdy.hpp"

#include <emmintrin.h>

#define TEST(cond) do { \
    if (!(cond)) \
        HG_PANIC("Test failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

using namespace hg;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    HG_LOG("Tests Begun\n");

    Clock timer{};

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

