#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <array>
#include <set>
#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

// The cards that the format reference allows only when a flag card is in the deck, and that flag card (an AXIC,
// an AXIF or an AXSLOT).
struct FlagRule
{
    std::string_view card;
    std::string_view flag;
};

constexpr std::array<FlagRule, 24> RULES{{
    // The conical shell problem: AxisymConeControl.
    {"AxisymConeRing", "AxisymConeControl"},
    {"AxisymConePoint", "AxisymConeControl"},
    {"AxisymConeSector", "AxisymConeControl"},
    {"AxisymRingMomentLoad", "AxisymConeControl"},
    {"AxisymConePressureLoad", "AxisymConeControl"},
    {"AxisymOmittedDofSet", "AxisymConeControl"},
    {"MultiPointConstrAxisym", "AxisymConeControl"},
    {"SinglePointConstrAxisym", "AxisymConeControl"},
    {"AxisymRingForceLoad", "AxisymConeControl"},
    {"AxisymConeTemperatureLoad", "AxisymConeControl"},
    {"AxisymConeSupport", "AxisymConeControl"},
    {"ShellConical2", "AxisymConeControl"},
    {"ConicalShellProp", "AxisymConeControl"},
    // The axisymmetric fluid: AxisymFluidControl.
    {"AxisymFluidRing", "AxisymFluidControl"},
    {"AxisymFluidFreePoint", "AxisymFluidControl"},
    {"AxisymFluidPressurePoint", "AxisymFluidControl"},
    {"AxisymFluidRingNode", "AxisymFluidControl"},
    {"AxisymFluidFreeSurface", "AxisymFluidControl"},
    {"AxisymFluidBoundaryList", "AxisymFluidControl"},
    // The slotted acoustic cavity: AcousticSlotControl.
    {"AcousticSlotFluidPoint", "AcousticSlotControl"},
    {"AcousticSlotSurfacePoint", "AcousticSlotControl"},
    {"AcousticSlotBoundary", "AcousticSlotControl"},
    {"AcousticSlot3", "AcousticSlotControl"},
    {"AcousticSlot4", "AcousticSlotControl"},
}};

// A card of the axisymmetric problems with no AXIC, AXIF or AXSLOT card in the deck: the format reference says each of
// them is allowed only when the flag card is present, so the model is not valid without it.
class AxisymmetricFlagCardCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.axisymmetric-flag-card", base::Severity::Error, "compatibility",
        "A conical shell, axisymmetric fluid or slot card is in a model that has no conical shell, axisymmetric fluid "
        "or acoustic slot control card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::CardRegistry& registry = model::CardRegistry::instance();
        // The flag card may be in the main Bulk Data while the card is in a part: the names of the whole deck count.
        std::set<std::string_view> present;
        for (const model::Model* each : context.allModels())
        {
            for (std::uint16_t type = 1; type < registry.size(); ++type)
            {
                const model::CardTable* table = each->findTable(type);
                if (table != nullptr && table->rows().aliveCount() > 0)
                    present.insert(registry.entry(type).info.name);
            }
        }
        for (const FlagRule& rule : RULES)
        {
            if (!present.contains(rule.card) || present.contains(rule.flag))
                continue;
            for (std::uint16_t type = 1; type < registry.size(); ++type)
            {
                if (registry.entry(type).info.name != rule.card)
                    continue;
                const model::CardTable* table = model.findTable(type);
                for (std::uint32_t row = 0; table != nullptr && row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row))
                        continue;
                    const std::string flag = std::string(rule.flag);
                    sink.reportCard(model::CardLocation{type, row}, "",
                                    std::string(rule.card) + " is allowed only when the model has an " + flag +
                                        " card, and it has none",
                                    "add the " + flag + " card");
                }
            }
        }
    }
};

} // namespace

void addAxisymmetricFlagCardCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AxisymmetricFlagCardCheck>());
}

} // namespace oor::analysis
