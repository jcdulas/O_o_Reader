#include "SeriesSource.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace oor::query
{

namespace
{

// peakResponse: the frequencies where the response of a quantity peaks, the highest first.
class PeakResponseQuery final : public ResultQuery
{
public:
    PeakResponseQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        if (!arguments.has("ids"))
            throw QueryError("peakResponse needs `ids`, the grid points (or elements) to look at");
        const std::string quantity = arguments.textOr("quantity", "displacement");
        const std::string component = arguments.textOr("component", "t3");
        const std::int64_t top = arguments.integerOr("top", 1);
        if (top < 1)
            throw QueryError("top is the number of peaks to keep, at least 1");
        const std::vector<Series> all =
            collectSeries(context, quantity, component, arguments.ids("ids"), results::StepKind::Frequency);
        if (all.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError("no frequency response of " + quantity + " '" + component +
                                               "' at the places asked for: the result file has no complex table of it");
            return emptyTable(info().defaultColumns());
        }
        std::vector<std::int32_t> subcase, id, rank;
        std::vector<double> frequency, magnitude, phase;
        std::vector<std::int32_t> design;
        bool single = true;
        for (const Series& series : all)
        {
            const std::size_t count = series.steps.size();
            std::vector<double> size(count);
            for (std::size_t index = 0; index < count; ++index)
                size[index] = std::hypot(series.real[index], series.isComplex() ? series.imaginary[index] : 0.0);
            // the peaks are the local maxima (an end counts when it is not below its neighbour)
            std::vector<std::size_t> peaks;
            for (std::size_t index = 0; index < count; ++index)
            {
                const bool leftOk = index == 0 || size[index] >= size[index - 1];
                const bool rightOk = index + 1 == count || size[index] >= size[index + 1];
                if (leftOk && rightOk)
                    peaks.push_back(index);
            }
            std::stable_sort(peaks.begin(), peaks.end(),
                             [&](std::size_t a, std::size_t b) { return size[a] > size[b]; });
            const std::size_t keep = std::min<std::size_t>(peaks.size(), static_cast<std::size_t>(top));
            for (std::size_t position = 0; position < keep; ++position)
            {
                const std::size_t index = peaks[position];
                subcase.push_back(series.subcase);
                id.push_back(series.id);
                rank.push_back(static_cast<std::int32_t>(position + 1));
                frequency.push_back(series.steps[index]);
                magnitude.push_back(size[index]);
                phase.push_back(std::atan2(series.isComplex() ? series.imaginary[index] : 0.0, series.real[index]) *
                                180.0 / std::numbers::pi);
                single = single && series.precision == Precision::Single;
            }
        }
        RowBlock block(subcase.size());
        const std::size_t rows = subcase.size();
        block.ints("subcase", std::move(subcase));
        block.ints("id", std::move(id));
        block.texts("quantity", std::vector<std::string>(rows, quantity));
        block.texts("component", std::vector<std::string>(rows, component));
        block.ints("rank", std::move(rank));
        block.reals("freq", std::move(frequency));
        block.reals("magnitude", std::move(magnitude));
        block.reals("phase", std::move(phase));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        block.provider(accumulator.addProvider("stored table", false));
        accumulator.add(std::move(block));
        return sortedResult(accumulator.finish(context, info()), context.exec(), {"subcase", "id", "rank"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "peakResponse";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "The peaks of the frequency response of a quantity at the grid points or elements `ids`: the `top` highest "
            "local "
            "maxima of the magnitude over the frequencies of each subcase and place, `rank` 1 being the highest, with "
            "their "
            "frequency, magnitude and phase (degrees). An end of the frequency range counts as a peak when it is not "
            "below its "
            "neighbour. `quantity` and `component` are those of `frfTable`.";
        info.parameters = {
            parameter("ids", ParameterKind::IdList, "(required)", "the grid points or elements"),
            parameter("quantity", ParameterKind::Text, "\"displacement\"", "the quantity"),
            parameter("component", ParameterKind::Text, "\"t3\"", "the component, or the column of the element table"),
            parameter("top", ParameterKind::Integer, "1", "the number of peaks to give for each place")};
        info.defaultSort = "subcase, id, rank";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("id").dictionary("quantity").dictionary("component").int32("rank").real("freq");
        columns.results("magnitude phase");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPeakResponseQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PeakResponseQuery>());
}

} // namespace oor::query
