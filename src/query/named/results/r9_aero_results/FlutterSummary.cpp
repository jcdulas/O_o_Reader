#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

// aeroFlutterSummary: the points of the flutter curves: for each mode, the velocity, the damping and the frequency.
class FlutterSummaryQuery final : public ResultQuery
{
public:
    FlutterSummaryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::set<std::int32_t> wanted;
        const auto selection = context.arguments().steps("modes");
        const bool filtered = selection && !selection->isSlice;
        if (filtered)
            for (const double mode : selection->values)
                wanted.insert(static_cast<std::int32_t>(std::lround(mode)));
        std::vector<ResultQueryContext::NamedRef> results = context.named("aeroFlutterSummary");
        std::stable_sort(results.begin(), results.end(),
                         [](const auto& a, const auto& b)
                         {
                             return std::tie(a.result->meta().designCycle, a.set, a.result->meta().subcase) <
                                    std::tie(b.result->meta().designCycle, b.set, b.result->meta().subcase);
                         });
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* mode = data->find("mode");
            const results::ResultArray* mach = data->find("mach");
            const results::ResultArray* density = data->find("densityRatio");
            const results::ResultArray* velocity = data->find("velocity");
            const results::ResultArray* damping = data->find("damping");
            const results::ResultArray* frequency = data->find("frequency");
            if (mode == nullptr || mach == nullptr || density == nullptr || velocity == nullptr || damping == nullptr ||
                frequency == nullptr)
                continue;
            const std::vector<std::int32_t> modes = arrayInts(*mode);
            const std::vector<double> m = arrayDoubles(*mach);
            const std::vector<double> rho = arrayDoubles(*density);
            const std::vector<double> v = arrayDoubles(*velocity);
            const std::vector<double> g = arrayDoubles(*damping);
            const std::vector<double> f = arrayDoubles(*frequency);
            const double chord = attributeOf(*ref.result, "referenceChord");
            std::vector<std::size_t> kept;
            for (std::size_t row = 0; row < modes.size(); ++row)
                if (!filtered || wanted.contains(modes[row]))
                    kept.push_back(row);
            if (kept.empty())
                continue;
            std::vector<std::int32_t> modeColumn;
            std::vector<double> machColumn, densityColumn, reduced, velocityColumn, dampingColumn, frequencyColumn,
                real, imaginary;
            for (const std::size_t row : kept)
            {
                modeColumn.push_back(modes[row]);
                machColumn.push_back(m[row]);
                densityColumn.push_back(rho[row]);
                // the reduced frequency k = omega b / V with the semi-chord b: from the reference chord of the table
                reduced.push_back(!std::isnan(chord) && v[row] > 0.0 ? std::numbers::pi * f[row] * chord / v[row]
                                                                     : std::nan(""));
                velocityColumn.push_back(v[row]);
                dampingColumn.push_back(g[row]);
                frequencyColumn.push_back(f[row]);
                // the eigenvalue of the PK method p = omega (g / 2 + i)
                const double omega = 2.0 * std::numbers::pi * f[row];
                real.push_back(omega * g[row] / 2.0);
                imaginary.push_back(omega);
            }
            RowBlock block(kept.size());
            block.ints("subcase", std::vector<std::int32_t>(kept.size(), ref.result->meta().subcase));
            block.ints("mode", std::move(modeColumn));
            block.reals("mach", std::move(machColumn));
            block.reals("densityRatio", std::move(densityColumn));
            block.reals("reducedFrequency", std::move(reduced));
            block.reals("velocity", std::move(velocityColumn));
            block.reals("damping", std::move(dampingColumn));
            block.reals("frequency", std::move(frequencyColumn));
            block.reals("eigenvalueRe", std::move(real));
            block.reals("eigenvalueIm", std::move(imaginary));
            block.origin(ref.set, context.sets()[ref.set].analysis, ref.result->meta().designCycle);
            block.precision(precisionOfNamed(*data));
            block.validation(statusOfNamed(*ref.result));
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroFlutterSummary";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "The points of the flutter curves, in the order of the table: for each subcase and mode the Mach number, "
            "the density "
            "ratio, the velocity, the damping, the frequency, the reduced frequency (from the reference chord when the "
            "table "
            "gives it, else null) and the eigenvalue of the PK method (`omega * damping / 2` and `omega`, the circular "
            "frequency). `modes` keeps some mode numbers.";
        info.parameters = {parameter("modes", ParameterKind::Steps, "None", "the mode numbers to keep")};
        info.defaultSort = "subcase, mode (the order of the table)";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("mode").results("mach densityRatio").real("reducedFrequency");
        columns.results("velocity damping frequency").reals("eigenvalueRe eigenvalueIm");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFlutterSummaryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FlutterSummaryQuery>());
}

} // namespace oor::query
