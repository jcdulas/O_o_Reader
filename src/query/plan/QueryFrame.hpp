#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/operators/ExpressionEvaluator.hpp"
#include "oor/query/plan/QuerySource.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::query
{

// The rows a query has at one moment, and its columns. A column is read from the source (or from a join) only when
// something asks for it, for the rows that are left, and kept: so a column that is not used is neither read nor
// computed, and a computed column used by a filter is not computed again for the output.
//
// A frame is what the steps of a plan work on; it is private to the executor.
class QueryFrame
{
public:
    // The frame of a source: every row, and a lazy column for each column of the source.
    QueryFrame(const QuerySource& source, ExecutionContext& context);
    // A frame of columns that are all in memory (after a group), with no source behind it.
    explicit QueryFrame(ExecutionContext& context);

    [[nodiscard]] std::size_t rowCount() const noexcept { return m_count; }
    [[nodiscard]] bool hasSource() const noexcept { return m_source != nullptr; }
    [[nodiscard]] const QuerySource* source() const noexcept { return m_source; }

    [[nodiscard]] bool has(std::string_view name) const;
    [[nodiscard]] ColumnOrigin originOf(std::string_view name) const;
    // True when asking for the column costs more than a copy: a computed column or a joined column not read yet.
    [[nodiscard]] bool isExpensive(std::string_view name) const;
    // The names of the columns a query returns when it selects none, and all the names.
    [[nodiscard]] std::vector<std::string> defaultColumns() const;
    [[nodiscard]] std::vector<std::string> allColumns() const;

    // The column, read now if it was not. Throws QueryError for a name the frame does not have.
    [[nodiscard]] const QueryColumn& column(std::string_view name);
    [[nodiscard]] ColumnLookup lookup();
    [[nodiscard]] ColumnInfo infoOf(std::string_view name) const;

    // Keeps the rows at these positions of the current rows, in this order (a position may repeat).
    void narrow(std::span<const std::uint32_t> positions);

    // Adds the columns of the cards a join follows to: `matches` has one entry per current row.
    struct JoinedColumn
    {
        std::string name; // in the target
        std::string outputName;
        ColumnOrigin origin = ColumnOrigin::Joined;
        std::string field;
        bool visible = true; // part of the default output
        bool computed = false;
    };
    void addJoin(std::shared_ptr<JoinTarget> target, TargetRows matches, const std::vector<JoinedColumn>& columns);
    // The matches of the last join, to drop the rows that have none (an inner join).
    [[nodiscard]] const TargetRows& lastMatches() const { return m_joins.back().matches; }

    // Adds a column in memory (the position of a list value, the result of an expression, an aggregate).
    void addValue(std::string name, QueryColumn column, ColumnOrigin origin, std::string field, bool visible);

    // What a select makes of the frame: each item is a column of the frame under a new name, or a new column.
    struct Projection
    {
        std::string name;                 // the name in the output
        std::string from;                 // the column of the frame (when `value` is empty)
        std::optional<QueryColumn> value; // a new column (an expression)
    };
    void project(const std::vector<Projection>& items);

    // The origin of the rows (the cards they come from) when the frame still has rows of cards.
    [[nodiscard]] RowOrigin rowOrigin() const;
    [[nodiscard]] ExecutionContext& context() noexcept { return *m_context; }

private:
    enum class Kind : std::uint8_t
    {
        Source,
        Join,
        Value
    };
    struct Slot
    {
        std::string name;
        ColumnOrigin origin = ColumnOrigin::Stored;
        std::string field;
        bool visible = true;
        Kind kind = Kind::Source;
        std::string sourceName; // the name in the source or in the target of the join
        std::size_t join = 0;
        std::optional<QueryColumn> cache;
    };
    struct FrameJoin
    {
        std::shared_ptr<JoinTarget> target;
        TargetRows matches;
    };

    Slot& slotOf(std::string_view name);
    [[nodiscard]] const Slot& slotOf(std::string_view name) const;
    void rebuildIndex();
    // The rows as a selection for the source.
    [[nodiscard]] RowSelection selection() const;
    void fill(Slot& slot);

    const QuerySource* m_source = nullptr;
    ExecutionContext* m_context = nullptr;
    std::size_t m_count = 0;
    bool m_identity = true; // the rows are all the rows of the source, in order
    std::vector<std::uint32_t> m_rows;
    std::deque<Slot> m_slots; // stable addresses
    std::unordered_map<std::string, std::size_t> m_index;
    std::vector<FrameJoin> m_joins;
};

} // namespace oor::query
