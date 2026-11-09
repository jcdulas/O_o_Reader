#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"

#include <map>
#include <set>

namespace oor::analysis
{

namespace
{

// LOAD combinations that contain one another in a circle (LOAD 1 uses set 2, LOAD 2 uses set 1). A LOAD that names its
// own ID is refused by the card.
class LoadCombinationCycleCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.combination-cycle", base::Severity::Error, "loads",
                                    "Load combinations contain one another in a cycle."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::map<base::EntityId, std::set<base::EntityId>> members;
        forEachCard<model::LoadCombinationTable>(context.model(),
                                                 [&](const model::LoadCombinationCard& card, model::CardLocation)
                                                 {
                                                     for (const model::LoadCombinationTerm& term : card.terms)
                                                         members[card.id].insert(term.loadCombinationId);
                                                 });
        forEachCard<model::LoadCombinationTable>(
            context.model(),
            [&](const model::LoadCombinationCard& card, model::CardLocation where)
            {
                if (reachesItself(members, card.id))
                    sink.reportCard(
                        where, "loadSetId1",
                        "load set " + std::to_string(card.id) + " contains itself through the sets it combines",
                        "remove the term that closes the circle");
            });
    }

private:
    static bool reachesItself(const std::map<base::EntityId, std::set<base::EntityId>>& members, base::EntityId start)
    {
        std::set<base::EntityId> seen;
        std::vector<base::EntityId> pending(members.at(start).begin(), members.at(start).end());
        while (!pending.empty())
        {
            const base::EntityId next = pending.back();
            pending.pop_back();
            if (next == start)
                return true;
            const auto found = members.find(next);
            if (found == members.end() || !seen.insert(next).second)
                continue;
            pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
        return false;
    }
};

} // namespace

void addLoadCombinationCycleCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LoadCombinationCycleCheck>());
}

} // namespace oor::analysis
