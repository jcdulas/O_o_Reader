#include "StressData.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

namespace
{

// strainTable: the strains of the elements, with the parameters of `stressTable`.
class StrainTableQuery final : public ResultQuery
{
public:
    StrainTableQuery() : ResultQuery(stressTableInfo(true)) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return stressTable(context, info(), true);
    }
};

} // namespace

void addStrainTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StrainTableQuery>());
}

} // namespace oor::query
