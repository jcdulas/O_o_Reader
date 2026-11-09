#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/plan/QueryPlan.hpp"
#include "oor/query/plan/QuerySource.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// The builder of a query: a source and a plan. Every method returns a new query, so a query can
// be kept and extended in two ways; nothing is read from the source until `run`.
//
//   Query(source).where(...).join(...).select(...).sortBy(...).limit(10).run(context)
//
// `run` executes the steps in order. A column of the source that no step uses is not read; a computed column (area,
// mass, quality) is evaluated for the rows that are left when it is first needed, and only if it is needed. Without a
// select the result has the stored columns of the source and of the joins; computed columns come only when selected.
class Query
{
public:
    explicit Query(std::shared_ptr<const QuerySource> source, QueryPlan plan = {})
        : m_source(std::move(source)), m_plan(std::move(plan))
    {
    }

    [[nodiscard]] const QuerySource& source() const noexcept { return *m_source; }
    [[nodiscard]] const std::shared_ptr<const QuerySource>& sourcePointer() const noexcept { return m_source; }
    [[nodiscard]] const QueryPlan& plan() const noexcept { return m_plan; }

    [[nodiscard]] Query where(Expression condition) const;
    [[nodiscard]] Query join(JoinStep step) const;
    [[nodiscard]] Query select(std::vector<SelectItem> items) const;
    [[nodiscard]] Query groupBy(std::vector<std::string> keys, std::vector<AggregateSpec> aggregates) const;
    [[nodiscard]] Query sortBy(std::vector<SortKey> keys) const;
    [[nodiscard]] Query limit(std::size_t count) const;

    // Executes the plan. The table keeps the report of the run (which computed columns were evaluated).
    [[nodiscard]] std::shared_ptr<const QueryTable> run(ExecutionContext& context) const;

    // The plan in words: the source, each step, and what the plan reads from the source: the stored columns and the
    // computed ones.
    [[nodiscard]] std::vector<std::string> explain() const;

private:
    std::shared_ptr<const QuerySource> m_source;
    QueryPlan m_plan;
};

} // namespace oor::query
