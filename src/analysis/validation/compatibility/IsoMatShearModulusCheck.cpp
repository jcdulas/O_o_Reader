#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"

#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The shear modulus of a MAT1 that is zero or negative, when something that needs it uses the material. A MAT1 given
// with E only has G = 0 (and NU = 0): that is a material for rods and springs, which need no shear modulus.
class IsoMatShearModulusCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.iso-mat-shear-modulus", base::Severity::Error, "compatibility",
                                    "The shear modulus of an isotropic material is zero or negative."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // The materials used by a property that needs a shear modulus (everything but a rod).
        std::unordered_set<base::EntityId> needing;
        for (const CardReference& use : context.references().propertyMaterials)
        {
            if (!isOneOf(cardNameAt(model, use.location), {"RodProp", "TubeProp"}))
                needing.insert(use.id);
        }
        forEachCard<model::IsoMatTable>(
            model,
            [&](const model::IsoMatCard& card, model::CardLocation where)
            {
                if (!(card.shearModulus > 0.0) && (card.shearModulus < 0.0 || needing.contains(card.id)))
                    sink.reportCard(
                        where, "shearModulus",
                        std::string("shearModulus") + " is " + std::to_string(card.shearModulus) +
                            ": the shear modulus must be positive",
                        "give a positive shear modulus, or leave " + std::string("shearModulus") + " blank");
            });
    }
};

} // namespace

void addIsoMatShearModulusCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<IsoMatShearModulusCheck>());
}

} // namespace oor::analysis
