#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroControlSurfaceNodes/AeroControlSurfaceNodesTable.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// An AESURFS gives the structural grids of a control surface and finds the surface by its LABEL (format reference,
// the ID is ignored). A label no AESURF has leaves the grids attached to nothing.
class SurfaceLabelCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.surface-label", base::Severity::Error, "aero",
                                    "A control surface node card names a control surface that no control surface card "
                                    "defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::unordered_set<std::string> surfaces;
        forEachCard<model::AeroControlSurfaceTable>(context.model(),
                                                    [&](const model::AeroControlSurfaceCard& card, model::CardLocation)
                                                    { surfaces.insert(std::string(card.label)); });
        forEachCard<model::AeroControlSurfaceNodesTable>(
            context.model(),
            [&](const model::AeroControlSurfaceNodesCard& card, model::CardLocation where)
            {
                if (!surfaces.contains(std::string(card.label)))
                    sink.reportCard(where, "label",
                                    "no " + std::string("AeroControlSurface") + " has the label " + card.label,
                                    "correct the label, or define the control surface");
            });
    }
};

} // namespace

void addSurfaceLabelCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SurfaceLabelCheck>());
}

} // namespace oor::analysis
