#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Table.hpp"

#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5; // G3

} // namespace

std::optional<AcousticInfiniteTria3Card> AcousticInfiniteTria3Format::read(FieldReader& reader)
{
    AcousticInfiniteTria3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, AcousticInfiniteTria3Card::NODE_COUNT);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticInfiniteTria3Format::check(const AcousticInfiniteTria3Card& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    return checkInterfaceNodes(card.nodes, AcousticInfiniteTria3Card::NODE_COUNT, false);
}

void AcousticInfiniteTria3Format::write(const AcousticInfiniteTria3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, AcousticInfiniteTria3Card::NODE_COUNT);
}

std::uint32_t AcousticInfiniteTria3Table::append(const AcousticInfiniteTria3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

AcousticInfiniteTria3Card AcousticInfiniteTria3Table::card(std::uint32_t row) const
{
    AcousticInfiniteTria3Card card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void AcousticInfiniteTria3Table::store(std::uint32_t row, const AcousticInfiniteTria3Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void AcousticInfiniteTria3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void AcousticInfiniteTria3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
}

} // namespace oor::model
