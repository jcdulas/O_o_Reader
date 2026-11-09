#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"

namespace oor::analysis
{

namespace
{

// An orthotropic ply whose NU12 makes the stiffness matrix lose its positive definiteness: NU12 squared times E2 over
// E1 must stay below 1 (the reciprocal relation gives NU21 = NU12 E2 / E1, and NU12 NU21 < 1).
class OrthoMat2DPoissonCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.ortho-mat2d-poisson", base::Severity::Error, "compatibility",
                                    "The Poisson ratio nu12 of a 2D orthotropic material is so large that nu12 times "
                                    "nu21 reaches 1."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::OrthoMat2DTable>(
            context.model(),
            [&](const model::OrthoMat2DCard& card, model::CardLocation where)
            {
                if (!(card.youngsModulus1 > 0.0) || !(card.youngsModulus2 > 0.0))
                    return; // the card refuses these
                if (card.poissonRatio12 * card.poissonRatio12 * card.youngsModulus2 / card.youngsModulus1 < 1.0)
                    return;
                const std::string nu12 = std::string("poissonRatio12");
                sink.reportCard(where, "poissonRatio12",
                                nu12 + " = " + std::to_string(card.poissonRatio12) + " with " +
                                    std::string("youngsModulus1") + " = " + std::to_string(card.youngsModulus1) +
                                    " and " + std::string("youngsModulus2") + " = " +
                                    std::to_string(card.youngsModulus2) +
                                    " gives a product of the two Poisson's ratios of 1 or more: the material is not "
                                    "stable",
                                "check " + nu12 + " and the two moduli");
            });
    }
};

} // namespace

void addOrthoMat2DPoissonCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OrthoMat2DPoissonCheck>());
}

} // namespace oor::analysis
