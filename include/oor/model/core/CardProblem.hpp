#pragma once
#include <string>
#include <string_view>

namespace oor::model
{
// A card validation problem, with the same neutral names as its record and API.
struct CardProblem
{
    static constexpr std::string_view FIELD = "{field}";
    [[nodiscard]] static std::string fieldNamed(std::string_view name) { return std::string(name); }
    [[nodiscard]] static std::string cardNamed(std::string_view name) { return std::string(name); }
    const char* code = "card.bad-value";
    std::string message;
    const char* field = "";
    [[nodiscard]] std::string text() const
    {
        std::string result;
        std::size_t position = 0;
        while (position < message.size())
        {
            const std::size_t open = message.find('{', position);
            if (open == std::string::npos)
            {
                result.append(message, position);
                break;
            }
            result.append(message, position, open - position);
            const std::size_t close = message.find('}', open + 1);
            if (close == std::string::npos)
            {
                result.append(message, open);
                break;
            }
            const std::string_view token(message.data() + open + 1, close - open - 1);
            if (token == "field")
                result += field;
            else if (token.starts_with("field:"))
                result += token.substr(6);
            else if (token.starts_with("card:"))
                result += token.substr(5);
            else
                result.append(message, open, close - open + 1);
            position = close + 1;
        }
        return result;
    }
};
} // namespace oor::model
