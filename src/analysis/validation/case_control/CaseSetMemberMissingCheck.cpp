#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// An output request that selects a SET with members, listed one by one, that are not in the model: grid points for
// DISPLACEMENT, SPCFORCES, ..., elements for STRESS, FORCE, ... The solver skips the missing ones; a request whose set
// has none left outputs nothing. The IDs of a THRU range are not judged (a range may have gaps).
class CaseSetMemberMissingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.set-member-missing",
                                    base::Severity::Warning,
                                    "case_control",
                                    "A SET selected by an output request has a grid point or element that does not "
                                    "exist.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.root();
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        forEachSection(
            caseControl,
            [&](const model::Subcase& section)
            {
                for (const model::CaseEntry* entry : commandsOutsideBlocks(section))
                {
                    if (entry->kind != model::EntryKind::Output || entry->value.kind != model::CaseValueKind::Integer)
                        continue;
                    const RequestTarget target = targetOfRequest(entry->name);
                    const model::CaseEntry* set = lineages.set(section.key, entry->value.integer);
                    if (target == RequestTarget::None || set == nullptr ||
                        set->set.kind != model::CaseSetKind::Integers)
                        continue;
                    // The members listed one by one: a missing one is a mistake. A run of consecutive IDs ("a Through
                    // b") may be a range, and the solver skips the IDs of a range that do not exist, so the runs are
                    // not judged.
                    std::size_t missing = 0;
                    std::string first;
                    for (const std::string& item : set->set.items())
                    {
                        if (item.find("Through") != std::string::npos)
                            continue;
                        const std::int64_t id = std::stoll(item);
                        const auto entityId = base::toEntityId(id);
                        const bool found =
                            entityId &&
                            model.locate(target == RequestTarget::Nodes ? base::IdSpace::Node : base::IdSpace::Element,
                                         *entityId);
                        if (!found && missing++ == 0)
                            first = item;
                    }
                    if (missing > 0)
                        sink.reportStatement(statementOf(section), section.id,
                                             std::string(model::SET_COMMAND) + " " +
                                                 std::to_string(entry->value.integer) + " of " +
                                                 commandLabel(entry->name) + " has " + std::to_string(missing) + " " +
                                                 (target == RequestTarget::Nodes ? "grid points" : "elements") +
                                                 " that do not exist, the first is " + first,
                                             "correct the members of the set");
                }
            });
    }
};

} // namespace

void addCaseSetMemberMissingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseSetMemberMissingCheck>());
}

} // namespace oor::analysis
