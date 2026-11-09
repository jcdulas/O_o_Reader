#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"

namespace oor::analysis
{

namespace
{

// A CONVM (forced convection) whose surface element is not a CHBDYP of type FTUBE (
// a CONVM goes with the FTUBE CHBDYP of the same EID). A surface element that does not exist is the
// business of ref.unresolved.
class ForcedConvectionSurfaceTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.forced-convection-surface-type", base::Severity::Error, "compatibility",
                                    "A forced convection card names a surface element that is not a property-defined "
                                    "heat surface of type FTUBE."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::ForcedConvectionTable>(
            model,
            [&](const model::ForcedConvectionCard& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> surface = model.locate(base::IdSpace::SurfaceElement, card.id);
                if (!surface)
                    return;
                const std::string tube = std::string("HeatSurfaceProperty");
                const std::string typeField = std::string("kind");
                if (!isType<model::HeatSurfacePropertyTable>(*surface))
                {
                    sink.reportCard(where, "id",
                                    "surface element " + std::to_string(card.id) + " is a " +
                                        std::string(cardNameAt(model, *surface)) + ": forced convection needs a " +
                                        tube,
                                    "define the surface as a " + tube + " of " + typeField +
                                        " ForcedConvectionTube with the same " + std::string("id"));
                    return;
                }
                const std::string type = cardAt<model::HeatSurfacePropertyTable>(model, *surface).kind;
                if (type != "ForcedConvectionTube")
                    sink.reportCard(where, "id",
                                    "the " + tube + " " + std::to_string(card.id) + " is of " + typeField + " " + type +
                                        ": forced convection needs the " + typeField + " ForcedConvectionTube",
                                    "give the " + tube + " the " + typeField + " ForcedConvectionTube");
            });
    }
};

} // namespace

void addForcedConvectionSurfaceTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ForcedConvectionSurfaceTypeCheck>());
}

} // namespace oor::analysis
