#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A heat transfer analysis (APP HEAT) whose conduction property names a material that only exists as a structural one:
// a MAT1 or MAT8 with that ID and no MAT4 or MAT5. Conduction reads the thermal conductivity, which only MAT4 and MAT5
// hold (format reference: PSOLID, PSHELL, PROD, ... "MID": a MAT4 or MAT5 for heat transfer). A MAT1 and a MAT4 may
// share an ID (a model that is analysed for both has the two), so the ID alone is not wrong: it is the missing thermal
// material that is.
class HeatMaterialStructuralCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.heat-material-structural", base::Severity::Error, "compatibility",
                                    "A heat transfer analysis has a conduction property whose material exists only as "
                                    "a structural one, with no isotropic or anisotropic heat material of that ID."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        if (!isHeatTransfer(context.root()))
            return;
        const model::Model& model = context.model();
        for (const CardReference& use : context.references().propertyMaterials)
        {
            const std::string_view property = cardNameAt(model, use.location);
            if (!isConductionMaterialField(property, use.field))
                continue;
            const std::optional<model::CardLocation> structural = model.locate(base::IdSpace::Material, use.id);
            if (!structural || model.locate(base::IdSpace::ThermalMaterial, use.id))
                continue; // no such material (ref.unresolved), or the thermal one is there
            sink.reportCard(use.location, use.field,
                            "uses material " + std::to_string(use.id) + ", which is a " +
                                std::string(cardNameAt(model, *structural)) + " and has no " +
                                std::string("HeatIsoMat") + " or " + std::string("HeatAnisoMat") +
                                " of the same ID: heat conduction needs a thermal material",
                            "add a " + std::string("HeatIsoMat") + " or " + std::string("HeatAnisoMat") +
                                " with this ID, or point the property at one");
        }
    }
};

} // namespace

void addHeatMaterialStructuralCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<HeatMaterialStructuralCheck>());
}

} // namespace oor::analysis
