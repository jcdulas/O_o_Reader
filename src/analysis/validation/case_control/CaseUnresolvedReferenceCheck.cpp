#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// A Case Control command that selects a set of cards that is not in the Bulk Data, of the residual structure or of a
// part (SPC = 5 with no SPC, SPC1 or SPCADD of set 5; METHOD = 3 with no EIGRL 3). A warning: the cards may be of a
// type the library does not read yet.
class CaseUnresolvedReferenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.unresolved-reference",
                                    base::Severity::Warning,
                                    "case_control",
                                    "A Case Control command selects a set (SPC, LOAD, METHOD, ...) that no Bulk Data "
                                    "card defines.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    // The Case Control applies to every part: a set (SPC = 20) that only a superelement defines is a set that exists.
    static bool definedInAPart(const ValidationContext& context, const UnresolvedUse& use)
    {
        for (const model::Model* part : context.allModels())
        {
            if (part != &context.root() && model::resolveReference(*part, use.target, use.id))
                return true;
        }
        return false;
    }

    // With PARAM,HEATSTAT,YES (a heat transfer then a thermal stress analysis in SOL 101) the TEMPERATURE(LOAD) of the
    // stress subcase does not name a set of TEMP cards but the heat transfer subcase whose temperatures it takes: a
    // number that is the number of a subcase of the deck is not a missing set.
    static bool namesHeatSubcase(const ValidationContext& context, const UnresolvedUse& use)
    {
        if (use.field != "TemperatureSelection" || !isHeatThenStress(context.root()))
            return false;
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const std::vector<const model::Subcase*> subcases = analysisSubcases(caseControl);
        return std::ranges::any_of(subcases, [&](const model::Subcase* subcase)
                                   { return subcase->kind == model::SubcaseKind::Subcase && subcase->id == use.id; });
    }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const UnresolvedUse& use : context.references().unresolved)
        {
            if (!use.inCaseControl || definedInAPart(context, use) || namesHeatSubcase(context, use))
                continue;
            FindingDraft draft;
            // The section by its kind and the command by its neutral name; the message names both by their keywords.
            draft.cardName = use.scope;
            if (const auto kind = model::subcaseKindOf(use.scope))
                draft.cardId = static_cast<base::EntityId>(use.scopeNumber);
            draft.field = use.field;
            draft.message = "selects " + std::string(describeTarget(use.target)) + " " + std::to_string(use.id) +
                            ", which no Bulk Data card defines";
            draft.suggestion = "define the set in the Bulk Data or correct the command";
            sink.report(std::move(draft));
        }
    }
};

} // namespace

void addCaseUnresolvedReferenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseUnresolvedReferenceCheck>());
}

} // namespace oor::analysis
