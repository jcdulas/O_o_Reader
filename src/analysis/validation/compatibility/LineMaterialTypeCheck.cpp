#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A rod, tube, bar, beam or shear panel property with a structural material that is not a MAT1 (the format reference:
// in a structural run, MID names a MAT1).
class LineMaterialTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.line-material-type", base::Severity::Error, "compatibility",
                                    "A rod, tube, bar, beam or shear panel property uses a structural material that is "
                                    "not an isotropic material."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportRefusedMaterial(
            context, sink,
            {"RodProp", "TubeProp", "BarProp", "BarShapeProp", "BeamProp", "BeamShapeProp", "ShearPanelProp"},
            {"AnisoMat2D", "AxisymOrthoMat", "OrthoMat2D", "AnisoMat3D", "FluidMat", "OrthoMat3D"}, {"IsoMat"});
    }
};

} // namespace

void addLineMaterialTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LineMaterialTypeCheck>());
}

} // namespace oor::analysis
