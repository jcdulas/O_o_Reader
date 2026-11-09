#include "oor/model/cards/constraints/autoConstrExclusion/AutoConstrExclusionTable.hpp"

namespace oor::model
{

namespace
{

// G1 C1 G2 C2 G3 C3 G4 C4 on the first line, then the same pairs on the continuation lines.
constexpr DofPairLayout LAYOUT{1, 4};

} // namespace

std::optional<AutoConstrExclusionCard> AutoConstrExclusionFormat::read(FieldReader& reader)
{
    AutoConstrExclusionCard card;
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AutoConstrExclusionFormat::check(const AutoConstrExclusionCard& card)
{
    return checkDofPairs(card.dofs);
}

void AutoConstrExclusionFormat::write(const AutoConstrExclusionCard& card, FieldWriter& writer)
{
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t AutoConstrExclusionTable::append(const AutoConstrExclusionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

AutoConstrExclusionCard AutoConstrExclusionTable::card(std::uint32_t row) const
{
    AutoConstrExclusionCard card;
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void AutoConstrExclusionTable::store(std::uint32_t row, const AutoConstrExclusionCard& card)
{
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void AutoConstrExclusionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void AutoConstrExclusionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
