#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// A THRU range of an SPC1 in which no grid point exists: the constraint fixes nothing.
class SinglePointConstrListEmptyRangeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.single-point-constr-list-empty-range", base::Severity::Warning,
                                    "constraints", "A range of a single-point constraint list contains no grid point."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::vector<base::EntityId>& nodes = context.sortedNodeIds();
        forEachCard<model::SinglePointConstrListTable>(
            context.model(),
            [&](const model::SinglePointConstrListCard& card, model::CardLocation where)
            {
                for (const model::NodeRun& run : card.nodes)
                {
                    if (run.first == run.last)
                        continue;
                    const auto found = std::ranges::lower_bound(nodes, run.first);
                    if (found == nodes.end() || *found > run.last)
                        sink.reportCard(where, "node1",
                                        "no grid point exists from " + std::to_string(run.first) + " THRU " +
                                            std::to_string(run.last),
                                        "check the range");
                }
            });
    }
};

} // namespace

void addSinglePointConstrListEmptyRangeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SinglePointConstrListEmptyRangeCheck>());
}

} // namespace oor::analysis
