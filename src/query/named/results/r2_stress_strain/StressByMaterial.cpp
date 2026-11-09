#include "StressData.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

namespace
{

// stressByMaterial: the extremes of a stress over the elements of each material.
class StressByMaterialQuery final : public ResultQuery
{
public:
    StressByMaterialQuery() : ResultQuery(stressByGroupInfo(true)) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return stressByGroup(context, info(), true);
    }
};

} // namespace

void addStressByMaterialQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StressByMaterialQuery>());
}

} // namespace oor::query
