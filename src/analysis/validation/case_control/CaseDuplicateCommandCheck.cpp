#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"

#include <map>

namespace oor::analysis
{

namespace
{

// The same command twice in one section with the same value: harmless (the last counts), and pointless.
class CaseDuplicateCommandCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.duplicate-command",
                                    base::Severity::Info,
                                    "case_control",
                                    "A Case Control command is given twice in one section with the same value.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachSection(context.root().control().caseControl,
                       [&](const model::Subcase& section)
                       {
                           std::map<std::string, const model::CaseEntry*> first;
                           for (const model::CaseEntry* entry : commandsOutsideBlocks(section))
                           {
                               const auto [position, inserted] = first.emplace(commandIdentity(*entry), entry);
                               if (!inserted && position->second->value == entry->value)
                                   sink.reportStatement(statementOf(section), section.id,
                                                        commandLabel(entry->name, entry->variant) +
                                                            " is given twice with the same value",
                                                        "remove one of them");
                           }
                       });
    }
};

} // namespace

void addCaseDuplicateCommandCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseDuplicateCommandCheck>());
}

} // namespace oor::analysis
