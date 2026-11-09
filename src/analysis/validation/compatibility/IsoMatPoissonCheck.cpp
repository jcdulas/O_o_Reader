#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"

namespace oor::analysis
{

namespace
{

// Poisson's ratio of a MAT1 outside the range of a stable isotropic material, from -1 up to but not including 0.5 (at
// 0.5 the material is incompressible: the bulk modulus is infinite).
class IsoMatPoissonCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.iso-mat-poisson", base::Severity::Error, "compatibility",
                                    "The Poisson ratio of an isotropic material is 0.5 or more, or -1 or less."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::IsoMatTable>(
            context.model(),
            [&](const model::IsoMatCard& card, model::CardLocation where)
            {
                if (card.poissonRatio >= 0.5 || card.poissonRatio <= -1.0)
                    sink.reportCard(where, "poissonRatio",
                                    std::string("poissonRatio") + " is " + std::to_string(card.poissonRatio) +
                                        ": it must lie between -1 and 0.5",
                                    "use a Poisson's ratio below 0.5 (0.49 for a nearly incompressible material)");
            });
    }
};

} // namespace

void addIsoMatPoissonCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<IsoMatPoissonCheck>());
}

} // namespace oor::analysis
