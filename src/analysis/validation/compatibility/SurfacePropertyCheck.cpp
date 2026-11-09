#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropTable.hpp"

namespace oor::analysis
{

namespace
{

// A CHBDYP whose PHBDY lacks what its type needs: the area factor AF for a
// POINT (the area), a LINE or an ELCYL (the effective width), and the diameter D1 for an ELCYL, a TUBE or an FTUBE.
// Without them the surface has no area and carries no heat. A PHBDY that does not exist is the business of
// ref.unresolved.
class SurfacePropertyCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.surface-property", base::Severity::Warning, "compatibility",
        "A property-defined heat surface element has a heat surface property without the area factor or the diameter "
        "its surface type needs."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::HeatSurfacePropertyTable>(
            model,
            [&](const model::HeatSurfacePropertyCard& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> found =
                    model.locateInType(model::typeIndexOf<model::HeatSurfacePropTable>(), card.propId);
                if (!found)
                    return;
                const model::HeatSurfacePropCard property = cardAt<model::HeatSurfacePropTable>(model, *found);
                const bool needsArea = card.kind == "Point" || card.kind == "Line" || card.kind == "EllipticCylinder";
                const bool needsDiameter =
                    card.kind == "EllipticCylinder" || card.kind == "Tube" || card.kind == "ForcedConvectionTube";
                const std::string surface = std::string("HeatSurfaceProp");
                const std::string area = std::string("area");
                const std::string diameter = std::string("diameter1");
                if (needsArea && (!property.area || *property.area <= 0.0))
                    sink.reportCard(where, "propId",
                                    surface + " " + std::to_string(card.propId) + " has no area factor " + area +
                                        ", which a " + card.kind + " surface needs",
                                    "give the " + surface + " a positive " + area);
                if (needsDiameter && (!property.diameter1 || *property.diameter1 <= 0.0))
                    sink.reportCard(where, "propId",
                                    surface + " " + std::to_string(card.propId) + " has no diameter " + diameter +
                                        ", which a " + card.kind + " surface needs",
                                    "give the " + surface + " a positive " + diameter);
            });
    }
};

} // namespace

void addSurfacePropertyCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SurfacePropertyCheck>());
}

} // namespace oor::analysis
