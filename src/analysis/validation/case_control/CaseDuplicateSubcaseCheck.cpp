#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// Two SUBCASE sections with the same number: the results of one cannot be told from the other.
class CaseDuplicateSubcaseCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.duplicate-subcase",
                                    base::Severity::Error,
                                    "case_control",
                                    "Two SUBCASE sections have the same number.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::set<std::int64_t> seen;
        for (const model::Subcase& segment : context.root().control().caseControl.segments())
        {
            // An auxiliary model (AUXCASE) numbers its subcases on its own.
            if (segment.kind == model::SubcaseKind::AuxiliaryCase)
                seen.clear();
            if (segment.kind == model::SubcaseKind::Subcase && !seen.insert(segment.id).second)
                sink.reportStatement(statementOf(segment), segment.id,
                                     "subcase " + std::to_string(segment.id) + " is defined twice",
                                     "number the subcases differently");
        }
    }
};

} // namespace

void addCaseDuplicateSubcaseCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseDuplicateSubcaseCheck>());
}

} // namespace oor::analysis
