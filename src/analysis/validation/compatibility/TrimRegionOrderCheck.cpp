#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/thermal/trimComponentCoupling/TrimComponentCouplingTable.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// An ACPEMCP of a region of a trim component (RID not blank) that comes before the ACPEMCP of the component with a
// blank RID, or that has none to come after: the format reference says a trim component may have
// many ACPEMCP cards but the one with a blank RID is a must, and it comes before the ones that give a region. The cards
// are looked at in the order of the deck.
class TrimRegionOrderCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.trim-region-order", base::Severity::Error, "compatibility",
        "A trim component coupling card of a region is not preceded by the coupling card of its trim component that "
        "gives no region."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::unordered_set<base::EntityId> opened; // the trim components that have their card with a blank RID
        forEachCard<model::TrimComponentCouplingTable>(
            context.model(),
            [&](const model::TrimComponentCouplingCard& card, model::CardLocation where)
            {
                if (card.regionId == 0)
                {
                    opened.insert(card.id);
                    return;
                }
                if (opened.contains(card.id))
                    return;
                sink.reportCard(where, "regionId",
                                "the trim component " + std::to_string(card.id) + " has an " +
                                    std::string("TrimComponentCoupling") + " for the region " +
                                    std::to_string(card.regionId) + " but no " + std::string("TrimComponentCoupling") +
                                    " with a blank " + std::string("regionId") + " before it",
                                "put the " + std::string("TrimComponentCoupling") +
                                    " of the component that has a blank " + std::string("regionId") + " first");
            });
    }
};

} // namespace

void addTrimRegionOrderCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TrimRegionOrderCheck>());
}

} // namespace oor::analysis
