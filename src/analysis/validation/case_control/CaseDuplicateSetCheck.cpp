#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// The same SET number defined twice in one section: the later definition replaces the earlier.
class CaseDuplicateSetCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.duplicate-set",
                                    base::Severity::Warning,
                                    "case_control",
                                    "A SET number is defined twice in one section of the Case Control.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachSection(context.root().control().caseControl,
                       [&](const model::Subcase& section)
                       {
                           std::set<std::int64_t> seen;
                           for (const model::CaseEntry& entry : section.entries)
                           {
                               if (entry.kind == model::EntryKind::Set && !seen.insert(entry.number).second)
                                   sink.reportStatement(statementOf(section), section.id,
                                                        std::string(model::SET_COMMAND) + " " +
                                                            std::to_string(entry.number) +
                                                            " is defined twice in this section: the last one counts",
                                                        "remove one of the definitions");
                           }
                       });
    }
};

} // namespace

void addCaseDuplicateSetCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseDuplicateSetCheck>());
}

} // namespace oor::analysis
