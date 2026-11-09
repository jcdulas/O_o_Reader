#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r1_nodal/NodalResultQueries.hpp"

#include <cmath>
#include <map>

namespace oor::query
{

namespace
{

// relativeDisplacement: the displacement of one grid point relative to another, in one frame (the basic system unless
// `frame` gives a coordinate system), with the initial distance and its change.
class RelativeDisplacementQuery final : public ResultQuery
{
public:
    RelativeDisplacementQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto pairs = context.arguments().pairs("pairs");
        if (!context.arguments().has("pairs"))
            throw QueryError("relativeDisplacement needs `pairs`, a list of (n1, n2) pairs of grid points");
        std::vector<TableRef> tables = context.resultantTables(results::ResultKind::Displacements);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        ResultNodePositions positions(context);
        std::vector<std::string> missing;
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const int t1 = values.component("t1");
            if (t1 < 0)
                continue;
            std::map<std::int32_t, std::size_t> rowOf;
            for (std::size_t row = 0; row < values.rows(); ++row)
                rowOf.emplace(decoded->ids[row], row);
            std::vector<std::size_t> steps;
            std::vector<std::int32_t> subcase;
            std::vector<std::int32_t> n1;
            std::vector<std::int32_t> n2;
            std::array<std::vector<double>, 6> real;
            std::array<std::vector<double>, 6> imaginary;
            std::vector<double> magnitude;
            std::vector<double> distance;
            std::vector<double> elongation;
            for (std::size_t step = 0; step < values.steps(); ++step)
                for (const auto& [first, second] : pairs)
                {
                    const auto a = rowOf.find(static_cast<std::int32_t>(first));
                    const auto b = rowOf.find(static_cast<std::int32_t>(second));
                    const bool present = a != rowOf.end() && b != rowOf.end();
                    if (!present)
                    {
                        if (step == 0)
                            missing.push_back(std::to_string(first) + "-" + std::to_string(second) + " in subcase " +
                                              std::to_string(meta.subcase));
                        if (context.selection().onMissing != OnMissing::Null)
                            continue;
                    }
                    steps.push_back(step);
                    subcase.push_back(meta.subcase);
                    n1.push_back(static_cast<std::int32_t>(first));
                    n2.push_back(static_cast<std::int32_t>(second));
                    double sum = 0.0;
                    for (std::size_t component = 0; component < 6; ++component)
                    {
                        double re = std::nan("");
                        double im = std::nan("");
                        if (present)
                        {
                            const auto index = static_cast<std::size_t>(t1) + component;
                            re = values.real(step, b->second, index) - values.real(step, a->second, index);
                            im = values.imag(step, b->second, index) - values.imag(step, a->second, index);
                            if (component < 3)
                                sum += re * re + im * im;
                        }
                        real[component].push_back(re);
                        imaginary[component].push_back(im);
                    }
                    magnitude.push_back(present ? std::sqrt(sum) : std::nan(""));
                    double initial = std::nan("");
                    double change = std::nan("");
                    if (present)
                    {
                        const auto p1 = positions.position(ref.set, static_cast<std::int32_t>(first));
                        const auto p2 = positions.position(ref.set, static_cast<std::int32_t>(second));
                        initial = std::sqrt((p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) +
                                            (p2[2] - p1[2]) * (p2[2] - p1[2]));
                        if (!values.isComplex() && context.resultantSystem() == 0)
                        {
                            double deformed = 0.0;
                            for (std::size_t axis = 0; axis < 3; ++axis)
                            {
                                const double span = p2[axis] - p1[axis] + real[axis].back();
                                deformed += span * span;
                            }
                            change = std::sqrt(deformed) - initial;
                        }
                    }
                    distance.push_back(initial);
                    elongation.push_back(change);
                }
            if (steps.empty())
                continue;
            RowBlock block(steps.size());
            block.ints("subcase", std::move(subcase));
            block.ints("n1", std::move(n1));
            block.ints("n2", std::move(n2));
            static const char* const NAMES[6] = {"dt1", "dt2", "dt3", "dr1", "dr2", "dr3"};
            for (std::size_t component = 0; component < 6; ++component)
            {
                if (values.isComplex())
                    block.complex(NAMES[component], std::move(real[component]), std::move(imaginary[component]));
                else
                    block.reals(NAMES[component], std::move(real[component]));
            }
            block.reals("magnitude", std::move(magnitude));
            block.reals("distance", std::move(distance));
            block.reals("elongation", std::move(elongation));
            addStepsToBlock(block, meta, *decoded, steps);
            block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
            block.precision(precisionOf(*decoded));
            block.storedAsMagnitudePhase(meta.storedFormat == results::ResultFormat::MagnitudePhase);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        if (!missing.empty() && context.selection().onMissing == OnMissing::Raise)
        {
            std::string text;
            for (const std::string& item : missing)
                text += (text.empty() ? "" : ", ") + item;
            throw QuantityUnavailableError("relativeDisplacement: a grid point of the pairs has no displacement (" +
                                           text +
                                           "); onMissing=\"null\" keeps the rows with nulls, \"skip\" drops them");
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "relativeDisplacement";
        info.family = "R1";
        info.resultQuery = true;
        info.needsModel = true;
        info.description =
            "The displacement of grid point `n2` relative to grid point `n1`, for each pair, subcase and step, in one "
            "frame "
            "(the basic system, or the coordinate system `frame` gives): the three relative translations and "
            "rotations, the "
            "norm of the translations, the initial distance between the points and its change (`elongation`, null for "
            "a "
            "complex result and for a frame other than the basic system). Needs the model for the positions.";
        info.parameters = {
            parameter("pairs", ParameterKind::Pairs, "(required)", "a list of (n1, n2) pairs of grid points")};
        info.defaultSort = "subcase, [step], pair";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("n1").int32("n2").results("dt1 dt2 dt3 dr1 dr2 dr3").result("magnitude");
        columns.real("distance").real("elongation");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addRelativeDisplacementQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<RelativeDisplacementQuery>());
}

} // namespace oor::query
