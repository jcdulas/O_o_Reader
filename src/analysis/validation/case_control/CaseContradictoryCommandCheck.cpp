#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"

#include <map>

namespace oor::analysis
{

namespace
{

// The same command twice in one section with two values (SPC = 1, then SPC = 2; DISPLACEMENT = ALL, then NONE): the
// last one counts and the first is dead text, which is rarely intended.
class CaseContradictoryCommandCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.contradictory-command",
                                    base::Severity::Warning,
                                    "case_control",
                                    "A Case Control command is given twice in one section with different values.",
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
                               if (!inserted && !(position->second->value == entry->value))
                                   sink.reportStatement(statementOf(section), section.id,
                                                        commandLabel(entry->name, entry->variant) +
                                                            " is given twice with different values (" +
                                                            position->second->value.format() + " and " +
                                                            entry->value.format() + "): the last one counts",
                                                        "keep the one that is meant");
                           }
                       });
    }
};

} // namespace

void addCaseContradictoryCommandCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseContradictoryCommandCheck>());
}

} // namespace oor::analysis
