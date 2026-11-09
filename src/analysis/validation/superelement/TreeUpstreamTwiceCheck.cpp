#include "SuperelementTreeEdges.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

#include <map>

namespace oor::analysis
{

namespace
{

// A superelement that two SETREE entries put upstream: it is reduced into one superelement only, so it appears once in
// the SEUPi of all the SETREE cards. The finding is on the second entry.
class TreeUpstreamTwiceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.tree-upstream-twice", base::Severity::Error, "superelement",
                                    "A superelement is upstream of more than one entry of the superelement tree "
                                    "cards."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::map<base::EntityId, base::EntityId> firstDownstream;
        for (const SuperelementTreeEdge& edge : superelementTreeEdges(context.model()))
        {
            const auto [found, isNew] = firstDownstream.emplace(edge.upstream, edge.downstream);
            if (!isNew)
                sink.reportCard(edge.where, "upstreamSuperelementId",
                                "superelement " + std::to_string(edge.upstream) + " is upstream of " +
                                    std::to_string(found->second) + " and again of " + std::to_string(edge.downstream),
                                "name each superelement once in the " + std::string("SuperelementTree") + " cards");
        }
    }
};

} // namespace

void addTreeUpstreamTwiceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TreeUpstreamTwiceCheck>());
}

} // namespace oor::analysis
