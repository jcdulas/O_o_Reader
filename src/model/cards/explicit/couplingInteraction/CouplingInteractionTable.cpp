#include "oor/model/cards/explicit/couplingInteraction/CouplingInteractionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<CouplingInteractionCard> CouplingInteractionFormat::read(FieldReader& reader)
{
    CouplingInteractionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.couplingId1 = reader.requiredInteger(2, "couplingId1");
    card.couplingId2 = reader.integer(3, "couplingId2", 0);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CouplingInteractionFormat::check(const CouplingInteractionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.couplingId1, "couplingId1"))
        return problem;
    return checkOptionalId(card.couplingId2, "couplingId2");
}

void CouplingInteractionFormat::write(const CouplingInteractionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.couplingId1);
    writer.integerUnlessDefault(3, card.couplingId2, 0);
}

std::uint32_t CouplingInteractionTable::append(const CouplingInteractionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CouplingInteractionTable::store(std::uint32_t row, const CouplingInteractionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CouplingInteractionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CouplingInteractionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CouplingInteractionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CouplingInteractionCard& card = m_cards.edit(row);
    visitId(visitor, "couplingId1", ReferenceTarget::Coupling, card.couplingId1);
    visitId(visitor, "couplingId2", ReferenceTarget::Coupling, card.couplingId2);
}

} // namespace oor::model
