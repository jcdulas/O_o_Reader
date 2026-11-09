#include "oor/model/cards/thermal/acousticAbsorber/AcousticAbsorberTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<AcousticAbsorberCard> AcousticAbsorberFormat::read(FieldReader& reader)
{
    AcousticAbsorberCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: the default of PID is EID
    card.nodes[0] = reader.requiredInteger(3, "node1");
    for (std::size_t index = 1; index < AcousticAbsorberCard::NODE_COUNT; ++index)
        card.nodes[index] = reader.integer(3 + static_cast<int>(index), "node", 0);
    requireUnusedBlank(reader, {}, 6);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticAbsorberFormat::check(const AcousticAbsorberCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodes[0] <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                           "node1"};
    // The points follow each other: a point impedance, a line, a triangle or a quadrilateral.
    for (std::size_t index = 1; index < AcousticAbsorberCard::NODE_COUNT; ++index)
    {
        if (card.nodes[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", "node"};
        if (card.nodes[index] != 0 && card.nodes[index - 1] == 0)
            return CardProblem{"card.bad-value",
                               "the grid points of the element follow each other: no blank before " +
                                   CardProblem::fieldNamed("node" + std::to_string(index + 1)),
                               "node"};
    }
    return std::nullopt;
}

void AcousticAbsorberFormat::write(const AcousticAbsorberCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // PID defaults to EID: always written.
    writer.integer(2, card.propId);
    writer.integer(3, card.nodes[0]);
    for (std::size_t index = 1; index < AcousticAbsorberCard::NODE_COUNT; ++index)
        writer.integerUnlessDefault(3 + static_cast<int>(index), card.nodes[index], 0);
}

std::uint32_t AcousticAbsorberTable::append(const AcousticAbsorberCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    return row;
}

AcousticAbsorberCard AcousticAbsorberTable::card(std::uint32_t row) const
{
    AcousticAbsorberCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void AcousticAbsorberTable::store(std::uint32_t row, const AcousticAbsorberCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
}

void AcousticAbsorberTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void AcousticAbsorberTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, m_propId.get(row)));
    std::array<base::EntityId, AcousticAbsorberCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
