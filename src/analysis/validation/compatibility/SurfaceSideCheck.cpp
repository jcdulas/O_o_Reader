#include "oor/analysis/geometry/SurfaceSides.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"

namespace oor::analysis
{

namespace
{

// A CHBDYE on a side the conduction element does not have (a tetrahedron has
// four sides, a prism or a pyramid five, a hexahedron six, a line element three, a triangle shell five and a
// quadrilateral shell six). A conduction element that does not exist is the business of ref.unresolved, and one with no
// side numbering in this library is not judged.
class SurfaceSideCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.surface-side", base::Severity::Error, "compatibility",
                                    "A heat surface element on an element side names a side that its conduction "
                                    "element does not have."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::HeatSurfaceSideTable>(
            model,
            [&](const model::HeatSurfaceSideCard& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> element =
                    model.locate(base::IdSpace::Element, card.conductionElemId);
                if (!element || surfaceSideOf(model, *element, card.sideCode) != SurfaceSide::NoSuchSide)
                    return;
                sink.reportCard(where, "sideCode",
                                "side " + std::to_string(card.sideCode) + " is not a side of " +
                                    std::string(cardNameAt(model, *element)) + " " +
                                    std::to_string(card.conductionElemId),
                                "use a side number the element has");
            });
    }
};

} // namespace

void addSurfaceSideCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SurfaceSideCheck>());
}

} // namespace oor::analysis
