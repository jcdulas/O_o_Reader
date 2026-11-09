#include "oor/api/Queries.hpp"

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/query/named/NamedQuery.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/table/CsvExport.hpp"
#include "oor/query/table/ModelLink.hpp"

#include <sstream>

namespace oor::api
{

namespace
{

// The model a query table keeps a link to (for the columns it computes later), held for the life of the table.
class HeldModelLink final : public query::ModelLink
{
public:
    explicit HeldModelLink(std::shared_ptr<model::Model> model) : m_model(std::move(model)) {}
    [[nodiscard]] std::shared_ptr<model::Model> resolve() const override { return m_model; }

private:
    std::shared_ptr<model::Model> m_model;
};

const query::NamedQuery& findQuery(const std::string& name)
{
    const query::NamedQuery* found = query::NamedQueryRegistry::instance().find(name);
    if (found == nullptr)
        throw query::QueryError("there is no named query called '" + name + "'");
    return *found;
}

} // namespace

std::vector<QueryDescription> queryCatalogue()
{
    std::vector<QueryDescription> catalogue;
    for (const std::unique_ptr<query::NamedQuery>& entry : query::NamedQueryRegistry::instance().all())
    {
        const query::QueryInfo& info = entry->info();
        QueryDescription description;
        description.name = info.name;
        description.family = info.family;
        description.description = info.description;
        description.resultQuery = info.resultQuery;
        description.needsModel = info.needsModel;
        description.parameters = info.parameters;
        for (const query::ParameterInfo& common : query::commonParametersFor(info))
            description.parameters.push_back(common);
        catalogue.push_back(std::move(description));
    }
    return catalogue;
}

query::ParameterKind parameterKind(const std::string& queryName, const std::string& parameter)
{
    const query::QueryInfo& info = findQuery(queryName).info();
    for (const query::ParameterInfo& own : info.parameters)
        if (own.name == parameter)
            return own.kind;
    for (const query::ParameterInfo& common : query::commonParametersFor(info))
        if (common.name == parameter)
            return common.kind;
    throw query::QueryError("the query '" + queryName + "' has no parameter '" + parameter + "'");
}

std::shared_ptr<const query::QueryTable> runQuery(const std::string& name, const std::shared_ptr<Deck>& deck,
                                                  const std::shared_ptr<Results>& results,
                                                  const query::QueryArguments& arguments)
{
    (void)findQuery(name);
    std::shared_ptr<model::Model> model = deck ? deck->model() : nullptr;
    std::vector<query::ResultSet> sets;
    if (results)
        sets.push_back(results->resultSet());
    std::shared_ptr<const query::ModelLink> link = model ? std::make_shared<HeldModelLink>(model) : nullptr;
#if defined(__EMSCRIPTEN__)
    base::ThreadPool* pool = nullptr; // one thread: everything on the calling thread
#else
    static base::ThreadPool sharedPool(base::ThreadPool::defaultWorkerCount() - 1);
    base::ThreadPool* pool = &sharedPool;
#endif
    query::ExecutionContext context(pool, nullptr, nullptr);
    const query::QueryRun run{model, arguments, context, link, sets};
    return query::NamedQueryRegistry::instance().run(name, run);
}

std::string toCsv(const query::QueryTable& table)
{
    std::ostringstream text;
    query::writeCsv(table, text);
    return text.str();
}

} // namespace oor::api
