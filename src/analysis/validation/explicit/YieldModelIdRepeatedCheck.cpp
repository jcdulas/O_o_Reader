#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"

namespace oor::analysis
{

namespace
{

// A yield model number that two kinds of card have: the YLDxxx cards are named by the YID of a MATDEUL, and the format
// reference says a YID is unique among all of them.
class YieldModelIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.yield-model-id-repeated", base::Severity::Error, "explicit",
                                    "A yield model card has the number of a yield model of another card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::YieldModel, "yield model");
    }
};

} // namespace

void addYieldModelIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<YieldModelIdRepeatedCheck>());
}

} // namespace oor::analysis
