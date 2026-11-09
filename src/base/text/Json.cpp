#include "oor/base/text/Json.hpp"

#include "oor/base/errors/Errors.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace oor::base
{

namespace
{

const char* typeName(JsonValue::Type type)
{
    switch (type)
    {
    case JsonValue::Type::Null:
        return "null";
    case JsonValue::Type::Bool:
        return "a boolean";
    case JsonValue::Type::Number:
        return "a number";
    case JsonValue::Type::Text:
        return "a text";
    case JsonValue::Type::Array:
        return "an array";
    case JsonValue::Type::Object:
        return "an object";
    }
    return "a value";
}

[[noreturn]] void wrongType(const std::string& what, const char* expected, JsonValue::Type actual)
{
    throw UsageError(what + " is " + expected + ", not " + typeName(actual));
}

} // namespace

// A recursive-descent reader over the text.
class JsonReader
{
public:
    explicit JsonReader(std::string_view text) : m_text(text) {}

    JsonValue readDocument()
    {
        JsonValue value = readValue();
        skipSpaces();
        if (m_at != m_text.size())
            fail("unexpected text after the value");
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& message) const
    {
        throw UsageError("JSON: " + message + " at character " + std::to_string(m_at));
    }

    void skipSpaces()
    {
        while (m_at < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_at])) != 0)
            ++m_at;
    }

    bool consume(std::string_view word)
    {
        if (m_text.substr(m_at, word.size()) != word)
            return false;
        m_at += word.size();
        return true;
    }

    JsonValue readValue()
    {
        skipSpaces();
        if (m_at >= m_text.size())
            fail("a value is missing");
        const char first = m_text[m_at];
        if (first == '{')
            return readObject();
        if (first == '[')
            return readArray();
        if (first == '"')
            return JsonValue::makeText(readString());
        if (consume("true"))
            return JsonValue::makeBool(true);
        if (consume("false"))
            return JsonValue::makeBool(false);
        if (consume("null"))
            return JsonValue();
        return JsonValue::makeNumber(readNumber());
    }

    JsonValue readObject()
    {
        ++m_at; // '{'
        std::vector<std::pair<std::string, JsonValue>> members;
        skipSpaces();
        if (consume("}"))
            return JsonValue::makeObject(std::move(members));
        while (true)
        {
            skipSpaces();
            if (m_at >= m_text.size() || m_text[m_at] != '"')
                fail("a member name (a text) is expected");
            std::string name = readString();
            skipSpaces();
            if (!consume(":"))
                fail("':' is expected after a member name");
            members.emplace_back(std::move(name), readValue());
            skipSpaces();
            if (consume("}"))
                return JsonValue::makeObject(std::move(members));
            if (!consume(","))
                fail("',' or '}' is expected");
        }
    }

    // An array. While its items are numbers (or null) they go to a vector of doubles; the first item of another kind
    // turns the numbers read so far into items.
    JsonValue readArray()
    {
        ++m_at; // '['
        JsonValue array;
        array.m_type = JsonValue::Type::Array;
        array.m_numbersOnly = true;
        skipSpaces();
        if (consume("]"))
            return array;
        while (true)
        {
            skipSpaces();
            const bool numberAhead =
                m_at < m_text.size() &&
                (m_text[m_at] == '-' || std::isdigit(static_cast<unsigned char>(m_text[m_at])) != 0);
            if (array.m_numbersOnly && numberAhead)
                array.m_numbers.push_back(readNumber());
            else if (array.m_numbersOnly && consume("null"))
                array.m_numbers.push_back(std::numeric_limits<double>::quiet_NaN());
            else
            {
                if (array.m_numbersOnly)
                {
                    for (const double number : array.m_numbers)
                        array.m_items.push_back(std::isnan(number) ? JsonValue() : JsonValue::makeNumber(number));
                    array.m_numbers.clear();
                    array.m_numbersOnly = false;
                }
                array.m_items.push_back(readValue());
            }
            skipSpaces();
            if (consume("]"))
                return array;
            if (!consume(","))
                fail("',' or ']' is expected");
        }
    }

    std::string readString()
    {
        ++m_at; // '"'
        std::string text;
        while (m_at < m_text.size() && m_text[m_at] != '"')
        {
            char character = m_text[m_at++];
            if (character == '\\')
            {
                if (m_at >= m_text.size())
                    fail("the text ends inside an escape");
                const char escaped = m_text[m_at++];
                switch (escaped)
                {
                case 'n':
                    character = '\n';
                    break;
                case 't':
                    character = '\t';
                    break;
                case 'r':
                    character = '\r';
                    break;
                case 'b':
                    character = '\b';
                    break;
                case 'f':
                    character = '\f';
                    break;
                case 'u':
                {
                    // a code point of the basic plane, written as UTF-8
                    if (m_at + 4 > m_text.size())
                        fail("\\u needs four hexadecimal digits");
                    const unsigned long code = std::strtoul(std::string(m_text.substr(m_at, 4)).c_str(), nullptr, 16);
                    m_at += 4;
                    if (code < 0x80)
                        text += static_cast<char>(code);
                    else if (code < 0x800)
                    {
                        text += static_cast<char>(0xC0 | (code >> 6));
                        text += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    else
                    {
                        text += static_cast<char>(0xE0 | (code >> 12));
                        text += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        text += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    continue;
                }
                default:
                    character = escaped; // '"', '\\', '/'
                    break;
                }
            }
            text += character;
        }
        if (m_at >= m_text.size())
            fail("a text is not closed");
        ++m_at; // '"'
        return text;
    }

    double readNumber()
    {
        // the longest run of the characters a JSON number is made of
        std::size_t end = m_at;
        while (end < m_text.size() &&
               (std::isdigit(static_cast<unsigned char>(m_text[end])) != 0 || m_text[end] == '-' ||
                m_text[end] == '+' || m_text[end] == '.' || m_text[end] == 'e' || m_text[end] == 'E'))
            ++end;
        if (end == m_at)
            fail("a value is expected");
        const std::string number(m_text.substr(m_at, end - m_at));
        char* stop = nullptr;
        const double value = std::strtod(number.c_str(), &stop);
        if (stop != number.c_str() + number.size())
            fail("'" + number + "' is not a number");
        m_at = end;
        return value;
    }

    std::string_view m_text;
    std::size_t m_at = 0;
};

bool JsonValue::asBool(const std::string& what) const
{
    if (m_type != Type::Bool)
        wrongType(what, "a boolean", m_type);
    return m_bool;
}

double JsonValue::asNumber(const std::string& what) const
{
    if (m_type != Type::Number)
        wrongType(what, "a number", m_type);
    return m_number;
}

const std::string& JsonValue::asText(const std::string& what) const
{
    if (m_type != Type::Text)
        wrongType(what, "a text", m_type);
    return m_text;
}

const std::vector<JsonValue>& JsonValue::asArray(const std::string& what) const
{
    if (m_type != Type::Array)
        wrongType(what, "an array", m_type);
    if (m_numbersOnly && m_items.size() != m_numbers.size())
    {
        m_items.clear();
        for (const double number : m_numbers)
            m_items.push_back(std::isnan(number) ? JsonValue() : makeNumber(number));
    }
    return m_items;
}

const std::vector<double>& JsonValue::asNumbers(const std::string& what) const
{
    if (m_type != Type::Array || !m_numbersOnly)
        throw UsageError(what + " is an array of numbers, not " +
                         (m_type == Type::Array ? std::string("an array of other values") : typeName(m_type)));
    return m_numbers;
}

const std::vector<std::pair<std::string, JsonValue>>& JsonValue::asObject(const std::string& what) const
{
    if (m_type != Type::Object)
        wrongType(what, "an object", m_type);
    return m_members;
}

const JsonValue* JsonValue::member(std::string_view name) const
{
    for (const auto& [key, value] : m_members)
        if (key == name)
            return &value;
    return nullptr;
}

JsonValue JsonValue::parse(std::string_view text)
{
    return JsonReader(text).readDocument();
}

JsonValue JsonValue::makeBool(bool value)
{
    JsonValue made;
    made.m_type = Type::Bool;
    made.m_bool = value;
    return made;
}

JsonValue JsonValue::makeNumber(double value)
{
    JsonValue made;
    made.m_type = Type::Number;
    made.m_number = value;
    return made;
}

JsonValue JsonValue::makeText(std::string value)
{
    JsonValue made;
    made.m_type = Type::Text;
    made.m_text = std::move(value);
    return made;
}

JsonValue JsonValue::makeArray(std::vector<JsonValue> items)
{
    JsonValue made;
    made.m_type = Type::Array;
    made.m_items = std::move(items);
    return made;
}

JsonValue JsonValue::makeObject(std::vector<std::pair<std::string, JsonValue>> members)
{
    JsonValue made;
    made.m_type = Type::Object;
    made.m_members = std::move(members);
    return made;
}

std::string jsonQuoted(std::string_view text)
{
    std::string quoted = "\"";
    for (const char character : text)
    {
        switch (character)
        {
        case '"':
            quoted += "\\\"";
            break;
        case '\\':
            quoted += "\\\\";
            break;
        case '\n':
            quoted += "\\n";
            break;
        case '\r':
            quoted += "\\r";
            break;
        case '\t':
            quoted += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                char escape[8];
                std::snprintf(escape, sizeof(escape), "\\u%04x", static_cast<unsigned>(character));
                quoted += escape;
            }
            else
            {
                quoted += character;
            }
        }
    }
    return quoted + "\"";
}

void appendJsonNumber(std::string& out, double value, bool singlePrecision)
{
    if (!std::isfinite(value))
    {
        out += "null";
        return;
    }
    char buffer[32];
    const auto result = singlePrecision ? std::to_chars(buffer, buffer + sizeof(buffer), static_cast<float>(value))
                                        : std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, result.ptr);
}

} // namespace oor::base
