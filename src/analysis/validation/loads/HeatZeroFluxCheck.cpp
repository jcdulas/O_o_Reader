#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadTable.hpp"
#include "oor/model/cards/thermal/heatFluxSurfaceLoad1/HeatFluxSurfaceLoad1Table.hpp"
#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadTable.hpp"
#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadTable.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

// A heat load whose flux is zero: it is in the deck and heats nothing, which is usually a blank field that was meant to
// be filled. QVECT may leave Q0 blank (the flux then comes from the tables), which is not a zero.
class HeatZeroFluxCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.heat-zero-flux", base::Severity::Warning, "loads",
                                    "A heat flux or volume heat source load has a heat flux of zero."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const auto zero = [&](model::CardLocation where, const char* field, std::string why)
        { sink.reportCard(where, field, std::move(why), "give the load a value, or remove the card"); };
        const auto zeroFlux = [&](std::string_view) { return "the heat flux " + std::string("heatFlux") + " is zero"; };
        forEachCard<model::HeatFluxSurfaceLoad1Table>(
            model,
            [&](const model::HeatFluxSurfaceLoad1Card& card, model::CardLocation where)
            {
                if (card.heatFlux == 0.0)
                    zero(where, "heatFlux", zeroFlux("HeatFluxSurfaceLoad1"));
            });
        forEachCard<model::HeatFluxNodalLoad2Table>(
            model,
            [&](const model::HeatFluxNodalLoad2Card& card, model::CardLocation where)
            {
                if (std::ranges::all_of(card.heatFlux, [](double flux) { return flux == 0.0; }))
                    zero(
                        where, "heatFlux1",
                        "every heat flux " + std::string("heatFlux1") + " to " + std::string("heatFlux8") + " is zero");
            });
        forEachCard<model::HeatFluxBoundaryLoad3Table>(
            model,
            [&](const model::HeatFluxBoundaryLoad3Card& card, model::CardLocation where)
            {
                if (card.heatFlux == 0.0)
                    zero(where, "heatFlux", zeroFlux("HeatFluxBoundaryLoad3"));
            });
        forEachCard<model::HeatFluxNodeSetLoadTable>(
            model,
            [&](const model::HeatFluxNodeSetLoadCard& card, model::CardLocation where)
            {
                if (card.heatFlux == 0.0)
                    zero(where, "heatFlux", zeroFlux("HeatFluxNodeSetLoad"));
            });
        forEachCard<model::HeatFluxVectorLoadTable>(
            model,
            [&](const model::HeatFluxVectorLoadCard& card, model::CardLocation where)
            {
                if (card.heatFlux && *card.heatFlux == 0.0)
                    zero(where, "heatFlux", zeroFlux("HeatFluxVectorLoad"));
            });
        forEachCard<model::HeatVolumeSourceLoadTable>(
            model,
            [&](const model::HeatVolumeSourceLoadCard& card, model::CardLocation where)
            {
                if (card.heatGeneration == 0.0)
                    zero(where, "heatGeneration",
                         "the volumetric heat generation " + std::string("heatGeneration") + " is zero");
            });
    }
};

} // namespace

void addHeatZeroFluxCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<HeatZeroFluxCheck>());
}

} // namespace oor::analysis
