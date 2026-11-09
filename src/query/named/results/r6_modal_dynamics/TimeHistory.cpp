#include "SeriesSource.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

namespace oor::query
{

namespace
{

// timeHistory: the values of a quantity at some places along the times of a transient analysis.
class TimeHistoryQuery final : public ResultQuery
{
public:
    TimeHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        if (!arguments.has("ids"))
            throw QueryError("timeHistory needs `ids`, the grid points (or elements) to give the history of");
        const std::string quantity = arguments.textOr("quantity", "displacement");
        const std::string component = arguments.textOr("component", quantity == "temperature" ? "temperature" : "t3");
        const std::vector<Series> all =
            collectSeries(context, quantity, component, arguments.ids("ids"), results::StepKind::Time);
        if (all.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(
                    "no time history of " + quantity + " '" + component +
                    "' at the places asked for: the result file has no transient table of it");
            return emptyTable(info().defaultColumns());
        }
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const Series& series : all)
        {
            const std::size_t count = series.steps.size();
            RowBlock block(count);
            block.ints("subcase", std::vector<std::int32_t>(count, series.subcase));
            block.reals("time", series.steps);
            block.ints("id", std::vector<std::int32_t>(count, series.id));
            block.texts("quantity", std::vector<std::string>(count, quantity));
            block.texts("component", std::vector<std::string>(count, component));
            block.reals("value", series.real);
            block.origin(series.set, series.analysis, series.designCycle);
            block.precision(series.precision);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        return sortedResult(accumulator.finish(context, info()), context.exec(),
                            {"designCycle", "analysis", "subcase", "id", "time"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "timeHistory";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "The history of a quantity at the grid points or elements `ids`, one row for each subcase, place and time "
            "of a "
            "transient analysis. `quantity` is \"displacement\", \"velocity\", \"acceleration\", \"spcForce\", "
            "\"appliedLoad\", \"temperature\" or an element quantity (\"stress\", \"force\", \"strain\") with the "
            "column of "
            "its table as `component`; the value of an element is its centre at the first layer.";
        info.parameters = {
            parameter("ids", ParameterKind::IdList, "(required)", "the grid points or elements"),
            parameter("quantity", ParameterKind::Text, "\"displacement\"", "the quantity"),
            parameter("component", ParameterKind::Text, "\"t3\"", "the component, or the column of the element table")};
        info.defaultSort = "subcase, id, time";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .real("time")
            .int32("id")
            .dictionary("quantity")
            .dictionary("component")
            .result("value");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTimeHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TimeHistoryQuery>());
}

} // namespace oor::query
