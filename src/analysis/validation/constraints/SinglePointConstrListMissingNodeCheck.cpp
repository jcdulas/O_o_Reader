#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/references/PartScopes.hpp"

#include <algorithm>
#include <vector>

namespace oor::analysis
{

namespace
{

// A grid point named by itself in an SPC1 that no card defines. The ends of a THRU range are allowed to be missing (the
// gaps of a range are skipped), so ref.unresolved does not report them; a single grid point is not a range.
class SinglePointConstrListMissingNodeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.single-point-constr-list-missing-node", base::Severity::Error,
                                    "constraints",
                                    "A single-point constraint list names a grid point, not as part of a range, that "
                                    "does not exist."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // A part may name the grid points of the models it shares its IDs with (the main Bulk Data of a TPA deck those
        // of its passive sub-model, a mass increment those of its base model: PartScopes).
        const std::vector<const model::Model*> shared = model::PartScopes::sharedModels(model);
        const auto exists = [&](base::EntityId id)
        {
            return model.locate(base::IdSpace::Node, id).has_value() ||
                   std::any_of(shared.begin(), shared.end(), [&](const model::Model* other)
                               { return other->locate(base::IdSpace::Node, id).has_value(); });
        };
        forEachCard<model::SinglePointConstrListTable>(
            model,
            [&](const model::SinglePointConstrListCard& card, model::CardLocation where)
            {
                for (const model::NodeRun& run : card.nodes)
                {
                    if (run.first == run.last && !exists(run.first))
                        sink.reportCard(where, "node1", "grid point " + std::to_string(run.first) + " does not exist",
                                        "define the grid point or remove it from the list");
                }
            });
    }
};

} // namespace

void addSinglePointConstrListMissingNodeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SinglePointConstrListMissingNodeCheck>());
}

} // namespace oor::analysis
