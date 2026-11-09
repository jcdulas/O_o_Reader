#pragma once

#include "oor/query/named/NamedQuery.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/results/RowAccumulator.hpp"
#include "oor/query/results/TableValues.hpp"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// What the queries of the families R1 to R10 derive from: it reads the result sets and the parameters common to the
// result queries before the query itself runs.
class ResultQuery : public QueryBase
{
public:
    using QueryBase::QueryBase;

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const final;

protected:
    [[nodiscard]] virtual std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                                    const QueryRun& run) const = 0;
};

// The tables in the order a query reports them: design cycle, result set, subcase (a stable order: the order of the
// file within a subcase).
void sortTables(std::vector<TableRef>& tables);

// The block of rows of a table of points (grid points, scalar points): one row for each step and point kept, with the
// columns `subcase`, the step, `nodeId`, one column for each of `components` that the table has (named as the table
// names them, NaN for one it lacks), and `frame`. `keep` lists the points the call asked for (null: every point). The
// values are in the precision of the table, a complex table gives complex columns.
struct PointRows
{
    RowBlock block;
    std::shared_ptr<const results::ResultData> data; // the rows decoded (those of the points kept); `rows` index them
    std::vector<std::size_t> rows; // the row of `data` of each row of the block (for the callers that look at values)
    std::vector<std::size_t> stepOf;
};
[[nodiscard]] PointRows pointRows(const ResultQueryContext& context, const TableRef& ref,
                                  const std::vector<std::string>& components,
                                  const std::optional<std::vector<std::int64_t>>& keep, bool withFrame = true);

// Adds the step columns of a block from a table and the steps its rows are at.
void addStepsToBlock(RowBlock& block, const results::ResultTableMeta& meta, const results::ResultData& data,
                     const std::vector<std::size_t>& stepOf);

// The positions of the grid points of the model each result set is attached to, in the basic system, found by the node
// ID the file uses (through the attachment: it follows a renumbering). NaN where there is no model or the node is not
// linked.
class ResultNodePositions
{
public:
    explicit ResultNodePositions(const ResultQueryContext& context) : m_context(context) {}
    [[nodiscard]] std::array<double, 3> position(std::size_t set, std::int32_t resultId);
    [[nodiscard]] bool available(std::size_t set) const { return m_context.modelOf(set) != nullptr; }

private:
    const ResultQueryContext& m_context;
    std::map<const model::Model*, std::unique_ptr<NodePositions>> m_positions;
};

// The table sorted by the columns of `keys` that it has, in this order (a stable sort, nulls last), with its
// provenance.
[[nodiscard]] std::shared_ptr<const QueryTable> sortedResult(const std::shared_ptr<const QueryTable>& table,
                                                             ExecutionContext& context,
                                                             const std::vector<std::string>& keys);

// The keys of the default sort of the queries of a subcase and step: design cycle, analysis, subcase, the step columns.
[[nodiscard]] std::vector<std::string> leadingKeys();

// The precision of a table.
[[nodiscard]] Precision precisionOf(const results::ResultData& data) noexcept;

} // namespace oor::query
