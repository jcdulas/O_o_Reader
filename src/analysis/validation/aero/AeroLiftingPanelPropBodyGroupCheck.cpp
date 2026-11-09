#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The bodies a PAERO1 lists must be in the same aerodynamic group as the panels ("in
// the same aerodynamic group, as specified in the IGID field on the CAERO2 entry"); elements of different groups are
// uncoupled (CAERO1 and CAERO2, IGID), so a body of another group than its panel does not interact with it. A warning:
// the run goes on, with an aerodynamic model that is not what the PAERO1 says.
class AeroLiftingPanelPropBodyGroupCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.aero-lifting-panel-prop1-body-group", base::Severity::Warning, "aero",
                                    "A body of a lifting panel property is in another interference group than the "
                                    "lifting panel."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const auto* paeros = model.findTable<model::AeroLiftingPanelProp1Table>();
        if (paeros == nullptr)
            return;
        forEachCard<model::AeroLiftingPanel1Table>(
            model,
            [&](const model::AeroLiftingPanel1Card& panel, model::CardLocation where)
            {
                const std::optional<model::CardLocation> property =
                    model.locate(base::IdSpace::AeroProperty, panel.propId);
                if (!property || !isType<model::AeroLiftingPanelProp1Table>(*property))
                    return;
                for (const base::EntityId id : paeros->card(property->row).bodies)
                {
                    const std::optional<model::CardLocation> body =
                        id != 0 ? model.locate(base::IdSpace::AeroElement, id) : std::nullopt;
                    // a number that is no CAERO2 is reported by `aero.aero-lifting-panel-prop1-body-kind`, or by the
                    // references
                    if (!body || !isType<model::AeroBody2Table>(*body))
                        continue;
                    const model::AeroBody2Card card = cardAt<model::AeroBody2Table>(model, *body);
                    if (card.interferenceGroupId != panel.interferenceGroupId)
                    {
                        sink.reportCard(where, "interferenceGroupId",
                                        "the body " + std::to_string(id) + " of the " +
                                            std::string("AeroLiftingPanelProp1") + " " + std::to_string(panel.propId) +
                                            " is in the group " + std::to_string(card.interferenceGroupId) +
                                            " and this panel in the group " + std::to_string(panel.interferenceGroupId),
                                        "give the body and the panel the same " + std::string("interferenceGroupId"));
                        return;
                    }
                }
            });
    }
};

} // namespace

void addAeroLiftingPanelPropBodyGroupCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AeroLiftingPanelPropBodyGroupCheck>());
}

} // namespace oor::analysis
