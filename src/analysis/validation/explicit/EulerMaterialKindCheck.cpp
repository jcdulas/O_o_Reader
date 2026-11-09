#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"
#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A PEULER whose MID names a material that is not a MATDEUL. The format of PEULER says the MID is the number of a
// MATDEUL, the constitutive model of an Eulerian material; the material space is shared by every material card, so a
// MAT1 with that number resolves as a reference and only the kind of card tells the mistake.
class EulerMaterialKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.euler-material-kind", base::Severity::Error, "explicit",
                                    "The material of an Euler property is not an Euler material."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::EulerPropTable>(
            model,
            [&](const model::EulerPropCard& card, model::CardLocation where)
            {
                if (card.matId == 0) // void elements
                    return;
                const std::optional<model::CardLocation> material = model.locate(base::IdSpace::Material, card.matId);
                if (!material || isType<model::EulerMatTable>(*material))
                    return;
                sink.reportCard(
                    where, "matId",
                    "the material " + std::to_string(card.matId) + " is a " +
                        std::string(cardNameAt(model, *material)) + ", not a " + std::string("EulerMat"),
                    "name a " + std::string("EulerMat") + ": the Eulerian elements need an equation of state");
            });
    }
};

} // namespace

void addEulerMaterialKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EulerMaterialKindCheck>());
}

} // namespace oor::analysis
