#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

std::string listOf(std::initializer_list<std::string_view> words)
{
    std::string text;
    for (const std::string_view word : words)
    {
        if (!text.empty())
            text += ", ";
        text += word;
    }
    return text;
}

} // namespace

void requireUnusedBlank(FieldReader& reader, std::initializer_list<int> fields, int lastField)
{
    for (const int field : fields)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return;
        }
    }
    if (reader.lastUsedField() > lastField)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has " + std::to_string(lastField) + " fields", "");
}

std::string readKeyword(FieldReader& reader, int field, const char* name, std::initializer_list<std::string_view> words,
                        std::string_view fallback)
{
    if (reader.isBlank(field))
        return std::string(fallback);
    const std::string word(reader.text(field));
    if (std::find(words.begin(), words.end(), std::string_view(word)) == words.end())
    {
        reader.error("card.bad-value", std::string(name) + " must be one of " + listOf(words) + ", not " + word, name);
        return std::string(fallback);
    }
    return word;
}

std::optional<CardProblem> checkKeyword(std::string_view word, const char* name,
                                        std::initializer_list<std::string_view> words)
{
    if (std::find(words.begin(), words.end(), word) == words.end())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be one of " + listOf(words),
                           name};
    return std::nullopt;
}

} // namespace oor::model
