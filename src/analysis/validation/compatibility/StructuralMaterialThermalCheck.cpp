#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// The structural solutions of the format reference: the static, normal modes, buckling and response sequences. The heat
// transfer ones (153, 159) are left out, as they take thermal materials.
bool isStructuralSolution(std::int64_t solution) noexcept
{
    return isStaticSolution(solution) || isDynamicSolution(solution);
}

// A structural analysis whose property names a material that only exists as a heat transfer one: a MAT4 or MAT5 with
// that ID and no MAT1, MAT2, ... The element has no stiffness (format reference: the material of a structural property
// is a MAT1, MAT2, MAT8, ...).
class StructuralMaterialThermalCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.structural-material-thermal", base::Severity::Error, "compatibility",
                                    "A structural analysis has a property whose material exists only as a heat "
                                    "transfer material."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || !isStructuralSolution(*solution) || isHeatTransfer(context.root()) ||
            isHeatThenStress(context.root()))
            return;
        const model::Model& model = context.model();
        for (const CardReference& use : context.references().propertyMaterials)
        {
            const std::string_view property = cardNameAt(model, use.location);
            if (!isConductionMaterialField(property, use.field) || model.locate(base::IdSpace::Material, use.id))
                continue;
            const std::optional<model::CardLocation> thermal = model.locate(base::IdSpace::ThermalMaterial, use.id);
            if (!thermal)
                continue; // no such material: ref.unresolved
            sink.reportCard(use.location, use.field,
                            "uses material " + std::to_string(use.id) + ", which is a " +
                                std::string(cardNameAt(model, *thermal)) +
                                " (heat transfer) and has no structural material of the same ID: the element has no "
                                "stiffness in SOL " +
                                std::to_string(*solution),
                            "add a " + cardLabels(model, {"IsoMat", "AnisoMat2D", "OrthoMat2D"}) +
                                " with this ID, or point the property at one");
        }
    }
};

} // namespace

void addStructuralMaterialThermalCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<StructuralMaterialThermalCheck>());
}

} // namespace oor::analysis
