#include "ConstraintDofs.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The dependent degree of freedom of an MPC equation (its first term) that another equation of the same set, or a rigid
// element, also makes dependent.
class MpcDependentTwiceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.mpc-dependent-twice", base::Severity::Error, "constraints",
                                    "A degree of freedom is dependent in two multipoint constraint equations of a set, "
                                    "or in one and a rigid element."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::unordered_set<std::uint64_t> rigid;
        for (const DofUse& use : rigidDependentDofs(model))
            rigid.insert(use.key);
        std::set<std::pair<std::uint32_t, std::uint32_t>> reported;
        for (const std::vector<base::EntityId>& sets : constraintSelections(model, model::MULTI_POINT_CONSTR_SELECTION))
        {
            std::unordered_map<std::uint64_t, model::CardLocation> first;
            for (const DofUse& use : mpcDependentDofs(model, sets))
            {
                const auto [position, inserted] = first.emplace(use.key, use.where);
                const bool twice = !inserted && !(position->second == use.where);
                if (!twice && !rigid.contains(use.key))
                    continue;
                if (!reported.emplace(use.where.table, use.where.row).second)
                    continue;
                sink.reportCard(
                    use.where, "",
                    "degree of freedom " + std::to_string(componentOfDof(use.key)) + " of grid point " +
                        std::to_string(nodeOfDof(use.key)) +
                        (twice ? " is the dependent term of another " + std::string("MultiPointConstr") + " of the set"
                               : std::string(" is dependent in a rigid element")),
                    "make the degree of freedom dependent once");
            }
        }
    }
};

} // namespace

void addMpcDependentTwiceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MpcDependentTwiceCheck>());
}

} // namespace oor::analysis
