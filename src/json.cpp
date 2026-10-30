#include "hg/json.hpp"

#include "hg/error.hpp"

namespace hg {

JsonValue& JsonObject::getValue(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return *val;
}

JsonObject& JsonObject::getObject(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->object();
}

JsonArray& JsonObject::getArray(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->array();
}

StringBuilder& JsonObject::getString(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->string();
}

f64& JsonObject::getNumber(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->number();
}

bool& JsonObject::getBoolean(StringView name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->boolean();
}

JsonValue& JsonObject::setValue(StringView name, JsonValue&& val)
{
    return *values.add(StringBuilder{values.arena, name}, std::move(val));
}

JsonObject& JsonObject::setObject(StringView name, JsonObject&& val)
{
    return values.add(StringBuilder{values.arena, name}, std::move(val))->object();
}

JsonArray& JsonObject::setArray(StringView name, JsonArray&& val)
{
    return values.add(StringBuilder{values.arena, name}, std::move(val))->array();
}

StringBuilder& JsonObject::setString(StringView name, StringView val)
{
    return values.add(StringBuilder{values.arena, name}, StringBuilder{values.arena, val})->string();
}

f64& JsonObject::setNumber(StringView name, f64 val)
{
    return values.add(StringBuilder{values.arena, name}, std::move(val))->number();
}

bool& JsonObject::setBoolean(StringView name, bool val)
{
    return values.add(StringBuilder{values.arena, name}, std::move(val))->boolean();
}

JsonValue& JsonArray::pushValue(JsonValue&& val)
{
    return values.push(std::move(val));
}

JsonObject& JsonArray::pushObject(JsonObject&& val)
{
    return values.push(std::move(val)).object();
}

JsonArray& JsonArray::pushArray(JsonArray&& val)
{
    return values.push(std::move(val)).array();
}

StringBuilder& JsonArray::pushString(StringView val)
{
    return values.push(StringBuilder{values.arena, val}).string();
}

f64& JsonArray::pushNumber(f64 val)
{
    return values.push(std::move(val)).number();
}

bool& JsonArray::pushBoolean(bool val)
{
    return values.push(std::move(val)).boolean();
}

JsonValue JsonArray::removeValue(u64 idx)
{
    return values.removeShift(idx);
}

JsonObject JsonArray::removeObject(u64 idx)
{
    return std::move(values.removeShift(idx).object());
}

JsonArray JsonArray::removeArray(u64 idx)
{
    return std::move(values.removeShift(idx).array());
}

StringBuilder JsonArray::removeString(u64 idx)
{
    return std::move(values.removeShift(idx).string());
}

f64 JsonArray::removeNumber(u64 idx)
{
    return values.removeShift(idx).number();
}

bool JsonArray::removeBoolean(u64 idx)
{
    return values.removeShift(idx).boolean();
}

JsonValue& JsonArray::getValue(u64 idx)
{
    return values[idx];
}

JsonObject& JsonArray::getObject(u64 idx)
{
    return values[idx].object();
}

JsonArray& JsonArray::getArray(u64 idx)
{
    return values[idx].array();
}

StringBuilder& JsonArray::getString(u64 idx)
{
    return values[idx].string();
}

f64& JsonArray::getNumber(u64 idx)
{
    return values[idx].number();
}

bool& JsonArray::getBoolean(u64 idx)
{
    return values[idx].boolean();
}

JsonValue* JsonArray::begin()
{
    return values.begin();
}

const JsonValue* JsonArray::begin() const
{
    return values.begin();
}

JsonValue* JsonArray::end()
{
    return values.end();
}

const JsonValue* JsonArray::end() const
{
    return values.end();
}

void addIndent(StringBuilder& str, u32 indent)
{
    for (u32 i = 0; i < indent; ++i)
    {
        str.append('\t');
    }
}

static void writeJsonValue(StringBuilder& str, u32 indent, const JsonValue& value);

static void writeJsonObject(StringBuilder& str, u32 indent, const JsonObject& object)
{
    str.append("{\n");
    object.forEach([&](StringView id, const JsonValue& val)
    {
        addIndent(str, indent + 1);
        str.append('"');
        str.append(id);
        str.append('"');
        str.append(" : ");
        writeJsonValue(str, indent + 1, val);
        str.append(",\n");
    });
    if (object.count() > 0)
        str[--str.length - 1] = '\n';
    addIndent(str, indent);
    str.append("}");
}

static void writeJsonArray(StringBuilder& str, u32 indent, const JsonArray& array)
{
    str.append("[\n");
    for (const JsonValue& val : array)
    {
        addIndent(str, indent + 1);
        writeJsonValue(str, indent + 1, val);
        str.append(",\n");
    }
    if (array.count() > 0)
        str[--str.length - 1] = '\n';
    addIndent(str, indent);
    str.append("]");
}

static void writeJsonString(StringBuilder& ret, StringView val)
{
    ret.append('"');
    for (char c : val)
    {
        switch (c)
        {
            case '"':
                ret.append("\\\"");
                break;
            case '\\':
                ret.append("\\\\");
                break;
            case '\b':
                ret.append("\\b");
                break;
            case '\f':
                ret.append("\\f");
                break;
            case '\n':
                ret.append("\\n");
                break;
            case '\r':
                ret.append("\\r");
                break;
            case '\t':
                ret.append("\\t");
                break;
            default:
                ret.append(c);
                break;
        }
    }
    ret.append('"');
}

static void writeJsonValue(StringBuilder& str, u32 indent, const JsonValue& value)
{
    value.match(
        [&](const JsonObject& val) { writeJsonObject(str, indent, val); },
        [&](const JsonArray& val) { writeJsonArray(str, indent, val); },
        [&](const StringView& val) { writeJsonString(str, val); },
        [&](const f64& val)
        {
            StringBuilder num = floatToString(getScratch(), val, 6);
            str.append(num);
        },
        [&](const bool& val)
        {
            if (val)
                str.append("true");
            else
                str.append("false");
        }
    );
}

StringView writeJson(Arena* arena, const JsonObject& json)
{
    StringBuilder str{arena};
    writeJsonObject(str, 0, json);
    return str;
}

static bool readJsonValue(Arena* arena, JsonValue& val, StringView str, u32& head);

static bool readJsonString(Arena* arena, StringBuilder& val, StringView str, u32& head);

static bool readJsonObject(Arena* arena, JsonObject& val, StringView str, u32& head)
{
    u32 count = head;
    if (count == str.length || str[count] != '{')
        return false;
    ++count;

    while (true)
    {
        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length)
            return false;

        if (str[count] == '}')
        {
            ++count;
            head = count;
            return true;
        }

        StringBuilder nextName{arena};
        if (!readJsonString(arena, nextName, str, count))
            return false;

        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length || str[count] != ':')
            return false;
        ++count;

        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        JsonValue newVal{};
        if (!readJsonValue(arena, newVal, str, count))
            return false;

        val.setValue(std::move(nextName), std::move(newVal));

        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count < str.length && str[count] == '}')
        {
            ++count;
            head = count;
            return true;
        }

        if (count == str.length || str[count] != ',')
            return false;
        ++count;
    }
}

static bool readJsonArray(Arena* arena, JsonArray& val, StringView str, u32& head)
{
    u32 count = head;
    if (count == str.length || str[count] != '[')
        return false;
    ++count;

    while (true)
    {
        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length)
            return false;

        if (str[count] == ']')
        {
            ++count;
            head = count;
            return true;
        }

        JsonValue newVal{};
        if (!readJsonValue(arena, newVal, str, count))
            return false;

        val.pushValue(std::move(newVal));

        while (count < str.length && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count < str.length && str[count] == ']')
        {
            ++count;
            head = count;
            return true;
        }

        if (count == str.length || str[count] != ',')
            return false;
        ++count;
    }
}

static bool readJsonString(Arena* arena, StringBuilder& val, StringView str, u32& head)
{
    ArenaScope scratch = getScratch(arena);

    u32 count = head;
    if (count == str.length || str[count] != '"')
        return false;

    StringBuilder temp{scratch};

    ++count;
    if (count == str.length)
        return false;

    while (str[count] != '"')
    {
        if (str[count] == '\n')
            return false;

        if (str[count] == '\\')
        {
            ++count;
            if (count == str.length)
                return false;

            switch (str[count])
            {
                case '"':
                    temp.append('"');
                    break;
                case '\\':
                    temp.append('\\');
                    break;
                case 'b':
                    temp.append('\b');
                    break;
                case 'f':
                    temp.append('\f');
                    break;
                case 'n':
                    temp.append('\n');
                    break;
                case 'r':
                    temp.append('\r');
                    break;
                case 't':
                    temp.append('\t');
                    break;
                default:
                    temp.append('\\');
                    temp.append(str[count]);
            }
        }
        else
        {
            temp.append(str[count]);
        }

        ++count;
        if (count == str.length)
            return false;
    }

    val = {arena, temp};
    head = count + 1;
    return true;
}

static bool readJsonNum(f64& val, StringView str, u32& head)
{
    u32 count = head;
    while (count < str.length && str[count] != ',' && !isWhitespace(str[count]))
    {
        ++count;
    }
    StringView valStr = {str.chars + head, count - head};

    if (isInteger(valStr))
    {
        val = static_cast<f64>(stringToInteger(valStr));
        head = count;
        return true;
    }
    else if (isFloat(valStr))
    {
        val = stringToFloat(valStr);
        head = count;
        return true;
    }
    return 0;
}

static bool readJsonBool(bool& val, StringView str, u32& head)
{
    if (str.length - head >= 4 && StringView{str.chars + head, 4} == "true")
    {
        val = true;
        head += 4;
        return true;
    }
    if (str.length - head >= 5 && StringView{str.chars + head, 5} == "false")
    {
        val = false;
        head += 5;
        return true;
    }
    return false;
}

static bool readJsonValue(Arena* arena, JsonValue& val, StringView str, u32& head)
{
    bool boolVal;
    if (readJsonBool(boolVal, str, head))
    {
        val = boolVal;
        return true;
    }

    f64 numVal;
    if (readJsonNum(numVal, str, head))
    {
        val = numVal;
        return true;
    }

    StringBuilder strVal;
    if (readJsonString(arena, strVal, str, head))
    {
        val = std::move(strVal);
        return true;
    }

    JsonArray arrVal{arena};
    if (readJsonArray(arena, arrVal, str, head))
    {
        val = std::move(arrVal);
        return true;
    }

    JsonObject objVal{arena};
    if (readJsonObject(arena, objVal, str, head))
    {
        val = std::move(objVal);
        return true;
    }

    return false;
}

Maybe<JsonObject> readJson(Arena* arena, StringView json)
{
    JsonObject ret{arena};

    u32 head = 0;
    if (!readJsonObject(arena, ret, json, head))
    {
        setError("Could not parse JSON");
        return {};
    }
    return ret;
}

} // namespace hg
