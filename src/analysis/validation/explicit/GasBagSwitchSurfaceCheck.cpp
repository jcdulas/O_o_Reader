#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/eulerLagrangeCoupling/EulerLagrangeCouplingTable.hpp"
#include "oor/model/cards/explicit/gasBag/GasBagTable.hpp"
#include "oor/model/cards/explicit/gasBagSwitch/GasBagSwitchTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace oor::analysis
{

namespace
{

// A GBAGCOU whose COUPLE and GBAG do not have the same BSID. The switch moves one air bag from the gas dynamics model
// (the COUPLE) to the uniform pressure model (the GBAG), so the surface that each of them encloses must be the same
// one. A GBAGCOU with no GBAG, or whose COUPLE or GBAG does not exist
// (`ref.unresolved` says so), has nothing to compare.
class GasBagSwitchSurfaceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.gas-bag-switch-surface", base::Severity::Error, "explicit",
                                    "The coupling and the gas bag of a gas bag switch do not have the same surface."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::GasBagSwitchTable>(
            model,
            [&](const model::GasBagSwitchCard& card, model::CardLocation where)
            {
                if (card.gasBagId == 0)
                    return;
                const std::vector<model::CardLocation> couples =
                    model::cardsOfSet(model, model::CardCategory::Coupling, card.couplingId);
                const std::vector<model::CardLocation> bags =
                    model::cardsOfSet(model, model::CardCategory::GasBag, card.gasBagId);
                const auto couple = std::find_if(couples.begin(), couples.end(), [](const model::CardLocation& location)
                                                 { return isType<model::EulerLagrangeCouplingTable>(location); });
                const auto bag = std::find_if(bags.begin(), bags.end(), [](const model::CardLocation& location)
                                              { return isType<model::GasBagTable>(location); });
                if (couple == couples.end() || bag == bags.end())
                    return;
                const base::EntityId couplingSurface =
                    cardAt<model::EulerLagrangeCouplingTable>(model, *couple).surfaceId;
                const base::EntityId bagSurface = cardAt<model::GasBagTable>(model, *bag).surfaceId;
                if (couplingSurface == bagSurface)
                    return;
                const std::string coupleCard = std::string("EulerLagrangeCoupling");
                const std::string bagCard = std::string("GasBag");
                const std::string coupleSurfaceField = std::string("surfaceId");
                const std::string bagSurfaceField = std::string("surfaceId");
                sink.reportCard(where, "gasBagId",
                                "the " + coupleCard + " " + std::to_string(card.couplingId) + " has the " +
                                    coupleSurfaceField + " " + std::to_string(couplingSurface) + " and the " + bagCard +
                                    " " + std::to_string(card.gasBagId) + " the " + bagSurfaceField + " " +
                                    std::to_string(bagSurface),
                                "give the " + coupleCard + " and the " + bagCard + " the same " + coupleSurfaceField +
                                    ", the surface of the air bag");
            });
    }
};

} // namespace

void addGasBagSwitchSurfaceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<GasBagSwitchSurfaceCheck>());
}

} // namespace oor::analysis
