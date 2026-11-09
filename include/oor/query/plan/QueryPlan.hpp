#pragma once

#include "oor/query/operators/Expression.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace oor::query
{

// A query is a source followed by steps, run in the order they were given. Nothing runs until the plan is executed.

// Keeps the rows where the condition is true.
struct WhereStep
{
    Expression condition;
};

enum class JoinKind : std::uint8_t
{
    Left, // every row stays; the columns of a row with no match are null
    Inner // a row with no match is dropped
};

// Follows the references of the rows to the cards they name and adds the columns of those cards.
//
// target   what to follow to: "property", "material", "node", "element" or "coordinate" (the source says which it has);
//          or a table (`table` is set), whose rows are matched by `tableKey`
// on       the columns of the query that hold the references (IDs). Several columns are read as one, the first one with
// a
//          value counting. Empty: the default of the source for the target. A list column (the nodes of an element)
//          gives one row for each value in the list: the rows of the query are repeated
// prefix   put in front of the names of the joined columns; by default none ("node" for a node: nodeX), and a name that
// is
//          already taken gets the name of the target in front instead
// columns  the joined columns to add; empty: every stored one
struct JoinStep
{
    std::string target;
    std::vector<std::string> on;
    JoinKind how = JoinKind::Left;
    std::optional<std::string> prefix;
    std::vector<std::string> columns;
    std::shared_ptr<const QueryTable> table;
    std::string tableKey;
};

// One column of a select: an existing column, or `name` = an expression.
struct SelectItem
{
    std::string name;
    Expression expression; // empty: `name` is the column itself
    std::string from;      // the column renamed to `name` (when it differs), else empty
};

struct SelectStep
{
    std::vector<SelectItem> items;
};

enum class AggregateFunction : std::uint8_t
{
    Count, // the rows of the group; with a column: the rows where it is not null and not NaN
    Sum,
    Min,
    Max,
    Mean,
    ArgMax, // the value of `reported` in the row where `column` is largest: the critical subcase, element or layer
    ArgMin  // the same where `column` is smallest
};

struct AggregateSpec
{
    std::string name;
    AggregateFunction function = AggregateFunction::Count;
    std::string column;   // empty for the count of rows
    std::string reported; // ArgMax and ArgMin: the column whose value they give
};

// Groups the rows by the keys (the groups come out in increasing order of the keys, null last) and computes the
// aggregates of each group. With no key there is one group, even with no row.
struct GroupStep
{
    std::vector<std::string> keys;
    std::vector<AggregateSpec> aggregates;
};

struct SortKey
{
    std::string column;
    bool descending = false;
};

// A stable sort by the keys in order; null and NaN last, in both directions.
struct SortStep
{
    std::vector<SortKey> keys;
};

struct LimitStep
{
    std::size_t count = 0;
};

using QueryStep = std::variant<WhereStep, JoinStep, SelectStep, GroupStep, SortStep, LimitStep>;

class QueryPlan
{
public:
    QueryPlan() = default;

    [[nodiscard]] const std::vector<QueryStep>& steps() const noexcept { return m_steps; }
    [[nodiscard]] QueryPlan with(QueryStep step) const
    {
        QueryPlan plan = *this;
        plan.m_steps.push_back(std::move(step));
        return plan;
    }
    // One line for each step.
    [[nodiscard]] std::vector<std::string> describe() const;

private:
    std::vector<QueryStep> m_steps;
};

[[nodiscard]] std::string_view aggregateName(AggregateFunction function) noexcept;
[[nodiscard]] std::optional<AggregateFunction> aggregateFromName(std::string_view name) noexcept;

} // namespace oor::query
