#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A PSOLID or PLSOLID with a material of the shell class (MAT2, MAT3, MAT8): the format reference gives MAT1, MAT4,
// MAT5, MAT9, MAT10, MATHP, MATHE for a solid.
class SolidMaterialTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.solid-material-type", base::Severity::Error, "compatibility",
                                    "A solid or hyperelastic solid property uses a material of the shell class (2D "
                                    "anisotropic, axisymmetric orthotropic or 2D orthotropic)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportRefusedMaterial(context, sink, {"SolidProp", "SolidPropHyperelastic"},
                              {"AnisoMat2D", "AxisymOrthoMat", "OrthoMat2D"},
                              {"IsoMat", "AnisoMat3D", "FluidMat", "HyperelasticPolynomialMat", "HyperelasticMat"});
    }
};

} // namespace

void addSolidMaterialTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SolidMaterialTypeCheck>());
}

} // namespace oor::analysis
