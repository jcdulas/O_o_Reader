#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"

namespace oor::query
{

namespace
{

// temperaturesTable: the temperature of every grid point, subcase and step.
class TemperaturesTableQuery final : public ResultQuery
{
public:
    TemperaturesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<TableRef> tables = context.tables(results::ResultKind::Temperatures, false);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        const auto nodes = context.arguments().ids("nodeIds");
        for (const TableRef& ref : tables)
        {
            PointRows rows = pointRows(context, ref, {"temperature"}, nodes, false);
            if (rows.block.rows() == 0)
                continue;
            rows.block.provider(provider);
            accumulator.add(std::move(rows.block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "temperaturesTable";
        info.envelopeValue = "temperature";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "The temperatures of the grid points of a heat transfer analysis, one row for each subcase, step and "
            "point.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to list")};
        info.defaultSort = "subcase, [step], nodeId";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("nodeId").result("temperature");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTemperaturesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TemperaturesTableQuery>());
}

} // namespace oor::query
