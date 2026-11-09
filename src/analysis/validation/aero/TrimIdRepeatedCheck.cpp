#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"

namespace oor::analysis
{

namespace
{

// The Case Control TRIM names a trim set by its SID, whether the card is a TRIM or a TRIM2, and the format reference
// wants the SID unique among both.
class TrimIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.trim-id-repeated", base::Severity::Error, "aero",
                                    "Two trim cards of different kinds have the same set number."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::AeroTrimSet, "trim set");
    }
};

} // namespace

void addTrimIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TrimIdRepeatedCheck>());
}

} // namespace oor::analysis
