#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A PSHELL whose MID1, MID2, MID3 or MID4 is not a MAT1, MAT2 or MAT8 (any combination of the
// three).
class ShellMaterialTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.shell-prop-material-type", base::Severity::Error, "compatibility",
                                    "A shell property uses a material that is not isotropic, 2D anisotropic or 2D "
                                    "orthotropic (or, in SOL 700, a rigid material or a user composite failure "
                                    "material)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // a rigid material of SOL 700 is named where a property names its material
        // (PSHELLn among them); format reference COMPUDS: the user failure model of SOL 700 for shell composites is a
        // material of its own.
        reportUnlistedMaterial(
            context, sink, {"ShellProp"},
            {"IsoMat", "AnisoMat2D", "OrthoMat2D", "HeatIsoMat", "HeatAnisoMat", "RigidMat", "UserCompositeFailureMat"},
            {"IsoMat", "AnisoMat2D", "OrthoMat2D"});
    }
};

} // namespace

void addShellMaterialTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellMaterialTypeCheck>());
}

} // namespace oor::analysis
