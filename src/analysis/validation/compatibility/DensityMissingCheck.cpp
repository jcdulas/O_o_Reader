#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// A structural material with no mass density, when the analysis needs mass: a dynamic solution, or PARAM,GRDPNT or
// PARAM,WTMASS (the grid point weight and the mass of the model are asked for).
class DensityMissingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.density-missing", base::Severity::Info, "compatibility",
                                    "A material used by a structural property has no mass density, and the analysis "
                                    "asks for mass."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::optional<std::int64_t> solution = context.solution();
        const bool massAsked = (solution && isDynamicSolution(*solution)) ||
                               findParameter(model, "WeightReferenceNode") || findParameter(model, "WeightToMass");
        if (!massAsked)
            return;
        for (const CardReference& use : context.references().propertyMaterials)
        {
            if (!isOneOf(cardNameAt(model, use.location),
                         {"ShellProp", "CompProp", "CompPropGlobal", "SolidProp", "SolidPropHyperelastic", "RodProp",
                          "TubeProp", "BarProp", "BarShapeProp", "BeamProp", "BeamShapeProp", "ShearPanelProp",
                          "BeamLumpedAreaProp"}))
                continue;
            const std::optional<double> density = MaterialProperties::density(model, use.id);
            if (!density || *density != 0.0)
                continue;
            const std::optional<model::CardLocation> material = model.locate(base::IdSpace::Material, use.id);
            const std::string densityField = material ? std::string("density") : std::string("density");
            sink.reportCard(
                use.location, use.field,
                "material " + std::to_string(use.id) + " has no mass density (" + densityField + " is zero or blank)",
                "give the material a density, or the mass is left out of the model");
        }
    }
};

} // namespace

void addDensityMissingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DensityMissingCheck>());
}

} // namespace oor::analysis
