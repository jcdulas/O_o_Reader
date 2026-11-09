#pragma once

#include "oor/query/named/NamedQuery.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// The named queries, by name, in the order of the catalogue. Built by hand (src/query/named/RegisterNamedQueries.cpp,
// one line per query), read-only afterwards and safe to read from any thread.
class NamedQueryRegistry
{
public:
    [[nodiscard]] static const NamedQueryRegistry& instance();

    [[nodiscard]] const std::vector<std::unique_ptr<NamedQuery>>& all() const noexcept { return m_queries; }
    // The query called `name`; null when there is none.
    [[nodiscard]] const NamedQuery* find(std::string_view name) const noexcept;

    // Runs a query: checks the argument names against the parameters of the query (and `columns`, `sort`, `limit`),
    // runs it, then applies the common parameters. Throws QueryError for an unknown query, parameter or column.
    [[nodiscard]] std::shared_ptr<const QueryTable> run(std::string_view name, const QueryRun& run) const;

    // Used by the registration functions.
    void add(std::unique_ptr<NamedQuery> query);

private:
    NamedQueryRegistry() = default;
    friend struct NamedQueryRegistryBuilder;

    std::vector<std::unique_ptr<NamedQuery>> m_queries;
};

// The parameters every query takes next to its own: `columns`, `sort`, `limit`.
[[nodiscard]] const std::vector<ParameterInfo>& commonParameters();
// The same for a given query: the queries of the R families also take the parameters common to the result queries.
// A parameter of the query that has the name of a common one (the `modes` of the flutter summary) replaces it.
[[nodiscard]] std::vector<ParameterInfo> commonParametersFor(const QueryInfo& info);

} // namespace oor::query
