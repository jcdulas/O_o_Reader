#include "oor/model/cards/solution_control/resultFileParams/ResultFileParamsTable.hpp"

namespace oor::model
{

namespace
{

// The integer that a card may have before the first pair: field 1 holds a whole number, never a name.
std::optional<int> leadingNumber(FieldReader& reader)
{
    const std::string_view text = reader.text(1);
    if (text.empty())
        return std::nullopt;
    if (const auto parsed = base::NumberParser::parseInteger(text))
    {
        if (const auto id = base::toEntityId(*parsed))
            return static_cast<int>(*id);
        reader.error("card.integer-too-large", "the number before the parameters does not fit in 32 bits",
                     "parameterName");
    }
    return std::nullopt;
}

} // namespace

std::optional<ResultFileParamsCard> ResultFileParamsFormat::read(FieldReader& reader)
{
    ResultFileParamsCard card;
    card.number = leadingNumber(reader);
    card.parameters = readHdf5Parameters(reader, card.number ? 2 : 1);
    if (card.parameters.empty() && !reader.failed())
        reader.error("card.missing-field",
                     "an " + std::string(reader.cardName()) + " needs at least one parameter and its value",
                     "parameterName");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ResultFileParamsFormat::check(const ResultFileParamsCard& card)
{
    if (card.number && *card.number < 0)
        return CardProblem{"card.bad-value", "the number before the parameters must not be negative", "parameterName"};
    if (card.parameters.empty())
        return CardProblem{"card.missing-field", "the card needs at least one parameter and its value",
                           "parameterName"};
    return checkHdf5Parameters(card.parameters);
}

void ResultFileParamsFormat::write(const ResultFileParamsCard& card, FieldWriter& writer)
{
    if (card.number)
        writer.integer(1, *card.number);
    writeHdf5Parameters(writer, card.number ? 2 : 1, card.parameters);
}

std::uint32_t ResultFileParamsTable::append(const ResultFileParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
