#include "NodalVectorTable.hpp"

#include "oor/query/named/TableBuilder.hpp"

#include <cmath>

namespace oor::query
{

QueryInfo nodalVectorInfo(const std::string& name, const std::string& description)
{
    QueryInfo info;
    info.name = name;
    info.family = "R1";
    info.resultQuery = true;
    info.description = description;
    info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to list")};
    info.defaultSort = "subcase, [step], nodeId";
    info.envelopeValue = "magnitude";
    ColumnListBuilder columns;
    columns.int32("subcase")
        .step()
        .int32("nodeId")
        .results("t1 t2 t3 r1 r2 r3")
        .result("magnitude")
        .dictionary("frame");
    info.columnSets.push_back(columns.build());
    return info;
}

std::shared_ptr<const QueryTable> nodalVectorTable(const ResultQueryContext& context, const QueryInfo& info,
                                                   results::ResultKind kind)
{
    std::vector<TableRef> tables = context.tables(kind);
    sortTables(tables);
    RowAccumulator accumulator;
    const std::int16_t provider = accumulator.addProvider("stored table", false);
    const auto nodes = context.arguments().ids("nodeIds");
    for (const TableRef& ref : tables)
    {
        PointRows rows = pointRows(context, ref, {"t1", "t2", "t3", "r1", "r2", "r3"}, nodes);
        if (rows.block.rows() == 0)
            continue;
        const TableValues values(*rows.data);
        std::vector<double> magnitude(rows.block.rows(), std::nan(""));
        const int first = values.component("t1");
        if (first >= 0)
        {
            for (std::size_t index = 0; index < magnitude.size(); ++index)
            {
                double sum = 0.0;
                for (int component = first; component < first + 3; ++component)
                {
                    const double re =
                        values.real(rows.stepOf[index], rows.rows[index], static_cast<std::size_t>(component));
                    const double im =
                        values.imag(rows.stepOf[index], rows.rows[index], static_cast<std::size_t>(component));
                    sum += re * re + im * im;
                }
                magnitude[index] = std::sqrt(sum);
            }
        }
        rows.block.reals("magnitude", std::move(magnitude));
        rows.block.provider(provider);
        accumulator.add(std::move(rows.block));
    }
    if (accumulator.empty())
        return emptyTable(info.defaultColumns());
    // The default sort: subcase, step, then the grid point, whatever order the file has them in.
    std::vector<std::string> keys = leadingKeys();
    keys.emplace_back("nodeId");
    return sortedResult(accumulator.finish(context, info), context.exec(), keys);
}

} // namespace oor::query
