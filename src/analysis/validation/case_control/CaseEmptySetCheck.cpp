#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"

namespace oor::analysis
{

namespace
{

// A SET with no member (every ID of it was excluded): a request that selects it outputs nothing.
class CaseEmptySetCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "case.empty-set", base::Severity::Warning, "case_control", "A SET of the Case Control has no member.", false,
        CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachSection(context.root().control().caseControl,
                       [&](const model::Subcase& section)
                       {
                           for (const model::CaseEntry& entry : section.entries)
                           {
                               if (entry.kind == model::EntryKind::Set && entry.set.kind != model::CaseSetKind::All &&
                                   entry.set.size() == 0)
                                   sink.reportStatement(statementOf(section), section.id,
                                                        std::string(model::SET_COMMAND) + " " +
                                                            std::to_string(entry.number) + " has no member",
                                                        "list the IDs of the set");
                           }
                       });
    }
};

} // namespace

void addCaseEmptySetCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseEmptySetCheck>());
}

} // namespace oor::analysis
