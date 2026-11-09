#include "oor/model/cards/constraints/freeBodySupportSet/FreeBodySupportSetTable.hpp"

namespace oor::model
{

namespace
{

// SID in field 1, then ID1 C1 ID2 C2 ID3 C3 on the first line (field 8 is blank), then the same pairs on the
// continuation lines.
constexpr DofPairLayout LAYOUT{2, 3};

} // namespace

std::optional<FreeBodySupportSetCard> FreeBodySupportSetFormat::read(FieldReader& reader)
{
    FreeBodySupportSetCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeBodySupportSetFormat::check(const FreeBodySupportSetCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the support set ID must be a positive integer", "id"};
    return checkDofPairs(card.dofs);
}

void FreeBodySupportSetFormat::write(const FreeBodySupportSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t FreeBodySupportSetTable::append(const FreeBodySupportSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

FreeBodySupportSetCard FreeBodySupportSetTable::card(std::uint32_t row) const
{
    FreeBodySupportSetCard card;
    card.id = m_id.get(row);
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void FreeBodySupportSetTable::store(std::uint32_t row, const FreeBodySupportSetCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void FreeBodySupportSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void FreeBodySupportSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
