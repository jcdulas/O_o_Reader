#include "ConstraintDofs.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

#include <set>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A degree of freedom that two rigid elements both make dependent: it cannot depend on two things.
class RigidDependentTwiceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.rigid-dependent-twice", base::Severity::Error, "constraints",
                                    "A degree of freedom is dependent in two rigid elements."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::unordered_map<std::uint64_t, model::CardLocation> first;
        std::set<std::pair<std::uint32_t, std::uint32_t>> reported;
        for (const DofUse& use : rigidDependentDofs(model))
        {
            const auto [position, inserted] = first.emplace(use.key, use.where);
            if (inserted || position->second == use.where)
                continue; // a card that names the same degree of freedom twice is its own affair
            if (!reported.emplace(use.where.table, use.where.row).second)
                continue;
            sink.reportCard(use.where, "",
                            "degree of freedom " + std::to_string(componentOfDof(use.key)) + " of grid point " +
                                std::to_string(nodeOfDof(use.key)) + " is also dependent in " +
                                std::string(cardNameAt(model, position->second)) + " " +
                                std::to_string(model.findTable(position->second.table)->idAt(position->second.row)),
                            "make the degree of freedom dependent in one rigid element only");
        }
    }
};

} // namespace

void addRigidDependentTwiceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RigidDependentTwiceCheck>());
}

} // namespace oor::analysis
