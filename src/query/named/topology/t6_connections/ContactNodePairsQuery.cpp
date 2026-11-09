#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

namespace
{

// contactNodePairs: the grid points of the secondary surfaces that meet the primary surfaces, with what a solver
// needs to tie them (glued contact for a linear solver).
class ContactNodePairsQuery final : public QueryBase
{
public:
    ContactNodePairsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        analysis::ContactOptions options = contactOptionsOf(run.arguments, "contactNodePairs");
        const analysis::ContactRule rule = contactRuleOf(run.arguments, "contactNodePairs");
        options.mode = analysis::ContactMode::Connection; // component1 and component2 are those of this mode
        const auto components = analysis::ComponentModel::of(model, options, run.context.pool());
        std::shared_ptr<const analysis::ContactSearch> solverSearch;
        if (rule == analysis::ContactRule::Solver)
            solverSearch = analysis::ContactSearch::of(model, options.tolerance, rule, run.context.pool());
        const analysis::ContactSearch* search = solverSearch ? solverSearch.get() : components->contactSearch();
        std::optional<std::vector<std::int64_t>> tables = run.arguments.ids("tables");
        if (tables)
            std::sort(tables->begin(), tables->end());
        if (search == nullptr)
            return emptyTable(info().defaultColumns());

        std::vector<std::string> table;
        std::vector<std::uint8_t> tableValid;
        std::vector<std::int32_t> secondary;
        std::vector<std::int32_t> primary;
        std::vector<std::int32_t> nid;
        std::vector<std::int32_t> primaryEid;
        std::vector<std::uint8_t> primaryEidValid;
        std::array<std::vector<double>, 3> point;
        std::vector<double> distance;
        std::vector<std::int64_t> component1;
        std::vector<std::uint8_t> component1Valid;
        std::vector<std::int64_t> component2;
        std::vector<std::uint8_t> component2Valid;
        std::vector<std::string> kind;
        std::vector<std::int64_t> faceOffsets{0};
        std::vector<std::int32_t> faceNodes;
        std::vector<double> xi;
        std::vector<double> eta;
        std::vector<double> gap;
        std::vector<std::string> searchedFrom;
        std::array<std::vector<double>, 3> surfaceOffset;
        for (std::size_t index = 0; index < search->pairs().size(); ++index)
        {
            const analysis::ContactPair& pair = search->pairs()[index];
            if (tables && (!pair.table || !std::binary_search(tables->begin(), tables->end(),
                                                              static_cast<std::int64_t>(pair.tableId))))
                continue;
            for (const analysis::ContactMeeting& meeting : search->results()[index].meetings)
            {
                table.push_back(pair.tableLabel());
                tableValid.push_back(pair.table ? 1 : 0);
                secondary.push_back(meeting.secondary->id);
                primary.push_back(meeting.primary->id);
                nid.push_back(meeting.nid);
                primaryEid.push_back(meeting.face->eid);
                primaryEidValid.push_back(meeting.face->element ? 1 : 0);
                for (std::size_t axis = 0; axis < 3; ++axis)
                    point[axis].push_back(meeting.point[axis]);
                distance.push_back(meeting.distance);
                const std::int32_t first = components->componentOfNode(meeting.nid);
                const std::int32_t second = meeting.face->element
                                                ? components->componentOfElement(*meeting.face->element)
                                                : components->componentOfNode(meeting.face->nodes[0]);
                component1.push_back(first);
                component1Valid.push_back(first != 0 ? 1 : 0);
                component2.push_back(second);
                component2Valid.push_back(second != 0 ? 1 : 0);
                kind.emplace_back(analysis::contactKindName(pair.kind));
                meeting.face->appendLayout(faceNodes);
                faceOffsets.push_back(static_cast<std::int64_t>(faceNodes.size()));
                xi.push_back(meeting.coordinates.xi);
                // a segment of a BLSEG line has one coordinate
                eta.push_back(meeting.face->cornerCount == 2 ? std::nan("") : meeting.coordinates.eta);
                gap.push_back(meeting.gap);
                searchedFrom.emplace_back(analysis::searchSideName(meeting.searchedFrom));
                for (std::size_t axis = 0; axis < 3; ++axis)
                    surfaceOffset[axis].push_back(meeting.surfaceOffset[axis]);
            }
        }
        if (nid.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.text("table", table, std::move(tableValid));
        builder.int32("secondaryBody", std::move(secondary));
        builder.int32("primaryBody", std::move(primary));
        builder.int32("nodeId", std::move(nid));
        builder.int32("primaryElementId", std::move(primaryEid), std::move(primaryEidValid));
        builder.real("x", std::move(point[0]));
        builder.real("y", std::move(point[1]));
        builder.real("z", std::move(point[2]));
        builder.real("distance", std::move(distance));
        builder.int64("component1", std::move(component1), std::move(component1Valid));
        builder.int64("component2", std::move(component2), std::move(component2Valid));
        builder.dictionary("kind", kind);
        builder.intList("primaryFace", std::move(faceOffsets), std::move(faceNodes));
        builder.real("xi", std::move(xi));
        builder.real("eta", std::move(eta));
        builder.real("gap", std::move(gap));
        builder.dictionary("searchedFrom", searchedFrom);
        builder.real("surfaceDx", std::move(surfaceOffset[0]));
        builder.real("surfaceDy", std::move(surfaceOffset[1]));
        builder.real("surfaceDz", std::move(surfaceOffset[2]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "contactNodePairs";
        info.family = "T6";
        info.description =
            "Contact between bodies: one row per grid point of a secondary surface "
            "that meets the primary surface, with the face it meets (primaryFace: its grid points in the order of "
            "the element, a solid face in the order that turns its normal outward; on the face of a quadratic element "
            "its corners then the mid-side points of its edges in order, corner 1-2, 2-3, ..., 0 for an edge with "
            "none; primaryElementId, null on a BLSEG segment), the projection on that face (x y z, basic system; xi "
            "and eta "
            "its local coordinates: -1 to 1 on a quadrilateral from corner 1, the area coordinates L2 and L3 on a "
            "triangle, on the quadratic surface of a quadratic face, eta null on a segment). The grid points of a "
            "surface are the corners and the mid-side points of its faces. gap "
            "(the signed distance between the skins, the shell thickness and ZOFFS included, positive outside the "
            "primary body) and distance (gap, 0 where the skins touch or overlap); the thickness of a grid point "
            "counts along the normal of the face, and a grid point whose shells are coplanar with the face it meets, "
            "beside it at its boundary (shells glued edge to edge), takes none. The pairs are those of every "
            "contact table of the Bulk Data (BCTABLE, BCONECT, BCTSET, BCONP; table is the card and its ID), or every "
            "two deformable bodies that meet when there is none (table null). rule=\"components\" (the default) gives "
            "the pairs of the components: the distance is the ERROR or DQNEAR of the pair (the MAXD of a BCTSET "
            "pair), else contactTolerance, else 5 % of the median edge length of the secondary surface, a grid point "
            "inside a shell's thickness meets it, and a pair whose secondary surface finds nothing is searched from "
            "its primary surface (ISEARCH 0 or 2). rule=\"solver\" gives the pairs the solver ties for a glued pair: "
            "the solver's ERROR and BIAS, the double-order search of ISEARCH 0, "
            "and no grid point tied twice, to its own face, or already dependent; a grid point of shells meets the "
            "face of a solid by the point of its shells' surface, (ZOFFS +- T/2) along its vertex normal (the normals "
            "of the corners of its shells weighted as Nelson Max weighs them) on the side nearer the face (the top "
            "side when both are as near): the face, xi, eta, x y z and gap are those of that point, and surfaceDx, "
            "surfaceDy, surfaceDz the vector from the grid point to it (0 for a grid point that meets the face "
            "itself, and under rule=\"components\"). searchedFrom is secondary for a "
            "grid point of the pair's secondary body, primary for one of its primary body (secondaryBody then names "
            "the primary body). component1 and component2 are the components of the grid point and of the primary "
            "element when contact is a connection; kind is glued, breakable or touching. tables keeps the pairs of "
            "these table IDs. The rules are written out in docs/developer/companion-packages.md, section 3.14.";
        info.parameters = {parameter("tables", ParameterKind::IdList, "None", "keep the pairs of these table IDs"),
                           contactToleranceParameter(), contactRuleParameter()};
        info.defaultSort = "the order of the pairs (table, secondaryBody, primaryBody), then searchedFrom, nodeId";
        ColumnListBuilder columns;
        columns.text("table").int32("secondaryBody").int32("primaryBody").int32("nodeId").int32("primaryElementId");
        columns.reals("x y z").real("distance").int64("component1").int64("component2").dictionary("kind");
        columns.intList("primaryFace").real("xi").real("eta").real("gap").dictionary("searchedFrom");
        columns.real("surfaceDx").real("surfaceDy").real("surfaceDz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addContactNodePairsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ContactNodePairsQuery>());
}

} // namespace oor::query
