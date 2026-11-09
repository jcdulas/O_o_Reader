#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"

namespace oor::analysis
{

namespace
{

// the REGION groups responses for the screening of constraints, and a response of a global type (the
// weight, the volume, the frequencies and eigenvalues, the total strain energy, the RMS of a displacement) is a single
// quantity with nothing to screen: REGION is to be left blank. The optimiser ignores it,
// which is why it is a warning.
class ResponseRegionCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.response-region", base::Severity::Warning, "design",
                                    "A basic design response of a global response type has a region."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::DesignResp1Table>(
            context.model(),
            [&](const model::DesignResp1Card& card, model::CardLocation where)
            {
                if (card.regionId != 0 &&
                    isOneOf(card.responseKind, {"Weight", "Volume", "BucklingFactor", "Eigenvalue", "Frequency",
                                                "ComplexEigenvalue", "TotalStrainEnergy", "RootMeanSquareDisplacement",
                                                "RootMeanSquareVelocity", "RootMeanSquareAcceleration"}))
                    sink.reportCard(
                        where, "regionId",
                        "a response of type " + card.responseKind + " has the region " + std::to_string(card.regionId),
                        "leave " + std::string("regionId") + " blank for this response type");
            });
    }
};

} // namespace

void addResponseRegionCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ResponseRegionCheck>());
}

} // namespace oor::analysis
