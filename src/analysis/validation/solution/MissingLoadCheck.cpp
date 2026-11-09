#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// A SOL 101 or 106 subcase with nothing that loads it: no LOAD, LOADSET, CLOAD, DEFORM, TEMPERATURE(LOAD),
// TEMPERATURE(BOTH) or P2G (a load matrix of DMIG, added to the load vector: format reference P2G), and no enforced
// displacement (an SPC set with a value in its D field). The response is zero.
class MissingLoadCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "solution.missing-load",
        base::Severity::Warning,
        "solution",
        "A subcase of SOL 101 or 106 has no LOAD, TEMPERATURE(LOAD), DEFORM, P2G or enforced displacement.",
        false,
        CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    // The SPC set of the subcase has a nonzero D: the subcase is driven by the displacement it imposes.
    static bool hasEnforcedDisplacement(const ValidationContext& context, const model::CaseLineages& lineages,
                                        const model::Subcase& subcase)
    {
        const model::Model& model = context.root();
        const std::optional<std::int64_t> set = effectiveInteger(lineages, subcase, "SinglePointConstrSelection");
        const auto id = set ? base::toEntityId(*set) : std::nullopt;
        if (!id)
            return false;
        const std::vector<base::EntityId> sets = expandedConstraintSet(model, "SinglePointConstrSelection", *id);
        bool driven = false;
        forEachCard<model::SinglePointConstrTable>(
            model,
            [&](const model::SinglePointConstrCard& card, model::CardLocation)
            {
                if (std::ranges::binary_search(sets, card.id) &&
                    (card.enforced[0] != 0.0 || (card.hasSecondGroup() && card.enforced[1] != 0.0)))
                    driven = true;
            });
        return driven;
    }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || (*solution != 101 && *solution != 106))
            return;
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : solvingSubcases(caseControl))
        {
            const std::uint32_t key = subcase->key;
            if (lineages.command(key, "StaticLoadSelection") != nullptr ||
                lineages.command(key, "DeformationLoadSelection") != nullptr ||
                lineages.command(key, "LoadSequenceSelection") != nullptr ||
                lineages.command(key, "SuperelementLoadCombinationSelection") != nullptr ||
                lineages.command(key, "DirectInputLoadMatrix") != nullptr ||
                lineages.command(key, "TemperatureSelection", "TemperatureLoadPart") != nullptr ||
                lineages.command(key, "TemperatureSelection", "BothLoadAndInitial") != nullptr ||
                hasEnforcedDisplacement(context, lineages, *subcase))
                continue;
            // The commands by their keywords in the dictionary of the model.
            const auto keyword = [](std::string_view name) { return std::string(name); };
            const std::string load = keyword("StaticLoadSelection");
            const std::string temperatureLoad =
                keyword("TemperatureSelection") + "(" + std::string("TemperatureLoadPart") + ")";
            sink.reportStatement(std::string(model::subcaseKindName(subcase->kind)), subcase->id,
                                 "nothing loads this subcase: it has no " + load + ", " + temperatureLoad + ", " +
                                     keyword("DeformationLoadSelection") + " or " + keyword("DirectInputLoadMatrix") +
                                     " command",
                                 "add " + load + " = n");
        }
    }
};

} // namespace

void addMissingLoadCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingLoadCheck>());
}

} // namespace oor::analysis
