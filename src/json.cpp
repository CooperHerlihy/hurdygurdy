#include "hg/json.hpp"

#include "hg/macros.hpp"
#include "hg/error.hpp"
#include "hg/memory.hpp"
#include "hg/strings.hpp"

#include <optional>
#include <string_view>

namespace hg {

JsonValue& JsonObject::getValue(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return *val;
}

JsonObject& JsonObject::getObject(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->object();
}

JsonArray& JsonObject::getArray(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->array();
}

std::string& JsonObject::getString(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->string();
}

f64& JsonObject::getNumber(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->number();
}

bool& JsonObject::getBoolean(std::string_view name)
{
    JsonValue* val = values.get(name);
    HG_ASSERT(val != nullptr);
    return val->boolean();
}

JsonValue& JsonObject::setValue(std::string_view name, JsonValue&& val)
{
    return *values.add(std::string{name}, std::move(val));
}

JsonObject& JsonObject::setObject(std::string_view name, JsonObject&& val)
{
    return values.add(std::string{name}, std::move(val))->object();
}

JsonArray& JsonObject::setArray(std::string_view name, JsonArray&& val)
{
    return values.add(std::string{name}, std::move(val))->array();
}

std::string& JsonObject::setString(std::string_view name, std::string_view val)
{
    return values.add(std::string{name}, std::string{val})->string();
}

f64& JsonObject::setNumber(std::string_view name, f64 val)
{
    return values.add(std::string{name}, std::move(val))->number();
}

bool& JsonObject::setBoolean(std::string_view name, bool val)
{
    return values.add(std::string{name}, std::move(val))->boolean();
}

u64 JsonArray::count() const
{
    return values.size();
}

JsonValue& JsonArray::pushValue(JsonValue&& val)
{
    return values.emplace_back(std::move(val));
}

JsonObject& JsonArray::pushObject(JsonObject&& val)
{
    return values.emplace_back(std::move(val)).object();
}

JsonArray& JsonArray::pushArray(JsonArray&& val)
{
    return values.emplace_back(std::move(val)).array();
}

std::string& JsonArray::pushString(std::string_view val)
{
    return values.emplace_back(std::string{val}).string();
}

f64& JsonArray::pushNumber(f64 val)
{
    return values.emplace_back(std::move(val)).number();
}

bool& JsonArray::pushBoolean(bool val)
{
    return values.emplace_back(std::move(val)).boolean();
}

JsonValue JsonArray::removeValue(u64 idx)
{
    JsonValue ret = std::move(values[idx]);
    values.erase(values.begin() + static_cast<i64>(idx));
    return ret;
}

JsonObject JsonArray::removeObject(u64 idx)
{
    return std::move(removeValue(idx).object());
}

JsonArray JsonArray::removeArray(u64 idx)
{
    return std::move(removeValue(idx).array());
}

std::string JsonArray::removeString(u64 idx)
{
    return std::move(removeValue(idx).string());
}

f64 JsonArray::removeNumber(u64 idx)
{
    return removeValue(idx).number();
}

bool JsonArray::removeBoolean(u64 idx)
{
    return removeValue(idx).boolean();
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

std::string& JsonArray::getString(u64 idx)
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
    return values.data();
}

const JsonValue* JsonArray::begin() const
{
    return values.data();
}

JsonValue* JsonArray::end()
{
    return values.data() + values.size();
}

const JsonValue* JsonArray::end() const
{
    return values.data() + values.size();
}

void addIndent(std::string& str, u32 indent)
{
    for (u32 i = 0; i < indent; ++i)
    {
        str += "\t";
    }
}

static void writeJsonValue(std::string& str, u32 indent, const JsonValue& value);

static void writeJsonObject(std::string& str, u32 indent, const JsonObject& object)
{
    str += "{\n";
    object.forEach([&](std::string_view id, const JsonValue& val)
    {
        addIndent(str, indent + 1);
        str += "\"";
        str += id;
        str += "\"";
        str += " : ";
        writeJsonValue(str, indent + 1, val);
        str += ",\n";
    });
    if (object.count() > 0)
    {
        str.erase(str.length() - 1);
        str[str.length() - 1] = '\n';
    }
    addIndent(str, indent);
    str += "}";
}

static void writeJsonArray(std::string& str, u32 indent, const JsonArray& array)
{
    str += "[\n";
    for (const JsonValue& val : array)
    {
        addIndent(str, indent + 1);
        writeJsonValue(str, indent + 1, val);
        str += ",\n";
    }
    if (array.count() > 0)
    {
        str.erase(str.length() - 1);
        str[str.length() - 1] = '\n';
    }
    addIndent(str, indent);
    str += "]";
}

static void writeJsonString(std::string& ret, std::string_view val)
{
    ret += "\"";
    for (char c : val)
    {
        switch (c)
        {
            case '"':
                ret += "\\\"";
                break;
            case '\\':
                ret += "\\\\";
                break;
            case '\b':
                ret += "\\b";
                break;
            case '\f':
                ret += "\\f";
                break;
            case '\n':
                ret += "\\n";
                break;
            case '\r':
                ret += "\\r";
                break;
            case '\t':
                ret += "\\t";
                break;
            default:
                ret += c;
                break;
        }
    }
    ret += '"';
}

static void writeJsonValue(std::string& str, u32 indent, const JsonValue& value)
{
    value.match(
        [&](const JsonObject& val) { writeJsonObject(str, indent, val); },
        [&](const JsonArray& val) { writeJsonArray(str, indent, val); },
        [&](const std::string& val) { writeJsonString(str, val); },
        [&](const f64& val) { str += floatToString(val, 6); },
        [&](const bool& val)
        {
            if (val)
                str += "true";
            else
                str += "false";
        }
    );
}

std::string writeJson(const JsonObject& json)
{
    std::string str;
    writeJsonObject(str, 0, json);
    return str;
}

static bool readJsonValue(JsonValue& val, std::string_view str, u32& head);

static bool readJsonString(std::string& val, std::string_view str, u32& head);

static bool readJsonObject(JsonObject& val, std::string_view str, u32& head)
{
    u32 count = head;
    if (count == str.length() || str[count] != '{')
        return false;
    ++count;

    while (true)
    {
        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length())
            return false;

        if (str[count] == '}')
        {
            ++count;
            head = count;
            return true;
        }

        std::string nextName{};
        if (!readJsonString(nextName, str, count))
            return false;

        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length() || str[count] != ':')
            return false;
        ++count;

        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        JsonValue newVal{};
        if (!readJsonValue(newVal, str, count))
            return false;

        val.setValue(std::move(nextName), std::move(newVal));

        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count < str.length() && str[count] == '}')
        {
            ++count;
            head = count;
            return true;
        }

        if (count == str.length() || str[count] != ',')
            return false;
        ++count;
    }
}

static bool readJsonArray(JsonArray& val, std::string_view str, u32& head)
{
    u32 count = head;
    if (count == str.length() || str[count] != '[')
        return false;
    ++count;

    while (true)
    {
        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count == str.length())
            return false;

        if (str[count] == ']')
        {
            ++count;
            head = count;
            return true;
        }

        JsonValue newVal{};
        if (!readJsonValue(newVal, str, count))
            return false;

        val.pushValue(std::move(newVal));

        while (count < str.length() && isWhitespace(str[count]))
        {
            ++count;
        }

        if (count < str.length() && str[count] == ']')
        {
            ++count;
            head = count;
            return true;
        }

        if (count == str.length() || str[count] != ',')
            return false;
        ++count;
    }
}

static bool readJsonString(std::string& val, std::string_view str, u32& head)
{
    u32 count = head;
    if (count == str.length() || str[count] != '"')
        return false;

    ArenaScope scratch = getScratch();
    string_temp temp{scratch};

    ++count;
    if (count == str.length())
        return false;

    while (str[count] != '"')
    {
        if (str[count] == '\n')
            return false;

        if (str[count] == '\\')
        {
            ++count;
            if (count == str.length())
                return false;

            switch (str[count])
            {
                case '"':
                    temp += '"';
                    break;
                case '\\':
                    temp += '\\';
                    break;
                case 'b':
                    temp += '\b';
                    break;
                case 'f':
                    temp += '\f';
                    break;
                case 'n':
                    temp += '\n';
                    break;
                case 'r':
                    temp += '\r';
                    break;
                case 't':
                    temp += '\t';
                    break;
                default:
                    temp += '\\';
                    temp += str[count];
            }
        }
        else
        {
            temp += str[count];
        }

        ++count;
        if (count == str.length())
            return false;
    }

    val = temp;
    head = count + 1;
    return true;
}

static bool readJsonNum(f64& val, std::string_view str, u32& head)
{
    u32 count = head;
    while (count < str.length() && str[count] != ',' && !isWhitespace(str[count]))
    {
        ++count;
    }
    std::string_view valStr = {str.data() + head, count - head};

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

static bool readJsonBool(bool& val, std::string_view str, u32& head)
{
    if (str.length() - head >= 4 && std::string_view{str.data() + head, 4} == "true")
    {
        val = true;
        head += 4;
        return true;
    }
    if (str.length() - head >= 5 && std::string_view{str.data() + head, 5} == "false")
    {
        val = false;
        head += 5;
        return true;
    }
    return false;
}

static bool readJsonValue(JsonValue& val, std::string_view str, u32& head)
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

    std::string strVal{};
    if (readJsonString(strVal, str, head))
    {
        val = std::move(strVal);
        return true;
    }

    JsonArray arrVal{};
    if (readJsonArray(arrVal, str, head))
    {
        val = std::move(arrVal);
        return true;
    }

    JsonObject objVal{};
    if (readJsonObject(objVal, str, head))
    {
        val = std::move(objVal);
        return true;
    }

    return false;
}

std::optional<JsonObject> readJson(std::string_view json)
{
    JsonObject ret{};

    u32 head = 0;
    if (!readJsonObject(ret, json, head))
    {
        setError("Could not parse JSON");
        return {};
    }
    return ret;
}

} // namespace hg
