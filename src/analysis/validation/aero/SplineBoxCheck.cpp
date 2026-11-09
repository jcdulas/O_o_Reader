#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Table.hpp"
#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"
#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A SPLINE1 moves the boxes BOX1 to BOX2 of the panel CAERO, a SPLINE2 the boxes or body elements ID1 to ID2, a SPLINE3
// the box BOXID: they must be IDs the panel owns, from its EID on. With the Mach
// box method the numbers count the (x, y) pairs of the CAERO3 lists, which its range counts too. A panel that does not
// exist is `ref.unresolved`.
class SplineBoxCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.spline-box", base::Severity::Error, "aero",
                                    "A spline names boxes that its aerodynamic panel does not own."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::AeroBoxRanges boxes(model);
        forEachCard<model::AeroSurfaceSpline1Table>(
            model,
            [&](const model::AeroSurfaceSpline1Card& card, model::CardLocation where)
            {
                look(boxes, sink, where, "firstBox", card.aeroPanelId, card.firstBox);
                look(boxes, sink, where, "lastBox", card.aeroPanelId, card.lastBox);
            });
        forEachCard<model::AeroBeamSpline2Table>(
            model,
            [&](const model::AeroBeamSpline2Card& card, model::CardLocation where)
            {
                look(boxes, sink, where, "firstBox", card.aeroPanelId, card.firstBox);
                look(boxes, sink, where, "lastBox", card.aeroPanelId, card.lastBox);
            });
        forEachCard<model::AeroSplineEquation3Table>(
            model, [&](const model::AeroSplineEquation3Card& card, model::CardLocation where)
            { look(boxes, sink, where, "boxId", card.aeroPanelId, card.boxId); });
    }

private:
    static void look(const model::AeroBoxRanges& boxes, FindingSink& sink, model::CardLocation where, const char* field,
                     base::EntityId caero, base::EntityId box)
    {
        const model::AeroBoxRange* panel = boxes.owner(caero);
        if (panel == nullptr || panel->panelId != caero)
            return;
        if (box < panel->panelId || box > panel->lastBox())
        {
            sink.reportCard(where, field,
                            "the box " + std::to_string(box) + " is not one of the boxes " +
                                std::to_string(panel->panelId) + " to " + std::to_string(panel->lastBox()) +
                                " of the panel " + std::to_string(caero),
                            "name a box of the panel the spline names");
        }
    }
};

} // namespace

void addSplineBoxCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SplineBoxCheck>());
}

} // namespace oor::analysis
