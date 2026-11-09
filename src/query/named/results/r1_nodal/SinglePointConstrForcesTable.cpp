#include "NodalVectorTable.hpp"

namespace oor::query
{

namespace
{

// singlePointConstrForcesTable: the forces of the single-point constraints
class SinglePointConstrForcesTableQuery final : public ResultQuery
{
public:
    SinglePointConstrForcesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return nodalVectorTable(context, info(), results::ResultKind::SinglePointConstrForces);
    }

private:
    static QueryInfo makeInfo()
    {
        return nodalVectorInfo("singlePointConstrForcesTable",
                               "The forces of the single point constraints (the reactions) at the grid points that "
                               "have one: three forces and three moments, their norm (`magnitude`, of the three "
                               "forces) and the frame, with the parameters of `displacementsTable`.");
    }
};

} // namespace

void addSinglePointConstrForcesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SinglePointConstrForcesTableQuery>());
}

} // namespace oor::query
