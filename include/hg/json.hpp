#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"
#include "hg/sum.hpp"
#include "hg/maybe.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include <type_traits>

namespace hg {

struct JsonValue;
struct JsonObject;
struct JsonArray;

/**
 * Make a JsonValue from a string, number, bool, object or array
 */
template<typename T> requires (std::is_convertible_v<T&&, StringView> || std::is_constructible_v<JsonValue, T&&>)
JsonValue makeJsonValue(Arena* arena, T&& val);

/**
 * A json object
 */
struct JsonObject {
    /**
     * The named values
     */
    MapTemp<StringBuilder, JsonValue> values;

    /**
     * Construct empty
     */
    JsonObject() noexcept = default;

    /**
     * Construct with an arena
     */
    JsonObject(Arena* arena)
        : values{arena, 16}
    {};

    /**
     * Construct with an arena and alternating name-value pairs
     */
    template<typename... Ts> requires (sizeof...(Ts) % 2 == 0)
    JsonObject(Arena* arena, Ts&&... list)
        : values{arena, 16}
    {
        addValues(arena, std::forward<Ts>(list)...);
    }

    /**
     * Add alternating name-value pairs recursively
     */
    template<typename N, typename V, typename... Rest>
    void addValues(Arena* arena, N&& name, V&& val, Rest&&... rest)
    {
        values.add(StringBuilder{arena, std::forward<N>(name)}, makeJsonValue(arena, std::forward<V>(val)));
        if constexpr (sizeof...(Rest) != 0)
        {
            addValues(arena, std::forward<Rest>(rest)...);
        }
    }

    /**
     * The number of values
     */
    u64 count() const
    {
        return values.count;
    }

    /**
     * Get a value by name, panics if missing or wrong type
     */
    JsonValue& getValue(StringView name);

    /**
     * Get an object by name, panics if missing or wrong type
     */
    JsonObject& getObject(StringView name);

    /**
     * Get an array by name, panics if missing or wrong type
     */
    JsonArray& getArray(StringView name);

    /**
     * Get a string by name, panics if missing or wrong type
     */
    StringBuilder& getString(StringView name);

    /**
     * Get a number by name, panics if missing or wrong type
     */
    f64& getNumber(StringView name);

    /**
     * Get a boolean by name, panics if missing or wrong type
     */
    bool& getBoolean(StringView name);

    /**
     * Set a value by name, replacing any existing value
     */
    JsonValue& setValue(StringView name, JsonValue&& val);

    /**
     * Set an object by name, replacing any existing value
     */
    JsonObject& setObject(StringView name, JsonObject&& val);

    /**
     * Set an array by name, replacing any existing value
     */
    JsonArray& setArray(StringView name, JsonArray&& val);

    /**
     * Set a string by name, replacing any existing value
     */
    StringBuilder& setString(StringView name, StringView val);

    /**
     * Set a number by name, replacing any existing value
     */
    f64& setNumber(StringView name, f64 val);

    /**
     * Set a boolean by name, replacing any existing value
     */
    bool& setBoolean(StringView name, bool val);

    /**
     * Call fn for each name and value
     */
    template<typename F> requires std::is_invocable_r_v<void, F, StringView, JsonValue&>
    void forEach(F fn)
    {
        values.forEach(fn);
    }

    /**
     * Call fn for each name and value
     */
    template<typename F> requires std::is_invocable_r_v<void, F, StringView, const JsonValue&>
    void forEach(F fn) const
    {
        values.forEach(fn);
    }
};

/**
 * A json array
 */
struct JsonArray {
    /**
     * The values
     */
    ArrayTemp<JsonValue> values;

    /**
     * Construct empty
     */
    JsonArray() noexcept = default;

    /**
     * Construct with an arena
     */
    JsonArray(Arena* arena)
        : values{arena, 0, 16}
    {}

    /**
     * Construct with an arena and values
     */
    template<typename... Ts>
    JsonArray(Arena* arena, Ts&&... list)
        : values{arena, 0, sizeof...(Ts)}
    {
        (this->values.push(makeJsonValue(arena, std::forward<Ts>(list))), ...);
    }

    /**
     * The number of values
     */
    u64 count() const
    {
        return values.count;
    }

    /**
     * Push a value to the end
     */
    JsonValue& pushValue(JsonValue&& val);

    /**
     * Push an object to the end
     */
    JsonObject& pushObject(JsonObject&& val);

    /**
     * Push an array to the end
     */
    JsonArray& pushArray(JsonArray&& val);

    /**
     * Push a string to the end
     */
    StringBuilder& pushString(StringView val);

    /**
     * Push a number to the end
     */
    f64& pushNumber(f64 val);

    /**
     * Push a boolean to the end
     */
    bool& pushBoolean(bool val);

    /**
     * Remove the value at idx, shifting later values down
     */
    JsonValue removeValue(u64 idx);

    /**
     * Remove the object at idx, shifting later values down
     */
    JsonObject removeObject(u64 idx);

    /**
     * Remove the array at idx, shifting later values down
     */
    JsonArray removeArray(u64 idx);

    /**
     * Remove the string at idx, shifting later values down
     */
    StringBuilder removeString(u64 idx);

    /**
     * Remove the number at idx, shifting later values down
     */
    f64 removeNumber(u64 idx);

    /**
     * Remove the boolean at idx, shifting later values down
     */
    bool removeBoolean(u64 idx);

    /**
     * Get a value by index, asserts if out of bounds or wrong type
     */
    JsonValue& getValue(u64 idx);

    /**
     * Get an object by index, asserts if out of bounds or wrong type
     */
    JsonObject& getObject(u64 idx);

    /**
     * Get an array by index, asserts if out of bounds or wrong type
     */
    JsonArray& getArray(u64 idx);

    /**
     * Get a string by index, asserts if out of bounds or wrong type
     */
    StringBuilder& getString(u64 idx);

    /**
     * Get a number by index, asserts if out of bounds or wrong type
     */
    f64& getNumber(u64 idx);

    /**
     * Get a boolean by index, asserts if out of bounds or wrong type
     */
    bool& getBoolean(u64 idx);

    /**
     * Begin iterating over the values
     */
    JsonValue* begin();

    /**
     * End iterating over the values
     */
    JsonValue* end();

    /**
     * Begin iterating over the values
     */
    const JsonValue* begin() const;

    /**
     * End iterating over the values
     */
    const JsonValue* end() const;
};

/**
 * A json value, an object, array, string, number or boolean
 */
struct JsonValue {
    /**
     * The value
     */
    Sum<JsonObject, JsonArray, StringBuilder, f64, bool> val;

    /**
     * Construct empty
     */
    JsonValue() noexcept = default;

    /**
     * Construct an object value
     */
    JsonValue(JsonObject&& object)
        : val{std::move(object)}
    {}

    /**
     * Construct an array value
     */
    JsonValue(JsonArray&& array)
        : val{std::move(array)}
    {}

    /**
     * Construct a string value
     */
    JsonValue(StringBuilder&& string)
        : val{std::move(string)}
    {}

    /**
     * Construct a number value
     */
    JsonValue(f64 number)
        : val{number}
    {}

    /**
     * Construct a boolean value
     */
    JsonValue(bool boolean)
        : val{boolean}
    {}

    /**
     * Get the value as an object, asserts if wrong type
     */
    JsonObject& object()
    {
        return val.get<JsonObject>();
    }

    /**
     * Get the value as an array, asserts if wrong type
     */
    JsonArray& array()
    {
        return val.get<JsonArray>();
    }

    /**
     * Get the value as a string, asserts if wrong type
     */
    StringBuilder& string()
    {
        return val.get<StringBuilder>();
    }

    /**
     * Get the value as a number, asserts if wrong type
     */
    f64& number()
    {
        return val.get<f64>();
    }

    /**
     * Get the value as a boolean, asserts if wrong type
     */
    bool& boolean()
    {
        return val.get<bool>();
    }

    /**
     * Call the function matching the value type
     */
    template<typename... Fs>
    void match(Fs&&... fns)
    {
        val.match(std::forward<Fs>(fns)...);
    }

    /**
     * Call the function matching the value type
     */
    template<typename... Fs>
    void match(Fs&&... fns) const
    {
        val.match(std::forward<Fs>(fns)...);
    }
};

/**
 * Write out a json object to a string
 */
StringView writeJson(Arena* arena, const JsonObject& json);

/**
 * Read a json string to an object
 */
Maybe<JsonObject> readJson(Arena* arena, StringView json);

/**
 * Make a JsonValue from a string, number, bool, object or array
 */
template<typename T> requires (std::is_convertible_v<T&&, StringView> || std::is_constructible_v<JsonValue, T&&>)
JsonValue makeJsonValue(Arena* arena, T&& val)
{
    if constexpr (std::is_convertible_v<T&&, StringView>)
    {
        return JsonValue{StringBuilder{arena, StringView{std::forward<T>(val)}}};
    }
    else
    {
        return JsonValue{std::forward<T>(val)};
    }
}

} // namespace hg
