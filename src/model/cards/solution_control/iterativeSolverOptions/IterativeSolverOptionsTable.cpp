#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsTable.hpp"

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsCatalogue.hpp"

#include <charconv>
#include <cmath>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIRST_OPTION_FIELD = 2;

// A real as the option writes it: the shortest text that reads back as the same number, in the E notation ("1.0E-7",
// "2.5E10") so that the value looks the way the format reference writes it. The compact exponent of the free format
// ("1.-7") is not used: the text of an option is not a field of the card.
std::string realText(double value)
{
    std::array<char, 40> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    const std::string text(buffer.data(), result.ptr);
    const std::size_t exponentAt = text.find('e');
    std::string mantissa = text.substr(0, exponentAt);
    if (mantissa.find('.') == std::string::npos)
        mantissa += ".0";
    if (exponentAt == std::string::npos)
        return mantissa;
    std::string exponent = text.substr(exponentAt + 1);
    const bool negative = !exponent.empty() && exponent.front() == '-';
    if (!exponent.empty() && (exponent.front() == '-' || exponent.front() == '+'))
        exponent.erase(0, 1);
    const std::size_t firstDigit = exponent.find_first_not_of('0');
    exponent = firstDigit == std::string::npos ? "0" : exponent.substr(firstDigit);
    return mantissa + "E" + (negative ? "-" : "") + exponent;
}

std::string valueText(const IterativeSolverOptionsValue& value)
{
    if (const auto* integer = std::get_if<std::int32_t>(&value))
        return base::NumberFormatter::formatInteger(*integer);
    if (const auto* number = std::get_if<double>(&value))
        return realText(*number);
    return std::get<std::string>(value);
}

// The value of an item "OPTION=VALUE" as the option takes it: a listed option has the kind of its entry (a real option
// takes an integer as a real that is whole); an option the format does not list is an integer, a real or else a word. A
// value that is not of the kind is reported.
std::optional<IterativeSolverOptionsValue> parseValue(FieldReader& reader, const IterativeSolverOptionsInfo* info,
                                                      std::string_view text)
{
    const auto integer = base::NumberParser::parseInteger(text);
    const auto number = base::NumberParser::parseReal(text);
    const std::optional<std::int32_t> id = integer ? base::toEntityId(*integer) : std::nullopt;
    if (info == nullptr)
    {
        if (id)
            return IterativeSolverOptionsValue(*id);
        if (number)
            return IterativeSolverOptionsValue(*number);
        return IterativeSolverOptionsValue(std::string(text));
    }
    switch (info->kind)
    {
    case IterativeSolverOptionsKind::Word:
        return IterativeSolverOptionsValue(std::string(text));
    case IterativeSolverOptionsKind::Real:
        if (number)
            return IterativeSolverOptionsValue(*number);
        break;
    case IterativeSolverOptionsKind::Integer:
        if (id)
            return IterativeSolverOptionsValue(*id);
        break;
    }
    reader.error("card.bad-value",
                 std::string(info->name) + " must be " + iterativeSolverOptionsRuleText(*info) + ", not \"" +
                     std::string(text) + "\"",
                 "optionValues");
    return std::nullopt;
}

// The name of an option that the writer can give back: a letter, then letters, digits and underscores.
bool isOptionName(const std::string& name)
{
    if (name.empty())
        return false;
    for (std::size_t index = 0; index < name.size(); ++index)
    {
        const char character = name[index];
        const bool letter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        const bool digit = character >= '0' && character <= '9';
        if (!letter && (index == 0 || (!digit && character != '_')))
            return false;
    }
    return true;
}

} // namespace

std::optional<IterativeSolverOptionsCard> IterativeSolverOptionsFormat::read(FieldReader& reader)
{
    IterativeSolverOptionsCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    // Each field after the SID that is not blank is one item, OPTION=VALUE.
    for (int field = FIRST_OPTION_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        const std::string_view text = reader.text(field);
        const std::size_t equals = text.find('=');
        const std::string_view name =
            equals == std::string_view::npos ? std::string_view{} : base::trimAscii(text.substr(0, equals));
        const std::string_view value =
            equals == std::string_view::npos ? std::string_view{} : base::trimAscii(text.substr(equals + 1));
        if (name.empty() || value.empty())
        {
            reader.error(
                "card.bad-value",
                "field " + std::to_string(field + 1) + ": expected OPTION=VALUE, found \"" + std::string(text) + "\"",
                "options");
            break;
        }
        IterativeSolverOptionsOption option;
        option.name = std::string(name);
        const std::optional<IterativeSolverOptionsValue> parsed =
            parseValue(reader, findIterativeSolverOption(option.name), value);
        if (!parsed)
            break;
        option.value = *parsed;
        card.options.push_back(std::move(option));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IterativeSolverOptionsFormat::check(const IterativeSolverOptionsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    for (std::size_t index = 0; index < card.options.size(); ++index)
    {
        const IterativeSolverOptionsOption& option = card.options[index];
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(option.name);
        if (!isOptionName(label))
            return CardProblem{"card.bad-value",
                               "an option name is a letter followed by letters, digits and underscores (at most 16)",
                               "options"};
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if ((card.options[earlier].name == option.name))
                return CardProblem{"card.bad-value", label + " appears twice", "options"};
        }
        if (const IterativeSolverOptionsInfo* info = findIterativeSolverOption(option.name))
        {
            if (const std::string problem = iterativeSolverOptionsValueProblem(*info, option.value); !problem.empty())
                return CardProblem{"card.bad-value", problem, "optionValues"};
            continue;
        }
        // A name the format does not list is kept: its value must still read back from the text of the item.
        if (const auto* number = std::get_if<double>(&option.value); number != nullptr && !std::isfinite(*number))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number", "optionValues"};
        if (const auto* word = std::get_if<std::string>(&option.value))
        {
            if (word->empty())
                return CardProblem{"card.bad-value", "the value of " + label + " must not be blank", "optionValues"};
            for (const char character : *word)
            {
                const auto code = static_cast<unsigned char>(character);
                if (code <= 0x20 || code >= 0x7F || character == ',' || character == '$' || character == '=' ||
                    character == '\'' || character == '"')
                    return CardProblem{
                        "card.bad-value",
                        "the value of " + label +
                            " must not contain a blank, a comma, an equals sign, a dollar sign or a quote",
                        "optionValues"};
            }
        }
    }
    return std::nullopt;
}

void IterativeSolverOptionsFormat::write(const IterativeSolverOptionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_OPTION_FIELD;
    for (const IterativeSolverOptionsOption& option : card.options)
    {
        const std::string name = std::string(option.name);
        writer.text(field++, name + "=" + valueText(option.value));
    }
}

std::uint32_t IterativeSolverOptionsTable::append(const IterativeSolverOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void IterativeSolverOptionsTable::store(std::uint32_t row, const IterativeSolverOptionsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void IterativeSolverOptionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void IterativeSolverOptionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
