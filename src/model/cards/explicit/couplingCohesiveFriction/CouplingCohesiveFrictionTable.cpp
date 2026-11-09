#include "oor/model/cards/explicit/couplingCohesiveFriction/CouplingCohesiveFrictionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CouplingCohesiveFrictionCard> CouplingCohesiveFrictionFormat::read(FieldReader& reader)
{
    CouplingCohesiveFrictionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.setId = reader.requiredInteger(2, "setId");
    card.subsurfaceId = reader.integer(3, "subsurfaceId", 0);
    card.cohesiveFrictionModelId = reader.requiredInteger(4, "cohesiveFrictionModelId");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CouplingCohesiveFrictionFormat::check(const CouplingCohesiveFrictionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkOptionalId(card.subsurfaceId, "subsurfaceId"))
        return problem;
    return checkRequiredId(card.cohesiveFrictionModelId, "cohesiveFrictionModelId");
}

void CouplingCohesiveFrictionFormat::write(const CouplingCohesiveFrictionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.setId);
    writer.integerUnlessDefault(3, card.subsurfaceId, 0);
    writer.integer(4, card.cohesiveFrictionModelId);
}

std::uint32_t CouplingCohesiveFrictionTable::append(const CouplingCohesiveFrictionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void CouplingCohesiveFrictionTable::store(std::uint32_t row, const CouplingCohesiveFrictionCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void CouplingCohesiveFrictionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void CouplingCohesiveFrictionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void CouplingCohesiveFrictionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CouplingCohesiveFrictionCard& card = m_cards.edit(row);
    visitId(visitor, "subsurfaceId", ReferenceTarget::ContactSurface, card.subsurfaceId);
    visitId(visitor, "cohesiveFrictionModelId", ReferenceTarget::CohesiveFriction, card.cohesiveFrictionModelId);
}

} // namespace oor::model
