#include "oor/model/cards/explicit/eulerBodyForceSet/EulerBodyForceSetTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerBodyForceSetCard> EulerBodyForceSetFormat::read(FieldReader& reader)
{
    EulerBodyForceSetCard card;
    card.id = reader.requiredInteger(1, "id");
    card.regionSetId = reader.requiredInteger(2, "regionSetId");
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerBodyForceSetFormat::check(const EulerBodyForceSetCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkRequiredId(card.regionSetId, "regionSetId");
}

void EulerBodyForceSetFormat::write(const EulerBodyForceSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.regionSetId);
}

std::uint32_t EulerBodyForceSetTable::append(const EulerBodyForceSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerBodyForceSetTable::store(std::uint32_t row, const EulerBodyForceSetCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerBodyForceSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerBodyForceSetTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerBodyForceSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "regionSetId", ReferenceTarget::EulerRegionSet, m_cards.edit(row).regionSetId);
}

} // namespace oor::model
