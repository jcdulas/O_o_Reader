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

// freebody: the resultant of the forces that the elements of a free body push on the grid points where it is cut from
// the rest of the model, or those forces grid point by grid point.
class FreebodyQuery final : public ResultQuery
{
public:
    FreebodyQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string detail = arguments.textOr("detail", "resultant");
        if (detail != "resultant" && detail != "nodes")
            throw QueryError("detail is \"resultant\" or \"nodes\", not '" + detail + "'");
        const auto elements = arguments.ids("elements");
        if (!elements || elements->empty())
            throw QueryError("freebody needs `elements`, the elements of the free body");
        const CutGeometry geometry(context, std::nullopt);
        auto cut = std::make_shared<CutDefinition>(geometry.freebody(*elements, arguments.ids("nodes")));
        if (!hasLoadPath(context, *cut))
            throw QueryError(
                "the free body has no grid point in common with the rest of the model and no rigid element or MPC "
                "equation links it to the rest (give `nodes`)");
        const std::int64_t system = context.resultantSystem();
        ResultantRequest request;
        request.cut = cut;
        request.point = arguments.point("point").value_or(cut->centroid);
        request.session = std::make_shared<CutSession>(context);
        request.nodeDetail = detail == "nodes";
        // the grid point forces first; the integration of the forces of the shells when the table is not there (the
        // force on each grid point is only in the grid point forces)
        request.method = detail == "nodes" ? "nodePointForces" : "auto";
        std::vector<ProviderInfo> providers;
        std::vector<ProviderAttempt> attempts;
        std::vector<ResultantRow> rows = resolveCut(context, request, providers, attempts);
        if (rows.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts(detail == "nodes"
                                                                    ? "the forces on each grid point of the free body"
                                                                    : "the resultant of the free body",
                                                                attempts));
            return emptyTable(info().columnsFor(detail));
        }
        addCombinations(rows, arguments.combinations("combinations"));
        turnRowsToSystem(context, rows, system);
        RowAccumulator accumulator;
        if (detail == "nodes")
        {
            addCutNodeRows(rows, accumulator, accumulator.addProvider("nodePointForces", false));
            return accumulator.finish(context, info(), "nodes");
        }
        addCutRows(rows, providers, request.point, CutColumns{}, accumulator,
                   registerCutProviders(providers, accumulator));
        return accumulator.finish(context, info(), "resultant");
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "freebody";
        info.family = "R4";
        info.resultQuery = true;
        info.needsModel = true;
        info.detailParameter = "detail";
        info.description =
            "The free body made of `elements`: the force and moment that its elements push on the grid points where "
            "it is cut from the rest of the model (the grid points it shares with other elements, or `nodes`), added "
            "up about `point` (the centroid of those grid points by default) in the basic system or along the axes of "
            "the coordinate system `frame`. They come from the grid point forces, with the rigid elements and MPC "
            "equations that link the body to the rest counted from the multiPointConstrForce rows of their grid "
            "points in the body when no other one acts there, or from their own rows when the multiPointConstrForce "
            "rows are split by relation (a row per relation and grid point, `elementIds` the relation); what is not "
            "counted is named in `uncountedCards` and `uncountedIds` and `isComplete` is false. A free body of shells "
            "without a grid point force table gets them integrated from the forces of its shells (marked approximate "
            "in `source`). Complex results are summed as complex numbers; `combinations={name: {subcase: factor}}` "
            "adds linear combinations of static subcases. With `detail=\"nodes\"` the force on each cut grid point is "
            "given, with the contribution of each connection.";
        info.parameters = {
            parameter("elements", ParameterKind::IdList, "(required)", "the elements of the free body"),
            parameter("nodes", ParameterKind::IdList, "None", "the grid points of the cut (default: the interface)"),
            parameter("point", ParameterKind::Point, "None",
                      "the point the moment is taken about (default: the centroid of the interface)"),
            parameter("detail", ParameterKind::Text, "\"resultant\"", "\"resultant\" or \"nodes\""),
            parameter("combinations", ParameterKind::Combinations, "None",
                      "linear combinations of static subcases: {name: {subcase: factor}}")};
        info.defaultSort = "combination, subcase, [step]";
        ColumnListBuilder resultant;
        resultant.text("combination").int32("subcase").step().results("fx fy fz mx my mz");
        resultant.reals("pointX pointY pointZ").int64("nodeCount");
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

void addFreebodyQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FreebodyQuery>());
}

} // namespace oor::query
