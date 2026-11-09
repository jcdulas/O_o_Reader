#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"

namespace oor::analysis
{

namespace
{

// True when a MATT4 of the same MID gives the conductivity K a table (T(K)): the MAT4 may leave K blank then.
bool conductivityHasTable(const model::Model& model, base::EntityId mid)
{
    bool found = false;
    forEachCard<model::HeatIsoMatTemperatureTable>(
        model,
        [&](const model::HeatIsoMatTemperatureCard& card, model::CardLocation)
        {
            if (card.id == mid && card.conductivityTableId != 0)
                found = true;
        });
    return found;
}

// A heat transfer analysis whose conduction property names a MAT4 with no conductivity K (and no MATT4 table for it),
// or a MAT5 whose conductivity matrix has a zero diagonal (format reference: K > 0 for MAT4; a MAT5 needs positive KXX,
// KYY and KZZ for a solid and the ones its element uses for a shell or a rod). The element conducts no heat in that
// direction, and the matrix of the model is singular unless something else holds the temperatures.
class ThermalConductivityMissingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.thermal-conductivity-missing", base::Severity::Warning, "compatibility",
        "A heat transfer analysis has a conduction property whose isotropic heat material has no conductivity or whose "
        "anisotropic heat material has a zero conductivity."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        if (!isHeatTransfer(context.root()))
            return;
        const model::Model& model = context.model();
        for (const CardReference& use : context.references().propertyMaterials)
        {
            if (!isConductionMaterialField(cardNameAt(model, use.location), use.field))
                continue;
            // A material ID may be a MAT1 and a MAT4 at once: heat conduction takes the thermal one.
            const std::optional<model::CardLocation> thermal = model.locate(base::IdSpace::ThermalMaterial, use.id);
            if (!thermal)
                continue;
            if (isType<model::HeatIsoMatTable>(*thermal))
            {
                const model::HeatIsoMatCard card = cardAt<model::HeatIsoMatTable>(model, *thermal);
                if (!card.conductivity && !conductivityHasTable(model, use.id))
                {
                    const std::string material = std::string("HeatIsoMat");
                    const std::string conductivity = std::string("conductivity");
                    sink.reportCard(use.location, use.field,
                                    "uses " + material + " " + std::to_string(use.id) + ", which has no conductivity " +
                                        conductivity,
                                    "give the " + material + " a conductivity " + conductivity + ", or a " +
                                        std::string("HeatIsoMatTemperature") + " table for it");
                }
            }
            else if (isType<model::HeatAnisoMatTable>(*thermal))
            {
                const model::HeatAnisoMatCard card = cardAt<model::HeatAnisoMatTable>(model, *thermal);
                if (card.conductivityXx == 0.0 && card.conductivityYy == 0.0 && card.conductivityZz == 0.0)
                    sink.reportCard(use.location, use.field,
                                    "uses " + std::string("HeatAnisoMat") + " " + std::to_string(use.id) +
                                        ", whose conductivities " + std::string("conductivityXx") + ", " +
                                        std::string("conductivityYy") + " and " + std::string("conductivityZz") +
                                        " are zero",
                                    "give the " + std::string("HeatAnisoMat") + " a conductivity matrix");
            }
        }
    }
};

} // namespace

void addThermalConductivityMissingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ThermalConductivityMissingCheck>());
}

} // namespace oor::analysis
