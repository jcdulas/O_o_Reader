#include "ConstraintDofs.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"

#include <map>

namespace oor::analysis
{

namespace
{

// Two SPC cards of one set that fix the same degree of freedom at different values of the enforced displacement.
class SpcConflictCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.spc-conflicting-values", base::Severity::Warning, "constraints",
                                    "Two single-point constraint cards of a set fix one degree of freedom at different "
                                    "enforced values."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        struct Fixed
        {
            double value;
            model::CardLocation where;
        };
        std::map<std::pair<base::EntityId, std::uint64_t>, Fixed> fixed;
        forEachCard<model::SinglePointConstrTable>(
            context.model(),
            [&](const model::SinglePointConstrCard& card, model::CardLocation where)
            {
                for (std::size_t group = 0; group < (card.hasSecondGroup() ? 2U : 1U); ++group)
                {
                    std::vector<DofUse> dofs;
                    appendDofs(card.nodes[group], card.components[group], where, dofs);
                    for (const DofUse& dof : dofs)
                    {
                        const auto [position, inserted] =
                            fixed.emplace(std::make_pair(card.id, dof.key), Fixed{card.enforced[group], where});
                        if (inserted || position->second.value == card.enforced[group] ||
                            position->second.where == where)
                            continue;
                        sink.reportCard(where, "enforcedValue" + std::to_string(group + 1),
                                        "degree of freedom " + std::to_string(componentOfDof(dof.key)) +
                                            " of grid point " + std::to_string(nodeOfDof(dof.key)) +
                                            " is already fixed at " + std::to_string(position->second.value) +
                                            " in set " + std::to_string(card.id),
                                        "give the degree of freedom one value");
                    }
                }
            });
    }
};

} // namespace

void addSpcConflictCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SpcConflictCheck>());
}

} // namespace oor::analysis
