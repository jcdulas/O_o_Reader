#include "oor/model/cards/explicit/beltProp/BeltPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<BeltPropCard> BeltPropFormat::read(FieldReader& reader)
{
    BeltPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.loadingTableId = reader.requiredInteger(2, "loadingTableId");
    card.unloadingTableId = reader.requiredInteger(3, "unloadingTableId");
    card.density = reader.requiredReal(4, "density");
    card.damping1 = reader.real(5, "damping1", 0.1);
    card.damping2 = reader.real(6, "damping2", 0.1);
    card.slackTableId = reader.integer(7, "slackTableId", 0);
    card.prestrainTableId = reader.integer(8, "prestrainTableId", 0);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeltPropFormat::check(const BeltPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.loadingTableId, "loadingTableId"))
        return problem;
    if (const auto problem = checkRequiredId(card.unloadingTableId, "unloadingTableId"))
        return problem;
    if (const auto problem = checkReal(card.density, "density", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.damping1, "damping1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.damping2, "damping2", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.slackTableId, "slackTableId"))
        return problem;
    return checkOptionalId(card.prestrainTableId, "prestrainTableId");
}

void BeltPropFormat::write(const BeltPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.loadingTableId);
    writer.integer(3, card.unloadingTableId);
    writer.real(4, card.density);
    writer.realUnlessDefault(5, card.damping1, 0.1);
    writer.realUnlessDefault(6, card.damping2, 0.1);
    writer.integerUnlessDefault(7, card.slackTableId, 0);
    writer.integerUnlessDefault(8, card.prestrainTableId, 0);
}

std::uint32_t BeltPropTable::append(const BeltPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BeltPropTable::store(std::uint32_t row, const BeltPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BeltPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BeltPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BeltPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    BeltPropCard& card = m_cards.edit(row);
    visitId(visitor, "loadingTableId", ReferenceTarget::DynamicTable, card.loadingTableId);
    visitId(visitor, "unloadingTableId", ReferenceTarget::DynamicTable, card.unloadingTableId);
    visitId(visitor, "slackTableId", ReferenceTarget::DynamicTable, card.slackTableId);
    visitId(visitor, "prestrainTableId", ReferenceTarget::DynamicTable, card.prestrainTableId);
}

} // namespace oor::model
