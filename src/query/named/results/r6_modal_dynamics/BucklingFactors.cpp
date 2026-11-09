#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"
#include "oor/query/providers/quantities/ModalQuantities.hpp"

#include <algorithm>
#include <tuple>

namespace oor::query
{

namespace
{

// bucklingFactors: the load factors of a buckling analysis, one row for each mode.
class BucklingFactorsQuery final : public ResultQuery
{
public:
    BucklingFactorsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<ModeRow> modes = modeRowsOf(context, ModeAnalysis::Buckling);
        if (modes.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(modes.begin(), modes.end(),
                         [](const ModeRow& a, const ModeRow& b)
                         {
                             return std::tie(a.designCycle, a.set, a.subcase, a.mode) <
                                    std::tie(b.designCycle, b.set, b.subcase, b.mode);
                         });
        std::vector<std::int32_t> subcase, mode;
        std::vector<double> factor;
        bool single = true;
        for (const ModeRow& row : modes)
        {
            subcase.push_back(row.subcase);
            mode.push_back(row.mode);
            factor.push_back(row.eigenvalue);
            single = single && row.precision == Precision::Single;
        }
        RowBlock block(modes.size());
        block.ints("subcase", std::move(subcase));
        block.ints("mode", std::move(mode));
        block.reals("loadFactor", std::move(factor));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        block.provider(accumulator.addProvider("stored table", false));
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "bucklingFactors";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "The load factors of a buckling analysis, one row for each subcase and mode: the eigenvalue of the "
            "buckling table "
            "(the factor that multiplies the load to reach the critical load). The table carries no subcase: it is the "
            "one "
            "subcase of the eigenvectors when there is only one, else 0.";
        info.defaultSort = "subcase, mode";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("mode").result("loadFactor");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addBucklingFactorsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<BucklingFactorsQuery>());
}

} // namespace oor::query
