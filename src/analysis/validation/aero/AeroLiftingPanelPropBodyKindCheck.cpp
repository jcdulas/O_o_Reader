#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The bodies B1 to B6 of a PAERO1 are CAERO2 cards (the Bi numbers must appear on a
// CAERO2 entry). The five CAERO cards share one number space, so a number that names a panel of another kind resolves
// and the PAERO1 reads it as a body. A number that names nothing is `ref.unresolved`.
class AeroLiftingPanelPropBodyKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.aero-lifting-panel-prop1-body-kind", base::Severity::Error, "aero",
                                    "A body of a lifting panel property is an aerodynamic panel of another kind than a "
                                    "slender body."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroLiftingPanelProp1Table>(
            model,
            [&](const model::AeroLiftingPanelProp1Card& card, model::CardLocation where)
            {
                for (std::size_t position = 0; position < card.bodies.size(); ++position)
                {
                    if (card.bodies[position] == 0)
                        continue;
                    const std::optional<model::CardLocation> body =
                        model.locate(base::IdSpace::AeroElement, card.bodies[position]);
                    if (!body || cardNameAt(model, *body) == "AeroBody2")
                        continue;
                    sink.reportCard(where, "body" + std::to_string(position + 1),
                                    "the body " + std::to_string(card.bodies[position]) + " is a " +
                                        std::string(cardNameAt(model, *body)) + ", not a " + std::string("AeroBody2"),
                                    "name a " + std::string("AeroBody2") + " card");
                }
            });
    }
};

} // namespace

void addAeroLiftingPanelPropBodyKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AeroLiftingPanelPropBodyKindCheck>());
}

} // namespace oor::analysis
