#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/connections/ConnectionClusters.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <cstring>

namespace oor::query
{

namespace
{

std::int64_t asInt64(std::uint64_t stamp)
{
    std::int64_t value = 0;
    std::memcpy(&value, &stamp, sizeof(value));
    return value;
}

// connectionClusters: the connections joined through grid points that only connections use.
class ConnectionClustersQuery final : public QueryBase
{
public:
    ConnectionClustersQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::string detail = run.arguments.textOr("detail", "clusters");
        if (detail != "clusters" && detail != "connections")
            throw QueryError("connectionClusters: detail is \"clusters\" or \"connections\", not \"" + detail + "\"");
        const auto components = analysis::ComponentModel::of(
            model, contactOptionsOf(run.arguments, "connectionClusters"), run.context.pool());
        const analysis::ConnectionClusters clusters =
            analysis::ConnectionClusters::build(model, *components, run.context.pool());
        if (detail == "connections")
        {
            if (components->connections().empty())
                return emptyTable(info().columnsFor(detail));
            return connectionsForm(model, *components, clusters, run.context.pool());
        }
        if (clusters.clusters().empty())
            return emptyTable(info().columnsFor(detail));
        std::vector<std::int64_t> id;
        std::vector<std::int64_t> count;
        std::vector<std::int64_t> connectionOffsets{0};
        std::vector<std::int32_t> connections;
        std::vector<std::int64_t> nodeOffsets{0};
        std::vector<std::int32_t> nodes;
        std::vector<std::int64_t> componentOffsets{0};
        std::vector<std::int32_t> touched;
        std::vector<std::uint8_t> single;
        std::vector<std::int64_t> revision;
        for (const analysis::ConnectionCluster& cluster : clusters.clusters())
        {
            id.push_back(cluster.id);
            count.push_back(static_cast<std::int64_t>(cluster.connections.size()));
            std::vector<std::int32_t> ids;
            for (const std::uint32_t connection : cluster.connections)
                ids.push_back(components->connections()[connection].id);
            std::sort(ids.begin(), ids.end());
            connections.insert(connections.end(), ids.begin(), ids.end());
            connectionOffsets.push_back(static_cast<std::int64_t>(connections.size()));
            nodes.insert(nodes.end(), cluster.connectionOnlyNodes.begin(), cluster.connectionOnlyNodes.end());
            nodeOffsets.push_back(static_cast<std::int64_t>(nodes.size()));
            touched.insert(touched.end(), cluster.components.begin(), cluster.components.end());
            componentOffsets.push_back(static_cast<std::int64_t>(touched.size()));
            single.push_back(cluster.isSingleComponent() ? 1 : 0);
            revision.push_back(asInt64(cluster.revision));
        }
        TableBuilder builder;
        builder.int64("clusterId", std::move(id));
        builder.int64("connectionCount", std::move(count));
        builder.intList("connections", std::move(connectionOffsets), std::move(connections));
        builder.intList("connectionOnlyNodeIds", std::move(nodeOffsets), std::move(nodes));
        builder.intList("components", std::move(componentOffsets), std::move(touched));
        builder.flag("isSingleComponent", single);
        builder.int64("revision", std::move(revision));
        return builder.finish();
    }

private:
    // detail="connections": one row per connection of the model, with its grid points and the stamp of its card: those
    // of each cluster first, then those in no cluster (the direct links between components) with a null cluster ID, by
    // card type name and ID. A solver that keeps connections at the assembly level reads them all here.
    static std::shared_ptr<const QueryTable> connectionsForm(const model::Model& model,
                                                             const analysis::ComponentModel& components,
                                                             const analysis::ConnectionClusters& clusters,
                                                             base::ThreadPool* pool)
    {
        const analysis::ConnectionSharing sharing = analysis::ConnectionSharing::build(components);
        std::vector<std::uint32_t> listed;
        std::vector<std::int64_t> clusterId;
        std::vector<std::uint8_t> clusterValid;
        std::vector<std::uint8_t> inCluster(components.connections().size(), 0);
        for (const analysis::ConnectionCluster& cluster : clusters.clusters())
        {
            for (const std::uint32_t connection : cluster.connections)
            {
                listed.push_back(connection);
                clusterId.push_back(cluster.id);
                clusterValid.push_back(1);
                inCluster[connection] = 1;
            }
        }
        const auto cardNameOf = [&](std::uint32_t connection) -> std::string_view
        { return model.findTable(components.connections()[connection].card.table)->info().name; };
        std::vector<std::uint32_t> direct;
        for (std::uint32_t connection = 0; connection < inCluster.size(); ++connection)
        {
            if (inCluster[connection] == 0)
                direct.push_back(connection);
        }
        std::sort(direct.begin(), direct.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const std::string_view leftName = cardNameOf(left);
                      const std::string_view rightName = cardNameOf(right);
                      if (leftName != rightName)
                          return leftName < rightName;
                      const base::EntityId leftId = components.connections()[left].id;
                      const base::EntityId rightId = components.connections()[right].id;
                      return leftId != rightId ? leftId < rightId : left < right;
                  });
        for (const std::uint32_t connection : direct)
        {
            listed.push_back(connection);
            clusterId.push_back(0);
            clusterValid.push_back(0);
        }
        std::vector<std::vector<model::CardLocation>> cards;
        for (const std::uint32_t connection : listed)
            cards.push_back({components.connections()[connection].card});
        const std::vector<std::uint64_t> stamps = analysis::RevisionStamp::computeFor(model, cards, pool);
        std::vector<std::string> kind;
        std::vector<std::string> card;
        std::vector<std::int32_t> id;
        std::vector<std::int64_t> nodeOffsets{0};
        std::vector<std::int32_t> nodes;
        std::vector<std::int64_t> revision;
        for (std::size_t row = 0; row < listed.size(); ++row)
        {
            const analysis::Connection& connection = components.connections()[listed[row]];
            kind.emplace_back(connection.kind == analysis::ConnectionKind::Mass
                                  ? std::string_view("mass")
                                  : analysis::connectionClassName(sharing.classOf(listed[row])));
            card.emplace_back(model.findTable(connection.card.table)->info().name);
            id.push_back(static_cast<std::int32_t>(connection.id));
            for (const base::EntityId node : components.connectionNodes(listed[row]))
            {
                if (node != 0 && std::find(nodes.begin() + nodeOffsets.back(), nodes.end(), node) == nodes.end())
                    nodes.push_back(node);
            }
            nodeOffsets.push_back(static_cast<std::int64_t>(nodes.size()));
            revision.push_back(asInt64(stamps[row]));
        }
        TableBuilder builder;
        builder.int64("clusterId", std::move(clusterId), std::move(clusterValid));
        builder.dictionary("kind", kind);
        builder.dictionary("card", card);
        builder.int32("id", std::move(id));
        builder.intList("nodeIds", std::move(nodeOffsets), std::move(nodes));
        builder.int64("revision", std::move(revision));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "connectionClusters";
        info.family = "T6";
        info.description =
            "The clusters of connections joined through grid points that only connections use (the "
            "connection system): a connection with such a grid point is in a cluster, alone or with the "
            "connections that share its connection-only points; a connection that links components directly is in "
            "none. Clusters are numbered from 1 by decreasing connection count, ties by the smallest connection ID. "
            "connections lists the element IDs (an MPC equation by its set ID), connectionOnlyNodeIds the grid points "
            "only they use, components the components they touch. isSingleComponent: a cluster that touches one "
            "component only, which a group holding that component absorbs (its stamp is then part of the group's "
            "stamp, groupRevisions). revision is the revision stamp of its cards, as a signed int64. "
            "detail=\"connections\": one row per connection of the model (units), those of the clusters "
            "first, then those in no cluster (the direct links between two components) by card type name and ID with "
            "a null clusterId: kind (rigid, rigidInterpolation, "
            "mpc, fastener, connector, contact, or mass for a mass on a point only connections use, which is a "
            "connection), card, id (the element ID; the set ID of an MPC equation, one row per equation), nodeIds (the "
            "grid points it acts on, in the order of the card: a CFAST or a CWELD the grid points of the shells it "
            "pierces (fastenerPiercings) and its GA and GB, never GS) and revision (the stamp of its card).";
        info.detailParameter = "detail";
        info.parameters = {parameter("detail", ParameterKind::Text, "\"clusters\"", "\"clusters\" or \"connections\"")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "clusterId";
        ColumnListBuilder columns;
        columns.int64("clusterId").int64("connectionCount").intList("connections").intList("connectionOnlyNodeIds");
        columns.intList("components").flag("isSingleComponent").int64("revision");
        info.columnSets.push_back(columns.build("clusters"));
        ColumnListBuilder connections;
        connections.int64("clusterId").dictionary("kind").dictionary("card").int32("id").intList("nodeIds");
        connections.int64("revision");
        info.columnSets.push_back(connections.build("connections"));
        return info;
    }
};

} // namespace

void addConnectionClustersQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConnectionClustersQuery>());
}

} // namespace oor::query
