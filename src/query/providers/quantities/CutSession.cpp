#include "oor/query/providers/quantities/CutSession.hpp"

#include "oor/query/providers/quantities/ElementQuantities.hpp"

namespace oor::query
{

void CutSession::prepare(bool integration, const std::optional<std::array<double, 3>>& axis)
{
    m_axis = axis;
    for (const NodePointForceTable& table : nodePointForces().tables())
        (void)connections(table.set, table.subcase);
    if (m_context.hasModel())
        (void)positions(*m_context.modelPointer());
    if (!integration || !m_context.hasModel())
        return;
    for (const ElementFlat& flat : shellForces().set.flats())
        (void)connections(flat.set, flat.subcase);
    (void)topology(*m_context.modelPointer());
    if (shellForces().set.rowCount() > 0)
        (void)integrationIndex(*m_context.modelPointer());
}

const CutIntegrationIndex& CutSession::integrationIndex(const model::Model& model)
{
    if (!m_integrationIndex)
        m_integrationIndex = std::make_unique<CutIntegrationIndex>(
            CutIntegrationIndex::build(m_context, model, shellForces().set, positions(model), topology(model), m_axis));
    return *m_integrationIndex;
}

const NodePointForceSet& CutSession::nodePointForces()
{
    if (!m_forces)
        m_forces = NodePointForceSet::read(m_context);
    return *m_forces;
}

const NodePositions& CutSession::positions(const model::Model& model)
{
    if (!m_positions || m_positionsModel != &model)
    {
        m_positions = std::make_unique<NodePositions>(model, m_context.exec().pool());
        m_positionsModel = &model;
    }
    return *m_positions;
}

const CutSession::ShellForces& CutSession::shellForces()
{
    if (!m_shellForces)
    {
        m_shellForces = std::make_unique<ShellForces>();
        ElementRequest asked;
        asked.tableFrame = "element";
        asked.location = "center";
        m_shellForces->attempts = shellResultantChain().resolve(m_context, asked, m_shellForces->set);
    }
    return *m_shellForces;
}

const TopologyList& CutSession::topology(const model::Model& model)
{
    if (!m_topology || m_topologyModel != &model)
    {
        m_topology = std::make_unique<TopologyList>(TopologyList::build(model, m_context.exec()));
        m_topologyModel = &model;
    }
    return *m_topology;
}

const CutConnections* CutSession::connections(std::size_t set, std::int32_t subcase)
{
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr)
        return nullptr;
    const auto key = std::make_pair(model, subcase);
    auto selection = m_selections.find(key);
    if (selection == m_selections.end())
        selection = m_selections.emplace(key, CutConnections::mpcSetsOf(*model, subcase)).first;
    const auto connectionKey = std::make_pair(model, selection->second);
    auto found = m_connections.find(connectionKey);
    if (found == m_connections.end())
        found = m_connections
                    .emplace(connectionKey,
                             std::make_unique<CutConnections>(*model, selection->second, m_context.exec().pool()))
                    .first;
    return found->second.get();
}

} // namespace oor::query
