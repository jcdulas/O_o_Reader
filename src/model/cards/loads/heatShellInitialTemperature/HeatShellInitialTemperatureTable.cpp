#include "oor/model/cards/loads/heatShellInitialTemperature/HeatShellInitialTemperatureTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;
constexpr int FIELDS_PER_POINT = 3; // Gi, Ci, Ti
constexpr int LAST_FIELD =
    FIRST_POINT_FIELD + static_cast<int>(HeatShellInitialTemperatureCard::MAX_POINTS) * FIELDS_PER_POINT - 1;

// The names of the fields of the first and of the second point, for the diagnostics.
constexpr const char* NODE_NAMES[HeatShellInitialTemperatureCard::MAX_POINTS] = {"node1", "node2"};
constexpr const char* COMPONENT_NAMES[HeatShellInitialTemperatureCard::MAX_POINTS] = {"components1", "components2"};
constexpr const char* TEMPERATURE_NAMES[HeatShellInitialTemperatureCard::MAX_POINTS] = {"temperature1", "temperature2"};

} // namespace

std::optional<HeatShellInitialTemperatureCard> HeatShellInitialTemperatureFormat::read(FieldReader& reader)
{
    HeatShellInitialTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < HeatShellInitialTemperatureCard::MAX_POINTS; ++index)
    {
        const int field = FIRST_POINT_FIELD + static_cast<int>(index) * FIELDS_PER_POINT;
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue;
        HeatShellInitialTemperaturePoint point;
        point.node = reader.requiredInteger(field, NODE_NAMES[index]);
        point.components = readShellComponents(reader, field + 1, COMPONENT_NAMES[index]);
        point.temperature = reader.requiredReal(field + 2, TEMPERATURE_NAMES[index]);
        card.points.push_back(point);
    }
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("temperature2"), "temperature2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatShellInitialTemperatureFormat::check(const HeatShellInitialTemperatureCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.points.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "node1"};
    if (card.points.size() > HeatShellInitialTemperatureCard::MAX_POINTS)
        return CardProblem{"card.bad-value", "the card gives the temperatures of at most two grid points", "node2"};
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const HeatShellInitialTemperaturePoint& point = card.points[index];
        if (const auto problem = checkRequiredId(point.node, NODE_NAMES[index]))
            return problem;
        if (const auto problem = checkShellComponents(point.components, COMPONENT_NAMES[index]))
            return problem;
        if (const auto problem = checkReal(point.temperature, TEMPERATURE_NAMES[index], RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void HeatShellInitialTemperatureFormat::write(const HeatShellInitialTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_POINT_FIELD;
    for (const HeatShellInitialTemperaturePoint& point : card.points)
    {
        writer.integer(field, point.node);
        writeShellComponents(writer, field + 1, point.components);
        writer.real(field + 2, point.temperature);
        field += FIELDS_PER_POINT;
    }
}

std::uint32_t HeatShellInitialTemperatureTable::append(const HeatShellInitialTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> temperatures;
    for (const HeatShellInitialTemperaturePoint& point : card.points)
    {
        nodes.push_back(point.node);
        components.push_back(point.components);
        temperatures.push_back(point.temperature);
    }
    m_node.push(nodes);
    m_components.push(components);
    m_temperature.push(temperatures);
    return row;
}

HeatShellInitialTemperatureCard HeatShellInitialTemperatureTable::card(std::uint32_t row) const
{
    HeatShellInitialTemperatureCard card;
    card.id = m_id.get(row);
    const auto nodes = m_node.get(row);
    const auto components = m_components.get(row);
    const auto temperatures = m_temperature.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.points.push_back(HeatShellInitialTemperaturePoint{nodes[index], components[index], temperatures[index]});
    return card;
}

void HeatShellInitialTemperatureTable::store(std::uint32_t row, const HeatShellInitialTemperatureCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> temperatures;
    for (const HeatShellInitialTemperaturePoint& point : card.points)
    {
        nodes.push_back(point.node);
        components.push_back(point.components);
        temperatures.push_back(point.temperature);
    }
    m_node.set(row, nodes);
    m_components.set(row, components);
    m_temperature.set(row, temperatures);
}

void HeatShellInitialTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_components.keepRows(rows);
    m_temperature.keepRows(rows);
}

void HeatShellInitialTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The points are grid points of heat shell elements, in the node ID space.
    const auto current = m_node.get(row);
    std::vector<base::EntityId> nodes(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("node1", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_node.set(row, nodes);
}

} // namespace oor::model
