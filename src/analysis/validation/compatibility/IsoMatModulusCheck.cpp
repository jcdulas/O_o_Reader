#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"

namespace oor::analysis
{

namespace
{

// Young's modulus of a MAT1 that is zero or negative (a blank E is taken from G and NU when it can be, so this is what
// the card means).
class IsoMatModulusCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.iso-mat-modulus", base::Severity::Error, "compatibility",
                                    "The Young modulus of an isotropic material is zero or negative."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::IsoMatTable>(context.model(),
                                        [&](const model::IsoMatCard& card, model::CardLocation where)
                                        {
                                            if (!(card.youngsModulus > 0.0))
                                                sink.reportCard(where, "youngsModulus",
                                                                std::string("youngsModulus") + " is " +
                                                                    std::to_string(card.youngsModulus) +
                                                                    ": Young's modulus must be positive",
                                                                "give a positive modulus of elasticity");
                                        });
    }
};

} // namespace

void addIsoMatModulusCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<IsoMatModulusCheck>());
}

} // namespace oor::analysis
