#include "SolutionRules.hpp"

#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/model/control/executive_control/SolutionSequences.hpp"

#include <algorithm>

namespace oor::analysis
{

std::vector<const model::Subcase*> solvingSubcases(const model::CaseControl& caseControl)
{
    std::vector<const model::Subcase*> subcases;
    for (const model::Subcase* subcase : analysisSubcases(caseControl))
    {
        if (subcase->kind == model::SubcaseKind::Subcase || subcase->kind == model::SubcaseKind::Global)
            subcases.push_back(subcase);
    }
    return subcases;
}

void reportMissingSelection(const ValidationContext& context, FindingSink& sink,
                            std::initializer_list<std::int64_t> solutions,
                            std::initializer_list<std::string_view> commands, const std::string& what)
{
    const std::optional<std::int64_t> solution = context.solution();
    if (!solution || std::ranges::find(solutions, *solution) == solutions.end())
        return;
    const model::CaseControl& caseControl = context.root().control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* subcase : solvingSubcases(caseControl))
    {
        for (const std::string_view command : commands)
        {
            if (hasEffectiveCommand(lineages, *subcase, command))
                return;
        }
    }
    // The commands by their keywords: "a TSTEP or TSTEPNL command".
    std::string keywords;
    for (const std::string_view command : commands)
    {
        keywords += (keywords.empty() ? "" : " or ") + std::string(command);
    }
    // The name of the sequence as the deck writes it (SESTATIC for 101).
    const std::string_view neutral = model::SolutionSequences::nameOf(*solution);
    const std::string name = neutral.empty() ? std::string() : std::string(neutral);
    sink.reportStatement("SolutionSequence", *solution,
                         "SOL " + std::to_string(*solution) + (name.empty() ? "" : " (" + name + ")") + " needs a " +
                             keywords + " command (" + what + "), and no subcase selects one",
                         "add the Case Control command to the subcase or to the global section");
}

} // namespace oor::analysis
