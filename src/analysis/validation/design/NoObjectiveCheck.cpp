#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// A SOL 200 run that optimises has an objective, DESOBJ. The format reference lets a run do a sensitivity analysis
// without one, so the rule is limited to the decks that say they optimise: SOL 200, design variables, and constraints
// selected by a DESSUB or a DESGLB in some subcase, with no DESOBJ (DESOBJ(MIN) or DESOBJ(MAX) either) in any of them.
class NoObjectiveCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.no-objective", base::Severity::Warning, "design",
                                    "A SOL 200 deck with design variables and design constraints has no DESOBJ."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    static constexpr std::string_view OBJECTIVE = "DesignObjectiveSelection";

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        if (context.solution() != 200)
            return;
        const model::DesignVarTable* table = context.model().findTable<model::DesignVarTable>();
        if (table == nullptr || table->rows().aliveCount() == 0)
            return;
        const model::CaseControl& caseControl = context.model().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        bool hasConstraints = false;
        for (const model::Subcase* subcase : analysisSubcases(caseControl))
        {
            for (const char* variant : {"", "MinimumObjective", "MaximumObjective"})
            {
                if (lineages.command(subcase->key, OBJECTIVE, variant) != nullptr)
                    return;
            }
            hasConstraints = hasConstraints ||
                             hasEffectiveCommand(lineages, *subcase, "SubcaseDesignConstrSelection") ||
                             hasEffectiveCommand(lineages, *subcase, "GlobalDesignConstrSelection");
        }
        if (!hasConstraints)
            return;
        const std::string objective = std::string(OBJECTIVE);
        const std::string minimum = std::string("MinimumObjective");
        sink.reportStatement(
            std::string(OBJECTIVE), 0,
            "the deck optimises (SOL 200 with design variables and constraints) and names no objective",
            "add " + objective + "(" + minimum + ") = n, where n is the response to minimise");
    }
};

} // namespace

void addNoObjectiveCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NoObjectiveCheck>());
}

} // namespace oor::analysis
