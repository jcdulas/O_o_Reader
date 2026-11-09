#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"

#include <map>
#include <set>
#include <vector>

namespace oor::analysis
{

namespace
{

// a DRESP2 may use other DRESP2 responses (the "DRESP2" line), but not itself, directly or through the
// ones it uses. Every DRESP2 on such a circle is reported, once.
class ResponseCycleCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.response-cycle", base::Severity::Error, "design",
                                    "Design responses defined by equations use one another in a cycle."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::map<base::EntityId, std::set<base::EntityId>> uses;
        forEachCard<model::DesignResp2Table>(
            context.model(),
            [&](const model::DesignResp2Card& card, model::CardLocation)
            {
                if (const model::DesignGroup* group = model::findDesignGroup(card.groups, "DesignResp2"))
                    uses[card.id].insert(group->ids.begin(), group->ids.end());
            });
        if (uses.empty())
            return;
        forEachCard<model::DesignResp2Table>(context.model(),
                                             [&](const model::DesignResp2Card& card, model::CardLocation where)
                                             {
                                                 if (reachesItself(uses, card.id))
                                                     sink.reportCard(where, "responseIds",
                                                                     std::string("DesignResp2") + " " +
                                                                         std::to_string(card.id) +
                                                                         " uses itself through the responses it uses",
                                                                     "remove the response that closes the circle");
                                             });
    }

private:
    static bool reachesItself(const std::map<base::EntityId, std::set<base::EntityId>>& uses, base::EntityId start)
    {
        const auto first = uses.find(start);
        if (first == uses.end())
            return false;
        std::set<base::EntityId> seen;
        std::vector<base::EntityId> pending(first->second.begin(), first->second.end());
        while (!pending.empty())
        {
            const base::EntityId next = pending.back();
            pending.pop_back();
            if (next == start)
                return true;
            const auto found = uses.find(next);
            if (found == uses.end() || !seen.insert(next).second)
                continue;
            pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
        return false;
    }
};

} // namespace

void addResponseCycleCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ResponseCycleCheck>());
}

} // namespace oor::analysis
