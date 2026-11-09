#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<NodeTemperatureLoadCard> NodeTemperatureLoadFormat::read(FieldReader& reader)
{
    NodeTemperatureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    // The pairs (Gi, Ti) follow each other from field 2. A pair with both fields blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_POINT_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        NodeTemperature point;
        point.node = reader.requiredInteger(field, "node");
        point.temperature = reader.requiredReal(field + 1, "temperature");
        card.points.push_back(point);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NodeTemperatureLoadFormat::check(const NodeTemperatureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the temperature set ID must be a positive integer", "id"};
    if (card.points.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "node1"};
    for (const NodeTemperature& point : card.points)
    {
        if (point.node <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "node1"};
    }
    return std::nullopt;
}

void NodeTemperatureLoadFormat::write(const NodeTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_POINT_FIELD;
    for (const NodeTemperature& point : card.points)
    {
        writer.integer(field, point.node);
        writer.real(field + 1, point.temperature);
        field += 2;
    }
}

std::uint32_t NodeTemperatureLoadTable::append(const NodeTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<double> temperatures;
    for (const NodeTemperature& point : card.points)
    {
        nodes.push_back(point.node);
        temperatures.push_back(point.temperature);
    }
    m_node.push(nodes);
    m_temperature.push(temperatures);
    return row;
}

NodeTemperatureLoadCard NodeTemperatureLoadTable::card(std::uint32_t row) const
{
    NodeTemperatureLoadCard card;
    card.id = m_id.get(row);
    const auto nodes = m_node.get(row);
    const auto temperatures = m_temperature.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.points.push_back(NodeTemperature{nodes[index], temperatures[index]});
    return card;
}

void NodeTemperatureLoadTable::store(std::uint32_t row, const NodeTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<double> temperatures;
    for (const NodeTemperature& point : card.points)
    {
        nodes.push_back(point.node);
        temperatures.push_back(point.temperature);
    }
    m_node.set(row, nodes);
    m_temperature.set(row, temperatures);
}

void NodeTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_temperature.keepRows(rows);
}

void NodeTemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> nodes(m_node.get(row).begin(), m_node.get(row).end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_node.set(row, nodes);
}

} // namespace oor::model
