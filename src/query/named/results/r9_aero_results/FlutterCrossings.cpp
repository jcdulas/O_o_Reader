#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <utility>

namespace oor::query
{

namespace
{

// The rows of a flutter summary, one entry for each point.
struct SummaryPoints
{
    std::vector<std::int32_t> mode;
    std::vector<std::int32_t> newCurve; // 1 at the first point of a curve
    std::vector<double> mach;
    std::vector<double> density;
    std::vector<double> velocity;
    std::vector<double> damping;
    std::vector<double> frequency;
};

// The curves the damping is followed along: the rows of each, in the order of the segments between them.
//
// The points of one mode, up to a point that the table says starts a new curve, are one run. A run is either
//   - a velocity sweep at each Mach number and density ratio (the K and PK methods loop over them): some pair of Mach
//     number and density ratio has several points, and each pair is a curve whose points are taken by increasing
//     velocity; or
//   - a path (the no-looping methods PKNL and PKNLS pair the lists: each point has its own Mach number, density ratio
//   and
//     velocity): no pair repeats, and the run is one curve in the order of the table, which is the order of the path.
// A run of one point has no segment.
std::vector<std::vector<std::size_t>> curvesOf(const SummaryPoints& points)
{
    std::vector<std::vector<std::size_t>> curves;
    const std::size_t count = points.mode.size();
    std::size_t begin = 0;
    while (begin < count)
    {
        std::size_t end = begin + 1;
        while (end < count && points.mode[end] == points.mode[begin] && points.newCurve[end] == 0)
            ++end;
        // the points of the run by (Mach number, density ratio), in the order each pair first appears
        std::vector<std::pair<double, double>> pairs;
        std::map<std::pair<double, double>, std::vector<std::size_t>> sweeps;
        for (std::size_t row = begin; row < end; ++row)
        {
            const std::pair<double, double> key{points.mach[row], points.density[row]};
            if (!sweeps.contains(key))
                pairs.push_back(key);
            sweeps[key].push_back(row);
        }
        const bool isPath =
            std::all_of(sweeps.begin(), sweeps.end(), [](const auto& sweep) { return sweep.second.size() == 1; });
        if (isPath)
        {
            std::vector<std::size_t> path(end - begin);
            std::iota(path.begin(), path.end(), begin);
            curves.push_back(std::move(path));
        }
        else
        {
            for (const std::pair<double, double>& key : pairs)
            {
                std::vector<std::size_t> sweep = sweeps[key];
                std::stable_sort(sweep.begin(), sweep.end(),
                                 [&](std::size_t a, std::size_t b) { return points.velocity[a] < points.velocity[b]; });
                curves.push_back(std::move(sweep));
            }
        }
        begin = end;
    }
    return curves;
}

// aeroFlutterCrossings: where the damping of a mode rises through a level (zero: the onset of flutter), by linear
// interpolation between the two points of the curve that bracket it.
class FlutterCrossingsQuery final : public ResultQuery
{
public:
    FlutterCrossingsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const double level = context.arguments().realOr("dampingLevel", 0.0);
        std::vector<ResultQueryContext::NamedRef> results = context.named("aeroFlutterSummary");
        RowAccumulator accumulator;
        // an estimate by linear interpolation: marked approximate
        const std::int16_t provider = accumulator.addProvider("interpolated from the flutter summary", true);
        struct Crossing
        {
            std::int32_t subcase;
            std::int32_t mode;
            double mach;
            double density;
            std::string kind;
            double velocity;
            double frequency;
            double below;
            double above;
            std::size_t set;
            std::string analysis;
            std::int32_t designCycle;
            Precision precision;
            std::string status;
        };
        std::vector<Crossing> crossings;
        for (const ResultQueryContext::NamedRef& ref : results)
        {
            const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
            const results::ResultArray* mode = data->find("mode");
            const results::ResultArray* mach = data->find("mach");
            const results::ResultArray* density = data->find("densityRatio");
            const results::ResultArray* velocity = data->find("velocity");
            const results::ResultArray* damping = data->find("damping");
            const results::ResultArray* frequency = data->find("frequency");
            const results::ResultArray* newCurve = data->find("newCurve");
            if (mode == nullptr || mach == nullptr || density == nullptr || velocity == nullptr || damping == nullptr ||
                frequency == nullptr)
                continue;
            SummaryPoints points;
            points.mode = arrayInts(*mode);
            points.mach = arrayDoubles(*mach);
            points.density = arrayDoubles(*density);
            points.velocity = arrayDoubles(*velocity);
            points.damping = arrayDoubles(*damping);
            points.frequency = arrayDoubles(*frequency);
            points.newCurve =
                newCurve != nullptr ? arrayInts(*newCurve) : std::vector<std::int32_t>(points.mode.size(), 0);
            const std::vector<double>& g = points.damping;
            for (const std::vector<std::size_t>& curve : curvesOf(points))
            {
                for (std::size_t position = 0; position + 1 < curve.size(); ++position)
                {
                    const std::size_t low = curve[position];
                    const std::size_t high = curve[position + 1];
                    // the damping rises through the level between the two points
                    if (!(g[low] < level && g[high] >= level))
                        continue;
                    const double fraction = g[high] == g[low] ? 0.0 : (level - g[low]) / (g[high] - g[low]);
                    const auto between = [&](const std::vector<double>& values)
                    { return values[low] + fraction * (values[high] - values[low]); };
                    Crossing crossing;
                    crossing.subcase = ref.result->meta().subcase;
                    crossing.mode = points.mode[low];
                    // the Mach number and the density ratio change along a path (PKNL): the crossing is at the same
                    // fraction
                    crossing.mach = between(points.mach);
                    crossing.density = between(points.density);
                    crossing.velocity = between(points.velocity);
                    crossing.frequency = between(points.frequency);
                    // a mode with no frequency loses its stiffness without oscillating: divergence
                    crossing.kind =
                        points.frequency[low] == 0.0 && points.frequency[high] == 0.0 ? "divergence" : "flutter";
                    crossing.below = points.velocity[low];
                    crossing.above = points.velocity[high];
                    crossing.set = ref.set;
                    crossing.analysis = context.sets()[ref.set].analysis;
                    crossing.designCycle = ref.result->meta().designCycle;
                    crossing.precision = precisionOfNamed(*data);
                    crossing.status = statusOfNamed(*ref.result);
                    crossings.push_back(std::move(crossing));
                }
            }
        }
        if (crossings.empty())
            return emptyTable(info().defaultColumns());
        // the critical speed first
        std::stable_sort(crossings.begin(), crossings.end(),
                         [](const Crossing& a, const Crossing& b) { return a.velocity < b.velocity; });
        for (const Crossing& crossing : crossings)
        {
            RowBlock block(1);
            block.ints("subcase", {crossing.subcase});
            block.ints("mode", {crossing.mode});
            block.reals("mach", {crossing.mach});
            block.reals("densityRatio", {crossing.density});
            block.texts("kind", {crossing.kind});
            block.reals("velocity", {crossing.velocity});
            block.reals("frequency", {crossing.frequency});
            block.reals("velocityBelow", {crossing.below});
            block.reals("velocityAbove", {crossing.above});
            block.origin(crossing.set, crossing.analysis, crossing.designCycle);
            block.precision(crossing.precision);
            block.validation(crossing.status);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        return sortedResult(accumulator.finish(context, info()), context.exec(), {"velocity"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroFlutterCrossings";
        info.family = "R9";
        info.resultQuery = true;
        info.description =
            "Where the damping of a mode rises through `dampingLevel` (zero by default: the onset of flutter), found "
            "by linear interpolation of the velocity and of the frequency between the two points of the flutter curve "
            "that bracket it (an estimate: the provenance marks it approximate). A curve is the points of one mode in "
            "the order of the table (a path, as the no-looping methods PKNL and PKNLS give, where each point has its "
            "own Mach number, density ratio and velocity: they are interpolated at the same fraction), or, when the "
            "table sweeps the velocities at each Mach number and density ratio, each such sweep by increasing "
            "velocity. `kind` is `flutter`, or `divergence` when the frequency is zero at both points. The "
            "crossings where the damping falls are not given. The first row, the lowest velocity, is the critical "
            "speed.";
        info.parameters = {
            parameter("dampingLevel", ParameterKind::Real, "0.0", "the damping the crossing is looked for")};
        info.defaultSort = "velocity";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("mode").results("mach densityRatio").dictionary("kind");
        columns.results("velocity frequency velocityBelow velocityAbove");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFlutterCrossingsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FlutterCrossingsQuery>());
}

} // namespace oor::query
