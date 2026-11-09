#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"

#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

// temperatureExtremes: the lowest, highest and mean temperature of each subcase and step, and where they are.
class TemperatureExtremesQuery final : public ResultQuery
{
public:
    TemperatureExtremesQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<TableRef> tables = context.tables(results::ResultKind::Temperatures, false);
        sortTables(tables);
        const auto nodes = context.arguments().ids("nodeIds");
        std::set<std::int64_t> wanted;
        if (nodes)
            wanted.insert(nodes->begin(), nodes->end());
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const int column = values.component("temperature");
            if (column < 0 || values.isComplex())
                continue;
            std::vector<std::size_t> steps;
            std::vector<double> low, high, mean;
            std::vector<std::int32_t> lowNode, highNode;
            for (std::size_t step = 0; step < values.steps(); ++step)
            {
                double minimum = 0.0, maximum = 0.0, sum = 0.0;
                std::int32_t minNode = 0, maxNode = 0;
                std::size_t count = 0;
                for (std::size_t row = 0; row < values.rows(); ++row)
                {
                    if (nodes && !wanted.contains(decoded->ids[row]))
                        continue;
                    const double value = values.real(step, row, static_cast<std::size_t>(column));
                    if (std::isnan(value))
                        continue;
                    if (count == 0 || value < minimum)
                    {
                        minimum = value;
                        minNode = decoded->ids[row];
                    }
                    if (count == 0 || value > maximum)
                    {
                        maximum = value;
                        maxNode = decoded->ids[row];
                    }
                    sum += value;
                    ++count;
                }
                if (count == 0)
                    continue;
                steps.push_back(step);
                low.push_back(minimum);
                high.push_back(maximum);
                mean.push_back(sum / static_cast<double>(count));
                lowNode.push_back(minNode);
                highNode.push_back(maxNode);
            }
            if (steps.empty())
                continue;
            RowBlock block(steps.size());
            block.ints("subcase", std::vector<std::int32_t>(steps.size(), meta.subcase));
            block.reals("min", std::move(low));
            block.ints("minNodeId", std::move(lowNode));
            block.reals("max", std::move(high));
            block.ints("maxNodeId", std::move(highNode));
            block.reals("mean", std::move(mean));
            addStepsToBlock(block, meta, *decoded, steps);
            block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
            block.precision(precisionOf(*decoded));
            block.validation(std::string(results::resultStatusName(meta.status)));
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
        info.name = "temperatureExtremes";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "For each subcase and step, the lowest and the highest temperature, the grid points where they occur, and "
            "the mean "
            "temperature over the points (the plain mean of the values, not weighted by the model).";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to look at")};
        info.defaultSort = "subcase, [step]";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .result("min")
            .int32("minNodeId")
            .result("max")
            .int32("maxNodeId")
            .result("mean");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTemperatureExtremesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TemperatureExtremesQuery>());
}

} // namespace oor::query
