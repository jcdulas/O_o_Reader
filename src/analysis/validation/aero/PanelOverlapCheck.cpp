#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/AeroBoxRanges.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The boxes of a CAERO are numbered from its EID on (the user must see that every
// box number is unique), so two panels whose ID ranges meet share a box. The solver would number two boxes alike and
// the splines and the results could not tell them apart.
class PanelOverlapCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.panel-overlap", base::Severity::Error, "aero",
                                    "The boxes of an aerodynamic panel take IDs that the boxes of another panel have."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::AeroBoxRanges boxes(context.model());
        const model::AeroBoxRange* widest = nullptr; // the range seen so far that reaches the highest ID
        for (const model::AeroBoxRange& range : boxes.ranges())
        {
            if (widest != nullptr && range.panelId <= widest->lastBox())
            {
                sink.reportCard(range.location, "id",
                                "the boxes " + std::to_string(range.panelId) + " to " +
                                    std::to_string(range.lastBox()) + " include IDs of the panel " +
                                    std::to_string(widest->panelId) + ", whose boxes are " +
                                    std::to_string(widest->panelId) + " to " + std::to_string(widest->lastBox()),
                                "give the panel an ID above the last box of the one before it");
            }
            if (widest == nullptr || range.lastBox() > widest->lastBox())
                widest = &range;
        }
    }
};

} // namespace

void addPanelOverlapCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PanelOverlapCheck>());
}

} // namespace oor::analysis
