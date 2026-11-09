#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"

namespace oor::analysis
{

namespace
{

// A shear model number that two kinds of card have: SHREL, SHRPOL and SHRUDS are named by the SID of a MATDEUL, so the
// SIDs must be unique among them.
class ShearModelIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.shear-model-id-repeated", base::Severity::Error, "explicit",
                                    "A shear model card has the number of a shear model of another card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::ShearModel, "shear model");
    }
};

} // namespace

void addShearModelIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShearModelIdRepeatedCheck>());
}

} // namespace oor::analysis
