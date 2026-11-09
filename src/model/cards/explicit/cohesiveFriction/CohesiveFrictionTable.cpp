#include "oor/model/cards/explicit/cohesiveFriction/CohesiveFrictionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CohesiveFrictionCard> CohesiveFrictionFormat::read(FieldReader& reader)
{
    CohesiveFrictionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.maxStress = reader.real(2, "maxStress", 0.0);
    card.friction = reader.real(3, "friction", 0.0);
    card.referenceVelocity = reader.real(4, "referenceVelocity", 0.0);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CohesiveFrictionFormat::check(const CohesiveFrictionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.maxStress, "maxStress", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.friction, "friction", RealRule::NonNegative))
        return problem;
    return checkReal(card.referenceVelocity, "referenceVelocity", RealRule::NonNegative);
}

void CohesiveFrictionFormat::write(const CohesiveFrictionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.maxStress, 0.0);
    writer.realUnlessDefault(3, card.friction, 0.0);
    writer.realUnlessDefault(4, card.referenceVelocity, 0.0);
}

std::uint32_t CohesiveFrictionTable::append(const CohesiveFrictionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CohesiveFrictionTable::store(std::uint32_t row, const CohesiveFrictionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CohesiveFrictionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CohesiveFrictionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
