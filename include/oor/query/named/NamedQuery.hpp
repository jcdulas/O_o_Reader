#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/QueryValue.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/table/ModelLink.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>

namespace oor::query
{

// What a named query runs on: the model, the arguments of the call, the execution context (pool, cancellation,
// progress), and the link tables use to find their model again for `apply` (null when nobody will apply). A query of
// the R families also reads result sets: `resultSets` when the caller gave them (`results.queries`: a result file
// alone, the model is then null), else the results attached to the model.
struct QueryRun
{
    std::shared_ptr<model::Model> model;
    const QueryArguments& arguments;
    ExecutionContext& context;
    std::shared_ptr<const ModelLink> link;
    std::vector<ResultSet> resultSets;
};

// A named query: one class per query, with its parameters and columns.
// `run` returns the table in the default sort of the query; the common parameters (`columns`, `sort`, `limit`) are
// applied afterwards by the registry, so a query never looks at them.
//
// A query is stateless: everything it needs is in the run. It must give the same table for any number of threads.
class NamedQuery
{
public:
    virtual ~NamedQuery() = default;

    [[nodiscard]] virtual const QueryInfo& info() const = 0;
    [[nodiscard]] virtual std::shared_ptr<const QueryTable> run(const QueryRun& run) const = 0;
};

// What every query class derives from: the catalogue entry is given once, at construction.
class QueryBase : public NamedQuery
{
public:
    explicit QueryBase(QueryInfo info) : m_info(std::move(info)) {}
    [[nodiscard]] const QueryInfo& info() const override { return m_info; }

private:
    QueryInfo m_info;
};

} // namespace oor::query
