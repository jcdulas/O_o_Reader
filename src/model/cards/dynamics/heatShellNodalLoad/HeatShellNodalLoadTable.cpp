#include "oor/model/cards/dynamics/heatShellNodalLoad/HeatShellNodalLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LOAD_FIELD = 2;
constexpr int FIELDS_PER_LOAD = 3; // Gi, Ci, Qi
constexpr int LAST_FIELD = FIRST_LOAD_FIELD + static_cast<int>(HeatShellNodalLoadCard::MAX_LOADS) * FIELDS_PER_LOAD - 1;

// The names of the fields of the first and of the second load, for the diagnostics.
constexpr const char* NODE_NAMES[HeatShellNodalLoadCard::MAX_LOADS] = {"node1", "node2"};
constexpr const char* COMPONENT_NAMES[HeatShellNodalLoadCard::MAX_LOADS] = {"components1", "components2"};
constexpr const char* POWER_NAMES[HeatShellNodalLoadCard::MAX_LOADS] = {"heatFlux1", "heatFlux2"};

} // namespace

std::optional<HeatShellNodalLoadCard> HeatShellNodalLoadFormat::read(FieldReader& reader)
{
    HeatShellNodalLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < HeatShellNodalLoadCard::MAX_LOADS; ++index)
    {
        const int field = FIRST_LOAD_FIELD + static_cast<int>(index) * FIELDS_PER_LOAD;
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue;
        HeatShellNodalLoadLoad load;
        load.node = reader.requiredInteger(field, NODE_NAMES[index]);
        load.components = readShellComponents(reader, field + 1, COMPONENT_NAMES[index]);
        load.power = reader.requiredReal(field + 2, POWER_NAMES[index]);
        card.loads.push_back(load);
    }
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("heatFlux2"), "heatFlux2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatShellNodalLoadFormat::check(const HeatShellNodalLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.loads.empty())
        return CardProblem{"card.missing-field", "the card needs at least one load", "node1"};
    if (card.loads.size() > HeatShellNodalLoadCard::MAX_LOADS)
        return CardProblem{"card.bad-value", "the card gives the loads on at most two grid points", "node2"};
    for (std::size_t index = 0; index < card.loads.size(); ++index)
    {
        const HeatShellNodalLoadLoad& load = card.loads[index];
        if (const auto problem = checkRequiredId(load.node, NODE_NAMES[index]))
            return problem;
        if (const auto problem = checkShellComponents(load.components, COMPONENT_NAMES[index]))
            return problem;
        if (const auto problem = checkReal(load.power, POWER_NAMES[index], RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void HeatShellNodalLoadFormat::write(const HeatShellNodalLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_LOAD_FIELD;
    for (const HeatShellNodalLoadLoad& load : card.loads)
    {
        writer.integer(field, load.node);
        writeShellComponents(writer, field + 1, load.components);
        writer.real(field + 2, load.power);
        field += FIELDS_PER_LOAD;
    }
}

std::uint32_t HeatShellNodalLoadTable::append(const HeatShellNodalLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> powers;
    for (const HeatShellNodalLoadLoad& load : card.loads)
    {
        nodes.push_back(load.node);
        components.push_back(load.components);
        powers.push_back(load.power);
    }
    m_node.push(nodes);
    m_components.push(components);
    m_power.push(powers);
    return row;
}

HeatShellNodalLoadCard HeatShellNodalLoadTable::card(std::uint32_t row) const
{
    HeatShellNodalLoadCard card;
    card.id = m_id.get(row);
    const auto nodes = m_node.get(row);
    const auto components = m_components.get(row);
    const auto powers = m_power.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.loads.push_back(HeatShellNodalLoadLoad{nodes[index], components[index], powers[index]});
    return card;
}

void HeatShellNodalLoadTable::store(std::uint32_t row, const HeatShellNodalLoadCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> powers;
    for (const HeatShellNodalLoadLoad& load : card.loads)
    {
        nodes.push_back(load.node);
        components.push_back(load.components);
        powers.push_back(load.power);
    }
    m_node.set(row, nodes);
    m_components.set(row, components);
    m_power.set(row, powers);
}

void HeatShellNodalLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_components.keepRows(rows);
    m_power.keepRows(rows);
}

void HeatShellNodalLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
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
