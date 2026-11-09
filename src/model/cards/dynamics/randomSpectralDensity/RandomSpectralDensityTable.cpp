#include "oor/model/cards/dynamics/randomSpectralDensity/RandomSpectralDensityTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<RandomSpectralDensityCard> RandomSpectralDensityFormat::read(FieldReader& reader)
{
    RandomSpectralDensityCard card;
    card.id = reader.requiredInteger(1, "id");
    card.subcaseJ = reader.requiredInteger(2, "subcaseJ");
    card.subcaseK = reader.requiredInteger(3, "subcaseK");
    card.realPart = reader.real(4, "realPart", 0.0);
    card.imaginaryPart = reader.real(5, "imaginaryPart", 0.0);
    card.tableId = reader.integer(6, "tableId", 0);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("tableId"), "tableId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RandomSpectralDensityFormat::check(const RandomSpectralDensityCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the random analysis set ID must be a positive integer", "id"};
    if (card.subcaseJ < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "subcaseJ"};
    if (card.subcaseK < card.subcaseJ)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("subcaseJ"), "subcaseK"};
    if (!std::isfinite(card.realPart) || !std::isfinite(card.imaginaryPart))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("imaginaryPart") +
                               " must be finite numbers",
                           "realPart"};
    if (card.subcaseJ == card.subcaseK && (!(card.realPart > 0.0) || card.imaginaryPart != 0.0))
        return CardProblem{"card.bad-value",
                           "an auto spectrum (" + CardProblem::fieldNamed("subcaseJ") + " = " +
                               CardProblem::fieldNamed("subcaseK") + ") has " + std::string(CardProblem::FIELD) +
                               " greater than zero and " + CardProblem::fieldNamed("imaginaryPart") + " equal to zero",
                           "realPart"};
    if (card.tableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "tableId"};
    return std::nullopt;
}

void RandomSpectralDensityFormat::write(const RandomSpectralDensityCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.subcaseJ);
    writer.integer(3, card.subcaseK);
    writer.realUnlessDefault(4, card.realPart, 0.0);
    writer.realUnlessDefault(5, card.imaginaryPart, 0.0);
    if (card.tableId != 0)
        writer.integer(6, card.tableId);
}

void RandomSpectralDensityTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (m_tableId.get(row) != 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::RandomTable, m_tableId.get(row)));
}

std::uint32_t RandomSpectralDensityTable::append(const RandomSpectralDensityCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_subcaseJ.push(card.subcaseJ);
    m_subcaseK.push(card.subcaseK);
    m_realPart.push(card.realPart);
    m_imaginaryPart.push(card.imaginaryPart);
    m_tableId.push(card.tableId);
    return row;
}

RandomSpectralDensityCard RandomSpectralDensityTable::card(std::uint32_t row) const
{
    return RandomSpectralDensityCard{m_id.get(row),       m_subcaseJ.get(row),      m_subcaseK.get(row),
                                     m_realPart.get(row), m_imaginaryPart.get(row), m_tableId.get(row)};
}

void RandomSpectralDensityTable::store(std::uint32_t row, const RandomSpectralDensityCard& card)
{
    m_id.set(row, card.id);
    m_subcaseJ.set(row, card.subcaseJ);
    m_subcaseK.set(row, card.subcaseK);
    m_realPart.set(row, card.realPart);
    m_imaginaryPart.set(row, card.imaginaryPart);
    m_tableId.set(row, card.tableId);
}

void RandomSpectralDensityTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_subcaseJ.keepRows(rows);
    m_subcaseK.keepRows(rows);
    m_realPart.keepRows(rows);
    m_imaginaryPart.keepRows(rows);
    m_tableId.keepRows(rows);
}

} // namespace oor::model
