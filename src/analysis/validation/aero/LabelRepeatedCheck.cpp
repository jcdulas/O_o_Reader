#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"
#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// The labels of the AESURF, AESTAT and AEPARM cards together are the set of trim variables of the model (format
// reference), and a TRIM or an AELINK names one by its label: two cards with one label make the name
// ambiguous. The cards have IDs, but the solver ignores them, so the label is the key.
class LabelRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.label-repeated", base::Severity::Error, "aero",
                                    "A control surface, a rigid trim variable or a controller has the label of another "
                                    "one."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::unordered_map<std::string, std::string> first; // the label, upper case, and the card that has it
        const auto look =
            [&](const std::string& label, std::string_view cardType, base::EntityId id, model::CardLocation where)
        {
            const std::string key = std::string(label);
            const auto [found, inserted] = first.try_emplace(key, std::string(cardType) + " " + std::to_string(id));
            if (!inserted)
            {
                sink.reportCard(where, "label",
                                "the label " + label + " is also the label of " + found->second +
                                    " (labels are compared without regard to the case)",
                                "give each trim variable its own label");
            }
        };
        forEachCard<model::AeroControlSurfaceTable>(
            context.model(), [&](const model::AeroControlSurfaceCard& card, model::CardLocation where)
            { look(card.label, "AeroControlSurface", card.id, where); });
        forEachCard<model::AeroRigidTrimVariableTable>(
            context.model(), [&](const model::AeroRigidTrimVariableCard& card, model::CardLocation where)
            { look(card.label, "AeroRigidTrimVariable", card.id, where); });
        forEachCard<model::AeroControllerTable>(context.model(),
                                                [&](const model::AeroControllerCard& card, model::CardLocation where)
                                                { look(card.label, "AeroController", card.id, where); });
    }
};

} // namespace

void addLabelRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LabelRepeatedCheck>());
}

} // namespace oor::analysis
