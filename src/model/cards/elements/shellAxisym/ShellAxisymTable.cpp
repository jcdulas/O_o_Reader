#include "oor/model/cards/elements/shellAxisym/ShellAxisymTable.hpp"

#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int OFFSET_FIELD = 6;
constexpr int LAST_FIELD = OFFSET_FIELD;

} // namespace

std::optional<ShellAxisymCard> ShellAxisymFormat::read(FieldReader& reader)
{
    ShellAxisymCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, ShellAxisymCard::END_COUNT);
    card.offset = reader.optionalReal(OFFSET_FIELD, "offset");
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellAxisymFormat::check(const ShellAxisymCard& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkInterfaceNodes(card.nodes, ShellAxisymCard::END_COUNT, true))
        return problem;
    return checkOptionalReal(card.offset, "offset", RealRule::Finite);
}

void ShellAxisymFormat::write(const ShellAxisymCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, ShellAxisymCard::END_COUNT);
    if (card.offset)
        writer.real(OFFSET_FIELD, *card.offset);
}

std::uint32_t ShellAxisymTable::append(const ShellAxisymCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_offset.push(realToColumn(card.offset));
    return row;
}

ShellAxisymCard ShellAxisymTable::card(std::uint32_t row) const
{
    ShellAxisymCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    card.offset = realFromColumn(m_offset.get(row));
    return card;
}

void ShellAxisymTable::store(std::uint32_t row, const ShellAxisymCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_offset.set(row, realToColumn(card.offset));
}

void ShellAxisymTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_offset.keepRows(rows);
}

void ShellAxisymTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The PAXISYM is a property card: the reference is a property reference.
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
}

} // namespace oor::model
