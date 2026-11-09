#include "oor/model/cards/sets/generalizedDofSet/GeneralizedDofSetTable.hpp"

namespace oor::model
{

namespace
{

// ID1 C1 ID2 C2 ID3 C3 ID4 C4 on the first line, then the same pairs on the continuation lines.
constexpr DofPairLayout LAYOUT{1, 4};

} // namespace

std::optional<GeneralizedDofSetCard> GeneralizedDofSetFormat::read(FieldReader& reader)
{
    GeneralizedDofSetCard card;
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeneralizedDofSetFormat::check(const GeneralizedDofSetCard& card)
{
    return checkDofPairs(card.dofs);
}

void GeneralizedDofSetFormat::write(const GeneralizedDofSetCard& card, FieldWriter& writer)
{
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t GeneralizedDofSetTable::append(const GeneralizedDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

GeneralizedDofSetCard GeneralizedDofSetTable::card(std::uint32_t row) const
{
    GeneralizedDofSetCard card;
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void GeneralizedDofSetTable::store(std::uint32_t row, const GeneralizedDofSetCard& card)
{
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void GeneralizedDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void GeneralizedDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
