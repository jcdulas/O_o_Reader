#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroFlutter/AeroFlutterTable.hpp"
#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsTable.hpp"

#include <optional>
#include <string>

namespace oor::analysis
{

namespace
{

// The no-looping PK methods (PKNL and PKNLS) analyse ordered triples (density, Mach number, velocity) instead of every
// combination, so the three FLFACT lists must be equally long. The other methods
// take all combinations of lists of any length. A list that does not exist is `ref.unresolved`.
class FlutterFactorCountCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.flutter-factor-count", base::Severity::Error, "aero",
                                    "A flutter card of the PKNL or PKNLS method has factor lists of different "
                                    "lengths."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const auto* factors = model.findTable<model::AeroFlutterFactorsTable>();
        if (factors == nullptr)
            return;
        const auto countOf = [&](base::EntityId id) -> std::optional<std::size_t>
        {
            const std::optional<model::CardLocation> found =
                model.locateInType(model::typeIndexOf<model::AeroFlutterFactorsTable>(), id);
            if (!found)
                return std::nullopt;
            const model::AeroFlutterFactorsCard card = factors->card(found->row);
            return card.isRange ? static_cast<std::size_t>(card.factorCount) : card.factors.size();
        };
        forEachCard<model::AeroFlutterTable>(
            model,
            [&](const model::AeroFlutterCard& card, model::CardLocation where)
            {
                if (card.method != model::AeroFlutterMethod::Pknl && card.method != model::AeroFlutterMethod::Pknls)
                    return;
                const std::optional<std::size_t> densities = countOf(card.densityFactorsId);
                const std::optional<std::size_t> machs = countOf(card.machFactorsId);
                const std::optional<std::size_t> velocities = countOf(card.reducedFreqFactorsId);
                if (!densities || !machs || !velocities)
                    return;
                if (*densities != *machs || *densities != *velocities)
                {
                    sink.reportCard(
                        where, "machFactorsId",
                        "the lists have " + std::to_string(*densities) + " densities, " + std::to_string(*machs) +
                            " Mach numbers and " + std::to_string(*velocities) + " velocities",
                        "give the three " + std::string("AeroFlutterFactors") + " lists the same number of values");
                }
            });
    }
};

} // namespace

void addFlutterFactorCountCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FlutterFactorCountCheck>());
}

} // namespace oor::analysis
