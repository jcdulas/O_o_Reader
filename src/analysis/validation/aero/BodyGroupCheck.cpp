#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// All the CAERO1 and CAERO2 cards of one group (IGID) interact, and elements of different groups are uncoupled (format
// reference): a body whose group no panel has interacts with nothing. Information, not a fault (a
// model of a body alone is legal); a model with no panel is not judged.
class BodyGroupCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.body-group", base::Severity::Info, "aero",
                                    "A slender body is in an interference group that no lifting panel has."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::unordered_set<base::EntityId> panelGroups;
        forEachCard<model::AeroLiftingPanel1Table>(model,
                                                   [&](const model::AeroLiftingPanel1Card& card, model::CardLocation)
                                                   { panelGroups.insert(card.interferenceGroupId); });
        if (panelGroups.empty())
            return;
        forEachCard<model::AeroBody2Table>(
            model,
            [&](const model::AeroBody2Card& card, model::CardLocation where)
            {
                if (!panelGroups.contains(card.interferenceGroupId))
                {
                    sink.reportCard(
                        where, "interferenceGroupId",
                        "no " + std::string("AeroLiftingPanel1") + " panel is in the group " +
                            std::to_string(card.interferenceGroupId) + ": the body interacts with nothing",
                        "give the body the " + std::string("interferenceGroupId") + " of the panels it interacts with");
                }
            });
    }
};

} // namespace

void addBodyGroupCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BodyGroupCheck>());
}

} // namespace oor::analysis
