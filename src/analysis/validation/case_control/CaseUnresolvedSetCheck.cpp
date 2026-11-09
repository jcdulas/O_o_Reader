#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// An output request that selects a SET (DISPLACEMENT = 10) that the Case Control does not define where the request is
// seen: not in the subcase, not in the section it inherits from, not in the global section.
class CaseUnresolvedSetCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.unresolved-set",
                                    base::Severity::Error,
                                    "case_control",
                                    "An output request selects a SET that the Case Control does not define.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        const std::string set = std::string(model::SET_COMMAND);
        forEachSection(caseControl,
                       [&](const model::Subcase& section)
                       {
                           for (const model::CaseEntry* entry : commandsOutsideBlocks(section))
                           {
                               if (entry->kind != model::EntryKind::Output ||
                                   entry->value.kind != model::CaseValueKind::Integer || !selectsSet(entry->name))
                                   continue;
                               if (lineages.set(section.key, entry->value.integer) != nullptr)
                                   continue;
                               sink.reportStatement(statementOf(section), section.id,
                                                    commandLabel(entry->name) + " selects " + set + " " +
                                                        std::to_string(entry->value.integer) + ", which is not defined",
                                                    "define " + set + " " + std::to_string(entry->value.integer) +
                                                        " = ... above the request, or use ALL");
                           }
                       });
    }
};

} // namespace

void addCaseUnresolvedSetCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseUnresolvedSetCheck>());
}

} // namespace oor::analysis
