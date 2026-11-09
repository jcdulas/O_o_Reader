#include "../NamedTables.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"

#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

// boundaryHeatFlowsTable: the heat that flows through each surface element, by the way it flows.
class BoundaryHeatFlowsTableQuery final : public ResultQuery
{
public:
    BoundaryHeatFlowsTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto elements = context.arguments().ids("elementIds");
        std::set<std::int64_t> wanted;
        if (elements)
            wanted.insert(elements->begin(), elements->end());
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const char* name : {"HeatSurfaceSide", "HeatSurfaceNodes", "HeatSurfaceProperty"})
        {
            std::vector<TableRef> tables = context.elementTables(results::ResultKind::HeatFlows, name, false);
            sortTables(tables);
            for (const TableRef& ref : tables)
            {
                const results::ResultTableMeta& meta = ref.table->meta();
                const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
                const TableValues values(*decoded);
                static const char* const NAMES[5] = {"applied", "FreeConvection", "forcedConvection", "radiation",
                                                     "total"};
                int component[5];
                for (int number = 0; number < 5; ++number)
                    component[number] = values.component(NAMES[number]);
                std::vector<std::size_t> kept;
                for (std::size_t row = 0; row < values.rows(); ++row)
                    if (!elements || wanted.contains(decoded->ids[row]))
                        kept.push_back(row);
                const std::size_t count = values.steps() * kept.size();
                if (count == 0)
                    continue;
                std::vector<std::int32_t> eid;
                std::vector<std::size_t> stepOf;
                std::array<std::vector<double>, 5> value;
                for (std::size_t step = 0; step < values.steps(); ++step)
                    for (const std::size_t row : kept)
                    {
                        eid.push_back(decoded->ids[row]);
                        stepOf.push_back(step);
                        for (std::size_t number = 0; number < 5; ++number)
                            value[number].push_back(
                                component[number] < 0
                                    ? std::nan("")
                                    : values.real(step, row, static_cast<std::size_t>(component[number])));
                    }
                RowBlock block(count);
                block.ints("subcase", std::vector<std::int32_t>(count, meta.subcase));
                block.ints("elementId", std::move(eid));
                for (std::size_t number = 0; number < 5; ++number)
                    block.reals(NAMES[number], std::move(value[number]));
                addStepsToBlock(block, meta, *decoded, stepOf);
                block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
                block.precision(precisionOf(*decoded));
                block.validation(std::string(results::resultStatusName(meta.status)));
                block.provider(provider);
                accumulator.add(std::move(block));
            }
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        std::vector<std::string> keys = leadingKeys();
        keys.push_back("elementId");
        return sortedResult(accumulator.finish(context, info()), context.exec(), keys);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "boundaryHeatFlowsTable";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "The heat that flows through each surface element (CHBDYE, CHBDYG, CHBDYP) of a heat transfer analysis, "
            "one row for "
            "each subcase, step and element: the applied flux, the free and the forced convection, the radiation and "
            "the total. "
            "A way that the table does not give is null.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the surface elements to list")};
        info.defaultSort = "subcase, [step], elementId";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .results("applied FreeConvection forcedConvection radiation total");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addBoundaryHeatFlowsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<BoundaryHeatFlowsTableQuery>());
}

} // namespace oor::query
