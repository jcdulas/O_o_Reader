#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// Each CAEROi names a property of its own kind: the CAERO1 a PAERO1, the CAERO2 a PAERO2, and so on (format reference,
// the PID fields). The five PAERO cards share one number space, so a PID that names a PAERO of another kind resolves,
// and the card reads it as if it were the right one.
class PanelPropertyCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.panel-property", base::Severity::Error, "aero",
                                    "An aerodynamic panel names an aerodynamic property of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroLiftingPanel1Table>(
            model, [&](const model::AeroLiftingPanel1Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroLiftingPanel1", "AeroLiftingPanelProp1", card.propId); });
        forEachCard<model::AeroBody2Table>(model, [&](const model::AeroBody2Card& card, model::CardLocation where)
                                           { look(model, sink, where, "AeroBody2", "AeroBodyProp2", card.propId); });
        forEachCard<model::AeroMachBoxPanel3Table>(
            model, [&](const model::AeroMachBoxPanel3Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroMachBoxPanel3", "AeroMachBoxProp3", card.propId); });
        forEachCard<model::AeroStripPanel4Table>(
            model, [&](const model::AeroStripPanel4Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroStripPanel4", "AeroStripProp4", card.propId); });
        forEachCard<model::AeroPistonPanel5Table>(
            model, [&](const model::AeroPistonPanel5Card& card, model::CardLocation where)
            { look(model, sink, where, "AeroPistonPanel5", "AeroPistonProp5", card.propId); });
    }

private:
    // A property that does not exist is `ref.unresolved`; one of the wrong kind is reported here. `panel` and
    // `expected` are card types; the message names them by their keywords.
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, std::string_view panel,
                     std::string_view expected, base::EntityId pid)
    {
        const std::optional<model::CardLocation> property = model.locate(base::IdSpace::AeroProperty, pid);
        if (!property)
            return;
        const std::string_view actual = cardNameAt(model, *property);
        if (actual != expected)
        {
            sink.reportCard(where, "propId",
                            "the property " + std::to_string(pid) + " is a " + std::string(actual) + ", and a " +
                                std::string(panel) + " names a " + std::string(expected),
                            "name a " + std::string(expected) + " card");
        }
    }
};

} // namespace

void addPanelPropertyCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PanelPropertyCheck>());
}

} // namespace oor::analysis
