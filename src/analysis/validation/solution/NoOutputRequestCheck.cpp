#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// A subcase with no output request at all (no DISPLACEMENT, STRESS, FORCE, SPCFORCES, ...), its own or inherited: the
// run writes nothing to the results file.
class NoOutputRequestCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.no-output-request",
                                    base::Severity::Info,
                                    "solution",
                                    "A subcase has no output request.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        // Only a deck that has Case Control commands is judged.
        if (caseControl.isEmpty())
            return;
        for (const model::Subcase* subcase : solvingSubcases(caseControl))
        {
            bool requested = false;
            for (const model::Subcase* section : lineages.of(subcase->key))
            {
                for (const model::CaseEntry& entry : section->entries)
                    requested = requested || entry.kind == model::EntryKind::Output;
            }
            if (requested)
                continue;
            const std::string displacement = std::string("DisplacementOutput");
            const std::string stress = std::string("StressOutput");
            sink.reportStatement(std::string(model::subcaseKindName(subcase->kind)), subcase->id,
                                 "no output is requested: add " + displacement + ", " + stress + " or another request",
                                 "add " + displacement + " = ALL (and the other requests wanted)");
        }
    }
};

} // namespace

void addNoOutputRequestCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NoOutputRequestCheck>());
}

} // namespace oor::analysis
