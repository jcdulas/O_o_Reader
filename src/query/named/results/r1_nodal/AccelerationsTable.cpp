#include "NodalVectorTable.hpp"

namespace oor::query
{

namespace
{

// accelerationsTable: the acceleration of every grid point
class AccelerationsTableQuery final : public ResultQuery
{
public:
    AccelerationsTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return nodalVectorTable(context, info(), results::ResultKind::Accelerations);
    }

private:
    static QueryInfo makeInfo()
    {
        return nodalVectorInfo("accelerationsTable",
                               "The accelerations of the grid points and scalar points of a dynamic analysis, with the "
                               "parameters and the columns of `displacementsTable`.");
    }
};

} // namespace

void addAccelerationsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AccelerationsTableQuery>());
}

} // namespace oor::query
