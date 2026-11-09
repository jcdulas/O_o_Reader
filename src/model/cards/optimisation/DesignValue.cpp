#include "oor/model/cards/optimisation/DesignValue.hpp"

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

// A word of a design card is a neutral name (a card type, a word of the format) or a label: long enough for the
// neutral names of the code.

bool isWordCharacter(char character) noexcept
{
    const auto code = static_cast<unsigned char>(character);
    return code >= 0x20 && code != 0x7F;
}

} // namespace

DesignValue DesignValue::ofWord(std::string value)
{
    DesignValue result;
    result.kind = Kind::Word;
    result.word = std::move(value);
    return result;
}

std::string designValueProblem(const DesignValue& value)
{
    switch (value.kind)
    {
    case DesignValue::Kind::Blank:
    case DesignValue::Kind::Integer:
        return {};
    case DesignValue::Kind::Real:
        return std::isfinite(value.real) ? std::string{} : "a real must be a finite number";
    case DesignValue::Kind::Word:
        break;
    }
    if (value.word.empty())
        return "a word must not be blank";
    for (const char character : value.word)
    {
        if (!isWordCharacter(character))
            return "a word must not contain a control character";
    }
    return {};
}

DesignValue readDesignValue(FieldReader& reader, int field, const char* name)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return {};
    if (const auto integer = base::NumberParser::parseInteger(text))
    {
        if (*integer < std::numeric_limits<std::int32_t>::min() || *integer > std::numeric_limits<std::int32_t>::max())
        {
            reader.error("card.bad-value", std::string(name) + " is not a 32-bit integer", name);
            return {};
        }
        return DesignValue::ofInteger(static_cast<std::int32_t>(*integer));
    }
    if (const auto real = base::NumberParser::parseReal(text))
        return DesignValue::ofReal(*real);
    DesignValue word = DesignValue::ofWord(std::string(text));
    if (const std::string problem = designValueProblem(word); !problem.empty())
    {
        reader.error("card.bad-value", std::string(name) + ": " + problem, name);
        return {};
    }
    return word;
}

void writeDesignValue(FieldWriter& writer, int field, const DesignValue& value)
{
    switch (value.kind)
    {
    case DesignValue::Kind::Blank:
        break;
    case DesignValue::Kind::Integer:
        writer.integer(field, value.integer);
        break;
    case DesignValue::Kind::Real:
        writer.real(field, value.real);
        break;
    case DesignValue::Kind::Word:
        writer.text(field, value.word);
        break;
    }
}

std::string describeDesignValue(const DesignValue& value)
{
    switch (value.kind)
    {
    case DesignValue::Kind::Blank:
        return "blank";
    case DesignValue::Kind::Integer:
        return std::to_string(value.integer);
    case DesignValue::Kind::Real:
        return base::NumberFormatter::formatRealFree(value.real);
    case DesignValue::Kind::Word:
        break;
    }
    return value.word;
}

std::optional<CardProblem> checkDesignLabel(const std::string& label, const char* field)
{
    return checkNameWord(label, field);
}

} // namespace oor::model
