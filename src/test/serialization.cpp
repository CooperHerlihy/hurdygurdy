#include "tests.hpp"

void testSerialization()
{
    // ============================================================================
    // Serialization
    // ============================================================================
    //
    // Tests for the serialization API: serialWriter, serialReader,
    // serialize primitives, serializeObject, binaryWriteSerial,
    // binaryReadSerial, jsonWriteSerial.

    // Primitives: bool, integer, floating-point
    {
        bool val = true;
        bool copy = false;
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        i64 val = -1234567890123;
        i64 copy = 0;
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        u32 val = 0xDEADBEEF;
        u32 copy = 0;
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        f64 val = 3.14159265358979;
        f64 copy = 0.0;
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    // Math types
    {
        Vec3 val{1.0f, 2.0f, 3.0f};
        Vec3 copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        Mat4 val{1.0f};
        Mat4 copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        Complex val{3.0f, 4.0f};
        Complex copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    {
        Quat val{0.707f, 0.0f, 0.707f, 0.0f};
        Quat copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    // Nested serializeBegin/End
    {
        ArenaScope arena = getScratch();
        i32 outer = 7;
        i32 inner = 42;
        i32 outerCopy = 0;
        i32 innerCopy = 0;

        Serializer w = serialWriter(arena);
        serializeBegin(&w);
        serialize(&w, &outer);
        serializeBegin(&w);
        serialize(&w, &inner);
        serializeEnd(&w);
        serializeEnd(&w);

        Serializer r = serialReader(arena, w.current);
        serializeBegin(&r);
        serialize(&r, &outerCopy);
        serializeBegin(&r);
        serialize(&r, &innerCopy);
        serializeEnd(&r);
        serializeEnd(&r);

        TEST(outerCopy == outer);
        TEST(innerCopy == inner);
    }

    // serializeBegin with size parameter
    {
        ArenaScope arena = getScratch();
        u32 childCount = 0;
        u32 val = 99;
        u32 copy = 0;

        Serializer w = serialWriter(arena);
        serializeBegin(&w);
        serialize(&w, &val);
        serializeEnd(&w);

        Serializer r = serialReader(arena, w.current);
        serializeBegin(&r, &childCount);
        serialize(&r, &copy);
        serializeEnd(&r);

        TEST(childCount == 1);
        TEST(copy == val);
    }

    // Composite struct via serializeObject
    {
        struct Data {
            i64 a;
            u16 b;
            f32 c;
            bool d;
            String e;
        };

        auto serializeData = [](Serializer* s, Data* val)
        {
            serializeObject(s,
                &val->a,
                &val->b,
                &val->c,
                &val->d,
                &val->e);
        };

        ArenaScope arena = getScratch();
        Data val{};
        val.a = -42;
        val.b = 99;
        val.c = 2.5f;
        val.d = true;
        val.e = String::create("composite");

        Data copy{};
        Serializer w = serialWriter(arena);
        serializeData(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serializeData(&r, &copy);
        TEST(copy.a == val.a);
        TEST(copy.b == val.b);
        TEST(copy.c == val.c);
        TEST(copy.d == val.d);
        TEST(copy.e == val.e);
    }

    // Lifecycle
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();

        Lifecycle val{};
        u64 savedId = val.id;

        Lifecycle copy{};
        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy.id == savedId);
        TEST(copy.valid == true);
    }

    // C array
    {
        u32 val[4] = {10, 20, 30, 40};
        u32 copy[4] = {};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy[0] == 10);
        TEST(copy[1] == 20);
        TEST(copy[2] == 30);
        TEST(copy[3] == 40);
    }

    // String
    {
        String val = String::create("hello world");
        String copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy == val);
    }

    // Binary
    {
        u8 raw[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
        Binary val = Binary::create({raw, 8});
        Binary copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy.size == val.size);
        TEST(memcmp(copy.data, val.data, val.size) == 0);
    }

    // UniquePtr
    {
        UniquePtr<i32> val = makeUnique<i32>(42);
        UniquePtr<i32> copy{};
        ArenaScope arena = getScratch();

        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(*copy == 42);
        TEST(copy.ptr != val.ptr); // New allocation, not same pointer
    }

    // Array
    {
        ArenaScope arena = getScratch();
        Array<u32> val{};
        val.push(1);
        val.push(2);
        val.push(3);

        Array<u32> copy{};
        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy.count == val.count);
        for (u32 i = 0; i < val.count; ++i)
            TEST(copy[i] == val[i]);
    }

    // Set
    {
        ArenaScope arena = getScratch();
        Set<u32> val{};
        val.add(10);
        val.add(20);
        val.add(30);

        Set<u32> copy{};
        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy.count == val.count);
        TEST(copy.has(10));
        TEST(copy.has(20));
        TEST(copy.has(30));
    }

    // Map
    {
        ArenaScope arena = getScratch();
        Map<u32, f32> val{};
        val.add(1, 1.5f);
        val.add(2, 2.5f);
        val.add(3, 3.5f);

        Map<u32, f32> copy{};
        Serializer w = serialWriter(arena);
        serialize(&w, &val);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        TEST(copy.count == val.count);
        TEST(*copy.get(1) == 1.5f);
        TEST(*copy.get(2) == 2.5f);
        TEST(*copy.get(3) == 3.5f);
    }

    // Binary format round-trip
    {
        struct Data {
            i64 a;
            f64 b;
            bool c;
        };

        auto serializeData = [](Serializer* s, Data* val)
        {
            serializeObject(s,
                &val->a,
                &val->b,
                &val->c);
        };

        ArenaScope arena = getScratch();
        Data val{};
        val.a = -999;
        val.b = 3.14;
        val.c = true;

        Data copy{};
        Serializer w = serialWriter(arena);
        serializeData(&w, &val);
        BinaryView bin = writeSerialBinary(arena, &w);
        Serializer r = readSerialBinary(arena, bin);
        serializeData(&r, &copy);
        TEST(copy.a == val.a);
        TEST(copy.b == val.b);
        TEST(copy.c == val.c);
    }

    // Binary format round-trip for String
    {
        struct Data {
            i64 a;
            String b;
        };

        auto serializeData = [](Serializer* s, Data* val)
        {
            serializeObject(s,
                &val->a,
                &val->b);
        };

        ArenaScope arena = getScratch();
        Data val{};
        val.a = -999;
        val.b = String::create("binary-test");

        Data copy{};
        Serializer w = serialWriter(arena);
        serializeData(&w, &val);
        BinaryView bin = writeSerialBinary(arena, &w);
        Serializer r = readSerialBinary(arena, bin);
        serializeData(&r, &copy);
        TEST(copy.a == val.a);
        TEST(copy.b == val.b);
    }

    // Binary format round-trip for Binary
    {
        struct Data {
            i64 a;
            Binary b;
        };

        auto serializeData = [](Serializer* s, Data* val)
        {
            serializeObject(s,
                &val->a,
                &val->b);
        };

        ArenaScope arena = getScratch();
        u8 raw[4] = {0xDE, 0xAD, 0xBE, 0xEF};
        Data val{};
        val.a = -123;
        val.b = Binary::create({raw, 4});

        Data copy{};
        Serializer w = serialWriter(arena);
        serializeData(&w, &val);
        BinaryView bin = writeSerialBinary(arena, &w);
        Serializer r = readSerialBinary(arena, bin);
        serializeData(&r, &copy);
        TEST(copy.a == val.a);
        TEST(copy.b.size == val.b.size);
        TEST(memcmp(copy.b.data, val.b.data, val.b.size) == 0);
    }

    {
        // Round-trip Product with primitive types (i64, f64, bool)
        {
            Product<i64, f64, bool> val{-42, 3.14, true};
            Product<i64, f64, bool> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.get<0>() == val.get<0>());
            TEST(copy.get<1>() == val.get<1>());
            TEST(copy.get<2>() == val.get<2>());
        }

        // Round-trip Product with String
        {
            Product<i64, String> val{7, String::create("hello")};
            Product<i64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.get<0>() == val.get<0>());
            TEST(copy.get<1>() == val.get<1>());
        }

        // Round-trip empty Product
        {
            Product<> val{};
            Product<> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
        }

        // Round-trip Product via binary format
        {
            Product<i32, String, f32> val{42, String::create("binary"), 2.5f};
            Product<i32, String, f32> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            BinaryView bin = writeSerialBinary(arena, &w);
            Serializer r = readSerialBinary(arena, bin);
            serialize(&r, &copy);
            TEST(copy.get<0>() == val.get<0>());
            TEST(copy.get<1>() == val.get<1>());
            TEST(copy.get<2>() == val.get<2>());
        }
    }

    {
        // Round-trip Sum with first type active (i64)
        {
            Sum<i64, f64, String> val{i64{-42}};
            Sum<i64, f64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.is<i64>());
            TEST(copy.get<i64>() == -42);
        }

        // Round-trip Sum with second type active (f64)
        {
            Sum<i64, f64, String> val{f64{3.14}};
            Sum<i64, f64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.is<f64>());
            TEST(copy.get<f64>() == 3.14);
        }

        // Round-trip Sum with third type active (String)
        {
            Sum<i64, f64, String> val{String::create("world")};
            Sum<i64, f64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.is<String>());
            TEST(copy.get<String>() == String::create("world"));
        }

        // Round-trip empty Sum (tag == count)
        {
            Sum<i64, f64, String> val{};
            Sum<i64, f64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(!copy.is<i64>());
            TEST(!copy.is<f64>());
            TEST(!copy.is<String>());
        }

        // Round-trip Sum via binary format
        {
            Sum<i64, f64, String> val{f64{99.9}};
            Sum<i64, f64, String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            BinaryView bin = writeSerialBinary(arena, &w);
            Serializer r = readSerialBinary(arena, bin);
            serialize(&r, &copy);
            TEST(copy.is<f64>());
            TEST(copy.get<f64>() == 99.9);
        }
    }

    // ============================================================================
    // Serialization: Maybe
    // ============================================================================
    //
    // Tests for serializing Maybe<T> via has-flag + value pattern.

    {
        // Round-trip Maybe with value (i64)
        {
            Maybe<i64> val{42};
            Maybe<i64> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.has);
            TEST(*copy == 42);
        }

        // Round-trip Maybe with value (String)
        {
            Maybe<String> val{String::create("maybe")};
            Maybe<String> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(copy.has);
            TEST(*copy == val.val);
        }

        // Round-trip Maybe without value
        {
            Maybe<i64> val{};
            Maybe<i64> copy{Maybe<i64>{99}};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);
            TEST(!copy.has);
        }

        // Round-trip Maybe via binary format
        {
            Maybe<f32> val{2.5f};
            Maybe<f32> copy{};
            ArenaScope arena = getScratch();

            Serializer w = serialWriter(arena);
            serialize(&w, &val);
            BinaryView bin = writeSerialBinary(arena, &w);
            Serializer r = readSerialBinary(arena, bin);
            serialize(&r, &copy);
            TEST(copy.has);
            TEST(*copy == 2.5f);
        }
    }
}

