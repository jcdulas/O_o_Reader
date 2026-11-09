#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"
#include "oor/query/providers/quantities/ModalQuantities.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// modeShapeMax: for each mode, the grid point where the mode shape is largest (by the norm of its translations).
class ModeShapeMaxQuery final : public ResultQuery
{
public:
    ModeShapeMaxQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto nodes = context.arguments().ids("nodeIds");
        std::set<std::int64_t> wanted;
        if (nodes)
            wanted.insert(nodes->begin(), nodes->end());
        std::vector<TableRef> tables = context.tables(results::ResultKind::Eigenvectors);
        sortTables(tables);
        // the mechanism shapes of a singular model (made in memory by a solver) after the modes, flagged
        std::vector<TableRef> mechanisms = context.tables(results::ResultKind::MechanismShapes);
        sortTables(mechanisms);
        const std::size_t modeTables = tables.size();
        tables.insert(tables.end(), mechanisms.begin(), mechanisms.end());
        const std::vector<ModeRow> modes = modeRowsOf(context, ModeAnalysis::RealModes);
        std::vector<std::int32_t> subcase, mode, nid;
        std::vector<double> frequency, magnitude;
        std::vector<std::uint8_t> mechanism;
        std::array<std::vector<double>, 3> translation;
        bool single = true;
        for (std::size_t tableIndex = 0; tableIndex < tables.size(); ++tableIndex)
        {
            const TableRef& ref = tables[tableIndex];
            const bool isMechanism = tableIndex >= modeTables;
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const int t1 = values.component("t1");
            if (t1 < 0 || values.isComplex())
                continue;
            for (std::size_t step = 0; step < values.steps(); ++step)
            {
                double best = -1.0;
                std::size_t bestRow = 0;
                bool found = false;
                for (std::size_t row = 0; row < values.rows(); ++row)
                {
                    if (nodes && !wanted.contains(decoded->ids[row]))
                        continue;
                    if (row < decoded->pointTypes.size() && decoded->pointTypes[row] != 1)
                        continue; // a scalar point has no translations
                    double sum = 0.0;
                    for (int offset = 0; offset < 3; ++offset)
                    {
                        const double value = values.real(step, row, static_cast<std::size_t>(t1 + offset));
                        sum += value * value;
                    }
                    if (!found || sum > best)
                    {
                        best = sum;
                        bestRow = row;
                        found = true;
                    }
                }
                if (!found)
                    continue;
                const auto modeNumber = static_cast<std::int32_t>(std::lround(decoded->steps[step]));
                // the frequency: the eigenvalue table of the file, else the cycles of the eigenvector table when they
                // agree with its eigenvalue (some files write the circular frequency there), else derived from that
                // eigenvalue
                double hertz = NOT_A_NUMBER;
                for (const ModeRow& row : modes)
                    if (std::isnan(hertz) && !isMechanism && row.set == ref.set && row.mode == modeNumber)
                        hertz = row.cycles;
                if (std::isnan(hertz))
                {
                    const double stored = step < decoded->cycles.size() ? decoded->cycles[step] : NOT_A_NUMBER;
                    const double eigenvalue =
                        step < decoded->eigenvalues.size() ? decoded->eigenvalues[step] : NOT_A_NUMBER;
                    const double derived =
                        eigenvalue > 0.0 ? std::sqrt(eigenvalue) / (2.0 * std::numbers::pi) : NOT_A_NUMBER;
                    const bool agree =
                        !std::isnan(stored) && (std::isnan(derived) || std::fabs(stored - derived) <= 1.0e-3 * derived);
                    hertz = agree || std::isnan(derived) ? stored : derived;
                }
                // a mechanism moves without strain: its frequency is zero unless the table gives one
                if (isMechanism && std::isnan(hertz))
                    hertz = 0.0;
                subcase.push_back(meta.subcase);
                mode.push_back(modeNumber);
                frequency.push_back(hertz);
                nid.push_back(decoded->ids[bestRow]);
                magnitude.push_back(std::sqrt(best));
                mechanism.push_back(isMechanism ? 1 : 0);
                for (int offset = 0; offset < 3; ++offset)
                    translation[static_cast<std::size_t>(offset)].push_back(
                        values.real(step, bestRow, static_cast<std::size_t>(t1 + offset)));
                single = single && decoded->type == results::ValueType::Float32;
            }
        }
        if (subcase.empty())
            return emptyTable(info().defaultColumns());
        RowBlock block(subcase.size());
        block.ints("subcase", std::move(subcase));
        block.ints("mode", std::move(mode));
        block.reals("freq", std::move(frequency));
        block.ints("nodeId", std::move(nid));
        block.reals("magnitude", std::move(magnitude));
        block.reals("t1", std::move(translation[0]));
        block.reals("t2", std::move(translation[1]));
        block.reals("t3", std::move(translation[2]));
        block.flags("isMechanism", std::move(mechanism));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        block.provider(accumulator.addProvider("stored table", false));
        accumulator.add(std::move(block));
        return sortedResult(accumulator.finish(context, info()), context.exec(),
                            {"designCycle", "analysis", "subcase", "mode"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "modeShapeMax";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "For each mode, the grid point where the mode shape is largest (the norm of its three translations) with "
            "that norm "
            "and the translations: the frequency of the mode is the one the eigenvector table carries, else that of "
            "the "
            "eigenvalue table. The mechanism shapes of a singular model (`mechanismShapes`, made in memory by a "
            "solver) follow, with `isMechanism` true and a frequency of zero unless their table gives one.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to look at")};
        info.defaultSort = "subcase, mode";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("mode").real("freq").int32("nodeId").result("magnitude").results("t1 t2 t3");
        columns.flag("isMechanism");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addModeShapeMaxQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ModeShapeMaxQuery>());
}

} // namespace oor::query
