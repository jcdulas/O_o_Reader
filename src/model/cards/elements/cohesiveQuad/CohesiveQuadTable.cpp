#include "oor/model/cards/elements/cohesiveQuad/CohesiveQuadTable.hpp"

#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 10; // G8

} // namespace

std::optional<CohesiveQuadCard> CohesiveQuadFormat::read(FieldReader& reader)
{
    CohesiveQuadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, CohesiveQuadCard::CORNER_COUNT);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CohesiveQuadFormat::check(const CohesiveQuadCard& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    return checkInterfaceNodes(card.nodes, CohesiveQuadCard::CORNER_COUNT, true);
}

void CohesiveQuadFormat::write(const CohesiveQuadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, CohesiveQuadCard::CORNER_COUNT);
}

std::uint32_t CohesiveQuadTable::append(const CohesiveQuadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

CohesiveQuadCard CohesiveQuadTable::card(std::uint32_t row) const
{
    CohesiveQuadCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void CohesiveQuadTable::store(std::uint32_t row, const CohesiveQuadCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void CohesiveQuadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void CohesiveQuadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
}

} // namespace oor::model
