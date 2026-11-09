#include "oor/model/cards/dynamics/complexEigenPoles/ComplexEigenPolesTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<ComplexEigenPolesCard> ComplexEigenPolesFormat::read(FieldReader& reader)
{
    ComplexEigenPolesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.first.realPart = reader.requiredReal(2, "realPart1");
    card.first.imaginaryPart = reader.requiredReal(3, "imaginaryPart1");
    card.first.multiplicity = reader.requiredInteger(4, "multiplicity1");
    // The second pole is there when any of its three fields is.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
    {
        ComplexEigenPolesPole second;
        second.realPart = reader.requiredReal(5, "realPart2");
        second.imaginaryPart = reader.requiredReal(6, "imaginaryPart2");
        second.multiplicity = reader.requiredInteger(7, "multiplicity2");
        card.second = second;
    }
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("multiplicity2"), "multiplicity2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ComplexEigenPolesFormat::check(const ComplexEigenPolesCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (!std::isfinite(card.first.realPart) || !std::isfinite(card.first.imaginaryPart))
        return CardProblem{"card.bad-value", "the coordinates of a pole must be finite numbers", "realPart1"};
    if (card.first.multiplicity < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "multiplicity1"};
    if (card.second)
    {
        if (!std::isfinite(card.second->realPart) || !std::isfinite(card.second->imaginaryPart))
            return CardProblem{"card.bad-value", "the coordinates of a pole must be finite numbers", "realPart2"};
        if (card.second->multiplicity < 1)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "multiplicity2"};
    }
    return std::nullopt;
}

void ComplexEigenPolesFormat::write(const ComplexEigenPolesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.first.realPart);
    writer.real(3, card.first.imaginaryPart);
    writer.integer(4, card.first.multiplicity);
    if (card.second)
    {
        writer.real(5, card.second->realPart);
        writer.real(6, card.second->imaginaryPart);
        writer.integer(7, card.second->multiplicity);
    }
}

std::uint32_t ComplexEigenPolesTable::append(const ComplexEigenPolesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_realPart1.push(card.first.realPart);
    m_imaginaryPart1.push(card.first.imaginaryPart);
    m_multiplicity1.push(card.first.multiplicity);
    m_realPart2.push(card.second ? card.second->realPart : 0.0);
    m_imaginaryPart2.push(card.second ? card.second->imaginaryPart : 0.0);
    m_multiplicity2.push(card.second ? card.second->multiplicity : 0);
    return row;
}

ComplexEigenPolesCard ComplexEigenPolesTable::card(std::uint32_t row) const
{
    ComplexEigenPolesCard card;
    card.id = m_id.get(row);
    card.first = ComplexEigenPolesPole{m_realPart1.get(row), m_imaginaryPart1.get(row), m_multiplicity1.get(row)};
    if (m_multiplicity2.get(row) != 0)
        card.second = ComplexEigenPolesPole{m_realPart2.get(row), m_imaginaryPart2.get(row), m_multiplicity2.get(row)};
    return card;
}

void ComplexEigenPolesTable::store(std::uint32_t row, const ComplexEigenPolesCard& card)
{
    m_id.set(row, card.id);
    m_realPart1.set(row, card.first.realPart);
    m_imaginaryPart1.set(row, card.first.imaginaryPart);
    m_multiplicity1.set(row, card.first.multiplicity);
    m_realPart2.set(row, card.second ? card.second->realPart : 0.0);
    m_imaginaryPart2.set(row, card.second ? card.second->imaginaryPart : 0.0);
    m_multiplicity2.set(row, card.second ? card.second->multiplicity : 0);
}

void ComplexEigenPolesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_realPart1.keepRows(rows);
    m_imaginaryPart1.keepRows(rows);
    m_multiplicity1.keepRows(rows);
    m_realPart2.keepRows(rows);
    m_imaginaryPart2.keepRows(rows);
    m_multiplicity2.keepRows(rows);
}

} // namespace oor::model
