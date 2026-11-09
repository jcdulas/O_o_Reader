#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// A SUBCOM with no SUBSEQ and no SUBSEQ1: a combination of subcases with no coefficient (a SUBSEQ
// must follow it; SUBSEQ1 is the other form). A SYMCOM needs no SYMSEQ: without one every coefficient is 1.0.
class CaseCombinationWithoutFactorsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.subcase-combination-without-factors",
                                    base::Severity::Error,
                                    "case_control",
                                    "A SUBCOM has no SUBSEQ or SUBSEQ1 command.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const model::Subcase& segment : context.root().control().caseControl.segments())
        {
            if (segment.kind != model::SubcaseKind::SubcaseCombination)
                continue;
            if (model::CaseControl::ownCommand(segment, "SubcaseCombinationFactors") == nullptr &&
                model::CaseControl::ownCommand(segment, "SubcaseCombinationFactors1") == nullptr)
            {
                const std::string factors = commandLabel("SubcaseCombinationFactors");
                sink.reportStatement(statementOf(segment), segment.id,
                                     "there is no " + factors + " or " + commandLabel("SubcaseCombinationFactors1") +
                                         " command: the coefficients are missing",
                                     "add " + factors + " = c1, c2, ...");
            }
        }
    }
};

} // namespace

void addCaseCombinationWithoutFactorsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseCombinationWithoutFactorsCheck>());
}

} // namespace oor::analysis
