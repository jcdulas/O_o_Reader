#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"

namespace oor::model
{

std::optional<Rod2InlineSectionCard> Rod2InlineSectionFormat::read(FieldReader& reader)
{
    Rod2InlineSectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodes[0] = reader.requiredInteger(2, "node1");
    card.nodes[1] = reader.requiredInteger(3, "node2");
    card.matId = reader.requiredInteger(4, "matId");
    card.area = reader.real(5, "area", 0.0);
    card.torsionConstant = reader.real(6, "torsionConstant", 0.0);
    card.torsionStressCoefficient = reader.real(7, "torsionStressCoefficient", 0.0);
    card.nonStructMass = reader.real(8, "nonStructMass", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Rod2InlineSectionFormat::check(const Rod2InlineSectionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.nodes[0] <= 0 || card.nodes[1] <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "node1"};
    if (card.nodes[0] == card.nodes[1])
        return CardProblem{"card.duplicate-node", "the two grid points must be different", "node1"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    return std::nullopt;
}

void Rod2InlineSectionFormat::write(const Rod2InlineSectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodes[0]);
    writer.integer(3, card.nodes[1]);
    writer.integer(4, card.matId);
    writer.realUnlessDefault(5, card.area, 0.0);
    writer.realUnlessDefault(6, card.torsionConstant, 0.0);
    writer.realUnlessDefault(7, card.torsionStressCoefficient, 0.0);
    writer.realUnlessDefault(8, card.nonStructMass, 0.0);
}

std::uint32_t Rod2InlineSectionTable::append(const Rod2InlineSectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    m_matId.push(card.matId);
    m_area.push(card.area);
    m_torsionConstant.push(card.torsionConstant);
    m_torsionStressCoefficient.push(card.torsionStressCoefficient);
    m_nonStructMass.push(card.nonStructMass);
    return row;
}

Rod2InlineSectionCard Rod2InlineSectionTable::card(std::uint32_t row) const
{
    return Rod2InlineSectionCard{m_id.get(row),           m_nodes.get(row),           m_matId.get(row),
                               m_area.get(row),         m_torsionConstant.get(row), m_torsionStressCoefficient.get(row),
                               m_nonStructMass.get(row)};
}

void Rod2InlineSectionTable::store(std::uint32_t row, const Rod2InlineSectionCard& card)
{
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
    m_matId.set(row, card.matId);
    m_area.set(row, card.area);
    m_torsionConstant.set(row, card.torsionConstant);
    m_torsionStressCoefficient.set(row, card.torsionStressCoefficient);
    m_nonStructMass.set(row, card.nonStructMass);
}

void Rod2InlineSectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
    m_matId.keepRows(rows);
    m_area.keepRows(rows);
    m_torsionConstant.keepRows(rows);
    m_torsionStressCoefficient.keepRows(rows);
    m_nonStructMass.keepRows(rows);
}

void Rod2InlineSectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 2> nodes = m_nodes.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < 2; ++index)
    {
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, nodes[index]);
        changed = changed || updated != nodes[index];
        nodes[index] = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
