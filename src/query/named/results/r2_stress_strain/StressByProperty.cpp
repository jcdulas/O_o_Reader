#include "StressData.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

namespace
{

// stressByProperty: the extremes of a stress over the elements of each property.
class StressByPropertyQuery final : public ResultQuery
{
public:
    StressByPropertyQuery() : ResultQuery(stressByGroupInfo(false)) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return stressByGroup(context, info(), false);
    }
};

} // namespace

void addStressByPropertyQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StressByPropertyQuery>());
}

} // namespace oor::query
