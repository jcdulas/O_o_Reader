#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Table.hpp"
#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The finite surface spline (SPLINE6) and the finite beam spline (SPLINE7) do not interpolate a body: a CAERO2 is not
// supported (format reference, SPLINE6 and SPLINE7). Their CAERO field is optional, so a spline that names none is not
// judged; the CAERO that does not exist is `ref.unresolved`.
class SplineBodyCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.spline-body", base::Severity::Error, "aero",
                                    "A finite surface or finite beam spline names a slender body, which these splines "
                                    "do not support."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroFiniteSurfaceSpline6Table>(
            model, [&](const model::AeroFiniteSurfaceSpline6Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroFiniteSurfaceSpline6", card.aeroPanelId); });
        forEachCard<model::AeroFiniteBeamSpline7Table>(
            model, [&](const model::AeroFiniteBeamSpline7Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroFiniteBeamSpline7", card.aeroPanelId); });
    }

private:
    // `spline`: the card type of the spline.
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, std::string_view spline,
                     base::EntityId caero)
    {
        if (caero == 0)
            return;
        const std::optional<model::CardLocation> panel = model.locate(base::IdSpace::AeroElement, caero);
        if (panel && cardNameAt(model, *panel) == "AeroBody2")
        {
            sink.reportCard(where, "aeroPanelId",
                            "the aerodynamic panel " + std::to_string(caero) + " is a body, and a " +
                                std::string(spline) + " does not interpolate bodies",
                            "use a " +
                                cardLabels(model, {"AeroSurfaceSpline1", "AeroBeamSpline2", "AeroSurfaceSpline4",
                                                   "AeroBeamSpline5"}) +
                                " for the body");
        }
    }
};

} // namespace

void addSplineBodyCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SplineBodyCheck>());
}

} // namespace oor::analysis
