#include "ConstraintDofs.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

#include <set>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// A degree of freedom that is the dependent term of an MPC equation and is also fixed by an SPC, SPC1 or the PS field
// of its grid point. The two commands are taken together for each subcase.
class SpcOnMpcDependentCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.spc-on-mpc-dependent", base::Severity::Error, "constraints",
                                    "A degree of freedom is constrained (single-point constraint or permanent "
                                    "constraint of a grid point) and is the dependent term of a multipoint "
                                    "constraint."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::vector<std::vector<base::EntityId>> spcSets =
            constraintSelections(model, model::SINGLE_POINT_CONSTR_SELECTION);
        const std::vector<std::vector<base::EntityId>> mpcSets =
            constraintSelections(model, model::MULTI_POINT_CONSTR_SELECTION);
        std::set<std::pair<std::uint32_t, std::uint32_t>> reported;
        for (const std::vector<base::EntityId>& mpc : mpcSets)
        {
            std::unordered_set<std::uint64_t> dependent;
            for (const DofUse& use : mpcDependentDofs(model, mpc))
                dependent.insert(use.key);
            if (dependent.empty())
                continue;
            for (const std::vector<base::EntityId>& spc : spcSets)
            {
                for (const DofUse& use : spcDofs(model, spc, context.sortedNodeIds()))
                {
                    if (!dependent.contains(use.key) || !reported.emplace(use.where.table, use.where.row).second)
                        continue;
                    sink.reportCard(use.where, "",
                                    "degree of freedom " + std::to_string(componentOfDof(use.key)) + " of grid point " +
                                        std::to_string(nodeOfDof(use.key)) + " is the dependent term of a " +
                                        std::string("MultiPointConstr"),
                                    "remove the constraint, or the " + std::string("MultiPointConstr") + " equation");
                }
            }
        }
    }
};

} // namespace

void addSpcOnMpcDependentCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SpcOnMpcDependentCheck>());
}

} // namespace oor::analysis
