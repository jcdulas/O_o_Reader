#include "oor/model/cards/sets/analysisDofSet/AnalysisDofSetTable.hpp"

namespace oor::model
{

namespace
{

// ID1 C1 ID2 C2 ID3 C3 ID4 C4 on the first line, then the same pairs on the continuation lines.
constexpr DofPairLayout LAYOUT{1, 4};

} // namespace

std::optional<AnalysisDofSetCard> AnalysisDofSetFormat::read(FieldReader& reader)
{
    AnalysisDofSetCard card;
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnalysisDofSetFormat::check(const AnalysisDofSetCard& card)
{
    return checkDofPairs(card.dofs);
}

void AnalysisDofSetFormat::write(const AnalysisDofSetCard& card, FieldWriter& writer)
{
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t AnalysisDofSetTable::append(const AnalysisDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

AnalysisDofSetCard AnalysisDofSetTable::card(std::uint32_t row) const
{
    AnalysisDofSetCard card;
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void AnalysisDofSetTable::store(std::uint32_t row, const AnalysisDofSetCard& card)
{
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void AnalysisDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void AnalysisDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
