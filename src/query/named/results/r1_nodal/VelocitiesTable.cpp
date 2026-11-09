#include "NodalVectorTable.hpp"

namespace oor::query
{

namespace
{

// velocitiesTable: the velocity of every grid point
class VelocitiesTableQuery final : public ResultQuery
{
public:
    VelocitiesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return nodalVectorTable(context, info(), results::ResultKind::Velocities);
    }

private:
    static QueryInfo makeInfo()
    {
        return nodalVectorInfo("velocitiesTable",
                               "The velocities of the grid points and scalar points of a dynamic analysis, with the "
                               "parameters and the columns of `displacementsTable`.");
    }
};

} // namespace

void addVelocitiesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<VelocitiesTableQuery>());
}

} // namespace oor::query
