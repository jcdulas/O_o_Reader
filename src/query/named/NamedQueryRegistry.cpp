#include "oor/query/named/NamedQueryRegistry.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/RegisterNamedQueries.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/plan/Query.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <algorithm>

namespace oor::query
{

struct NamedQueryRegistryBuilder
{
    static NamedQueryRegistry build()
    {
        NamedQueryRegistry registry;
        registerNamedQueries(registry);
        return registry;
    }
};

const NamedQueryRegistry& NamedQueryRegistry::instance()
{
    static const NamedQueryRegistry REGISTRY = NamedQueryRegistryBuilder::build();
    return REGISTRY;
}

void NamedQueryRegistry::add(std::unique_ptr<NamedQuery> query)
{
    m_queries.push_back(std::move(query));
}

const NamedQuery* NamedQueryRegistry::find(std::string_view name) const noexcept
{
    const auto found = std::find_if(m_queries.begin(), m_queries.end(), [&](const std::unique_ptr<NamedQuery>& query)
                                    { return query->info().name == name; });
    return found == m_queries.end() ? nullptr : found->get();
}

std::vector<ParameterInfo> commonParametersFor(const QueryInfo& info)
{
    std::vector<ParameterInfo> all = commonParameters();
    if (info.resultQuery)
        for (const ParameterInfo& entry : resultParameters())
            all.push_back(entry);
    std::vector<ParameterInfo> kept;
    for (const ParameterInfo& entry : all)
    {
        const bool shadowed = std::any_of(info.parameters.begin(), info.parameters.end(),
                                          [&](const ParameterInfo& own) { return own.name == entry.name; });
        if (!shadowed)
            kept.push_back(entry);
    }
    return kept;
}

const std::vector<ParameterInfo>& commonParameters()
{
    static const std::vector<ParameterInfo> PARAMETERS = {
        parameter("columns", ParameterKind::TextList, "None", "keep only these columns, in this order"),
        parameter("sort", ParameterKind::TextList, "None",
                  "a column name or a list of names; a leading '-' sorts that column in descending order"),
        parameter("limit", ParameterKind::Integer, "None", "keep the first N rows after sorting"),
    };
    return PARAMETERS;
}

namespace
{

std::string joinNames(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

void checkArgumentNames(const QueryInfo& info, const QueryArguments& arguments)
{
    std::vector<std::string> known;
    for (const ParameterInfo& parameter : info.parameters)
        known.push_back(parameter.name);
    for (const ParameterInfo& parameter : commonParametersFor(info))
        known.push_back(parameter.name);
    for (const auto& [name, value] : arguments.values())
    {
        if (std::find(known.begin(), known.end(), name) == known.end())
            throw QueryError("the query '" + info.name + "' has no parameter '" + name + "' (its parameters are " +
                             joinNames(known) + ")");
    }
}

// columns, sort and limit, with the engine of the builder over the table the query made. A column keeps its origin
// (stored columns stay editable), the rows keep the cards they come from.
std::shared_ptr<const QueryTable> applyCommonParameters(const std::shared_ptr<const QueryTable>& table,
                                                        const QueryArguments& arguments, ExecutionContext& context)
{
    const std::vector<std::string> columns = arguments.texts("columns");
    const std::vector<std::string> sorts = arguments.texts("sort");
    const bool limited = arguments.has("limit");
    if (columns.empty() && sorts.empty() && !limited)
        return table;
    Query query(std::make_shared<const TableSource>(table));
    if (!sorts.empty())
    {
        std::vector<SortKey> keys;
        for (std::string name : sorts)
        {
            SortKey key;
            if (!name.empty() && name.front() == '-')
            {
                key.descending = true;
                name.erase(name.begin());
            }
            key.column = std::move(name);
            keys.push_back(std::move(key));
        }
        query = query.sortBy(std::move(keys));
    }
    if (limited)
    {
        const std::int64_t count = arguments.integerOr("limit", 0);
        if (count < 0)
            throw QueryError("limit is a number of rows, not negative");
        query = query.limit(static_cast<std::size_t>(count));
    }
    if (!columns.empty())
    {
        std::vector<SelectItem> items;
        for (const std::string& name : columns)
        {
            SelectItem item;
            item.name = name;
            items.push_back(std::move(item));
        }
        query = query.select(std::move(items));
    }
    const std::shared_ptr<const QueryTable> result = query.run(context);
    if (table->sources().empty())
        return result;
    // the provenance of the values goes with the table through `columns`, `sort` and `limit`
    auto carried = std::make_shared<QueryTable>(*result);
    carried->setSources(table->sources());
    return carried;
}

} // namespace

std::shared_ptr<const QueryTable> NamedQueryRegistry::run(std::string_view name, const QueryRun& run) const
{
    const NamedQuery* query = find(name);
    if (query == nullptr)
        throw QueryError("there is no named query called '" + std::string(name) + "'");
    checkArgumentNames(query->info(), run.arguments);
    std::shared_ptr<const QueryTable> table = query->run(run);
    if (table->rowCount() == 0 && table->columnCount() == 0)
    {
        // A result with no column at all (a query that gave nothing back) has the columns the catalogue declares.
        const QueryInfo& info = query->info();
        const std::string detail =
            info.detailParameter.empty() ? std::string() : run.arguments.formLabel(info.detailParameter);
        table = emptyTable(info.columnsFor(detail));
    }
    return applyCommonParameters(table, run.arguments, run.context);
}

} // namespace oor::query
