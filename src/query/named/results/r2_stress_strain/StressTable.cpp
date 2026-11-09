#include "StressData.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

namespace
{

// stressTable: the stresses of the elements, one table for every element type.
class StressTableQuery final : public ResultQuery
{
public:
    StressTableQuery() : ResultQuery(stressTableInfo(false)) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return stressTable(context, info(), false);
    }
};

} // namespace

void addStressTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StressTableQuery>());
}

} // namespace oor::query
