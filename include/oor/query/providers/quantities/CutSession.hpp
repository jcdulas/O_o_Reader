#pragma once

#include "oor/query/named/common/Topology.hpp"
#include "oor/query/providers/quantities/CutConnections.hpp"
#include "oor/query/providers/quantities/CutIntegrationIndex.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/providers/quantities/NodePointForceSet.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

namespace oor::query
{

// What the cuts of one call share: the grid point forces, decoded once, the rigid elements and MPC equations of each
// MPC selection, found once, the positions of the grid points, and the forces of the shells the integration reads.
// `sectionCuts` evaluates every cut of the series on one session. Each part is made on first use; after `prepare`
// the session is only read, and the cuts may run on several threads.
class CutSession
{
public:
    explicit CutSession(const ResultQueryContext& context) : m_context(context) {}

    // The forces of the shells the integration of a cut reads, and what each way of obtaining them did.
    struct ShellForces
    {
        ElementFlatSet set;
        std::vector<ProviderAttempt> attempts;
    };

    // Makes every part the cuts read: the grid point forces with the connections of their subcases, the positions,
    // and with `integration` the forces of the shells with the connections of their subcases, the elements of the
    // model and the index of the shells (sorted along `axis`, the normal of the stations of the call, when given).
    void prepare(bool integration, const std::optional<std::array<double, 3>>& axis = std::nullopt);

    // The grid point force tables of the call (read on first use).
    [[nodiscard]] const NodePointForceSet& nodePointForces();
    // The connections of the MPC selection of a subcase, in the model of a result set; null when the set has no model.
    [[nodiscard]] const CutConnections* connections(std::size_t set, std::int32_t subcase);
    // The positions of the grid points of the model of the call (made on first use).
    [[nodiscard]] const NodePositions& positions(const model::Model& model);
    // The forces of the shells in their element frames, at their centres (the integration of a cut).
    [[nodiscard]] const ShellForces& shellForces();
    // Every element of the model with its grid points.
    [[nodiscard]] const TopologyList& topology(const model::Model& model);
    // The shells of the shell forces with their geometry and rows, for the integration (made on first use).
    [[nodiscard]] const CutIntegrationIndex& integrationIndex(const model::Model& model);

private:
    using Selection = std::optional<std::set<base::EntityId>>;

    const ResultQueryContext& m_context;
    std::shared_ptr<const NodePointForceSet> m_forces;
    std::unique_ptr<NodePositions> m_positions;
    const model::Model* m_positionsModel = nullptr;
    std::unique_ptr<ShellForces> m_shellForces;
    std::unique_ptr<TopologyList> m_topology;
    std::unique_ptr<CutIntegrationIndex> m_integrationIndex;
    std::optional<std::array<double, 3>> m_axis;
    const model::Model* m_topologyModel = nullptr;
    std::map<std::pair<const model::Model*, std::int32_t>, Selection> m_selections;
    std::map<std::pair<const model::Model*, Selection>, std::unique_ptr<CutConnections>> m_connections;
};

} // namespace oor::query
