#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"

namespace oor::query
{

namespace
{

// constraintHeatTable: the heat that flows through the grid points of fixed temperature.
class ConstraintHeatTableQuery final : public ResultQuery
{
public:
    ConstraintHeatTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        std::vector<TableRef> tables = context.tables(results::ResultKind::ConstraintHeat, false);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        const auto nodes = context.arguments().ids("nodeIds");
        for (const TableRef& ref : tables)
        {
            PointRows rows = pointRows(context, ref, {"heatFlow"}, nodes, false);
            if (rows.block.rows() == 0)
                continue;
            rows.block.rename("heatFlow", "power");
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
        info.name = "constraintHeatTable";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "The heat that flows through the grid points of fixed temperature (the heat flow at the constraints, the "
            "reaction of "
            "a heat transfer analysis): one row for each subcase, step and point, with its power.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to list")};
        info.defaultSort = "subcase, [step], nodeId";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("nodeId").result("power");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addConstraintHeatTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConstraintHeatTableQuery>());
}

} // namespace oor::query
