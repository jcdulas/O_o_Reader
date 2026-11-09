#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <vector>

namespace oor::analysis
{

// Connections joined through grid points that only connections use.
struct ConnectionCluster
{
    std::int32_t id = 0;                              // from 1, by decreasing size, ties by the smallest connection ID
    std::vector<std::uint32_t> connections;           // indexes into ComponentModel::connections(), increasing
    std::vector<base::EntityId> connectionOnlyNodes; // sorted
    std::vector<std::int32_t> components;             // the components its connections touch, increasing
    std::uint64_t revision = 0; // the revision stamp of its connections (RevisionStamp::computeFor)
    [[nodiscard]] bool isSingleComponent() const noexcept { return components.size() == 1; }
};

// The connection system of a model: which grid points only connections use, and the clusters of
// connections joined through such points. A connection with no connection-only grid point (it links components
// directly) is in no cluster; a connection that has one is in a cluster, alone or with the connections that share its
// connection-only points. A cluster that touches one component only (a mass on springs hanging from a panel, the
// reference point of a spider) is absorbed into a group that holds that component (GroupSet), and its stamp is then
// part of the group's stamp.
class ConnectionClusters
{
public:
    [[nodiscard]] static ConnectionClusters build(const model::Model& model, const ComponentModel& components,
                                                  base::ThreadPool* pool = nullptr);

    [[nodiscard]] const std::vector<ConnectionCluster>& clusters() const noexcept { return m_clusters; }
    // The cluster of a connection, 0 for a connection in none.
    [[nodiscard]] std::int32_t clusterOf(std::size_t connection) const noexcept { return m_clusterOf[connection]; }
    // The grid points that connections use and no component holds, sorted.
    [[nodiscard]] const std::vector<base::EntityId>& connectionOnlyNodes() const noexcept { return m_connectionOnly; }
    [[nodiscard]] bool isConnectionOnly(base::EntityId node) const noexcept;
    // The other components the connections of a component link it to, directly or through a cluster, increasing.
    [[nodiscard]] std::vector<std::int32_t> linkedComponents(const ComponentModel& components,
                                                             std::int32_t component) const;

private:
    std::vector<ConnectionCluster> m_clusters;
    std::vector<std::int32_t> m_clusterOf;
    std::vector<base::EntityId> m_connectionOnly;
};

} // namespace oor::analysis
