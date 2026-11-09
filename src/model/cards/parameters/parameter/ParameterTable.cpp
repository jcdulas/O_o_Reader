#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"

namespace oor::model
{

std::optional<CardProblem> ParameterFormat::check(const ParameterCard& card)
{
    if (card.name.empty())
        return CardProblem{"card.missing-field", "the card needs a name", "parameterName"};
    return std::nullopt;
}

std::optional<ParameterCard> ParameterFormat::read(FieldReader& reader)
{
    ParameterCard card;
    card.name = std::string(reader.text(1));
    if (card.name.empty())
    {
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " card needs a name",
                     "parameterName");
        return card;
    }
    const std::string_view first = reader.text(2);
    if (first.empty())
        return card;
    if (!reader.isBlank(3))
    {
        // Two values: a complex parameter.
        card.kind = ParameterKind::Complex;
        card.realValue = reader.requiredReal(2, "value1");
        card.imaginaryValue = reader.requiredReal(3, "value2");
    }
    else if (const auto integer = base::NumberParser::parseInteger(first))
    {
        if (*integer < INT32_MIN || *integer > INT32_MAX)
        {
            reader.error("card.bad-value", "the integer value does not fit in 32 bits", "value1");
            return card;
        }
        card.kind = ParameterKind::Integer;
        card.integerValue = static_cast<std::int32_t>(*integer);
    }
    else if (const auto real = base::NumberParser::parseReal(first))
    {
        card.kind = ParameterKind::Real;
        card.realValue = *real;
    }
    else
    {
        card.kind = ParameterKind::Text;
        card.textValue = std::string(first); // a word is a neutral name (Yes, Automatic), case-sensitive
    }
    // A parameter of the format reference given a value of another kind (PARAM,WTMASS,YES) is kept as written, with a
    // warning: the solver refuses it, but the card is the deck's. The options a format reference entry lists are for
    // the validation check.
    if (reader.failed())
        return card;
    if (const auto problem = ParameterCatalogue::instance().problemWith(card);
        problem && problem->fit == ParameterFit::WrongKind)
        reader.warning("card.parameter-kind", problem->message, "value1");
    return card;
}

void ParameterFormat::write(const ParameterCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    switch (card.kind)
    {
    case ParameterKind::Blank:
        break;
    case ParameterKind::Integer:
        writer.integer(2, card.integerValue);
        break;
    case ParameterKind::Real:
        writer.real(2, card.realValue);
        break;
    case ParameterKind::Text:
        writer.text(2, card.textValue);
        break;
    case ParameterKind::Complex:
        writer.real(2, card.realValue);
        writer.real(3, card.imaginaryValue);
        break;
    }
}

std::uint32_t ParameterTable::append(const ParameterCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_integerValue.push(card.integerValue);
    m_realValue.push(card.realValue);
    m_imaginaryValue.push(card.imaginaryValue);
    m_textValue.push(card.textValue);
    return row;
}

ParameterCard ParameterTable::card(std::uint32_t row) const
{
    ParameterCard card;
    card.name = m_name.get(row);
    card.kind = static_cast<ParameterKind>(m_kind.get(row));
    card.integerValue = m_integerValue.get(row);
    card.realValue = m_realValue.get(row);
    card.imaginaryValue = m_imaginaryValue.get(row);
    card.textValue = m_textValue.get(row);
    return card;
}

void ParameterTable::store(std::uint32_t row, const ParameterCard& card)
{
    m_name.set(row, card.name);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_integerValue.set(row, card.integerValue);
    m_realValue.set(row, card.realValue);
    m_imaginaryValue.set(row, card.imaginaryValue);
    m_textValue.set(row, card.textValue);
}

void ParameterTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_kind.keepRows(rows);
    m_integerValue.keepRows(rows);
    m_realValue.keepRows(rows);
    m_imaginaryValue.keepRows(rows);
    m_textValue.keepRows(rows);
}

} // namespace oor::model
