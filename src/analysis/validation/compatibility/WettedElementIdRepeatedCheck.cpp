#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// A wetted element number that two kinds of card have: a WETELME and a WETELMG each define a wetted element that a
// WETSURF names by its WEID, so the number must be unique among the two (the format reference: "wetted element
// identification number" on both). Two cards of one type are the reader's business (`ref.duplicate-id`).
class WettedElementIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.wetted-element-id-repeated", base::Severity::Error, "compatibility",
                                    "A wetted element side card and a wetted element node card have the same wetted "
                                    "element number."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::WettedElement, "wetted element");
    }
};

} // namespace

void addWettedElementIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<WettedElementIdRepeatedCheck>());
}

} // namespace oor::analysis
