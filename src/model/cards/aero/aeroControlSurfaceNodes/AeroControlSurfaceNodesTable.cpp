#include "oor/model/cards/aero/aeroControlSurfaceNodes/AeroControlSurfaceNodesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6;

} // namespace

std::optional<AeroControlSurfaceNodesCard> AeroControlSurfaceNodesFormat::read(FieldReader& reader)
{
    AeroControlSurfaceNodesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.label = std::string(reader.text(2));
    card.nodeSetId1 = reader.requiredInteger(4, "nodeSetId1");
    card.nodeSetId2 = reader.integer(6, "nodeSetId2", 0);
    // Fields 3 and 5 are the blank cells of the Card layout.
    requireBlankFields(reader, {3, 5}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroControlSurfaceNodesFormat::check(const AeroControlSurfaceNodesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.label, "label"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId1, "nodeSetId1"))
        return problem;
    return checkOptionalId(card.nodeSetId2, "nodeSetId2");
}

void AeroControlSurfaceNodesFormat::write(const AeroControlSurfaceNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.label);
    writer.integer(4, card.nodeSetId1);
    writer.integerUnlessDefault(6, card.nodeSetId2, 0);
}

std::uint32_t AeroControlSurfaceNodesTable::append(const AeroControlSurfaceNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroControlSurfaceNodesTable::store(std::uint32_t row, const AeroControlSurfaceNodesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroControlSurfaceNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroControlSurfaceNodesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroControlSurfaceNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroControlSurfaceNodesCard& card = m_cards.edit(row);
    visitId(visitor, "nodeSetId1", ReferenceTarget::Set, card.nodeSetId1);
    visitId(visitor, "nodeSetId2", ReferenceTarget::Set, card.nodeSetId2);
}

} // namespace oor::model
