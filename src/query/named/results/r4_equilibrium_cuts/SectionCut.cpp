#include "CutRows.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"
#include "oor/query/providers/quantities/CutGeometry.hpp"
#include "oor/query/providers/quantities/CutSession.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

namespace oor::query
{

namespace
{

// sectionCut: the resultant of the forces across a section: by a plane, or along a line of grid points.
class SectionCutQuery final : public ResultQuery
{
public:
    SectionCutQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string method = arguments.textOr("method", "auto");
        if (method != "auto" && method != "nodePointForces" && method != "integration")
            throw QueryError("method is \"auto\", \"nodePointForces\" or \"integration\", not '" + method + "'");
        const std::string detail = arguments.textOr("detail", "resultant");
        if (detail != "resultant" && detail != "nodes")
            throw QueryError("detail is \"resultant\" or \"nodes\", not '" + detail + "'");
        if (detail == "nodes" && method == "integration")
            throw QueryError(
                "detail=\"nodes\" gives the grid point forces of the side at each grid point of the cut: the "
                "integration of the shell forces gives none");
        const CutGeometry geometry(context, arguments.ids("elements"));
        std::shared_ptr<CutDefinition> cut;
        if (arguments.has("plane"))
        {
            const std::vector<double> plane = arguments.reals("plane");
            if (plane.size() != 6)
                throw QueryError(
                    "plane is (point, normal): six numbers, the point then the normal, in the basic system");
            cut = std::make_shared<CutDefinition>(
                geometry.byPlane({plane[0], plane[1], plane[2]}, {plane[3], plane[4], plane[5]}));
        }
        else if (arguments.has("nodes") && arguments.has("sideElements"))
        {
            cut = std::make_shared<CutDefinition>(
                geometry.byNodes(*arguments.ids("nodes"), *arguments.ids("sideElements")));
        }
        else
        {
            throw QueryError(
                "sectionCut needs the cut: `plane=(point, normal)`, or `nodes` (a line of grid points) with "
                "`sideElements` (the elements of one side)");
        }
        if (!hasLoadPath(context, *cut) && context.selection().onMissing == OnMissing::Raise)
            throw QueryError(
                "the cut passes through no grid point shared by the two sides and no rigid element or MPC equation "
                "links them: the plane meets no node, or the elements given have none in common with the rest");
        const bool inCutAxes = context.selection().frameName == "cut";
        if (inCutAxes && !cut->isPlane)
            throw QueryError("frame=\"cut\" (the axes of the cut, z along the normal) is defined for a plane only");
        const std::optional<std::array<double, 3>> xAxis = xAxisOf(arguments);
        if (xAxis && !inCutAxes)
            throw QueryError("xAxis fixes the x axis of frame=\"cut\": give frame=\"cut\" with it");
        const std::int64_t system = context.resultantSystem();

        ResultantRequest request;
        request.cut = cut;
        request.method = detail == "nodes" ? "nodePointForces" : method;
        request.point = arguments.point("point").value_or(cut->centroid);
        request.session = std::make_shared<CutSession>(context);
        request.nodeDetail = detail == "nodes";
        std::vector<ProviderInfo> providers;
        std::vector<ProviderAttempt> attempts;
        std::vector<ResultantRow> rows = resolveCut(context, request, providers, attempts);
        if (rows.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts(
                    detail == "nodes" ? "the forces on each grid point of the cut" : "the resultant across the cut",
                    attempts));
            return emptyTable(info().columnsFor(detail));
        }
        addCombinations(rows, arguments.combinations("combinations"));
        if (inCutAxes)
            turnRows(rows, cutAxes(cut->planeNormal, xAxis));
        else
            turnRowsToSystem(context, rows, system);
        RowAccumulator accumulator;
        if (detail == "nodes")
        {
            addCutNodeRows(rows, accumulator, accumulator.addProvider("nodePointForces", false));
            return accumulator.finish(context, info(), detail);
        }
        CutColumns columns;
        columns.elementCountColumn = "cutElementCount";
        addCutRows(rows, providers, request.point, columns, accumulator, registerCutProviders(providers, accumulator));
        return accumulator.finish(context, info(), detail);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "sectionCut";
        info.family = "R4";
        info.resultQuery = true;
        info.needsModel = true;
        info.detailParameter = "detail";
        info.description =
            "The resultant of the forces across a section of the structure, for each subcase and step: the force and "
            "moment that the side pushes on the rest across the cut, about `point` (the centroid of the cut grid "
            "points by default). The cut is a plane `plane=(point, normal)` (the side is the elements whose centre is "
            "strictly behind the plane, against the normal, which points from the side into the rest; an element "
            "whose centre lies in the plane is not on the side; the cut is the grid points the side shares with the "
            "others; `elements` restricts the cut to a group) or a line of grid points `nodes` with the elements of "
            "one side `sideElements`, the section of a member: the rows of the side elements at those grid points (a "
            "rigid element among the side elements is part of the side: the multiPointConstrForce rows of its grid "
            "points on the side are counted, as for a plane; a rigid element that is not a side element, or an MPC "
            "equation, at the other grid points of the side elements is not counted). `method=\"nodePointForces\"` "
            "sums the grid point forces of the side at the cut grid points (exact), and counts a rigid element or an "
            "MPC equation that links the two sides from the multiPointConstrForce rows of its grid points on the side "
            "when no other one acts there, or from its own rows wherever others act when the multiPointConstrForce "
            "rows are split by relation (a row per relation and grid point, `elementIds` the element ID of a rigid "
            "element or minus the set ID of an MPC equation); what links the two sides and is not counted (a row that "
            "cannot be split, an element with no grid point force) is named in `uncountedCards` and `uncountedIds`, "
            "and `isComplete` says whether the resultant is whole. `\"integration\"` integrates the forces of the "
            "shells over the cut (the force of each element taken as constant over it: an estimate, marked "
            "`integration (approximate)` in `source`; only shells count); `\"auto\"` uses the grid point forces and "
            "falls back on the integration. `frame=\"cut\"` gives the resultant along the axes of a plane cut (z "
            "along the normal, x the projection of `xAxis`, by default of the basic x axis). Complex results are "
            "summed as complex numbers. `combinations={name: {subcase: factor}}` adds the linear combinations of "
            "static subcases. `detail=\"nodes\"` gives the force of the side on each grid point of the cut, with the "
            "contribution of each cut connection on its grid points of the side.";
        info.parameters = {
            parameter("plane", ParameterKind::Reals, "None", "the cutting plane: (point, normal) in the basic system"),
            parameter("nodes", ParameterKind::IdList, "None", "a line of grid points the cut passes through"),
            parameter("sideElements", ParameterKind::IdList, "None",
                      "the elements of one side of the cut (with `nodes`)"),
            parameter("elements", ParameterKind::IdList, "None", "restrict the cut to a group of elements"),
            parameter("point", ParameterKind::Point, "None",
                      "the point the moment is taken about (default: the centroid of the cut)"),
            parameter("method", ParameterKind::Text, "\"auto\"", "\"auto\", \"nodePointForces\" or \"integration\""),
            parameter("xAxis", ParameterKind::Reals, "None",
                      "the x axis of frame=\"cut\", projected on the plane (a vector in the basic system)"),
            parameter("detail", ParameterKind::Text, "\"resultant\"", "\"resultant\" or \"nodes\""),
            parameter("combinations", ParameterKind::Combinations, "None",
                      "linear combinations of static subcases: {name: {subcase: factor}}")};
        info.defaultSort = "combination, subcase, [step]";
        ColumnListBuilder resultant;
        resultant.text("combination").int32("subcase").step().results("fx fy fz mx my mz");
        resultant.reals("pointX pointY pointZ").int64("cutElementCount").int64("nodeCount");
        resultant.flag("isComplete").textList("uncountedCards").intList("uncountedIds").dictionary("source");
        info.columnSets.push_back(resultant.build("resultant"));
        ColumnListBuilder nodes;
        nodes.text("combination").int32("subcase").step().int32("nodeId").results("fx fy fz mx my mz");
        nodes.dictionary("contribution");
        info.columnSets.push_back(nodes.build("nodes"));
        return info;
    }
};

} // namespace

void addSectionCutQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SectionCutQuery>());
}

} // namespace oor::query
