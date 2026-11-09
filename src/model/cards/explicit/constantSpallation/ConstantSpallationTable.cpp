#include "oor/model/cards/explicit/constantSpallation/ConstantSpallationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<ConstantSpallationCard> ConstantSpallationFormat::read(FieldReader& reader)
{
    ConstantSpallationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.minPressure = reader.real(2, "minPressure", 0.0);
    card.volumeCutoffTolerance = reader.real(3, "volumeCutoffTolerance", CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE);
    card.maxVoidFraction = reader.real(4, "maxVoidFraction", 0.0);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ConstantSpallationFormat::check(const ConstantSpallationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!std::isfinite(card.minPressure) || card.minPressure > 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a finite number that is not positive",
                           "minPressure"};
    if (const auto problem = checkReal(card.volumeCutoffTolerance, "volumeCutoffTolerance", RealRule::Positive))
        return problem;
    return checkReal(card.maxVoidFraction, "maxVoidFraction", RealRule::NonNegative);
}

void ConstantSpallationFormat::write(const ConstantSpallationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.minPressure, 0.0);
    writer.realUnlessDefault(3, card.volumeCutoffTolerance, CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE);
    writer.realUnlessDefault(4, card.maxVoidFraction, 0.0);
}

std::uint32_t ConstantSpallationTable::append(const ConstantSpallationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ConstantSpallationTable::store(std::uint32_t row, const ConstantSpallationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ConstantSpallationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ConstantSpallationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
