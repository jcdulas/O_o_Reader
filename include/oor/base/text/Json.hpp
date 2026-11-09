#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::base
{

// A JSON value, read from text: the arguments of a query sent by JavaScript, a results report. The numbers are doubles
// and the members of an object keep their order.
//
// An array whose items are all numbers (or null) is kept as a vector of doubles, null as NaN: a results report holds
// millions of numbers, and a full JsonValue for each would take twenty times the memory. Read such an array with
// `asNumbers`; `asArray` also works on it (it makes the items the first time it is asked).
class JsonValue
{
public:
    enum class Type
    {
        Null,
        Bool,
        Number,
        Text,
        Array,
        Object
    };

    JsonValue() = default;

    [[nodiscard]] Type type() const noexcept { return m_type; }
    [[nodiscard]] bool isNull() const noexcept { return m_type == Type::Null; }
    [[nodiscard]] bool isNumber() const noexcept { return m_type == Type::Number; }
    [[nodiscard]] bool isText() const noexcept { return m_type == Type::Text; }
    [[nodiscard]] bool isArray() const noexcept { return m_type == Type::Array; }
    [[nodiscard]] bool isObject() const noexcept { return m_type == Type::Object; }
    // An array of numbers only (null counts as a number: NaN); an empty array is one.
    [[nodiscard]] bool isNumberArray() const noexcept { return m_type == Type::Array && m_numbersOnly; }

    // The value; throws UsageError naming `what` when the value is of another type.
    [[nodiscard]] bool asBool(const std::string& what) const;
    [[nodiscard]] double asNumber(const std::string& what) const;
    [[nodiscard]] const std::string& asText(const std::string& what) const;
    [[nodiscard]] const std::vector<JsonValue>& asArray(const std::string& what) const;
    [[nodiscard]] const std::vector<std::pair<std::string, JsonValue>>& asObject(const std::string& what) const;
    // The numbers of an array of numbers (null is NaN); throws UsageError for anything else.
    [[nodiscard]] const std::vector<double>& asNumbers(const std::string& what) const;

    // The member of an object, or null when there is none.
    [[nodiscard]] const JsonValue* member(std::string_view name) const;

    // Reads a JSON text; throws UsageError with the position of the first error.
    [[nodiscard]] static JsonValue parse(std::string_view text);

    static JsonValue makeBool(bool value);
    static JsonValue makeNumber(double value);
    static JsonValue makeText(std::string value);
    static JsonValue makeArray(std::vector<JsonValue> items);
    static JsonValue makeObject(std::vector<std::pair<std::string, JsonValue>> members);

private:
    friend class JsonReader;

    Type m_type = Type::Null;
    bool m_bool = false;
    double m_number = 0.0;
    std::string m_text;
    bool m_numbersOnly = false;
    std::vector<double> m_numbers;          // an array of numbers only
    mutable std::vector<JsonValue> m_items; // the other arrays; for an array of numbers, made by asArray when asked
    std::vector<std::pair<std::string, JsonValue>> m_members;
};

// Writing JSON: a text between quotes with the characters JSON escapes, and a number in its shortest form that reads
// back to the same value (`singlePrecision`: the shortest form of the float, for values that were single precision).
// NaN and the infinities are written as null.
[[nodiscard]] std::string jsonQuoted(std::string_view text);
void appendJsonNumber(std::string& out, double value, bool singlePrecision = false);

} // namespace oor::base
