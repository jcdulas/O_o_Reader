#include "oor/analysis/adjacency/MeshAdjacency.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// elementNeighbors: one row for each element and each of its neighbours, by edge or by node, with the shared nodes.
class ElementNeighborsQuery final : public QueryBase
{
public:
    ElementNeighborsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string by = run.arguments.textOr("by", "edge");
        analysis::AdjacencyKind kind = analysis::AdjacencyKind::Edge;
        if (by == "node")
            kind = analysis::AdjacencyKind::Node;
        else if (by != "edge")
            throw QueryError("elementNeighbors: by is \"edge\" or \"node\", not \"" + by + "\"");
        const std::vector<std::string> typeNames = run.arguments.texts("ofTypes");
        std::vector<std::uint16_t> ofTypes;
        if (!typeNames.empty())
            ofTypes = typesNamed(typeNames, model::CardCategory::Element, "ofTypes");

        const TopologyList topology = TopologyList::build(*run.model, run.context);
        const CardList& list = topology.list;
        const auto only = sortedIds(run.arguments.ids("elementIds"));
        std::vector<std::size_t> sources;
        std::vector<model::CardLocation> locations;
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            if (only && !std::binary_search(only->begin(), only->end(), static_cast<std::int64_t>(list.ids[index])))
                continue;
            sources.push_back(index);
            locations.push_back(list.location(index));
        }
        if (sources.empty())
            return emptyTable(info().defaultColumns());
        const analysis::AdjacencyCsr csr =
            analysis::MeshAdjacency::adjacency(*run.model, locations, kind, ofTypes, run.context.pool());

        std::vector<std::int32_t> eid;
        std::vector<std::int32_t> neighborEid;
        std::vector<std::string> type;
        std::vector<std::string> neighborType;
        std::vector<std::int64_t> sharedCount;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> shared;
        std::vector<std::int32_t> mine;
        std::vector<std::int32_t> theirs;
        for (std::size_t position = 0; position < sources.size(); ++position)
        {
            const std::size_t source = sources[position];
            mine.assign(topology.nodes.of(source).begin(), topology.nodes.of(source).end());
            std::sort(mine.begin(), mine.end());
            mine.erase(std::unique(mine.begin(), mine.end()), mine.end());
            for (std::int64_t at = csr.offsets[position]; at < csr.offsets[position + 1]; ++at)
            {
                const std::int32_t other = csr.values[static_cast<std::size_t>(at)];
                const std::int64_t found = topology.indexOf(other);
                if (found < 0)
                    continue;
                const auto otherIndex = static_cast<std::size_t>(found);
                theirs.assign(topology.nodes.of(otherIndex).begin(), topology.nodes.of(otherIndex).end());
                std::sort(theirs.begin(), theirs.end());
                const std::size_t before = shared.size();
                std::set_intersection(mine.begin(), mine.end(), theirs.begin(), theirs.end(),
                                      std::back_inserter(shared));
                eid.push_back(list.ids[source]);
                neighborEid.push_back(other);
                type.emplace_back(typeName(list.types[source]));
                neighborType.emplace_back(typeName(list.types[otherIndex]));
                sharedCount.push_back(static_cast<std::int64_t>(shared.size() - before));
                offsets.push_back(static_cast<std::int64_t>(shared.size()));
            }
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid));
        builder.int32("neighborElementId", std::move(neighborEid));
        builder.dictionary("type", type);
        builder.dictionary("neighborType", neighborType);
        builder.int64("sharedNodeCount", std::move(sharedCount));
        builder.intList("sharedNodes", std::move(offsets), std::move(shared));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementNeighbors";
        info.family = "T2";
        info.description =
            "One row for each element and each of its neighbours (a pair appears once for each element), by edge (they "
            "share a "
            "side or an edge: both ends) or by node (they share a node), with the shared nodes. A shell is its own "
            "face, so "
            "two shells on the same corners are neighbours by edge. Sorted by element, then neighbour.";
        info.parameters = {
            parameter("by", ParameterKind::Text, "\"edge\"", "\"edge\" or \"node\""),
            parameter("elementIds", ParameterKind::IdList, "None", "the elements to list the neighbours of"),
            parameter("ofTypes", ParameterKind::TextList, "None", "keep the neighbours of these card types")};
        info.defaultSort = "elementId, neighborElementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .int32("neighborElementId")
            .dictionary("type")
            .dictionary("neighborType")
            .int64("sharedNodeCount");
        columns.intList("sharedNodes");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addElementNeighborsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementNeighborsQuery>());
}

} // namespace oor::query
