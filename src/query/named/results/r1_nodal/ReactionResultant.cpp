#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r1_nodal/NodalResultQueries.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

namespace oor::query
{

namespace
{

// reactionResultant: the force and moment of the reactions of each subcase and step about a point. The reactions come
// from the SPC force table, else from the grid point force balance.
class ReactionResultantQuery final : public ResultQuery
{
public:
    ReactionResultantQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        ResultantRequest request;
        request.point = context.arguments().point("point").value_or(std::array<double, 3>{0.0, 0.0, 0.0});
        const std::int64_t system = context.resultantSystem();
        ResultantSet set;
        const std::vector<ProviderAttempt> attempts = reactionChain().resolve(context, request, set);
        if (set.rowCount() == 0)
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts("the reactions", attempts));
            return emptyTable(info().defaultColumns());
        }
        RowAccumulator accumulator;
        addResultantRows(context, set, request, system, accumulator, registerProviders(set, accumulator));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "reactionResultant";
        info.family = "R1";
        info.resultQuery = true;
        info.needsModel = true;
        info.description =
            "The resultant of the reactions of each subcase and step: the sum of the forces of the constraints and of "
            "their moments about `point` (basic system, the origin by default), in the basic system or along the axes "
            "of the coordinate system `frame`. The reactions come from the SPC force table of the result file, else "
            "from the singlePointConstrForce rows of the grid point force balance (`source` says which); the grid "
            "points are those of the model (the moments need their positions).";
        info.parameters = {parameter("point", ParameterKind::Point, "(0, 0, 0)",
                                     "the point the moment is taken about (basic system)")};
        info.defaultSort = "subcase, [step]";
        ColumnListBuilder columns;
        columns.int32("subcase").step().results("fx fy fz mx my mz").reals("pointX pointY pointZ").int64("nodeCount");
        columns.dictionary("source");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addReactionResultantQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ReactionResultantQuery>());
}

} // namespace oor::query
