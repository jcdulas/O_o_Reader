#include "oor/model/cards/sets/omittedDofSet/OmittedDofSetTable.hpp"

namespace oor::model
{

namespace
{

// ID1 C1 ID2 C2 ID3 C3 ID4 C4 on the first line, then the same pairs on the continuation lines.
constexpr DofPairLayout LAYOUT{1, 4};

} // namespace

std::optional<OmittedDofSetCard> OmittedDofSetFormat::read(FieldReader& reader)
{
    OmittedDofSetCard card;
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OmittedDofSetFormat::check(const OmittedDofSetCard& card)
{
    return checkDofPairs(card.dofs);
}

void OmittedDofSetFormat::write(const OmittedDofSetCard& card, FieldWriter& writer)
{
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t OmittedDofSetTable::append(const OmittedDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

OmittedDofSetCard OmittedDofSetTable::card(std::uint32_t row) const
{
    OmittedDofSetCard card;
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void OmittedDofSetTable::store(std::uint32_t row, const OmittedDofSetCard& card)
{
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void OmittedDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void OmittedDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
