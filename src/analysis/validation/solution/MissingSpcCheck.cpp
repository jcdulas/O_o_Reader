#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// A static solution whose Bulk Data has SPC or SPC1 cards while no subcase selects a set with SPC: the constraints are
// in the deck and not used.
class MissingSpcCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-spc",
                                    base::Severity::Warning,
                                    "solution",
                                    "A static solution has single-point constraint cards and no subcase selects them.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || !isStaticSolution(*solution))
            return;
        const model::Model& root = context.root();
        const model::CardRegistry& registry = model::CardRegistry::instance();
        bool hasSpc = false;
        for (std::uint16_t type = 1; type < registry.size() && !hasSpc; ++type)
        {
            const model::CardTable* table = root.findTable(type);
            hasSpc = table != nullptr && table->rows().aliveCount() > 0 &&
                     isOneOf(registry.entry(type).info.name,
                             {"SinglePointConstr", "SinglePointConstrList", "SinglePointConstrUnion"});
        }
        if (!hasSpc)
            return;
        const model::CaseControl& caseControl = root.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : solvingSubcases(caseControl))
        {
            if (hasEffectiveCommand(lineages, *subcase, "SinglePointConstrSelection"))
                return;
        }
        sink.reportStatement(
            "SolutionSequence", *solution,
            "the deck has " +
                cardLabels(root, {"SinglePointConstr", "SinglePointConstrList", "SinglePointConstrUnion"}) +
                " cards and no subcase has an SPC command",
            "add SPC = n to the subcase or to the global section");
    }
};

} // namespace

void addMissingSpcCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingSpcCheck>());
}

} // namespace oor::analysis
