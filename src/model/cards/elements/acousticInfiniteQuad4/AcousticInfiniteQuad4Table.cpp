#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Table.hpp"

#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6; // G4

} // namespace

std::optional<AcousticInfiniteQuad4Card> AcousticInfiniteQuad4Format::read(FieldReader& reader)
{
    AcousticInfiniteQuad4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, AcousticInfiniteQuad4Card::NODE_COUNT);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticInfiniteQuad4Format::check(const AcousticInfiniteQuad4Card& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    return checkInterfaceNodes(card.nodes, AcousticInfiniteQuad4Card::NODE_COUNT, false);
}

void AcousticInfiniteQuad4Format::write(const AcousticInfiniteQuad4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, AcousticInfiniteQuad4Card::NODE_COUNT);
}

std::uint32_t AcousticInfiniteQuad4Table::append(const AcousticInfiniteQuad4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

AcousticInfiniteQuad4Card AcousticInfiniteQuad4Table::card(std::uint32_t row) const
{
    AcousticInfiniteQuad4Card card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void AcousticInfiniteQuad4Table::store(std::uint32_t row, const AcousticInfiniteQuad4Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void AcousticInfiniteQuad4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void AcousticInfiniteQuad4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
}

} // namespace oor::model
