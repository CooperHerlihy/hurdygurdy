#pragma once

#include "hg/inttypes.hpp"
#include "hg/utility.hpp"
#include "hg/hash.hpp"

#include <type_traits>
#include <optional>
#include <variant>
#include <string_view>
#include <string>
#include <vector>

namespace hg {

struct JsonValue;
struct JsonObject;
struct JsonArray;

/**
 * Make a JsonValue from a string, number, bool, object or array
 */
template<typename T> requires (std::is_convertible_v<T&&, std::string> || std::is_constructible_v<JsonValue, T&&>)
JsonValue makeJsonValue(T&& val);

/**
 * A json object
 */
struct JsonObject {
    /**
     * The named values
     */
    HashMap<std::string, JsonValue> values{};

    /**
     * Construct with an arena and alternating name-value pairs
     */
    template<typename... Ts> requires (sizeof...(Ts) % 2 == 0)
    JsonObject(Ts&&... list)
    {
        addValues(std::forward<Ts>(list)...);
    }

    /**
     * Add alternating name-value pairs recursively
     */
    void addValues()
    {
    }

    template<typename N, typename V, typename... Rest> requires (sizeof...(Rest) % 2 == 0)
    void addValues(N&& name, V&& val, Rest&&... rest)
    {
        values.add(std::forward<N>(name), makeJsonValue(std::forward<V>(val)));
        if constexpr (sizeof...(Rest) != 0)
        {
            addValues(std::forward<Rest>(rest)...);
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
    JsonValue& getValue(std::string_view name);

    /**
     * Get an object by name, panics if missing or wrong type
     */
    JsonObject& getObject(std::string_view name);

    /**
     * Get an array by name, panics if missing or wrong type
     */
    JsonArray& getArray(std::string_view name);

    /**
     * Get a string by name, panics if missing or wrong type
     */
    std::string& getString(std::string_view name);

    /**
     * Get a number by name, panics if missing or wrong type
     */
    f64& getNumber(std::string_view name);

    /**
     * Get a boolean by name, panics if missing or wrong type
     */
    bool& getBoolean(std::string_view name);

    /**
     * Set a value by name, replacing any existing value
     */
    JsonValue& setValue(std::string_view name, JsonValue&& val);

    /**
     * Set an object by name, replacing any existing value
     */
    JsonObject& setObject(std::string_view name, JsonObject&& val);

    /**
     * Set an array by name, replacing any existing value
     */
    JsonArray& setArray(std::string_view name, JsonArray&& val);

    /**
     * Set a string by name, replacing any existing value
     */
    std::string& setString(std::string_view name, std::string_view val);

    /**
     * Set a number by name, replacing any existing value
     */
    f64& setNumber(std::string_view name, f64 val);

    /**
     * Set a boolean by name, replacing any existing value
     */
    bool& setBoolean(std::string_view name, bool val);

    /**
     * Call fn for each name and value
     */
    template<typename F> requires std::is_invocable_r_v<void, F, std::string_view, JsonValue&>
    void forEach(F fn)
    {
        values.forEach(fn);
    }

    /**
     * Call fn for each name and value
     */
    template<typename F> requires std::is_invocable_r_v<void, F, std::string_view, const JsonValue&>
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
    std::vector<JsonValue> values;

    /**
     * Construct empty
     */
    JsonArray() noexcept = default;

    /**
     * Move construct
     */
    JsonArray(JsonArray&& other) noexcept = default;

    /**
     * Move assign
     */
    JsonArray& operator=(JsonArray&& other) noexcept = default;

    /**
     * Construct with values
     */
    template<typename... Ts>
    JsonArray(Ts&&... list)
    {
        (this->values.emplace_back(makeJsonValue(std::forward<Ts>(list))), ...);
    }

    /**
     * The number of values
     */
    u64 count() const;

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
    std::string& pushString(std::string_view val);

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
    std::string removeString(u64 idx);

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
    std::string& getString(u64 idx);

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
    std::variant<JsonObject, JsonArray, std::string, f64, bool> val;

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
    JsonValue(std::string&& string)
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
        return std::get<JsonObject>(val);
    }

    /**
     * Get the value as an array, asserts if wrong type
     */
    JsonArray& array()
    {
        return std::get<JsonArray>(val);
    }

    /**
     * Get the value as a string, asserts if wrong type
     */
    std::string& string()
    {
        return std::get<std::string>(val);
    }

    /**
     * Get the value as a number, asserts if wrong type
     */
    f64& number()
    {
        return std::get<f64>(val);
    }

    /**
     * Get the value as a boolean, asserts if wrong type
     */
    bool& boolean()
    {
        return std::get<bool>(val);
    }

    /**
     * Call the function matching the value type
     */
    template<typename... Fs>
    auto match(Fs&&... fns)
    {
        return hg::match(val, std::forward<Fs>(fns)...);
    }

    /**
     * Call the function matching the value type
     */
    template<typename... Fs>
    auto match(Fs&&... fns) const
    {
        return hg::match(val, std::forward<Fs>(fns)...);
    }
};

/**
 * Write out a json object to a string
 */
std::string writeJson(const JsonObject& json);

/**
 * Read a json string to an object
 */
std::optional<JsonObject> readJson(std::string_view json);

/**
 * Make a JsonValue from a string, number, bool, object or array
 */
template<typename T> requires (std::is_convertible_v<T&&, std::string> || std::is_constructible_v<JsonValue, T&&>)
JsonValue makeJsonValue(T&& val)
{
    if constexpr (std::is_convertible_v<T&&, std::string>)
    {
        return JsonValue{std::string{std::forward<T>(val)}};
    }
    else
    {
        return JsonValue{std::forward<T>(val)};
    }
}

} // namespace hg
