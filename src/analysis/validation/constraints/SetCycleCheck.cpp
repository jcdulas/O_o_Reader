#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"

#include <map>
#include <set>

namespace oor::analysis
{

namespace
{

// SPCADD or MPCADD sets that combine one another in a circle (a set that contains itself through other sets).
class SetCycleCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.set-cycle", base::Severity::Error, "constraints",
                                    "Single-point or multipoint constraint unions contain one another in a cycle."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        scan<model::SinglePointConstrUnionTable>(context, sink);
        scan<model::MultiPointConstrUnionTable>(context, sink);
    }

private:
    // True when a set reaches itself.
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

    template <typename AddTable>
    static void scan(const ValidationContext& context, FindingSink& sink)
    {
        std::map<base::EntityId, std::set<base::EntityId>> members;
        forEachCard<AddTable>(context.model(), [&](const typename AddTable::CardType& card, model::CardLocation)
                              { members[card.id].insert(card.sets.begin(), card.sets.end()); });
        forEachCard<AddTable>(
            context.model(),
            [&](const typename AddTable::CardType& card, model::CardLocation where)
            {
                if (reachesItself(members, card.id))
                    sink.reportCard(where, "memberSetId1",
                                    "set " + std::to_string(card.id) + " contains itself through the sets it combines",
                                    "remove the set that closes the circle");
            });
    }
};

} // namespace

void addSetCycleCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SetCycleCheck>());
}

} // namespace oor::analysis
