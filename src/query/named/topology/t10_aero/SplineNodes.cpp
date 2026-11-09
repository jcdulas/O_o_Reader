#include "AeroQuerySupport.hpp"
#include "SplineEntries.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <optional>

namespace oor::query
{

namespace
{

// splineNodes: the structural points each spline is attached to, with the component each belongs to.
class SplineNodesQuery final : public QueryBase
{
public:
    SplineNodesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("splineIds"));
        std::vector<std::int32_t> spline, node;
        for (const aero::SplineEntry& entry : aero::readSplines(model))
        {
            if (wanted && !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{entry.eid}))
                continue;
            for (const base::EntityId point : pointsOf(model, entry))
            {
                spline.push_back(entry.eid);
                node.push_back(point);
            }
        }
        if (spline.empty())
            return emptyTable(info().defaultColumns());

        // The component of a node is null for an ID that is no grid point and for a node no component uses.
        const auto components = analysis::ComponentModel::of(model, run.context.pool());
        std::vector<std::int32_t> component(node.size());
        std::vector<std::uint8_t> componentValid(node.size());
        for (std::size_t row = 0; row < node.size(); ++row)
        {
            component[row] = components->componentOfNode(node[row]);
            componentValid[row] = component[row] > 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("spline", std::move(spline));
        builder.int32("nodeId", std::move(node));
        builder.int32("componentId", std::move(component), std::move(componentValid));
        return builder.finish();
    }

private:
    // The distinct points of a spline in increasing order: the IDs of its SETG, or the points of a SPLINE3.
    static std::vector<base::EntityId> pointsOf(const model::Model& model, const aero::SplineEntry& entry)
    {
        if (entry.hasPointList)
            return entry.points;
        if (entry.setg == 0)
            return {};
        const std::optional<aero::IdRuns> members = aero::setMembers(model, entry.setg);
        return members ? members->expand("SET " + std::to_string(entry.setg) + " of the spline " +
                                         std::to_string(entry.eid))
                       : std::vector<base::EntityId>();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "splineNodes";
        info.family = "T10";
        info.description =
            "One row for each structural point of each spline: the distinct IDs of the SET1 or SET3 cards of its SETG "
            "in increasing order (the points G of a SPLINE3). `componentId` is the component of the model the node "
            "belongs to (T4); it is null for an ID that is not a grid point (a scalar point, an ID with no card) and "
            "for a node no component uses.";
        info.parameters = {parameter("splineIds", ParameterKind::IdList, "None", "the splines to list")};
        info.defaultSort = "spline, nodeId";
        ColumnListBuilder columns;
        columns.int32("spline").int32("nodeId").int32("componentId");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSplineNodesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SplineNodesQuery>());
}

} // namespace oor::query
