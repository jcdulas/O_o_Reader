#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The deflection limits PLLIM and PULIM of a control surface, and its hinge moment limits HMLLIM and HMULIM, are a
// lower and an upper limit: a lower limit above the upper one leaves the surface no position (or no hinge moment) that
// is allowed. The card does not refuse it (no rule of the format reference says it), so this is a warning. A limit
// given by a table (TQLLIM, TQULIM) is a function of the dynamic pressure and is not compared with a number.
class SurfaceLimitsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.surface-limits", base::Severity::Warning, "aero",
                                    "The lower limit of a control surface is above its upper limit."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroControlSurfaceTable>(
            model,
            [&](const model::AeroControlSurfaceCard& card, model::CardLocation where)
            {
                if (card.lowerLimitTableId == 0 && card.upperLimitTableId == 0 &&
                    card.deflectionLowerLimit > card.deflectionUpperLimit)
                {
                    sink.reportCard(where, "deflectionLowerLimit",
                                    "the lower deflection limit " + std::to_string(card.deflectionLowerLimit) +
                                        " is above the upper limit " + std::to_string(card.deflectionUpperLimit),
                                    "give " + std::string("deflectionLowerLimit") + " below " +
                                        std::string("deflectionUpperLimit"));
                }
                if (card.hingeMomentLowerLimit && card.hingeMomentUpperLimit &&
                    *card.hingeMomentLowerLimit > *card.hingeMomentUpperLimit)
                {
                    sink.reportCard(where, "hingeMomentLowerLimit",
                                    "the lower hinge moment limit " + std::to_string(*card.hingeMomentLowerLimit) +
                                        " is above the upper limit " + std::to_string(*card.hingeMomentUpperLimit),
                                    "give " + std::string("hingeMomentLowerLimit") + " below " +
                                        std::string("hingeMomentUpperLimit"));
                }
            });
    }
};

} // namespace

void addSurfaceLimitsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SurfaceLimitsCheck>());
}

} // namespace oor::analysis
