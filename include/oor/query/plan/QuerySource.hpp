#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/table/QueryColumn.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// The rows of a source that a query has at a given moment: indexes into the rows of the source, in the order of the
// query, with repeats where a join gave a row several matches. `all` stands for every row of the source in its
// own order (the first stage of a query, which then needs no gather and shares the buffers of the source).
struct RowSelection
{
    const std::uint32_t* rows = nullptr; // not used when `all`
    std::size_t count = 0;

    bool all = false;

    [[nodiscard]] bool isAll() const noexcept { return all; }
    [[nodiscard]] std::uint32_t at(std::size_t index) const noexcept
    {
        return all ? static_cast<std::uint32_t>(index) : rows[index];
    }
};

// What a source says about one of its columns.
struct ColumnDescription
{
    std::string name;
    ColumnOrigin origin = ColumnOrigin::Stored;
    std::string field; // the field of the card for a stored column
    // True when a query that selects nothing returns the column: the stored columns, not the computed ones.
    bool byDefault = true;
};

// A column and its name: what a source gives back (the requested column, and the other columns that came out of the
// same computation for nothing, so that the query does not compute them twice).
struct NamedColumn
{
    std::string name;
    QueryColumn column;
};

// Where the rows a join follows to are: for each row of the query, the card type and the row in its table, or NO_ROW.
struct TargetRows
{
    std::vector<std::uint16_t> types; // empty for a target made of one table of one type
    std::vector<std::uint32_t> rows;
};

// The cards a join looks up: the referenced cards of one kind (the properties, the materials, the grid points, another
// table).
class JoinTarget
{
public:
    virtual ~JoinTarget() = default;

    // The columns of the target, with the order and the origins the join gives them (a computed column of the target is
    // evaluated only when selected or used).
    [[nodiscard]] virtual std::vector<ColumnDescription> columns() const = 0;
    // For each key (a card ID, or a key of a table) the matching card; NO_ROW for a key that matches nothing.
    [[nodiscard]] virtual TargetRows locate(std::span<const std::int64_t> keys, ExecutionContext& context) const = 0;
    // The values of a column for the cards `matches` names, one per entry, null where there is no match; plus the
    // columns that came out of the same computation.
    [[nodiscard]] virtual std::vector<NamedColumn> fetch(std::string_view name, const TargetRows& matches,
                                                         ExecutionContext& context) const = 0;
};

// What a query reads: a table of the model (a card type, a category, a selection of cards) or a table of arrays. It
// gives columns on request, for the rows the query has left, so that a column that is not used is never read or
// computed.
class QuerySource
{
public:
    virtual ~QuerySource() = default;

    // A short description of the source for `explain`.
    [[nodiscard]] virtual std::string description() const = 0;
    [[nodiscard]] virtual std::size_t rowCount() const = 0;
    // Every column a query can use: the stored ones first, in the order of the card, then the computed ones.
    [[nodiscard]] virtual std::vector<ColumnDescription> columns() const = 0;

    // The values of the column `name` for `rows`. Throws QueryError for a column the source does not have. A stored
    // column of the whole source is shared with the source, not copied. A computed column is evaluated over `rows`
    // only; the columns of the same computation that cost nothing more come back with it.
    [[nodiscard]] virtual std::vector<NamedColumn> fetch(std::string_view name, const RowSelection& rows,
                                                         ExecutionContext& context) const = 0;

    // The cards the rows come from, for a source that has cards (nothing otherwise).
    [[nodiscard]] virtual RowOrigin originOf(const RowSelection& rows) const;

    // What a join to `target` ("property", "material", "node", "element", "coordinate") follows to, or null for a
    // source that has no such references.
    [[nodiscard]] virtual std::unique_ptr<JoinTarget> joinTarget(std::string_view target) const;

    // The columns a join to `target` uses as keys when none is given (the first one present wins), in the order of
    // preference. Empty when the source has no default.
    [[nodiscard]] virtual std::vector<std::string> defaultJoinKeys(std::string_view target) const;

    // The version of the data contract written in the schema of the tables of this source (0 for none).
    [[nodiscard]] virtual int contractVersion() const { return 0; }
};

// A source over a table (its columns are all stored ones): the first operand of a query on arrays, and what the tests
// of the operators use.
class TableSource final : public QuerySource
{
public:
    explicit TableSource(std::shared_ptr<const QueryTable> table) : m_table(std::move(table)) {}

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] std::size_t rowCount() const override { return static_cast<std::size_t>(m_table->rowCount()); }
    [[nodiscard]] std::vector<ColumnDescription> columns() const override;
    [[nodiscard]] std::vector<NamedColumn> fetch(std::string_view name, const RowSelection& rows,
                                                 ExecutionContext& context) const override;
    [[nodiscard]] RowOrigin originOf(const RowSelection& rows) const override;

    [[nodiscard]] const std::shared_ptr<const QueryTable>& table() const noexcept { return m_table; }

private:
    std::shared_ptr<const QueryTable> m_table;
};

// The rows of a table as a join target: `key` is a column of integers (unique), the keys the query holds are looked up
// in it.
[[nodiscard]] std::unique_ptr<JoinTarget> makeTableJoinTarget(std::shared_ptr<const QueryTable> table,
                                                              const std::string& key);

} // namespace oor::query
