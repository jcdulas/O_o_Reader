#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A ply of a PCOMP or PCOMPG made of a material that is not a MAT1, MAT2 or MAT8.
class PlyMaterialTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.ply-material-type", base::Severity::Error, "compatibility",
                                    "A ply of a composite property uses a material that is not isotropic, 2D "
                                    "anisotropic or 2D orthotropic (or a progressive failure material, and for a "
                                    "composite property without global ply IDs a fabric or user composite failure "
                                    "material)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // format reference MATCB: a ply of a PCOMP, PCOMPG or PCOMPLS may name it; a
        // ply of a PCOMP may name the woven fabric of SOL 700; format reference COMPUDS: the user failure model of SOL
        // 700 for shell composites.
        reportUnlistedMaterial(context, sink, {"CompProp"},
                               {"IsoMat", "AnisoMat2D", "OrthoMat2D", "HeatIsoMat", "HeatAnisoMat",
                                "ProgressiveFailureMat", "FabricMat", "UserCompositeFailureMat"},
                               {"IsoMat", "AnisoMat2D", "OrthoMat2D"});
        reportUnlistedMaterial(
            context, sink, {"CompPropGlobal"},
            {"IsoMat", "AnisoMat2D", "OrthoMat2D", "HeatIsoMat", "HeatAnisoMat", "ProgressiveFailureMat"},
            {"IsoMat", "AnisoMat2D", "OrthoMat2D"});
    }
};

} // namespace

void addPlyMaterialTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PlyMaterialTypeCheck>());
}

} // namespace oor::analysis
