#include "tests.hpp"
#include "hg/json.hpp"

using namespace hg;

TEST(testJsonDefaultConstructors)
{
    JsonValue value{};
    JsonObject obj{};
    JsonArray arr{};

    ASSERT(obj.count() == 0);
    ASSERT(arr.count() == 0);

    bool matched = false;
    value.match(
        [&](const JsonObject&) { matched = true; },
        [&](const JsonArray&) { matched = true; },
        [&](const StringBuilder&) { matched = true; },
        [&](const f64&) { matched = true; },
        [&](const bool&) { matched = true; }
    );
    ASSERT(!matched);
}

TEST(testJsonPackConstructors)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena,
        "a", 1.0,
        "b", true,
        "c", "three",
    };
    ASSERT(obj.count() == 3);
    ASSERT(obj.getNumber("a") == 1.0);
    ASSERT(obj.getBoolean("b") == true);
    ASSERT(obj.getString("c") == "three");

    JsonArray arr{arena,
        1.0,
        true,
        "three",
    };
    ASSERT(arr.count() == 3);
    ASSERT(arr.getNumber(0) == 1.0);
    ASSERT(arr.getBoolean(1) == true);
    ASSERT(arr.getString(2) == "three");

    JsonObject nested{arena,
        "num", 1.0,
        "arr", JsonArray{arena, 1.0, 2.0},
        "obj", JsonObject{arena, "inner", true},
    };
    ASSERT(nested.count() == 3);
    ASSERT(nested.getNumber("num") == 1.0);
    ASSERT(nested.getArray("arr").count() == 2);
    ASSERT(nested.getArray("arr").getNumber(1) == 2.0);
    ASSERT(nested.getObject("obj").getBoolean("inner") == true);
}

TEST(testJsonValueConstruction)
{
    ArenaScope arena = getScratch();

    JsonValue num{3.5};
    ASSERT(num.number() == 3.5);

    JsonValue boolVal{true};
    ASSERT(boolVal.boolean() == true);

    JsonValue objVal{JsonObject{arena, "b", true}};
    ASSERT(objVal.object().count() == 1);
    ASSERT(objVal.object().getBoolean("b") == true);

    JsonValue arrVal{JsonArray{arena, 1.0}};
    ASSERT(arrVal.array().getNumber(0) == 1.0);

    JsonValue strVal{StringBuilder{arena, "str"}};
    ASSERT(strVal.string() == "str");
}

TEST(testJsonObjectSetters)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena};

    obj.setNumber("n", 1.0) = 2.0;
    ASSERT(obj.getNumber("n") == 2.0);

    obj.setBoolean("b", false) = true;
    ASSERT(obj.getBoolean("b") == true);

    obj.setString("s", "old") = StringBuilder{arena, "new"};
    ASSERT(obj.getString("s") == "new");

    JsonObject& child = obj.setObject("o", JsonObject{arena});
    child.setNumber("pi", 3.5);
    ASSERT(obj.getObject("o").getNumber("pi") == 3.5);

    JsonArray& arr = obj.setArray("a", JsonArray{arena});
    arr.pushNumber(1.0);
    ASSERT(obj.getArray("a").getNumber(0) == 1.0);

    obj.setValue("v", JsonValue{f64{9.0}}) = JsonValue{f64{10.0}};
    ASSERT(obj.getNumber("v") == 10.0);

    ASSERT(obj.count() == 6);
}

TEST(testJsonArrayPushers)
{
    ArenaScope arena = getScratch();

    JsonArray arr{arena};

    arr.pushNumber(1.0) = 2.0;
    ASSERT(arr.getNumber(0) == 2.0);

    arr.pushBoolean(false) = true;
    ASSERT(arr.getBoolean(1) == true);

    arr.pushString("old") = StringBuilder{arena, "new"};
    ASSERT(arr.getString(2) == "new");

    arr.pushArray(JsonArray{arena}).pushArray(JsonArray{arena}).pushNumber(3.0);
    ASSERT(arr.getArray(3).getArray(0).getNumber(0) == 3.0);

    arr.pushObject(JsonObject{arena}).setBoolean("b", true);
    ASSERT(arr.getObject(4).getBoolean("b") == true);

    ASSERT(arr.pushValue(JsonValue{f64{5.0}}).number() == 5.0);
    ASSERT(arr.getNumber(5) == 5.0);

    ASSERT(arr.count() == 6);
}

TEST(testJsonArrayRemovers)
{
    ArenaScope arena = getScratch();

    JsonArray arr{arena,
        1.0,
        "two",
        true,
        3.0,
        4.0,
    };

    ASSERT(arr.removeBoolean(2) == true);
    ASSERT(arr.count() == 4);

    ASSERT(arr.removeNumber(0) == 1.0);
    ASSERT(arr.count() == 3);
    ASSERT(arr.getString(0) == "two");
    ASSERT(arr.getNumber(1) == 3.0);

    arr.pushNumber(5.0);
    arr.pushArray(JsonArray{arena}).pushNumber(6.0);
    arr.pushObject(JsonObject{arena}).setNumber("seven", 7.0);
    ASSERT(arr.count() == 6);

    JsonArray six = arr.removeArray(4);
    ASSERT(six.getNumber(0) == 6.0);
    ASSERT(arr.count() == 5);

    JsonObject seven = arr.removeObject(4);
    ASSERT(seven.getNumber("seven") == 7.0);
    ASSERT(arr.count() == 4);

    ASSERT(arr.getNumber(3) == 5.0);

    ASSERT(arr.removeValue(0).string() == "two");
    ASSERT(arr.count() == 3);
    ASSERT(arr.getNumber(0) == 3.0);
    ASSERT(arr.getNumber(2) == 5.0);
}

TEST(testJsonForEachAndIteration)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena,
        "a", 1.0,
        "b", 2.0,
        "c", 3.0,
    };

    {
        f64 total = 0.0;
        bool sawA = false;
        bool sawB = false;
        bool sawC = false;
        obj.forEach([&](StringView name, JsonValue& val)
        {
            if (name == "a")
            {
                sawA = true;
                total += val.number();
            }
            else if (name == "b")
            {
                sawB = true;
                total += val.number();
            }
            else if (name == "c")
            {
                sawC = true;
                total += val.number();
            }
        });
        ASSERT(total == 6.0);
        ASSERT(sawA);
        ASSERT(sawB);
        ASSERT(sawC);
    }

    {
        f64 total = 0.0;
        const JsonObject& cobj = obj;
        cobj.forEach([&](StringView, const JsonValue& val)
        {
            val.match(
                [&](const JsonObject&) {},
                [&](const JsonArray&) {},
                [&](const StringBuilder&) {},
                [&](const f64& num) { total += num; },
                [&](const bool&) {}
            );
        });
        ASSERT(total == 6.0);
    }

    {
        JsonArray arr{arena, 1.0, 2.0, 3.0};

        f64 sum = 0.0;
        for (JsonValue& val : arr)
        {
            sum += val.number();
        }
        ASSERT(sum == 6.0);

        u64 counted = 0;
        const JsonArray& carr = arr;
        for (const JsonValue* it = carr.begin(); it != carr.end(); ++it)
        {
            ++counted;
        }
        ASSERT(counted == 3);
    }
}

TEST(testWriteJsonEmptyObject)
{
    ArenaScope arena = getScratch();
    JsonObject obj{arena};
    ASSERT(writeJson(arena, obj) == "{\n}");
}

TEST(testWriteJsonEmptyArray)
{
    ArenaScope arena = getScratch();
    JsonObject obj{arena, "a", JsonArray{arena}};
    ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t]\n}");
}

TEST(testWriteJsonPrimitives)
{
    ArenaScope arena = getScratch();

    {
        JsonObject obj{arena, "b", true};
        ASSERT(writeJson(arena, obj) == "{\n\t\"b\" : true\n}");
    }
    {
        JsonObject obj{arena, "b", false};
        ASSERT(writeJson(arena, obj) == "{\n\t\"b\" : false\n}");
    }
    {
        JsonObject obj{arena, "f", 2.5};
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : 2.5\n}");
    }
    {
        JsonObject obj{arena, "f", -2.5};
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : -2.5\n}");
    }
    {
        JsonObject obj{arena, "f", 0.0};
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : 0\n}");
    }
    {
        JsonObject obj{arena, "s", "hello"};
        ASSERT(writeJson(arena, obj) == "{\n\t\"s\" : \"hello\"\n}");
    }
}

TEST(testWriteJsonArrays)
{
    ArenaScope arena = getScratch();

    {
        JsonObject obj{arena, "a", JsonArray{arena, true}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject obj{arena, "a", JsonArray{arena, 2.5}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\t2.5\n\t]\n}");
    }
    {
        JsonObject obj{arena, "a", JsonArray{arena, "one", "two"}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\t\"one\",\n\t\t\"two\"\n\t]\n}");
    }
}

TEST(testWriteJsonNested)
{
    ArenaScope arena = getScratch();

    {
        JsonObject obj{arena, "a", JsonObject{arena, "b", true}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : {\n\t\t\"b\" : true\n\t}\n}");
    }
    {
        JsonObject obj{arena, "a", JsonArray{arena, true}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject obj{arena, "a", JsonArray{arena, JsonObject{arena, "x", 2.5}}};
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\t{\n\t\t\t\"x\" : 2.5\n\t\t}\n\t]\n}");
    }
}

TEST(testWriteJsonDocument)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena,
        "name", "hurdy",
        "count", 3.0,
        "valid", true,
        "list", JsonArray{arena, 1.0, false},
    };

    StringView out = writeJson(arena, obj);

    auto contains = [](const StringView& hay, const char* needle)
    {
        u64 needleLen = 0;
        while (needle[needleLen] != '\0')
            ++needleLen;
        if (needleLen > hay.length)
            return false;
        for (u64 i = 0; i <= hay.length - needleLen; ++i)
        {
            bool match = true;
            for (u64 j = 0; j < needleLen; ++j)
            {
                if (hay[i + j] != needle[j])
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return true;
        }
        return false;
    };

    ASSERT(out.length >= 2);
    ASSERT(out[0] == '{');
    ASSERT(out[out.length - 1] == '}');

    u64 openBrace = 0;
    u64 closeBrace = 0;
    u64 openBracket = 0;
    u64 closeBracket = 0;
    for (u64 i = 0; i < out.length; ++i)
    {
        switch (out[i])
        {
        case '{': ++openBrace; break;
        case '}': ++closeBrace; break;
        case '[': ++openBracket; break;
        case ']': ++closeBracket; break;
        default: break;
        }
    }
    ASSERT(openBrace == closeBrace);
    ASSERT(openBracket == closeBracket);

    bool trailingComma = false;
    for (u64 i = 0; i + 2 < out.length; ++i)
    {
        if (out[i] == ',' && out[i + 1] == '\n' && (out[i + 2] == '}' || out[i + 2] == ']'))
            trailingComma = true;
    }
    ASSERT(!trailingComma);

    ASSERT(contains(out, "\"name\" : \"hurdy\""));
    ASSERT(contains(out, "\"count\" : 3"));
    ASSERT(contains(out, "\"valid\" : true"));
    ASSERT(contains(out, "\"list\" : ["));
    ASSERT(contains(out, "1"));
    ASSERT(contains(out, "false"));
}

TEST(testWriteJsonEscaping)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena, "s", "a\"b\\c\bd\fe\nf\rg\th"};
    ASSERT(writeJson(arena, obj) == "{\n\t\"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\"\n}");
}

TEST(testReadJsonEmpty)
{
    ArenaScope arena = getScratch();

    ASSERT(readJson(arena, "{}").has);
    ASSERT(readJson(arena, "{ }").has);
    ASSERT(readJson(arena, "{\n}").has);
    ASSERT(readJson(arena, "{}")->count() == 0);

    Maybe<JsonObject> result = readJson(arena, "{\n\t\"a\" : [\n\t]\n}");
    ASSERT(result.has);
    ASSERT(result->count() == 1);
    ASSERT(result->getArray("a").count() == 0);
}

TEST(testReadJsonPrimitives)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"b\" : true , \"c\" : false , \"i\" : 12345 , \"n\" : -7 , \"f\" : 2.5 , \"g\" : -0.5 , \"s\" : \"hi\" }");
    ASSERT(result.has);
    ASSERT(result->count() == 7);

    ASSERT(result->getBoolean("b") == true);
    ASSERT(result->getBoolean("c") == false);
    ASSERT(result->getNumber("i") == 12345.0);
    ASSERT(result->getNumber("n") == -7.0);
    ASSERT(result->getNumber("f") == 2.5);
    ASSERT(result->getNumber("g") == -0.5);
    ASSERT(result->getString("s") == "hi");
}

TEST(testReadJsonWhitespace)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{\n\t\"a\" : 1,\n\t\"b\" : \"two words\"\n}");
    ASSERT(result.has);
    ASSERT(result->count() == 2);
    ASSERT(result->getNumber("a") == 1.0);
    ASSERT(result->getString("b") == "two words");

    Maybe<JsonObject> trailing = readJson(arena, "{ \"a\" : 1 , }");
    ASSERT(trailing.has);
    ASSERT(trailing->count() == 1);
}

TEST(testReadJsonArrays)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"a\" : [] , \"b\" : [ 1 ] , \"c\" : [ \"x\" , true ] }");
    ASSERT(result.has);
    ASSERT(result->count() == 3);

    JsonArray& b = result->getArray("b");
    ASSERT(b.count() == 1);
    ASSERT(b.getNumber(0) == 1.0);

    JsonArray& c = result->getArray("c");
    ASSERT(c.count() == 2);
    ASSERT(c.getString(0) == "x");
    ASSERT(c.getBoolean(1) == true);
}

TEST(testReadJsonNested)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"child\" : { \"inner\" : true } , \"list\" : [ { \"id\" : 1 } , { \"id\" : 2 } ] }");
    ASSERT(result.has);

    JsonObject& child = result->getObject("child");
    ASSERT(child.count() == 1);
    ASSERT(child.getBoolean("inner") == true);

    JsonArray& list = result->getArray("list");
    ASSERT(list.count() == 2);
    ASSERT(list.getObject(0).getNumber("id") == 1.0);
    ASSERT(list.getObject(1).getNumber("id") == 2.0);
}

TEST(testReadJsonEscapes)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\" }");
    ASSERT(result.has);
    ASSERT(result->getString("s") == "a\"b\\c\bd\fe\nf\rg\th");

    Maybe<JsonObject> empty = readJson(arena, "{ \"s\" : \"\" }");
    ASSERT(empty.has);
    ASSERT(empty->getString("s") == "");
}

TEST(testReadJsonFailure)
{
    ArenaScope arena = getScratch();

    ASSERT(!readJson(arena, "").has);
    ASSERT(!readJson(arena, "junk").has);
    ASSERT(!readJson(arena, "\"a\" : 1").has);
    ASSERT(!readJson(arena, "[1,2]").has);
    ASSERT(!readJson(arena, "{").has);
    ASSERT(!readJson(arena, "{ \"a\" : }").has);
    ASSERT(!readJson(arena, "{ : 1 }").has);
    ASSERT(!readJson(arena, "{ \"a\" 1 }").has);
    ASSERT(!readJson(arena, "{ \"a\" : \"unterminated }").has);
    ASSERT(!readJson(arena, "{ \"a\" : 1, \"b\" }").has);
    ASSERT(!readJson(arena, "{ \"a\" : [ 1, 2 }").has);
    ASSERT(!readJson(arena, "{ \"a\" : [ 1, 2").has);
    ASSERT(!readJson(arena, "{ \"a\" : 1, \"b\" : }").has);
}

TEST(testWriteReadJsonRoundTrip)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena,
        "name", "hurdy",
        "count", 42.0,
        "valid", true,
        "empty", "",
        "list", JsonArray{arena, 1.0, -2.5, false, "x"},
        "child", JsonObject{arena, "pi", 3.5},
    };

    StringView out = writeJson(arena, obj);
    ASSERT(out.length >= 2);

    Maybe<JsonObject> json = readJson(arena, out);
    ASSERT(json.has);
    ASSERT(json->count() == 6);
    ASSERT(json->getString("name") == "hurdy");
    ASSERT(json->getNumber("count") == 42.0);
    ASSERT(json->getBoolean("valid") == true);
    ASSERT(json->getString("empty") == "");
    ASSERT(json->getArray("list").count() == 4);
    ASSERT(json->getObject("child").count() == 1);

    StringView out2 = writeJson(arena, *json);
    ASSERT(out2 == out);
}

TEST(testReadWriteJsonRoundTrip)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"a\" : 1 , \"b\" : \"str\" , \"c\" : [ true, false, -2.5, \"x\" ] , \"d\" : { \"e\" : [] } }");
    ASSERT(result.has);

    StringView out = writeJson(arena, *result);
    ASSERT(out ==
        "{\n\t\"a\" : 1,\n\t\"b\" : \"str\",\n\t\"c\" : [\n\t\ttrue,\n\t\tfalse,\n\t\t-2.5,\n\t\t\"x\"\n\t],\n\t\"d\" : {\n\t\t\"e\" : [\n\t\t]\n\t}\n}");

    Maybe<JsonObject> json = readJson(arena, out);
    ASSERT(json.has);
    ASSERT(json->count() == 4);
    ASSERT(json->getNumber("a") == 1.0);
    ASSERT(json->getString("b") == "str");
    ASSERT(json->getArray("c").count() == 4);
    ASSERT(json->getObject("d").count() == 1);
    ASSERT(json->getObject("d").getArray("e").count() == 0);
}

