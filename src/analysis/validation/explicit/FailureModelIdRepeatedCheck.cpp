#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"

namespace oor::analysis
{

namespace
{

// A failure model number that two kinds of card have: FAILJC, FAILMPS and FAILUDS are named by the FID of a MATDEUL, so
// the FIDs must be unique among them.
class FailureModelIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.failure-model-id-repeated", base::Severity::Error, "explicit",
                                    "A failure model card has the number of a failure model of another card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::FailureModel, "failure model");
    }
};

} // namespace

void addFailureModelIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FailureModelIdRepeatedCheck>());
}

} // namespace oor::analysis
