#include "oor/model/cards/elements/cohesivePenta/CohesivePentaTable.hpp"

#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 17; // G15

} // namespace

std::optional<CohesivePentaCard> CohesivePentaFormat::read(FieldReader& reader)
{
    CohesivePentaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, CohesivePentaCard::CORNER_COUNT);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CohesivePentaFormat::check(const CohesivePentaCard& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    return checkInterfaceNodes(card.nodes, CohesivePentaCard::CORNER_COUNT, true);
}

void CohesivePentaFormat::write(const CohesivePentaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, CohesivePentaCard::CORNER_COUNT);
}

std::uint32_t CohesivePentaTable::append(const CohesivePentaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

CohesivePentaCard CohesivePentaTable::card(std::uint32_t row) const
{
    CohesivePentaCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void CohesivePentaTable::store(std::uint32_t row, const CohesivePentaCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void CohesivePentaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void CohesivePentaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
}

} // namespace oor::model
