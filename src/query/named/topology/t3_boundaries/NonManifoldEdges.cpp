#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// nonManifoldEdges: the edges that three shells or more have (T junctions).
class NonManifoldEdgesQuery final : public QueryBase
{
public:
    NonManifoldEdgesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const ShellSet shells(run, run.arguments.ids("elementIds"), ShellNeeds{});
        const ShellEdges edges(run, shells);
        const NodePositions positions(*run.model, run.context.pool());
        std::vector<std::int32_t> edgeId;
        std::vector<std::int32_t> n1;
        std::vector<std::int32_t> n2;
        std::vector<double> length;
        std::vector<std::int64_t> elementCount;
        std::vector<std::int64_t> elementOffsets{0};
        std::vector<std::int32_t> elements;
        std::vector<std::int64_t> propertyOffsets{0};
        std::vector<std::int32_t> properties;
        std::vector<std::int32_t> distinct;
        for (const ShellEdge& edge : edges.edges())
        {
            if (edge.count < 3)
                continue;
            const analysis::EdgeKey key = edges.key(edge);
            edgeId.push_back(static_cast<std::int32_t>(edge.edge));
            n1.push_back(key.first);
            n2.push_back(key.second);
            length.push_back(positions.distance(key.first, key.second));
            elementCount.push_back(static_cast<std::int64_t>(edge.count));
            distinct.clear();
            for (const std::uint32_t shell : edges.shellsOf(edge))
            {
                elements.push_back(shells.id(shell));
                if (shells.property(shell) != 0)
                    distinct.push_back(shells.property(shell));
            }
            elementOffsets.push_back(static_cast<std::int64_t>(elements.size()));
            std::sort(distinct.begin(), distinct.end());
            distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
            properties.insert(properties.end(), distinct.begin(), distinct.end());
            propertyOffsets.push_back(static_cast<std::int64_t>(properties.size()));
        }
        TableBuilder builder;
        builder.int32("edgeId", std::move(edgeId));
        builder.int32("n1", std::move(n1));
        builder.int32("n2", std::move(n2));
        builder.real("length", std::move(length));
        builder.int64("elementCount", std::move(elementCount));
        builder.intList("elements", std::move(elementOffsets), std::move(elements));
        builder.intList("propertyIds", std::move(propertyOffsets), std::move(properties));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "nonManifoldEdges";
        info.family = "T3";
        info.description =
            "The edges shared by three shells or more (T junctions): the edge, its length, the shells on it and their "
            "properties. A line element or a solid on the edge does not count. With elementIds only those shells are "
            "counted.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "only these elements are counted")};
        info.defaultSort = "edgeId";
        ColumnListBuilder columns;
        columns.int32("edgeId")
            .int32("n1")
            .int32("n2")
            .real("length")
            .int64("elementCount")
            .intList("elements")
            .intList("propertyIds");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNonManifoldEdgesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NonManifoldEdgesQuery>());
}

} // namespace oor::query
