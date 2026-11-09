#include "oor/analysis/superelements/PartBoundaries.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t14_superelements/SuperelementQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// partBoundaries: the boundary points of each superelement and the downstream point each is.
class PartBoundariesQuery final : public QueryBase
{
public:
    PartBoundariesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const analysis::PartBoundaries boundaries(run.model->root());
        const auto onlyParts = run.arguments.ids("partIds");
        std::vector<std::string> kind;
        std::vector<std::int32_t> partId;
        std::vector<std::int32_t> nid;
        std::vector<std::int32_t> downstreamId;
        std::vector<std::int32_t> downstreamNid;
        std::vector<std::uint8_t> downstreamValid;
        std::vector<std::int32_t> boundaryPoint;
        std::vector<std::string> source;
        std::vector<std::string> dof;
        std::vector<double> distance;
        for (const analysis::PartBoundaryRow& row : boundaries.rows())
        {
            if (onlyParts && std::find(onlyParts->begin(), onlyParts->end(), row.seid) == onlyParts->end())
                continue;
            kind.emplace_back("SuperelementPart");
            partId.push_back(row.seid);
            nid.push_back(row.nid);
            downstreamId.push_back(row.downstreamId);
            downstreamNid.push_back(row.downstreamNid.value_or(0));
            downstreamValid.push_back(row.downstreamNid ? 1 : 0);
            boundaryPoint.push_back(row.boundaryPoint);
            source.emplace_back(analysis::boundarySourceName(row.source));
            dof.push_back(row.dof);
            distance.push_back(row.distance);
        }
        TableBuilder builder;
        builder.dictionary("partKind", kind);
        builder.int32("partId", std::move(partId));
        builder.int32("nodeId", std::move(nid));
        builder.int32("downstreamId", std::move(downstreamId));
        builder.int32("downstreamNodeId", std::move(downstreamNid), std::move(downstreamValid));
        builder.int32("boundaryPoint", std::move(boundaryPoint));
        builder.dictionary("source", source);
        builder.text("dof", dof);
        builder.real("distance", std::move(distance));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "partBoundaries";
        info.family = "T14";
        info.description =
            "The boundary points of each superelement, one row per point of the superelement (nodeId in its own IDs: a "
            "part has an ID space of its own, an image lists the IDs of its primary, a list superelement those of the "
            "main Bulk Data) with the point of the downstream superelement it is (downstreamId: its SETREE, else 0, "
            "the residual structure; downstreamNodeId null where that superelement has no point there and the solver "
            "creates "
            "one). source says what made it a boundary point: `automatic` (SEBULK METHOD = AUTO: within the tolerance "
            "of a grid point of the residual structure or of another part searched automatically, after the SELOC and "
            "SEMPLN placement; the tolerance is the SECONCT TOL between the two, else their SEBULK TOL, else PARAM "
            "CONFAC), `SuperelementBoundarySearch` (the search restricted to the points of an SEBNDRY), "
            "`SuperelementConnection` (a pair of an SECONCT), `SuperelementExternalBoundary` (a point of an external "
            "superelement that nothing joins), `SuperelementInteriorNodes` (an exterior point of a list superelement: "
            "a "
            "point of its elements that is not interior to it), `SuperelementExteriorPoints` (CSUPEXT), "
            "`SuperelementCopyPoints` (CSUPER). boundaryPoint is the solver's boundary sequence number: the "
            "points of several superelements that are one boundary point share it. dof gives the components it "
            "connects (`123456`, less what a RELEASE frees for a list superelement, the EXTRN components of an "
            "external one, `0` for a scalar point); distance the distance after placement to the downstream point (to "
            "the nearest point it joins when the solver creates the downstream one), null without a location (a list "
            "superelement, a CSUPER, an SECONCT with LOC = NO, a scalar point). Auxiliary models have no boundary.";
        info.parameters = {
            parameter("partIds", ParameterKind::IdList, "None", "keep the points of these superelements")};
        info.defaultSort = "partId, nodeId";
        ColumnListBuilder columns;
        columns.dictionary("partKind").int32("partId").int32("nodeId").int32("downstreamId").int32("downstreamNodeId");
        columns.int32("boundaryPoint").dictionary("source").text("dof").real("distance");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPartBoundariesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PartBoundariesQuery>());
}

} // namespace oor::query
