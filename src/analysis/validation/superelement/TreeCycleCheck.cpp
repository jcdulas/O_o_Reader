#include "SuperelementTreeEdges.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

#include <algorithm>
#include <map>

namespace oor::analysis
{

namespace
{

// A loop in the superelement tree: a superelement that is upstream of itself through the SETREE cards (3 is upstream of
// 4 and 4 of 3). The tree gives the order of the reduction, which a loop leaves without a first superelement. A
// superelement that two entries put upstream is reported by superelement.tree-upstream-twice: the first of its entries
// is the one followed here.
class TreeCycleCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.tree-cycle", base::Severity::Error, "superelement",
                                    "The superelement tree cards make a superelement upstream of itself."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::map<base::EntityId, SuperelementTreeEdge> downstreamOf;
        for (const SuperelementTreeEdge& edge : superelementTreeEdges(context.model()))
            downstreamOf.emplace(edge.upstream, edge);

        // True once a superelement is known to lead out of the tree or to be in a loop that was reported.
        std::map<base::EntityId, bool> settled;
        for (const auto& [start, firstEdge] : downstreamOf)
        {
            if (settled[start])
                continue;
            std::vector<base::EntityId> path;
            base::EntityId at = start;
            while (true)
            {
                if (settled[at])
                    break;
                const auto inPath = std::find(path.begin(), path.end(), at);
                if (inPath != path.end())
                {
                    reportLoop(sink, downstreamOf, std::vector<base::EntityId>(inPath, path.end()));
                    break;
                }
                path.push_back(at);
                const auto next = downstreamOf.find(at);
                if (next == downstreamOf.end())
                    break;
                at = next->second.downstream;
            }
            for (const base::EntityId member : path)
                settled[member] = true;
        }
    }

private:
    // The finding is on the entry that leaves the smallest superelement of the loop.
    static void reportLoop(FindingSink& sink, const std::map<base::EntityId, SuperelementTreeEdge>& downstreamOf,
                           std::vector<base::EntityId> loop)
    {
        std::sort(loop.begin(), loop.end());
        std::string members;
        for (const base::EntityId member : loop)
            members += (members.empty() ? "" : ", ") + std::to_string(member);
        const std::string what = loop.size() == 1 ? "superelement " + members + " is upstream of itself"
                                                  : "superelements " + members + " are upstream of one another";
        sink.reportCard(downstreamOf.at(loop.front()).where, "upstreamSuperelementId", what,
                        "break the loop: the tree has to lead to the residual structure, 0");
    }
};

} // namespace

void addTreeCycleCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TreeCycleCheck>());
}

} // namespace oor::analysis
