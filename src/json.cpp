#include "hg/json.hpp"

#include "hg/error.hpp"
#include "hg/utility.hpp"
#include "hg/memory.hpp"

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
    values.erase(values.begin() + assertCast<i64>(idx));
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
        },
        [&](const JsonNull&) { str += "null"; }
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

    bool expectValue = false;
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
            if (expectValue)
                return false;
            ++count;
            head = count;
            return true;
        }
        expectValue = false;

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
        expectValue = true;
    }
}

static bool readJsonArray(JsonArray& val, std::string_view str, u32& head)
{
    u32 count = head;
    if (count == str.length() || str[count] != '[')
        return false;
    ++count;

    bool expectValue = false;
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
            if (expectValue)
                return false;
            ++count;
            head = count;
            return true;
        }
        expectValue = false;

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
        expectValue = true;
    }
}

static bool readHex4(u32& val, std::string_view str, u32 head)
{
    // head points at 'u', four hex digits must follow
    if (head + 4 >= str.length())
        return false;

    u32 ret = 0;
    for (u32 i = 1; i <= 4; ++i)
    {
        char c = str[head + i];
        u32 d;
        if (c >= '0' && c <= '9')
            d = assertCast<u32>(c - '0');
        else if (c >= 'a' && c <= 'f')
            d = assertCast<u32>(c - 'a') + 10;
        else if (c >= 'A' && c <= 'F')
            d = assertCast<u32>(c - 'A') + 10;
        else
            return false;
        ret = (ret << 4) | d;
    }
    val = ret;
    return true;
}

template<typename S>
static void appendUtf8(S& str, u32 code)
{
    if (code < 0x80)
    {
        str += static_cast<char>(code);
    }
    else if (code < 0x800)
    {
        str += static_cast<char>(0xC0 | (code >> 6));
        str += static_cast<char>(0x80 | (code & 0x3F));
    }
    else if (code < 0x10000)
    {
        str += static_cast<char>(0xE0 | (code >> 12));
        str += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        str += static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
        str += static_cast<char>(0xF0 | (code >> 18));
        str += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        str += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        str += static_cast<char>(0x80 | (code & 0x3F));
    }
}

static bool readJsonString(std::string& val, std::string_view str, u32& head)
{
    u32 count = head;
    if (count == str.length() || str[count] != '"')
        return false;

    ArenaScope scratch = getScratch();
    StringTemp temp{scratch};

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

            if (str[count] == 'u')
            {
                u32 code = 0;
                if (!readHex4(code, str, count))
                    return false;

                if (code >= 0xD800 && code <= 0xDBFF)
                {
                    // a low surrogate must follow as \uXXXX
                    if (count + 10 >= str.length() ||
                        str[count + 5] != '\\' || str[count + 6] != 'u')
                        return false;

                    u32 low = 0;
                    if (!readHex4(low, str, count + 6))
                        return false;
                    if (low < 0xDC00 || low > 0xDFFF)
                        return false;

                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    count += 10;
                }
                else if (code >= 0xDC00 && code <= 0xDFFF)
                {
                    return false;
                }
                else
                {
                    count += 4;
                }

                appendUtf8(temp, code);

                ++count;
                if (count == str.length())
                    return false;
                continue;
            }

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
    while (count < str.length() && str[count] != ',' && str[count] != '}' &&
        str[count] != ']' && !isWhitespace(str[count]))
    {
        ++count;
    }
    std::string_view valStr = {str.data() + head, count - head};

    if (isInteger(valStr))
    {
        val = assertCast<f64>(stringToInteger(valStr));
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

static bool readJsonNull(std::string_view str, u32& head)
{
    if (str.length() - head >= 4 && std::string_view{str.data() + head, 4} == "null")
    {
        head += 4;
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

    if (readJsonNull(str, head))
    {
        val = JsonNull{};
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

    while (head < json.length() && isWhitespace(json[head]))
    {
        ++head;
    }
    if (head != json.length())
    {
        setError("Could not parse JSON: trailing data after object");
        return {};
    }

    return ret;
}

} // namespace hg
