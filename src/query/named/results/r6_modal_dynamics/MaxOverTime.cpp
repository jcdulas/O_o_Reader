#include "SeriesSource.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

namespace
{

// maxOverTime: the extremes of a quantity over the times of a transient analysis, and when they occur.
class MaxOverTimeQuery final : public ResultQuery
{
public:
    MaxOverTimeQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string quantity = arguments.textOr("quantity", "displacement");
        const std::string component = arguments.textOr("component", quantity == "temperature" ? "temperature" : "t3");
        const std::vector<Series> all =
            collectSeries(context, quantity, component, arguments.ids("ids"), results::StepKind::Time);
        if (all.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError("no time history of " + quantity + " '" + component +
                                               "': the result file has no transient table of it");
            return emptyTable(info().defaultColumns());
        }
        std::vector<std::int32_t> subcase, id;
        std::vector<double> maximum, timeOfMaximum, minimum, timeOfMinimum, absolute;
        bool single = true;
        std::int32_t design = -1;
        std::size_t set = 0;
        std::string analysis;
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const Series& series : all)
        {
            if (series.steps.empty())
                continue;
            std::size_t high = 0;
            std::size_t low = 0;
            for (std::size_t index = 1; index < series.real.size(); ++index)
            {
                if (series.real[index] > series.real[high])
                    high = index;
                if (series.real[index] < series.real[low])
                    low = index;
            }
            subcase.push_back(series.subcase);
            id.push_back(series.id);
            maximum.push_back(series.real[high]);
            timeOfMaximum.push_back(series.steps[high]);
            minimum.push_back(series.real[low]);
            timeOfMinimum.push_back(series.steps[low]);
            absolute.push_back(std::max(std::fabs(series.real[high]), std::fabs(series.real[low])));
            single = single && series.precision == Precision::Single;
            design = series.designCycle;
            set = series.set;
            analysis = series.analysis;
        }
        const std::size_t rows = subcase.size();
        RowBlock block(rows);
        block.ints("subcase", std::move(subcase));
        block.ints("id", std::move(id));
        block.texts("quantity", std::vector<std::string>(rows, quantity));
        block.texts("component", std::vector<std::string>(rows, component));
        block.reals("max", std::move(maximum));
        block.reals("timeOfMax", std::move(timeOfMaximum));
        block.reals("min", std::move(minimum));
        block.reals("timeOfMin", std::move(timeOfMinimum));
        block.reals("absmax", std::move(absolute));
        block.origin(set, analysis, design);
        block.precision(single ? Precision::Single : Precision::Double);
        block.provider(provider);
        accumulator.add(std::move(block));
        return sortedResult(accumulator.finish(context, info()), context.exec(), {"subcase", "id"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "maxOverTime";
        info.family = "R6";
        info.resultQuery = true;
        info.description =
            "For each subcase and place, the largest and the smallest value of a quantity over the times of a "
            "transient "
            "analysis, the times where they occur, and the largest magnitude. `ids` are the grid points or elements "
            "(None: all "
            "places of the tables); `quantity` and `component` are those of `timeHistory`.";
        info.parameters = {
            parameter("ids", ParameterKind::IdList, "None", "the grid points or elements (None: all)"),
            parameter("quantity", ParameterKind::Text, "\"displacement\"", "the quantity"),
            parameter("component", ParameterKind::Text, "\"t3\"", "the component, or the column of the element table")};
        info.defaultSort = "subcase, id";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .int32("id")
            .dictionary("quantity")
            .dictionary("component")
            .result("max")
            .real("timeOfMax");
        columns.result("min").real("timeOfMin").result("absmax");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMaxOverTimeQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MaxOverTimeQuery>());
}

} // namespace oor::query
