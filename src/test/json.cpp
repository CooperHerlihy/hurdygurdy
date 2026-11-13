#include "tests.hpp"
#include "hg/json.hpp"
#include "hg/error.hpp"

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
        [&](const JsonArray&) {},
        [&](const std::string&) {},
        [&](const f64&) {},
        [&](const bool&) {}
    );
    ASSERT(matched);
}

TEST(testJsonPackConstructors)
{
    JsonObject obj{
        "a", 1.0,
        "b", true,
        "c", "three",
    };
    ASSERT(obj.count() == 3);
    ASSERT(obj.getNumber("a") == 1.0);
    ASSERT(obj.getBoolean("b") == true);
    ASSERT(obj.getString("c") == "three");

    JsonArray arr{
        1.0,
        true,
        "three",
    };
    ASSERT(arr.count() == 3);
    ASSERT(arr.getNumber(0) == 1.0);
    ASSERT(arr.getBoolean(1) == true);
    ASSERT(arr.getString(2) == "three");

    JsonObject nested{
        "num", 1.0,
        "arr", JsonArray{1.0, 2.0},
        "obj", JsonObject{"inner", true},
    };
    ASSERT(nested.count() == 3);
    ASSERT(nested.getNumber("num") == 1.0);
    ASSERT(nested.getArray("arr").count() == 2);
    ASSERT(nested.getArray("arr").getNumber(1) == 2.0);
    ASSERT(nested.getObject("obj").getBoolean("inner") == true);
}

TEST(testJsonValueConstruction)
{
    JsonValue num{3.5};
    ASSERT(num.number() == 3.5);

    JsonValue boolVal{true};
    ASSERT(boolVal.boolean() == true);

    JsonValue objVal{JsonObject{"b", true}};
    ASSERT(objVal.object().count() == 1);
    ASSERT(objVal.object().getBoolean("b") == true);

    JsonValue arrVal{JsonArray{1.0}};
    ASSERT(arrVal.array().getNumber(0) == 1.0);

    JsonValue strVal{std::string{"str"}};
    ASSERT(strVal.string() == "str");
}

TEST(testJsonObjectSetters)
{
    JsonObject obj{};

    obj.setNumber("n", 1.0) = 2.0;
    ASSERT(obj.getNumber("n") == 2.0);

    obj.setBoolean("b", false) = true;
    ASSERT(obj.getBoolean("b") == true);

    obj.setString("s", "old");
    ASSERT(obj.getString("s") == "old");

    JsonObject& child = obj.setObject("o", JsonObject{});
    child.setNumber("pi", 3.5);
    ASSERT(obj.getObject("o").getNumber("pi") == 3.5);

    JsonArray& arr = obj.setArray("a", JsonArray{});
    arr.pushNumber(1.0);
    ASSERT(obj.getArray("a").getNumber(0) == 1.0);

    obj.setValue("v", JsonValue{f64{9.0}}) = JsonValue{f64{10.0}};
    ASSERT(obj.getNumber("v") == 10.0);

    ASSERT(obj.count() == 6);
}

TEST(testJsonArrayPushers)
{
    JsonArray arr{};

    arr.pushNumber(1.0) = 2.0;
    ASSERT(arr.getNumber(0) == 2.0);

    arr.pushBoolean(false) = true;
    ASSERT(arr.getBoolean(1) == true);

    arr.pushString("old");
    ASSERT(arr.getString(2) == "old");

    arr.pushArray(JsonArray{}).pushArray(JsonArray{}).pushNumber(3.0);
    ASSERT(arr.getArray(3).getArray(0).getNumber(0) == 3.0);

    arr.pushObject(JsonObject{}).setBoolean("b", true);
    ASSERT(arr.getObject(4).getBoolean("b") == true);

    ASSERT(arr.pushValue(JsonValue{f64{5.0}}).number() == 5.0);
    ASSERT(arr.getNumber(5) == 5.0);

    ASSERT(arr.count() == 6);
}

TEST(testJsonArrayRemovers)
{
    JsonArray arr{
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
    arr.pushArray(JsonArray{}).pushNumber(6.0);
    arr.pushObject(JsonObject{}).setNumber("seven", 7.0);
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
    JsonObject obj{
        "a", 1.0,
        "b", 2.0,
        "c", 3.0,
    };

    {
        f64 total = 0.0;
        bool sawA = false;
        bool sawB = false;
        bool sawC = false;
        obj.forEach([&](std::string_view name, JsonValue& val)
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
        cobj.forEach([&](std::string_view, const JsonValue& val)
        {
            val.match(
                [&](const JsonObject&) {},
                [&](const JsonArray&) {},
                [&](const std::string&) {},
                [&](const f64& num) { total += num; },
                [&](const bool&) {}
            );
        });
        ASSERT(total == 6.0);
    }

    {
        JsonArray arr{1.0, 2.0, 3.0};

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
    JsonObject obj{};
    ASSERT(writeJson(obj) == "{\n}");
}

TEST(testWriteJsonEmptyArray)
{
    JsonObject obj{"a", JsonArray{}};
    ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t]\n}");
}

TEST(testWriteJsonPrimitives)
{
    {
        JsonObject obj{"b", true};
        ASSERT(writeJson(obj) == "{\n\t\"b\" : true\n}");
    }
    {
        JsonObject obj{"b", false};
        ASSERT(writeJson(obj) == "{\n\t\"b\" : false\n}");
    }
    {
        JsonObject obj{"f", 2.5};
        ASSERT(writeJson(obj) == "{\n\t\"f\" : 2.5\n}");
    }
    {
        JsonObject obj{"f", -2.5};
        ASSERT(writeJson(obj) == "{\n\t\"f\" : -2.5\n}");
    }
    {
        JsonObject obj{"f", 0.0};
        ASSERT(writeJson(obj) == "{\n\t\"f\" : 0\n}");
    }
    {
        JsonObject obj{"s", "hello"};
        ASSERT(writeJson(obj) == "{\n\t\"s\" : \"hello\"\n}");
    }
}

TEST(testWriteJsonArrays)
{
    {
        JsonObject obj{"a", JsonArray{true}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject obj{"a", JsonArray{2.5}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t\t2.5\n\t]\n}");
    }
    {
        JsonObject obj{"a", JsonArray{"one", "two"}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t\t\"one\",\n\t\t\"two\"\n\t]\n}");
    }
}

TEST(testWriteJsonNested)
{
    {
        JsonObject obj{"a", JsonObject{"b", true}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : {\n\t\t\"b\" : true\n\t}\n}");
    }
    {
        JsonObject obj{"a", JsonArray{true}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject obj{"a", JsonArray{JsonObject{"x", 2.5}}};
        ASSERT(writeJson(obj) == "{\n\t\"a\" : [\n\t\t{\n\t\t\t\"x\" : 2.5\n\t\t}\n\t]\n}");
    }
}

TEST(testWriteJsonDocument)
{
    JsonObject obj{
        "name", "hurdy",
        "count", 3.0,
        "valid", true,
        "list", JsonArray{1.0, false},
    };

    std::string out = writeJson(obj);

    auto contains = [](const std::string& hay, const char* needle)
    {
        return hay.find(needle) != std::string::npos;
    };

    ASSERT(out.length() >= 2);
    ASSERT(out[0] == '{');
    ASSERT(out[out.length() - 1] == '}');

    u64 openBrace = 0;
    u64 closeBrace = 0;
    u64 openBracket = 0;
    u64 closeBracket = 0;
    for (u64 i = 0; i < out.length(); ++i)
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
    for (u64 i = 0; i + 2 < out.length(); ++i)
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
    JsonObject obj{"s", "a\"b\\c\bd\fe\nf\rg\th"};
    ASSERT(writeJson(obj) == "{\n\t\"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\"\n}");
}

TEST(testReadJsonEmpty)
{
    ASSERT(readJson("{}").has_value());
    ASSERT(readJson("{ }").has_value());
    ASSERT(readJson("{\n}").has_value());
    ASSERT(readJson("{}")->count() == 0);

    std::optional<JsonObject> result = readJson("{\n\t\"a\" : [\n\t]\n}");
    ASSERT(result.has_value());
    ASSERT(result->count() == 1);
    ASSERT(result->getArray("a").count() == 0);
}

TEST(testReadJsonPrimitives)
{
    std::optional<JsonObject> result = readJson(
        "{ \"b\" : true , \"c\" : false , \"i\" : 12345 , \"n\" : -7 , \"f\" : 2.5 , \"g\" : -0.5 , \"s\" : \"hi\" }");
    ASSERT(result.has_value());
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
    std::optional<JsonObject> result = readJson(
        "{\n\t\"a\" : 1,\n\t\"b\" : \"two words\"\n}");
    ASSERT(result.has_value());
    ASSERT(result->count() == 2);
    ASSERT(result->getNumber("a") == 1.0);
    ASSERT(result->getString("b") == "two words");

    std::optional<JsonObject> trailing = readJson("{ \"a\" : 1 , }");
    ASSERT(trailing.has_value());
    ASSERT(trailing->count() == 1);
}

TEST(testReadJsonArrays)
{
    std::optional<JsonObject> result = readJson(
        "{ \"a\" : [] , \"b\" : [ 1 ] , \"c\" : [ \"x\" , true ] }");
    ASSERT(result.has_value());
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
    std::optional<JsonObject> result = readJson(
        "{ \"child\" : { \"inner\" : true } , \"list\" : [ { \"id\" : 1 } , { \"id\" : 2 } ] }");
    ASSERT(result.has_value());

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
    std::optional<JsonObject> result = readJson(
        "{ \"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\" }");
    ASSERT(result.has_value());
    ASSERT(result->getString("s") == "a\"b\\c\bd\fe\nf\rg\th");

    std::optional<JsonObject> empty = readJson("{ \"s\" : \"\" }");
    ASSERT(empty.has_value());
    ASSERT(empty->getString("s") == "");
}

TEST(testReadJsonFailure)
{
    static constexpr const char* inputs[] = {
        "",
        "junk",
        "\"a\" : 1",
        "[1,2]",
        "{",
        "{ \"a\" : }",
        "{ : 1 }",
        "{ \"a\" 1 }",
        "{ \"a\" : \"unterminated }",
        "{ \"a\" : 1, \"b\" }",
        "{ \"a\" : [ 1, 2 }",
        "{ \"a\" : [ 1, 2",
        "{ \"a\" : 1, \"b\" : }",
    };

    for (const char* input : inputs)
    {
        setError("");
        ASSERT(!readJson(input).has_value());
        ASSERT(getError().length() > 0);
    }
}

TEST(testWriteReadJsonRoundTrip)
{
    JsonObject obj{
        "name", "hurdy",
        "count", 42.0,
        "valid", true,
        "empty", "",
        "list", JsonArray{1.0, -2.5, false, "x"},
        "child", JsonObject{"pi", 3.5},
    };

    std::string out = writeJson(obj);
    ASSERT(out.length() >= 2);

    std::optional<JsonObject> json = readJson(out);
    ASSERT(json.has_value());
    ASSERT(json->count() == 6);
    ASSERT(json->getString("name") == "hurdy");
    ASSERT(json->getNumber("count") == 42.0);
    ASSERT(json->getBoolean("valid") == true);
    ASSERT(json->getString("empty") == "");
    ASSERT(json->getArray("list").count() == 4);
    ASSERT(json->getObject("child").count() == 1);

    std::string out2 = writeJson(*json);
    ASSERT(out2 == out);
}

TEST(testReadWriteJsonRoundTrip)
{
    std::optional<JsonObject> result = readJson(
        "{ \"a\" : 1 , \"b\" : \"str\" , \"c\" : [ true, false, -2.5, \"x\" ] , \"d\" : { \"e\" : [] } }");
    ASSERT(result.has_value());

    std::string out = writeJson(*result);
    ASSERT(out ==
        "{\n\t\"a\" : 1,\n\t\"b\" : \"str\",\n\t\"c\" : [\n\t\ttrue,\n\t\tfalse,\n\t\t-2.5,\n\t\t\"x\"\n\t],\n\t\"d\" : {\n\t\t\"e\" : [\n\t\t]\n\t}\n}");

    std::optional<JsonObject> json = readJson(out);
    ASSERT(json.has_value());
    ASSERT(json->count() == 4);
    ASSERT(json->getNumber("a") == 1.0);
    ASSERT(json->getString("b") == "str");
    ASSERT(json->getArray("c").count() == 4);
    ASSERT(json->getObject("d").count() == 1);
    ASSERT(json->getObject("d").getArray("e").count() == 0);
}
