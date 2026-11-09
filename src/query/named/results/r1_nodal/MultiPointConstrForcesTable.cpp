#include "NodalVectorTable.hpp"

namespace oor::query
{

namespace
{

// multiPointConstrForcesTable: the forces of the multipoint constraints
class MultiPointConstrForcesTableQuery final : public ResultQuery
{
public:
    MultiPointConstrForcesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return nodalVectorTable(context, info(), results::ResultKind::MultiPointConstrForces);
    }

private:
    static QueryInfo makeInfo()
    {
        return nodalVectorInfo("multiPointConstrForcesTable",
                               "The forces of the multipoint constraints at the grid points that have one, with the "
                               "parameters and the columns of `singlePointConstrForcesTable`.");
    }
};

} // namespace

void addMultiPointConstrForcesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MultiPointConstrForcesTableQuery>());
}

} // namespace oor::query
