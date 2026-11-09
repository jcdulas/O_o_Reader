#include "oor/analysis/connections/ConnectionClusters.hpp"

#include "oor/analysis/components/RevisionStamp.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <numeric>

namespace oor::analysis
{

namespace
{

// Union-find over the connections; the smaller index becomes the root, so the result does not depend on the order.
class ConnectionSets
{
public:
    explicit ConnectionSets(std::size_t count) : m_parent(count) { std::iota(m_parent.begin(), m_parent.end(), 0U); }
    [[nodiscard]] std::uint32_t find(std::uint32_t item) noexcept
    {
        while (m_parent[item] != item)
        {
            m_parent[item] = m_parent[m_parent[item]];
            item = m_parent[item];
        }
        return item;
    }
    void join(std::uint32_t first, std::uint32_t second) noexcept
    {
        const std::uint32_t left = find(first);
        const std::uint32_t right = find(second);
        if (left < right)
            m_parent[right] = left;
        else if (right < left)
            m_parent[left] = right;
    }

private:
    std::vector<std::uint32_t> m_parent;
};

} // namespace

ConnectionClusters ConnectionClusters::build(const model::Model& model, const ComponentModel& components,
                                             base::ThreadPool* pool)
{
    ConnectionClusters result;
    const std::vector<Connection>& connections = components.connections();
    result.m_clusterOf.assign(connections.size(), 0);
    // The connection-only grid points, each with the first connection met on it.
    std::map<base::EntityId, std::uint32_t> firstOnNode;
    ConnectionSets sets(connections.size());
    std::vector<std::uint8_t> hasConnectionOnly(connections.size(), 0);
    for (std::size_t index = 0; index < connections.size(); ++index)
    {
        for (const base::EntityId node : components.connectionNodes(index))
        {
            if (node == 0 || components.componentOfNode(node) != 0)
                continue;
            hasConnectionOnly[index] = 1;
            const auto [entry, inserted] = firstOnNode.emplace(node, static_cast<std::uint32_t>(index));
            if (!inserted)
                sets.join(entry->second, static_cast<std::uint32_t>(index));
        }
    }
    for (const auto& entry : firstOnNode)
        result.m_connectionOnly.push_back(entry.first);

    // The clusters, gathered by root.
    std::map<std::uint32_t, ConnectionCluster> byRoot;
    for (std::size_t index = 0; index < connections.size(); ++index)
    {
        if (hasConnectionOnly[index] == 0)
            continue;
        ConnectionCluster& cluster = byRoot[sets.find(static_cast<std::uint32_t>(index))];
        cluster.connections.push_back(static_cast<std::uint32_t>(index));
        for (const base::EntityId node : components.connectionNodes(index))
        {
            if (node == 0)
                continue;
            if (const std::int32_t component = components.componentOfNode(node); component != 0)
                cluster.components.push_back(component);
            else
                cluster.connectionOnlyNodes.push_back(node);
        }
    }
    std::vector<ConnectionCluster> clusters;
    for (auto& [root, cluster] : byRoot)
    {
        std::vector<base::EntityId>& only = cluster.connectionOnlyNodes;
        std::sort(only.begin(), only.end());
        only.erase(std::unique(only.begin(), only.end()), only.end());
        std::sort(cluster.components.begin(), cluster.components.end());
        cluster.components.erase(std::unique(cluster.components.begin(), cluster.components.end()),
                                 cluster.components.end());
        clusters.push_back(std::move(cluster));
    }
    const auto smallest = [&](const ConnectionCluster& cluster)
    {
        base::EntityId value = std::numeric_limits<base::EntityId>::max();
        for (const std::uint32_t connection : cluster.connections)
            value = std::min(value, connections[connection].id);
        return value;
    };
    std::stable_sort(clusters.begin(), clusters.end(),
                     [&](const ConnectionCluster& left, const ConnectionCluster& right)
                     {
                         if (left.connections.size() != right.connections.size())
                             return left.connections.size() > right.connections.size();
                         return smallest(left) < smallest(right);
                     });
    std::vector<std::vector<model::CardLocation>> members(clusters.size());
    for (std::size_t index = 0; index < clusters.size(); ++index)
    {
        clusters[index].id = static_cast<std::int32_t>(index + 1);
        for (const std::uint32_t connection : clusters[index].connections)
        {
            result.m_clusterOf[connection] = clusters[index].id;
            members[index].push_back(connections[connection].card);
        }
    }
    const std::vector<std::uint64_t> stamps = RevisionStamp::computeFor(model, members, pool);
    for (std::size_t index = 0; index < clusters.size(); ++index)
        clusters[index].revision = stamps[index];
    result.m_clusters = std::move(clusters);
    return result;
}

bool ConnectionClusters::isConnectionOnly(base::EntityId node) const noexcept
{
    return std::binary_search(m_connectionOnly.begin(), m_connectionOnly.end(), node);
}

std::vector<std::int32_t> ConnectionClusters::linkedComponents(const ComponentModel& components,
                                                               std::int32_t component) const
{
    std::vector<std::int32_t> linked;
    for (const InterfaceNode& node : components.interfaceNodes(component))
    {
        for (std::uint32_t item = 0; item < node.connectionCount; ++item)
        {
            const std::uint32_t connection = components.interfaceConnections()[node.firstConnection + item];
            if (const std::int32_t cluster = m_clusterOf[connection]; cluster != 0)
            {
                const auto& touched = m_clusters[static_cast<std::size_t>(cluster - 1)].components;
                linked.insert(linked.end(), touched.begin(), touched.end());
                continue;
            }
            for (const base::EntityId other : components.connectionNodes(connection))
            {
                if (const std::int32_t found = components.componentOfNode(other); found != 0)
                    linked.push_back(found);
            }
        }
    }
    std::sort(linked.begin(), linked.end());
    linked.erase(std::unique(linked.begin(), linked.end()), linked.end());
    linked.erase(std::remove(linked.begin(), linked.end(), component), linked.end());
    return linked;
}

} // namespace oor::analysis
