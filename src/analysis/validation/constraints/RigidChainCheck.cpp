#include "ConstraintDofs.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

#include <set>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A degree of freedom that is dependent in one rigid element and independent in another: the elements are chained, and
// the result depends on the order the solver resolves them in. Allowed, but worth knowing.
class RigidChainCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.rigid-chain", base::Severity::Info, "constraints",
                                    "A degree of freedom is dependent in one rigid element and independent in another.",
                                    true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::vector<DofLink> links = rigidDofLinks(model);
        std::unordered_map<std::uint64_t, model::CardLocation> dependentIn;
        for (const DofLink& each : links)
            dependentIn.emplace(each.dependent, each.where);
        std::set<std::pair<std::uint32_t, std::uint32_t>> reported;
        for (const DofLink& each : links)
        {
            const auto found = dependentIn.find(each.independent);
            if (found == dependentIn.end() || found->second == each.where)
                continue;
            if (!reported.emplace(each.where.table, each.where.row).second)
                continue;
            sink.reportCard(each.where, "",
                            "degree of freedom " + std::to_string(componentOfDof(each.independent)) +
                                " of grid point " + std::to_string(nodeOfDof(each.independent)) +
                                " is independent here and dependent in " +
                                std::string(cardNameAt(model, found->second)) + " " +
                                std::to_string(model.findTable(found->second.table)->idAt(found->second.row)),
                            "check that the chain is intended");
        }
    }
};

} // namespace

void addRigidChainCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RigidChainCheck>());
}

} // namespace oor::analysis
