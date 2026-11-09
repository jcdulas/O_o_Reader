#include "SeriesSource.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

#include <cmath>
#include <numbers>

namespace oor::query
{

namespace
{

// frfTable: the response of a quantity at some places along the frequencies, as real and imaginary parts and as
// magnitude and phase.
class FrfTableQuery final : public ResultQuery
{
public:
    FrfTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        if (!arguments.has("ids"))
            throw QueryError("frfTable needs `ids`, the grid points (or elements) to give the response of");
        const std::string quantity = arguments.textOr("quantity", "displacement");
        const std::string component = arguments.textOr("component", "t3");
        const std::vector<Series> all =
            collectSeries(context, quantity, component, arguments.ids("ids"), results::StepKind::Frequency);
        if (all.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError("no frequency response of " + quantity + " '" + component +
                                               "' at the places asked for: the result file has no complex table of it");
            return emptyTable(info().defaultColumns());
        }
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const Series& series : all)
        {
            const std::size_t count = series.steps.size();
            RowBlock block(count);
            std::vector<double> magnitude(count), phase(count), imaginary(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                imaginary[index] = series.isComplex() ? series.imaginary[index] : 0.0;
                magnitude[index] = std::hypot(series.real[index], imaginary[index]);
                phase[index] = std::atan2(imaginary[index], series.real[index]) * 180.0 / std::numbers::pi;
            }
            block.ints("subcase", std::vector<std::int32_t>(count, series.subcase));
            block.reals("freq", series.steps);
            block.ints("id", std::vector<std::int32_t>(count, series.id));
            block.texts("quantity", std::vector<std::string>(count, quantity));
            block.texts("component", std::vector<std::string>(count, component));
            block.reals("real", series.real);
            block.reals("imag", std::move(imaginary));
            block.reals("magnitude", std::move(magnitude));
            block.reals("phase", std::move(phase));
            block.origin(series.set, series.analysis, series.designCycle);
            block.precision(series.precision);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        return sortedResult(accumulator.finish(context, info()), context.exec(),
                            {"designCycle", "analysis", "subcase", "id", "freq"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "frfTable";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "The frequency response of a quantity at the grid points or elements `ids`, one row for each subcase, "
            "place and "
            "frequency, with both forms of the complex value (`real`, `imag` and `magnitude`, `phase` in degrees) "
            "whatever the "
            "file stores. `quantity` is \"displacement\", \"velocity\", \"acceleration\", \"spcForce\", "
            "\"appliedLoad\" or an "
            "element quantity (\"stress\", \"force\", \"strain\"), with the column of its table as `component` (`t3`, "
            "`oxx`, "
            "`axial`, ...); the value of an element is its centre at the first layer.";
        info.parameters = {
            parameter("ids", ParameterKind::IdList, "(required)", "the grid points or elements"),
            parameter("quantity", ParameterKind::Text, "\"displacement\"", "the quantity"),
            parameter("component", ParameterKind::Text, "\"t3\"", "the component, or the column of the element table")};
        info.defaultSort = "subcase, id, freq";
        ColumnListBuilder columns;
        columns.int32("subcase").real("freq").int32("id").dictionary("quantity").dictionary("component");
        columns.results("real imag magnitude phase");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFrfTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FrfTableQuery>());
}

} // namespace oor::query
