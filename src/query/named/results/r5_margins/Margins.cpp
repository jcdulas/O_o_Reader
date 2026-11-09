#include "MarginData.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r5_margins/MarginQueries.hpp"

#include <map>
#include <tuple>

namespace oor::query
{

namespace
{

// margins: the reserve factor and the margin of safety of each element by a criterion.
class MarginsQuery final : public ResultQuery
{
public:
    MarginsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const MarginSet set = computeMargins(context);
        if (set.rows.empty())
            return emptyTable(info().defaultColumns());
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : set.providers)
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        // one block for each (design cycle, result set, subcase)
        std::map<std::tuple<std::int32_t, std::size_t, std::int32_t, int>, std::vector<const MarginRow*>> groups;
        for (const MarginRow& row : set.rows)
            groups[{row.designCycle, row.set, row.subcase, static_cast<int>(row.stepKind)}].push_back(&row);
        for (const auto& [key, rows] : groups)
            accumulator.add(marginBlock(context, rows, providers));
        return sortedResult(accumulator.finish(context, info()), context.exec(), {"ms", "elementId"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "margins";
        info.family = "R5";
        info.resultQuery = true;
        info.description =
            "The reserve factor `rf` and the margin of safety `ms` (`rf - 1`) of each element, for each subcase and "
            "step, by a "
            "criterion, at the place of the element where it is the most critical (`layer`, `location`). `vonMises`, "
            "`maxPrincipal` (against the tension allowable ST of the MAT1) and `minPrincipal` (against the "
            "compression "
            "allowable SC) judge the stress of the shells, solids and rods; `rf = allowable / (factor * stress)`. "
            "`composite` "
            "judges the failure index of the plies (the stored index, else recomputed with the theory of the property "
            "or the "
            "`theory` asked for) against the allowable 1, ply by ply: `rf` is the factor the loads can be multiplied "
            "by before the index of a ply reaches the allowable (its strength ratio: the index itself for the linear "
            "theories, the root of the quadratic for Tsai-Wu, Hill and Hoffman), and the element's is the smallest of "
            "its plies, given with that ply (`layer`, `value` its index; with a quadratic theory it need not be the "
            "ply of the largest index). `bond` judges the bonding index of the plies the same way: the solver's "
            "failure "
            "index for bonding (the interlaminar shear against the PCOMP SB) when the result file holds it, else the "
            "larger of `t1z`, `t2z` over SB, linear in the loads. `fastener` judges the tension and shear of the "
            "fasteners by the "
            "interaction `(T/Ta)^p + (S/Sa)^q` with `exponents=(p, q)`; `allowables` gives (Ta, Sa). `allowables` is a "
            "dict "
            "keyed by (\"material\", mid), (\"property\", propId) or (\"element\", elementId), the most specific key "
            "winning, "
            "or one number "
            "for all; `allowableSource` says where each allowable came from. The query needs the model unless every "
            "allowable is given. Sorted by `ms`, the most critical first.";
        info.parameters = marginParameters();
        info.parameters.push_back(parameter("elementIds", ParameterKind::IdList, "None", "the elements to judge"));
        info.parameters.push_back(parameter("types", ParameterKind::TextList, "None", "the element types"));
        info.parameters.push_back(
            parameter("propertyIds", ParameterKind::IdList, "None", "the properties (needs the model)"));
        info.defaultSort = "ms (ascending)";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .int32("matId")
            .int32("layer")
            .dictionary("location");
        columns.dictionary("criterion")
            .result("value")
            .real("allowable")
            .dictionary("allowableSource")
            .real("factor")
            .real("rf")
            .real("ms");
        columns.result("tension").result("shear").real("tensionAllowable").real("shearAllowable");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMarginsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MarginsQuery>());
}

} // namespace oor::query
