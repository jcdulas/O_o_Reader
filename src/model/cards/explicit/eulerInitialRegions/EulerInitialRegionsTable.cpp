#include "oor/model/cards/explicit/eulerInitialRegions/EulerInitialRegionsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerInitialRegionsCard> EulerInitialRegionsFormat::read(FieldReader& reader)
{
    EulerInitialRegionsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.regionGroupId = reader.requiredInteger(2, "regionGroupId");
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerInitialRegionsFormat::check(const EulerInitialRegionsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkRequiredId(card.regionGroupId, "regionGroupId");
}

void EulerInitialRegionsFormat::write(const EulerInitialRegionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.regionGroupId);
}

std::uint32_t EulerInitialRegionsTable::append(const EulerInitialRegionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerInitialRegionsTable::store(std::uint32_t row, const EulerInitialRegionsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerInitialRegionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerInitialRegionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerInitialRegionsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "regionGroupId", ReferenceTarget::EulerInitialRegions, m_cards.edit(row).regionGroupId);
}

} // namespace oor::model
